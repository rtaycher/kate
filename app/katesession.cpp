/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katesession.h"
#include "katesession.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <klistview.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kmdcodec.h>
#include <kstdguiitem.h>

#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qcheckbox.h>
#include <qdatetime.h>

#include <time.h>

KateSession::KateSession (KateSessionManager *manager, const QString &fileName, const QString &name)
  : m_sessionFileRel (fileName)
  , m_sessionName (name)
  , m_documents (0)
  , m_manager (manager)
  , m_readConfig (0)
  , m_writeConfig (0)
{
  // given file exists, use it to load some stuff ;)
  if (!fileName.isEmpty() && KGlobal::dirs()->exists(sessionFile ()))
  {
    KSimpleConfig config (sessionFile (), true);

    if (m_sessionName.isEmpty())
    {
      // get the name out of the file
      if (fileName == "default.katesession")
        m_sessionName = i18n("Default Session");
      else
      {
        config.setGroup ("General");
        m_sessionName = config.readEntry ("Name", i18n ("Unnamed Session"));
      }
    }

    // get the document count
    config.setGroup ("Open Documents");
    m_documents = config.readUnsignedNumEntry("Count", 0);

    return;
  }

  // filename not empty, create the file
  if (!fileName.isEmpty())
  {
     // uhh, no name given
    if (m_sessionName.isEmpty())
    {
      if (fileName == "default.katesession")
        m_sessionName = i18n("Default Session");
      else
        m_sessionName = i18n("Session (%1)").arg(QTime::currentTime().toString(Qt::LocalDate));
    }

    // create the file, write name to it!
    KSimpleConfig config (sessionFile ());
    config.setGroup ("General");
    config.writeEntry ("Name", m_sessionName);
    config.sync ();
  }
}

KateSession::~KateSession ()
{
  delete m_readConfig;
  delete m_writeConfig;
}

QString KateSession::sessionFile () const
{
  return m_manager->sessionsDir() + "/" + m_sessionFileRel;
}

bool KateSession::create (const QString &name)
{
  if (name.isEmpty() || !m_sessionFileRel.isEmpty())
    return false;

  m_sessionName = name;

  // get a usable filename
  int s = time(0);
  QCString tname;
  while (true)
  {
    tname.setNum (s++);
    KMD5 md5 (tname);
    m_sessionFileRel = md5.hexDigest() + ".katesession";

    if (!KGlobal::dirs()->exists(sessionFile ()))
      break;
  }

   // create the file, write name to it!
  KSimpleConfig config (sessionFile ());
  config.setGroup ("General");
  config.writeEntry ("Name", m_sessionName);
  config.sync ();

  return true;
}

KConfig *KateSession::configRead ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_readConfig)
    return m_readConfig;

  return m_readConfig = new KSimpleConfig (sessionFile (), true);
}

KConfig *KateSession::configWrite ()
{
  if (m_sessionFileRel.isEmpty())
    return 0;

  if (m_writeConfig)
    return m_writeConfig;

  m_writeConfig = new KSimpleConfig (sessionFile ());
  m_writeConfig->setGroup ("General");
  m_writeConfig->writeEntry ("Name", m_sessionName);

  return m_writeConfig;
}

KateSessionManager::KateSessionManager (QObject *parent)
 : QObject (parent)
 , m_sessionsDir (locateLocal( "data", "kate/sessions"))
 , m_dirWatch (new KDirWatch (this))
 , m_activeSession (new KateSession (this, "", ""))
{
  kdDebug() << "LOCAL SESSION DIR: " << m_sessionsDir << endl;

  // create dir if needed
  KGlobal::dirs()->makeDir (m_sessionsDir);

  // add this dir to the watch
  connect (m_dirWatch, SIGNAL(dirty (const QString &)), this, SLOT(dirty (const QString &)));
  m_dirWatch->addDir (m_sessionsDir);

  // initial setup of the sessions list
  updateSessionList ();
}

KateSessionManager::~KateSessionManager()
{
}

KateSessionManager *KateSessionManager::self()
{
  return KateApp::self()->kateSessionManager ();
}

void KateSessionManager::dirty (const QString &)
{
  updateSessionList ();
}

void KateSessionManager::updateSessionList ()
{
  m_sessionList.clear ();

  // Let's get a list of all session we have atm
  QDir dir (m_sessionsDir, "*.katesession");

  bool foundDefault = false;
  for (unsigned int i=0; i < dir.count(); ++i)
  {
    KateSession *session = new KateSession (this, dir[i], "");
    m_sessionList.append (session);

    kdDebug () << "FOUND SESSION: " << session->sessionName() << " FILE: " << session->sessionFile() << endl;

    if (!foundDefault && (dir[i] == "default.katesession"))
      foundDefault = true;
  }

  // add default session, if not there
  if (!foundDefault)
    m_sessionList.append (new KateSession (this, "default.katesession", i18n("Default Session")));
}

void KateSessionManager::activateSession (KateSession::Ptr session, bool closeLast, bool saveLast, bool loadNew)
{
  // try to close last session
  if (closeLast)
  {
    if (((KateApp *)kapp)->activeKateMainWindow())
    {
      if (!((KateApp *)kapp)->activeKateMainWindow()->queryClose_internal())
        return;
    }
  }

  // save last session or not?
  if (saveLast)
    saveActiveSession (true);

  // really close last
  if (closeLast)
  {
    KateDocManager::self()->closeAllDocuments ();
  }

  // set the new session
  m_activeSession = session;

  if (loadNew)
  {
    // open the new session
    Kate::Document::setOpenErrorDialogsActivated (false);

    KConfig *sc = activeSession()->configRead();

    if (sc)
      ((KateApp *)kapp)->kateDocumentManager()->restoreDocumentList (sc);

    KateMainWindow *win = ((KateApp *)kapp)->activeKateMainWindow();

    if (!win)
      win = ((KateApp *)kapp)->newMainWindow(false);

    // window config
    if (sc)
    {
      KConfig *c = kapp->config();
      c->setGroup("General");

      if (c->readBoolEntry("Restore Window Configuration", false))
        win->readProperties (sc);
    }

    Kate::Document::setOpenErrorDialogsActivated (true);
    win->show ();
  }
}

KateSession::Ptr KateSessionManager::createSession (const QString &name)
{
  KateSession::Ptr s = new KateSession (this, "", "");
  s->create (name);

  return s;
}

KateSession::Ptr KateSessionManager::giveSession (const QString &name)
{
  if (name.isEmpty())
    return new KateSession (this, "", "");

  for (unsigned int i=0; i < m_sessionList.count(); ++i)
  {
    if (m_sessionList[i]->sessionName() == name)
      return m_sessionList[i];
  }

  return createSession (name);
}

bool KateSessionManager::saveActiveSession (bool tryAsk)
{
  if (tryAsk)
  {
    // app config
    KConfig *c = kapp->config();
    c->setGroup("General");

    QString sesExit (c->readEntry ("Session Exit", "save"));

    if (sesExit == "discard")
      return true;

    if (sesExit == "ask")
    {
      KDialogBase *dlg = new KDialogBase (
                    i18n ("Save Session?")
                    , KDialogBase::Yes | KDialogBase::No
                    , KDialogBase::Yes, KDialogBase::No
                  );

      bool dontAgain = false;
      int res = KMessageBox::createKMessageBox(dlg, QMessageBox::Question,
                              i18n("Save current session?"), QStringList(),
                              i18n("Don't ask again"), &dontAgain, KMessageBox::Notify);

      // remember to not ask again with right setting
      if (dontAgain)
      {
        c->setGroup("General");

        if (res == KDialogBase::No)
          c->writeEntry ("Session Exit", "discard");
        else
          c->writeEntry ("Session Exit", "save");
      }

      if (res == KDialogBase::No)
        return true;
    }
  }

  KConfig *sc = activeSession()->configWrite();

  if (!sc)
    return false;

  KateDocManager::self()->saveDocumentList (sc);
  ((KateApp *)kapp)->activeKateMainWindow()->saveProperties (sc);
  sc->sync();

  return true;
}

void KateSessionManager::chooseSession ()
{
  // app config
  KConfig *c = kapp->config();
  c->setGroup("General");

  // get last used session, default to default session
  QString lastSession (c->readEntry ("Last Session", "default.katesession"));
  QString sesStart (c->readEntry ("Startup Session", "manual"));

  // uhh, just open last used session, show no chooser
  if (sesStart == "last")
  {
    activateSession (new KateSession (this, lastSession, ""), false, false);
    return;
  }

  // start with empty new session
  if (sesStart == "new")
  {
    activateSession (new KateSession (this, "", ""), false, false);
    return;
  }

  KateSessionChooser *chooser = new KateSessionChooser (0, lastSession);

  bool retry = true;
  int res = 0;
  while (retry)
  {
    res = chooser->exec ();

    switch (res)
    {
      case KateSessionChooser::resultOpen:
      {
        KateSession::Ptr s = chooser->selectedSession ();

        if (!s)
        {
          KMessageBox::error (chooser, i18n("No Session selected to open!"), i18n ("No Session selected"));
          break;
        }

        activateSession (s, false, false);
        retry = false;
        break;
      }

      default:
        activateSession (new KateSession (this, "", ""), false, false);
        retry = false;
        break;
    }
  }

  // write back our nice boolean :)
  if (chooser->reopenLastSession ())
  {
    c->setGroup("General");

    if (res == KateSessionChooser::resultOpen)
      c->writeEntry ("Startup Session", "last");
    else if (res == KateSessionChooser::resultNew)
      c->writeEntry ("Startup Session", "new");

    c->sync ();
  }

  delete chooser;
}

void KateSessionManager::sessionNew ()
{
  activateSession (new KateSession (this, "", ""));
}

void KateSessionManager::sessionOpen ()
{
  KateSessionOpenDialog *chooser = new KateSessionOpenDialog (0);

  int res = chooser->exec ();

  if (res == KateSessionOpenDialog::resultCancel)
  {
    delete chooser;
    return;
  }

  KateSession::Ptr s = chooser->selectedSession ();

  if (s)
    activateSession (s);

  delete chooser;
}

void KateSessionManager::sessionSave ()
{
  // if the active session is valid, just save it :)
  if (saveActiveSession ())
    return;

  bool ok = false;
  QString name = KInputDialog::getText (i18n("Specify a Name for Current Session"), i18n("Session Name"), "", &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a new session, you must specify a name!"), i18n ("Missing Session Name"));
    return;
  }

  activeSession()->create (name);
  saveActiveSession ();
}

//BEGIN CHOOSER DIALOG

class KateSessionChooserItem : public QListViewItem
{
  public:
    KateSessionChooserItem (KListView *lv, KateSession::Ptr s)
     : QListViewItem (lv, s->sessionName())
     , session (s)
    {
      QString docs;
      docs.setNum (s->documents());
      setText (1, docs);
    }

    KateSession::Ptr session;
};

KateSessionChooser::KateSessionChooser (QWidget *parent, const QString &lastSession)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Session Chooser")
                  , KDialogBase::User1 | KDialogBase::User2
                  , KDialogBase::User1
                  , true
                  , KGuiItem (i18n ("Open Session"), "fileopen")
                  , KGuiItem (i18n ("New Session"), "filenew")
                )
{
  QHBox *page = new QHBox (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBox *hb = new QHBox (page);
  hb->setSpacing (KDialog::spacingHint());

  QLabel *label = new QLabel (hb);
  label->setPixmap (UserIcon("sessionchooser.png"));
  label->setFrameStyle (QFrame::Panel | QFrame::Sunken);

  QVBox *vb = new QVBox (hb);
  vb->setSpacing (KDialog::spacingHint());

  m_sessions = new KListView (vb);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (QListView::AllColumns);
  m_sessions->setSelectionMode (QListView::Single);
  m_sessions->setAllColumnsShowFocus (true);

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (unsigned int i=0; i < slist.count(); ++i)
  {
    KateSessionChooserItem *item = new KateSessionChooserItem (m_sessions, slist[i]);

    if (slist[i]->sessionFileRelative() == lastSession)
      m_sessions->setSelected (item, true);
  }

  m_useLast = new QCheckBox (i18n ("&Always use this choice"), vb);

  setResult (resultNone);
}

KateSessionChooser::~KateSessionChooser ()
{
}

KateSession::Ptr KateSessionChooser::selectedSession ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item)
    return 0;

  return item->session;
}

bool KateSessionChooser::reopenLastSession ()
{
  return m_useLast->isChecked ();
}

void KateSessionChooser::slotUser1 ()
{
  done (resultOpen);
}

void KateSessionChooser::slotUser2 ()
{
  done (resultNew);
}

KateSessionOpenDialog::KateSessionOpenDialog (QWidget *parent)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Open Session")
                  , KDialogBase::User1 | KDialogBase::User2
                  , KDialogBase::User2
                  , false
                  , KStdGuiItem::cancel ()
                  , KStdGuiItem::open ()
                )
{
  QHBox *page = new QHBox (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBox *hb = new QHBox (page);

  QVBox *vb = new QVBox (hb);

  m_sessions = new KListView (vb);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (QListView::AllColumns);
  m_sessions->setSelectionMode (QListView::Single);
  m_sessions->setAllColumnsShowFocus (true);

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (unsigned int i=0; i < slist.count(); ++i)
  {
    new KateSessionChooserItem (m_sessions, slist[i]);
  }

  setResult (resultCancel);
}

KateSessionOpenDialog::~KateSessionOpenDialog ()
{
}

KateSession::Ptr KateSessionOpenDialog::selectedSession ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item)
    return 0;

  return item->session;
}

void KateSessionOpenDialog::slotUser1 ()
{
  done (resultCancel);
}

void KateSessionOpenDialog::slotUser2 ()
{
  done (resultOk);
}

//END CHOOSER DIALOG
