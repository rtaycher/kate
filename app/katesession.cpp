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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katesession.h"
#include "katesession.moc"

#include "kateapp.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"

#include "katetooltipmenu.h"

#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdirwatch.h>
#include <klistview.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kcodecs.h>
#include <kstdguiitem.h>
#include <kpushbutton.h>
#include <kmenu.h>
#include <kactioncollection.h>

#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringList>

#include <unistd.h>
#include <time.h>


KateSession::KateSession (KateSessionManager *manager, const QString &fileName, const QString &name)
  : m_sessionFileRel (fileName)
  , m_sessionName (name)
  , m_documents (0)
  , m_manager (manager)
  , m_readConfig (0)
  , m_writeConfig (0)
{
  init ();
}

void KateSession::init ()
{
  // given file exists, use it to load some stuff ;)
  if (!m_sessionFileRel.isEmpty() && KGlobal::dirs()->exists(sessionFile ()))
  {
    KSimpleConfig config (sessionFile (), true);

    if (m_sessionName.isEmpty())
    {
      // get the name out of the file
      if (m_sessionFileRel == "default.katesession")
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
  if (!m_sessionFileRel.isEmpty())
  {
     // uhh, no name given
    if (m_sessionName.isEmpty())
    {
      if (m_sessionFileRel == "default.katesession")
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

bool KateSession::create (const QString &name, bool force)
{
  if (!force && (name.isEmpty() || !m_sessionFileRel.isEmpty()))
    return false;

  delete m_writeConfig;
  m_writeConfig = 0;

  delete m_readConfig;
  m_readConfig = 0;

  m_sessionName = name;

  // get a usable filename
  int s = time(0);
  QByteArray tname;
  while (true)
  {
    tname.setNum (s++);
    KMD5 md5 (tname);
    m_sessionFileRel = QString ("%1.katesession").arg (md5.hexDigest().data());

    if (!KGlobal::dirs()->exists(sessionFile ()))
      break;
  }

  // create the file, write name to it!
  KSimpleConfig config (sessionFile ());
  config.setGroup ("General");
  config.writeEntry ("Name", m_sessionName);
  config.sync ();

  // reinit ourselfs ;)
  init ();

  return true;
}

bool KateSession::rename (const QString &name)
{
  if (name.isEmpty () || m_sessionFileRel.isEmpty() || m_sessionFileRel == "default.katesession")
    return false;

  m_sessionName = name;

  KConfig config (sessionFile (), false, false);
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
 , m_activeSession (new KateSession (this, "", ""))
{
  kdDebug() << "LOCAL SESSION DIR: " << m_sessionsDir << endl;

  // create dir if needed
  KGlobal::dirs()->makeDir (m_sessionsDir);
}

KateSessionManager::~KateSessionManager()
{
}

KateSessionManager *KateSessionManager::self()
{
  return KateApp::self()->sessionManager ();
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
    if (KateApp::self()->activeMainWindow())
    {
      if (!KateApp::self()->activeMainWindow()->queryClose_internal())
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
#warning fixme later
    //KTextEditor::Document::setOpenErrorDialogsActivated (false);

    KConfig *sc = activeSession()->configRead();

    if (sc)
      KateApp::self()->documentManager()->restoreDocumentList (sc);

    // window config
    if (sc)
    {
      KConfig *c = KateApp::self()->config();
      c->setGroup("General");

      if (c->readBoolEntry("Restore Window Configuration", true))
      {
        sc->setGroup ("Open MainWindows");
        int wCount = sc->readUnsignedNumEntry("Count", 1);

        for (int i=0; i < wCount; ++i)
        {
          if (i >= KateApp::self()->mainWindows())
          {
            KateApp::self()->newMainWindow(sc, QString ("MainWindow%1").arg(i));
          }
          else
          {
            sc->setGroup(QString ("MainWindow%1").arg(i));
            KateApp::self()->mainWindow(i)->readProperties (sc);
          }
        }

        if (wCount > 0)
        {
          while (wCount < KateApp::self()->mainWindows())
          {
            KateMainWindow *w = KateApp::self()->mainWindow(KateApp::self()->mainWindows()-1);
            KateApp::self()->removeMainWindow (w);
            delete w;
          }
        }
      }
    }

#warning fixme later
    //KTextEditor::Document::setOpenErrorDialogsActivated (true);
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
    
  updateSessionList();

  for (int i=0; i < m_sessionList.count(); ++i)
  {
    if (m_sessionList[i]->sessionName() == name)
      return m_sessionList[i];
  }

  return createSession (name);
}

bool KateSessionManager::saveActiveSession (bool tryAsk, bool rememberAsLast)
{
  if (tryAsk)
  {
    // app config
    KConfig *c = KateApp::self()->config();
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
                              i18n("Do not ask again"), &dontAgain, KMessageBox::Notify);

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

  sc->setGroup ("Open MainWindows");
  sc->writeEntry ("Count", KateApp::self()->mainWindows ());

  KatePluginManager::self()->storeGeneralConfig(sc);

  // save config for all windows around ;)
  for (int i=0; i < KateApp::self()->mainWindows (); ++i )
  {
    sc->setGroup(QString ("MainWindow%1").arg(i));
    KateApp::self()->mainWindow(i)->saveProperties (sc);
    KatePluginManager::self()->storeViewConfig(sc,i);
  }

  sc->sync();

  if (rememberAsLast)
  {
    KConfig *c = KateApp::self()->config();
    c->setGroup("General");
    c->writeEntry ("Last Session", activeSession()->sessionFileRelative());
    c->sync ();
  }

  return true;
}

bool KateSessionManager::chooseSession ()
{
  bool success = true;
  
  // app config
  KConfig *c = KateApp::self()->config();
  c->setGroup("General");

  // get last used session, default to default session
  QString lastSession (c->readEntry ("Last Session", "default.katesession"));
  QString sesStart (c->readEntry ("Startup Session", "manual"));

  // uhh, just open last used session, show no chooser
  if (sesStart == "last")
  {
    activateSession (new KateSession (this, lastSession, ""), false, false);
    return success;
  }

  // start with empty new session
  if (sesStart == "new")
  {
    activateSession (new KateSession (this, "", ""), false, false);
    return success;
  }

  QList<KateSessionChooserTemplate> templates;
  templates.append(KateSessionChooserTemplate("Profile 1 (default)","blah.desktop","<img source=\"/home/jowenn/development/kde/binary/share/icons/hicolor/32x32/actions/show_side_panel.png\"><b>Test 1</b> adfafsdfdsf<br>asdfasdfsdfsdf adsfad adf asdf df<br> adfasdf jasdlfj�"));
  templates.append(KateSessionChooserTemplate("Profile 2","blah.desktop"," Test 2"));
  templates.append(KateSessionChooserTemplate("Profile 3","blah.desktop"," Test 3"));
  KateSessionChooser *chooser = new KateSessionChooser (0, lastSession,templates);

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
          KMessageBox::error (chooser, i18n("No session selected to open."), i18n ("No Session Selected"));
          break;
        }

        activateSession (s, false, false);
        retry = false;
        break;
      }

      // exit the app lateron
      case KateSessionChooser::resultQuit:
        success = false;
        retry = false;
        break;

      default:
        activateSession (new KateSession (this, "", ""), false, false);
        retry = false;
        break;
    }
  }

  // write back our nice boolean :)
  if (success && chooser->reopenLastSession ())
  {
    c->setGroup("General");

    if (res == KateSessionChooser::resultOpen)
      c->writeEntry ("Startup Session", "last");
    else if (res == KateSessionChooser::resultNew)
      c->writeEntry ("Startup Session", "new");

    c->sync ();
  }

  delete chooser;

  return success;
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
  QString name = KInputDialog::getText (i18n("Specify Name for Current Session"), i18n("Session name:"), "", &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a new session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  activeSession()->create (name);
  saveActiveSession ();
}

void KateSessionManager::sessionSaveAs ()
{
  bool ok = false;
  QString name = KInputDialog::getText (i18n("Specify New Name for Current Session"), i18n("Session name:"), "", &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  activeSession()->create (name, true);
  saveActiveSession ();
}


void KateSessionManager::sessionManage ()
{
  KateSessionManageDialog *dlg = new KateSessionManageDialog (0);

  dlg->exec ();

  delete dlg;
}

//BEGIN CHOOSER DIALOG

class KateSessionChooserItem : public Q3ListViewItem
{
  public:
    KateSessionChooserItem (KListView *lv, KateSession::Ptr s)
     : Q3ListViewItem (lv, s->sessionName())
     , session (s)
    {
      QString docs;
      docs.setNum (s->documents());
      setText (1, docs);
    }

    KateSession::Ptr session;
};

KateSessionChooser::KateSessionChooser (QWidget *parent, const QString &lastSession,const QList<KateSessionChooserTemplate> &templates)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Session Chooser")
                  , KDialogBase::User1 | KDialogBase::User2 | KDialogBase::User3
                  , KDialogBase::User2
                  , true
                  , KStdGuiItem::quit ()
                  , KGuiItem (i18n ("Open Session"), "fileopen")
                  , KGuiItem ((templates.count()>1)?i18n ("New Session (hold down for template)"):i18n ("New Session"), "filenew")
                ),m_templates(templates)
{
  m_delayTimer=new QTimer(this);
  m_delayTimer->setSingleShot(true);
  int delay=style()->styleHint(QStyle::SH_ToolButton_PopupDelay, 0, this);
  m_delayTimer->setInterval((delay<=0) ? 150:delay);
  QFrame *page = new QFrame (this);
  QHBoxLayout *tll=new QHBoxLayout(page);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBoxLayout *hb = new QHBoxLayout ();
  hb->setSpacing (KDialog::spacingHint());
  tll->addItem(hb);

  QLabel *label = new QLabel (page);
  hb->addWidget(label);
  label->setPixmap (UserIcon("sessionchooser"));
  label->setFrameStyle (QFrame::Panel | QFrame::Sunken);
  label->setAlignment(Qt::AlignTop);
  QVBoxLayout *vb = new QVBoxLayout ();
  vb->setSpacing (KDialog::spacingHint());
  tll->addItem(vb);
  
  m_sessions = new KListView (page);
  vb->addWidget(m_sessions);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (Q3ListView::AllColumns);
  m_sessions->setSelectionMode (Q3ListView::Single);
  m_sessions->setAllColumnsShowFocus (true);
  
  connect (m_sessions, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i=0; i < slist.count(); ++i)
  {
    KateSessionChooserItem *item = new KateSessionChooserItem (m_sessions, slist[i]);

    if (slist[i]->sessionFileRelative() == lastSession)
      m_sessions->setSelected (item, true);
  }

  m_useLast = new QCheckBox (i18n ("&Always use this choice"), page);
  vb->addWidget(m_useLast);

  setResult (resultNone);

  connect(actionButton(KDialogBase::User3),SIGNAL(pressed()),m_delayTimer,SLOT(start()));
  connect(m_delayTimer,SIGNAL(timeout()),this,SLOT(slotProfilePopup()));
  // trigger action update
  selectionChanged ();
}

KateSessionChooser::~KateSessionChooser ()
{
}

void KateSessionChooser::slotProfilePopup() {
 KateToolTipMenu popup;
 bool defaultA=true;
 foreach(const KateSessionChooserTemplate &t,m_templates) {
   QAction *a=popup.addAction(t.displayName);
   if (defaultA) popup.setDefaultAction(a);
   defaultA=false;
   a->setToolTip(t.toolTip);
   a->setData(t.configFileName);
 }
 connect(&popup,SIGNAL(triggered(QAction *)),this,SLOT(slotTemplateAction(QAction*)));
 actionButton(KDialogBase::User3)->setMenu(&popup);
 actionButton(KDialogBase::User3)->showMenu();
 actionButton(KDialogBase::User3)->setMenu(0);
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

void KateSessionChooser::slotUser2 ()
{
  done (resultOpen);
}

void KateSessionChooser::slotUser3 ()
{
  m_delayTimer->stop();
  if (m_templates.count()>0) m_selectedTemplate=m_templates[0].configFileName;
  done (resultNew);
}
void KateSessionChooser::slotTemplateAction(QAction* a)
{
  m_selectedTemplate=a->data().toString();
  done (resultNew);
}

void KateSessionChooser::slotUser1 ()
{
  done (resultQuit);
}

void KateSessionChooser::selectionChanged ()
{
  enableButton (KDialogBase::User2, m_sessions->selectedItem ());
}

//END CHOOSER DIALOG

//BEGIN OPEN DIALOG

KateSessionOpenDialog::KateSessionOpenDialog (QWidget *parent)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Open Session")
                  , KDialogBase::User1 | KDialogBase::User2
                  , KDialogBase::User2
                  , false
                  , KStdGuiItem::cancel ()
                  , KGuiItem( i18n("&Open"), "fileopen")
                )
{
  QFrame *page = new QFrame (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBoxLayout *hb = new QHBoxLayout (page);
  
  QVBoxLayout *vb = new QVBoxLayout ();
  hb->addItem(vb);
  m_sessions = new KListView (this);
  vb->addWidget(m_sessions);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (Q3ListView::AllColumns);
  m_sessions->setSelectionMode (Q3ListView::Single);
  m_sessions->setAllColumnsShowFocus (true);

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i=0; i < slist.count(); ++i)
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

//END OPEN DIALOG

//BEGIN MANAGE DIALOG

KateSessionManageDialog::KateSessionManageDialog (QWidget *parent)
 : KDialogBase (  parent
                  , ""
                  , true
                  , i18n ("Manage Sessions")
                  , KDialogBase::User1
                  , KDialogBase::User1
                  , false
                  , KStdGuiItem::close ()
                )
{
  QFrame *page = new QFrame (this);
  page->setMinimumSize (400, 200);
  setMainWidget(page);

  QHBoxLayout *hb = new QHBoxLayout (page);
  hb->setSpacing (KDialog::spacingHint());

  m_sessions = new KListView (page);
  hb->addWidget(m_sessions);
  m_sessions->addColumn (i18n("Session Name"));
  m_sessions->addColumn (i18n("Open Documents"));
  m_sessions->setResizeMode (Q3ListView::AllColumns);
  m_sessions->setSelectionMode (Q3ListView::Single);
  m_sessions->setAllColumnsShowFocus (true);

  connect (m_sessions, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));

  updateSessionList ();

  QVBoxLayout *vb = new QVBoxLayout ();
  hb->addItem(vb);
  vb->setSpacing (KDialog::spacingHint());

  m_rename = new KPushButton (i18n("&Rename..."), page);
  connect (m_rename, SIGNAL(clicked()), this, SLOT(rename()));
  vb->addWidget (m_rename);

  m_del = new KPushButton (KStdGuiItem::del (), page);
  connect (m_del, SIGNAL(clicked()), this, SLOT(del()));
  vb->addWidget (m_del);

  vb->addStretch ();

  // trigger action update
  selectionChanged ();
}

KateSessionManageDialog::~KateSessionManageDialog ()
{
}

void KateSessionManageDialog::slotUser1 ()
{
  done (0);
}


void KateSessionManageDialog::selectionChanged ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->selectedItem ();

  m_rename->setEnabled (item && item->session->sessionFileRelative() != "default.katesession");
  m_del->setEnabled (item && item->session->sessionFileRelative() != "default.katesession");
}

void KateSessionManageDialog::rename ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item || item->session->sessionFileRelative() == "default.katesession")
    return;

  bool ok = false;
  QString name = KInputDialog::getText (i18n("Specify New Name for Session"), i18n("Session name:"), item->session->sessionName(), &ok);

  if (!ok)
    return;

  if (name.isEmpty())
  {
    KMessageBox::error (0, i18n("To save a session, you must specify a name."), i18n ("Missing Session Name"));
    return;
  }

  item->session->rename (name);
  updateSessionList ();
}

void KateSessionManageDialog::del ()
{
  KateSessionChooserItem *item = (KateSessionChooserItem *) m_sessions->selectedItem ();

  if (!item || item->session->sessionFileRelative() == "default.katesession")
    return;

  QFile::remove (item->session->sessionFile());
  KateSessionManager::self()->updateSessionList ();
  updateSessionList ();
}

void KateSessionManageDialog::updateSessionList ()
{
  m_sessions->clear ();

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i=0; i < slist.count(); ++i)
  {
    new KateSessionChooserItem (m_sessions, slist[i]);
  }
}

//END MANAGE DIALOG


KateSessionsAction::KateSessionsAction(const QString& text, KActionCollection* parent, const char* name )
  : KActionMenu(text, parent, name)
{
  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateSessionsAction::slotAboutToShow()
{
  popupMenu()->clear ();

  KateSessionList &slist (KateSessionManager::self()->sessionList());
  for (int i=0; i < slist.count(); ++i)
  {
      popupMenu()->insertItem (
          slist[i]->sessionName(),
          this, SLOT (openSession (int)), 0,
          i );
  }
}

void KateSessionsAction::openSession (int i)
{
  KateSessionList &slist (KateSessionManager::self()->sessionList());

  if (i >= slist.count())
    return;

  KateSessionManager::self()->activateSession(slist[(uint)i]);
}
