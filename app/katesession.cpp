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

#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qcheckbox.h>

KateSession::KateSession (KateSessionManager *manager, const QString &fileName, const QString &name)
  : m_sessionFileRel (fileName)
  , m_sessionName (name)
  , m_documents (0)
  , m_manager (manager)
{
  bool ex (!fileName.isEmpty() && KGlobal::dirs()->exists(sessionFile ()));

  if (ex)
  {
    if (m_sessionName.isEmpty())
    {
      KSimpleConfig config (sessionFile (), true);

      // get the name out of the file
      if (fileName == "default.katesession")
        m_sessionName = i18n("Default Session");
      else
      {
        config.setGroup ("General");
        m_sessionName = config.readEntry ("Name", i18n ("Unnamed Session"));
      }

      // get the document count
      config.setGroup ("Open Documents");
      m_documents = config.readUnsignedNumEntry("Count", 0);
    }
  }
  else if (!fileName.isEmpty())
  {
    // create the file, write name to it!
    if (m_sessionName.isEmpty())
    {
      if (fileName == "default.katesession")
        m_sessionName = i18n("Default Session");
      else
        m_sessionName =  i18n ("Unnamed Session");
    }

    KSimpleConfig config (sessionFile ());
    config.setGroup ("General");
    config.writeEntry ("Name", m_sessionName);
    config.sync ();
  }
}

KateSession::~KateSession ()
{
}

QString KateSession::sessionFile () const
{
  return m_manager->sessionsDir() + "/" + m_sessionFileRel;
}

KConfig *KateSession::configRead ()
{
  return isValid () ? new KSimpleConfig (sessionFile (), true) : 0;
}

KConfig *KateSession::configWrite ()
{
  if (!isValid())
    return 0;

  KConfig *c = new KSimpleConfig (sessionFile ());
  c->setGroup ("General");
  c->writeEntry ("Name", m_sessionName);

  return c;
}

KateSessionManager::KateSessionManager (QObject *parent)
 : QObject (parent)
 , m_sessionsDir (locateLocal( "data", "kate/sessions"))
 , m_dirWatch (new KDirWatch (this))
 , m_activeSession (this, "", "")
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
  for (unsigned int i=0; i < m_sessionList.size(); ++i)
    delete m_sessionList[i];
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
  for (unsigned int i=0; i < m_sessionList.size(); ++i)
    delete m_sessionList[i];

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

void KateSessionManager::activateSession (const KateSession &session, bool closeLast, bool saveLast)
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

  // save last
  if (saveLast)
  {
    KConfig *sc = activeSession().configWrite();

    if (sc)
    {
      KateDocManager::self()->saveDocumentList (sc);
      ((KateApp *)kapp)->activeKateMainWindow()->saveProperties (sc);
      sc->sync();
    }

    delete sc;
  }

  // really close last
  if (closeLast)
  {
    KateDocManager::self()->closeAllDocuments ();
  }

  // set the new session
  m_activeSession = session;

  // open the new session
  Kate::Document::setOpenErrorDialogsActivated (false);

  KConfig *sc = activeSession().configRead();

  if (sc)
    ((KateApp *)kapp)->kateDocumentManager()->restoreDocumentList (sc);

  KateMainWindow *win = ((KateApp *)kapp)->activeKateMainWindow();

  if (!win)
    win = ((KateApp *)kapp)->newMainWindow(false);

  // window config
  if (sc)
    win->readProperties (sc);

  delete sc;

  Kate::Document::setOpenErrorDialogsActivated (true);
  win->show ();
}

KateSession KateSessionManager::createSession (const QString &name)
{
  KMD5 md5 (name.utf8());
  return KateSession (this, QString (md5.hexDigest()) + ".katesession", name);
}

void KateSessionManager::chooseSession ()
{
  // app config
  KConfig *c = kapp->config();
  c->setGroup("General");

  // get last used session, default to default session
  QString lastSession (c->readEntry ("Last Session", "default.katesession"));
  bool reopenLast (c->readBoolEntry ("Auto Open Last Session", false));

  // uhh, just open last used session, show no chooser
  if (reopenLast)
  {
    activateSession (KateSession (this, lastSession, ""), false, false);
    return;
  }

  KateSessionChooser *chooser = new KateSessionChooser (0, lastSession, reopenLast);

  bool retry = true;
  while (retry)
  {
    int res = chooser->exec ();

    switch (res)
    {
      case KateSessionChooser::resultOpen:
      {
        KateSession *s = chooser->selectedSession ();

        if (!s)
        {
          KMessageBox::error (chooser, i18n("No Session selected to open!"), i18n ("No Session selected"));
          break;
        }

        activateSession (*s, false, false);
        retry = false;
        break;
      }

      case KateSessionChooser::resultNew:
      {
        QString name = KInputDialog::getText (i18n("Specify a Name for New Session"), i18n("Session Name"));

        if (name.isEmpty())
        {
          KMessageBox::error (chooser, i18n("To start a new session, you must specify a name!"), i18n ("Missing Session Name"));
          break;
        }

        activateSession (createSession (name), false, false);
        retry = false;
        break;
      }

      default:
        activateSession (KateSession (this, "", ""), false, false);
        retry = false;
        break;
    }
  }

  // write back our nice boolean :)
  c->writeEntry ("Auto Open Last Session", chooser->reopenLastSession ());

  delete chooser;
}

void KateSessionManager::sessionNew ()
{
  QString name = KInputDialog::getText (i18n("Specify a Name for New Session"), i18n("Session Name"));

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To start a new session, you must specify a name!"), i18n ("Missing Session Name"));
    return;
  }

  activateSession (createSession (name));
}

//BEGIN CHOOSER DIALOG

class KateSessionChooserItem : public QListViewItem
{
  public:
    KateSessionChooserItem (KListView *lv, KateSession *s)
     : QListViewItem (lv, s->sessionName())
     , session (*s)
    {
      QString docs;
      docs.setNum (s->documents());
      setText (1, docs);
    }

    KateSession session;
};

KateSessionChooser::KateSessionChooser (QWidget *parent, const QString &lastSession, bool reopenLast)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Session Chooser")
                  , KDialogBase::User1 | KDialogBase::User2 |KDialogBase::User3
                  , KDialogBase::User1
                  , true
                  , KGuiItem (i18n ("Open Session"), "fileopen")
                  , KGuiItem (i18n ("New Session"), "filenew")
                  , KGuiItem (i18n ("Skip"), "fileclose")
                )
{
  QHBox *page = new QHBox (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBox *hb = new QHBox (page);

  QLabel *label = new QLabel (hb);
  label->setPixmap (BarIcon("kate",64));
  label->setMargin (16);

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
    KateSessionChooserItem *item = new KateSessionChooserItem (m_sessions, slist[i]);

    if (slist[i]->sessionFileRelative() == lastSession)
      m_sessions->setSelected (item, true);
  }

  m_useLast = new QCheckBox (i18n ("&Don't ask again"), vb);
  m_useLast->setChecked (reopenLast);

  setResult (resultNone);
}

KateSessionChooser::~KateSessionChooser ()
{
}

KateSession *KateSessionChooser::selectedSession ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item)
    return 0;

  return &item->session;
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

void KateSessionChooser::slotUser3 ()
{
  done (resultNone);
}

//END CHOOSER DIALOG
