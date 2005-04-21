/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

//BEGIN Includes
#include "katemainwindow.h"
#include "katemainwindow.moc"

#include "kateconfigdialog.h"
#include "kateconsole.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katefileselector.h"
#include "katefilelist.h"
#include "kategrepdialog.h"
#include "katemailfilesdialog.h"
#include "katemainwindowiface.h"
#include "kateexternaltools.h"
#include "katesavemodifieddialog.h"
#include "katemwmodonhddialog.h"
#include "katesession.h"

#include <kmdi/tabwidget.h>

#include <dcopclient.h>
#include <kinstance.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kdiroperator.h>
#include <kdockwidget.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kglobalaccel.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kopenwith.h>
#include <kpopupmenu.h>
#include <ksimpleconfig.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <ktrader.h>
#include <kuniqueapplication.h>
#include <kurldrag.h>
#include <kdesktopfile.h>
#include <khelpmenu.h>
#include <kmultitabbar.h>
#include <ktip.h>
#include <kmenubar.h>
#include <kstringhandler.h>
#include <qlayout.h>
#include <qptrvector.h>

#include <assert.h>
#include <unistd.h>
//END

uint KateMainWindow::uniqueID = 1;

KateMainWindow::KateMainWindow (KConfig *sconfig, const QString &sgroup)
  : KateMDI::MainWindow (0,(QString("__KateMainWindow#%1").arg(uniqueID)).latin1())
{
 // setToolViewStyle(KMultiTabBar::KDEV3ICON);
  // make the dockwidgets keep their size if possible
  //manager()->setSplitterKeepSize(true);
  // first the very important id
  myID = uniqueID;
  uniqueID++;

  m_modignore = false;

  console = 0;
  greptool = 0;

  // now the config
  KConfig *config = kapp->config();

  // first init size while we are still invisible, avoid flicker
  if (!initialGeometrySet())
  {
    config->setGroup ("Kate Main Window");
    int scnum = QApplication::desktop()->screenNumber(parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);
    QSize s ( config->readNumEntry( QString::fromLatin1("Width %1").arg(desk.width()), 700 ),
              config->readNumEntry( QString::fromLatin1("Height %1").arg(desk.height()), 480 ) );

    resize (kMin (s.width(), desk.width()), kMin(s.height(), desk.height()));
  }

  // start session restore if needed
  startRestore (sconfig, sgroup);

  m_mainWindow = new Kate::MainWindow (this);
  m_toolViewManager = new Kate::ToolViewManager (this);

  m_dcop = new KateMainWindowDCOPIface (this);

  // setup the most important widgets
  setupMainWindow();

  // setup the actions
  setupActions();

  setStandardToolBarMenuEnabled( true );
  setXMLFile( "kateui.rc" );
  createShellGUI ( true );

  KatePluginManager::self()->enableAllPluginsGUI (this);

  if ( kapp->authorize("shell_access") )
    Kate::Document::registerCommand(KateExternalToolsCommand::self());

  // connect documents menu aboutToshow
  documentMenu = (QPopupMenu*)factory()->container("documents", this);
  connect(documentMenu, SIGNAL(aboutToShow()), this, SLOT(documentMenuAboutToShow()));

  // caption update
  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
    slotDocumentCreated (KateDocManager::self()->document(i));

  connect(KateDocManager::self(),SIGNAL(documentCreated(Kate::Document *)),this,SLOT(slotDocumentCreated(Kate::Document *)));

  readOptions(config);

  if (console)
    console->loadConsoleIfNeeded();

  setAutoSaveSettings ("Kate Main Window");

  finishRestore ();

  if (sconfig)
    m_viewManager->restoreViewConfiguration (sconfig, sgroup);

  setAcceptDrops(true);
}

KateMainWindow::~KateMainWindow()
{
  saveOptions(kapp->config());

  ((KateApp *)kapp)->removeMainWindow (this);

  KatePluginManager::self()->disableAllPluginsGUI (this);

  delete m_dcop;
  delete kscript;
}

void KateMainWindow::setupMainWindow ()
{
  m_viewManager = new KateViewManager (this);

  KateMDI::ToolView *t = createToolView("kate_filelist", KMultiTabBar::Left, SmallIcon("kmultiple"), i18n("Documents"));
  filelist = new KateFileList (this, m_viewManager, t, "filelist");
  filelist->readConfig(kapp->config(), "Filelist");

  t = createToolView("kate_fileselector", KMultiTabBar::Left, SmallIcon("fileopen"), i18n("Filesystem Browser"));
  fileselector = new KateFileSelector( this, m_viewManager, t, "operator");
  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));

  // ONLY ALLOW SHELL ACCESS IF ALLOWED ;)
  if (kapp->authorize("shell_access"))
  {
    t = createToolView("kate_greptool", KMultiTabBar::Bottom, SmallIcon("filefind"), i18n("Find in Files") );
    greptool = new GrepTool( this, t, "greptool" );
    greptool->installEventFilter( this );
    connect(greptool, SIGNAL(itemSelected(const QString &,int)), this, SLOT(slotGrepToolItemSelected(const QString &,int)));
    // WARNING HACK - anders: showing the greptool seems to make the menu accels work
    greptool->show();

    t = createToolView("kate_console", KMultiTabBar::Bottom, SmallIcon("konsole"), i18n("Terminal"));
    console = new KateConsole (t, "console",viewManager());
    console->installEventFilter( this );
  }
}

void KateMainWindow::setupActions()
{
  KAction *a;

  kscript = new KScriptManager(this, "scriptmanager");
  scriptMenu = new KActionMenu( i18n("&KDE Scripts"), actionCollection(), "scripts");
  scriptMenu->setWhatsThis(i18n("This shows all available scripts and allows them to be executed."));
  setupScripts();
  connect( scriptMenu->popupMenu(), SIGNAL(activated( int)), this, SLOT(runScript( int )) );

  KStdAction::openNew( m_viewManager, SLOT( slotDocumentNew() ), actionCollection(), "file_new" )->setWhatsThis(i18n("Create a new document"));
  KStdAction::open( m_viewManager, SLOT( slotDocumentOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Open an existing document for editing"));

  fileOpenRecent = KStdAction::openRecent (m_viewManager, SLOT(openURL (const KURL&)), actionCollection());
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=new KAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, KateDocManager::self(), SLOT( saveAll() ), actionCollection(), "file_save_all" );
  a->setWhatsThis(i18n("Save all open, modified documents to disk."));

  KStdAction::close( m_viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" )->setWhatsThis(i18n("Close the current document."));

  a=new KAction( i18n( "Clos&e All" ), 0, this, SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );
  a->setWhatsThis(i18n("Close all open documents."));

  KStdAction::mail( this, SLOT(slotMail()), actionCollection() )->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" )->setWhatsThis(i18n("Close this window"));

  a=new KAction(i18n("&New Window"), "window_new", 0, this, SLOT(newWindow()), actionCollection(), "view_new_view");
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  if ( kapp->authorize("shell_access") )
  {
    externalTools = new KateExternalToolsMenuAction( i18n("External Tools"), actionCollection(), "tools_external", this );
    externalTools->setWhatsThis( i18n("Launch external helper applications") );
  }

  showFullScreenAction = KStdAction::fullScreen( 0, 0, actionCollection(),this);
  connect( showFullScreenAction,SIGNAL(toggled(bool)), this,SLOT(slotFullScreen(bool)));

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), actionCollection(), "file_open_with");
  documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
  connect(documentOpenWith->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotOpenWithMenuAction(int)));

  a=KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");
  settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

  // pipe to terminal action
  if (kapp->authorize("shell_access"))
    new KAction(i18n("&Pipe to Console"), "pipe", 0, this, SLOT(slotPipeToConsole()), actionCollection(), "tools_pipe_to_terminal");

  // tip of the day :-)
  KStdAction::tipOfDay( this, SLOT( tipOfTheDay() ), actionCollection() )->setWhatsThis(i18n("This shows useful tips on the use of this application."));

  if (KatePluginManager::self()->pluginList().count() > 0)
  {
    a=new KAction(i18n("&Plugins Handbook"), 0, this, SLOT(pluginHelp()), actionCollection(), "help_plugins_contents");
    a->setWhatsThis(i18n("This shows help files for various available plugins."));
  }

  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));
  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotUpdateOpenWith()));

  slotWindowActivated ();

  // session actions
  new KAction(i18n("&New"), "filenew", 0, KateSessionManager::self(), SLOT(sessionNew()), actionCollection(), "sessions_new");
  new KAction(i18n("&Open..."), "fileopen", 0, KateSessionManager::self(), SLOT(sessionOpen()), actionCollection(), "sessions_open");
  new KAction(i18n("&Save"), "filesave", 0, KateSessionManager::self(), SLOT(sessionSave()), actionCollection(), "sessions_save");
  new KAction(i18n("Save &As..."), "filesaveas", 0, KateSessionManager::self(), SLOT(sessionSaveAs()), actionCollection(), "sessions_save_as");
  new KAction(i18n("&Manage..."), "view_choose", 0, KateSessionManager::self(), SLOT(sessionManage()), actionCollection(), "sessions_manage");

  // quick open menu ;)
  new KateSessionsAction (i18n("&Quick Open"), actionCollection(), "sessions_list");
}

void KateMainWindow::slotDocumentCloseAll() {
  if (queryClose_internal())
    KateDocManager::self()->closeAllDocuments(false);
}

bool KateMainWindow::queryClose_internal() {
   uint documentCount=KateDocManager::self()->documents();

  if ( ! showModOnDiskPrompt() )
    return false;

  QPtrList<Kate::Document> modifiedDocuments=KateDocManager::self()->modifiedDocumentList();
  bool shutdown=(modifiedDocuments.count()==0);

  if (!shutdown) {
    shutdown=KateSaveModifiedDialog::queryClose(this,modifiedDocuments);
  }

  if ( KateDocManager::self()->documents() > documentCount ) {
    KMessageBox::information (this,
                              i18n ("New file opened while trying to close Kate, closing aborted."),
                              i18n ("Closing Aborted"));
    shutdown=false;
  }

  return shutdown;
}

/**
 * queryClose(), take care that after the last mainwindow the stuff is closed
 */
bool KateMainWindow::queryClose()
{
  // session saving, can we close all views ?
  // just test, not close them actually
  if (kapp->sessionSaving())
  {
    return queryClose_internal ();
  }

  // normal closing of window
  // allow to close all windows until the last without restrictions
  if ( ((KateApp *)kapp)->mainWindows () > 1 )
    return true;

  // last one: check if we can close all documents, try run
  // and save docs if we really close down !
  if ( queryClose_internal () )
  {
    ((KateApp *)kapp)->kateSessionManager()->saveActiveSession(true, true);

    // detach the dcopClient
    ((KUniqueApplication *)kapp)->dcopClient()->detach();

    return true;
  }

  return false;
}

void KateMainWindow::newWindow ()
{
  ((KateApp *)kapp)->newMainWindow ();
}

void KateMainWindow::slotEditToolbars()
{
  KEditToolbar dlg( factory() );

  dlg.exec();
}

void KateMainWindow::slotFileQuit()
{
  ((KateApp *)kapp)->shutdownKate (this);
}

void KateMainWindow::readOptions(KConfig *config)
{
  config->setGroup("General");
  syncKonsole =  config->readBoolEntry("Sync Konsole", true);
  modNotification = config->readBoolEntry("Modified Notification", false);
  KateDocManager::self()->setSaveMetaInfos(config->readBoolEntry("Save Meta Infos", true));
  KateDocManager::self()->setDaysMetaInfos(config->readNumEntry("Days Meta Infos", 30));

  m_viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));

  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");
}

void KateMainWindow::saveOptions(KConfig *config)
{
  config->setGroup("General");

  if (console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);

  config->writeEntry("Save Meta Infos", KateDocManager::self()->getSaveMetaInfos());

  config->writeEntry("Days Meta Infos", KateDocManager::self()->getDaysMetaInfos());

  config->writeEntry("Show Full Path in Title", m_viewManager->getShowFullPath());

  config->writeEntry("Sync Konsole", syncKonsole);

  fileOpenRecent->saveEntries(config, "Recent Files");

  fileselector->writeConfig(config, "fileselector");

  filelist->writeConfig(config, "Filelist");
}

void KateMainWindow::slotWindowActivated ()
{
  static QString path;

  if (m_viewManager->activeView())
  {
    if (console && syncKonsole)
    {
      QString newPath = m_viewManager->activeView()->getDoc()->url().directory();

      if ( newPath != path )
      {
        path = newPath;
        console->cd (KURL( path ));
      }
    }

    updateCaption (m_viewManager->activeView()->getDoc());
  }
}

void KateMainWindow::slotUpdateOpenWith()
{
  if (m_viewManager->activeView())
    documentOpenWith->setEnabled(!m_viewManager->activeView()->document()->url().isEmpty());
  else
    documentOpenWith->setEnabled(false);
}

void KateMainWindow::documentMenuAboutToShow()
{
  // remove documents
  while (documentMenu->count() > 3)
    documentMenu->removeItemAt (3);

  QListViewItem * item = filelist->firstChild();
  while( item ) {
    documentMenu->insertItem (
          KStringHandler::rsqueeze( ((KateFileListItem *)item)->document()->docName(), 150 ), // would it be saner to use the screen width as a limit that some random number??
          m_viewManager, SLOT (activateView (int)), 0,
          ((KateFileListItem *)item)->documentNumber () );

    item = item->nextSibling();
  }
  if (m_viewManager->activeView())
    documentMenu->setItemChecked ( m_viewManager->activeView()->getDoc()->documentNumber(), true);
}

void KateMainWindow::slotGrepToolItemSelected(const QString &filename,int linenumber)
{
  KURL fileURL;
  fileURL.setPath( filename );
  m_viewManager->openURL( fileURL );
  if ( m_viewManager->activeView() == 0 ) return;
  m_viewManager->activeView()->gotoLineNumber( linenumber );
  raise();
  setActiveWindow();
}

void KateMainWindow::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(KURLDrag::canDecode(event));
}

void KateMainWindow::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void KateMainWindow::slotDropEvent( QDropEvent * event )
{
  KURL::List textlist;
  if (!KURLDrag::decode(event, textlist)) return;

  for (KURL::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
  {
    m_viewManager->openURL (*i);
  }
}

void KateMainWindow::editKeys()
{
  KKeyDialog dlg ( false, this );

  QPtrList<KXMLGUIClient> clients = guiFactory()->clients();

  for( QPtrListIterator<KXMLGUIClient> it( clients ); it.current(); ++it )
    dlg.insert ( (*it)->actionCollection(), (*it)->instance()->aboutData()->programName() );

  dlg.insert( externalTools->actionCollection(), i18n("External Tools") );

  dlg.configure();

  QPtrList<Kate::Document>  l=KateDocManager::self()->documentList();
  for (uint i=0;i<l.count();i++) {
//     kdDebug(13001)<<"reloading Keysettings for document "<<i<<endl;
    l.at(i)->reloadXML();
    QPtrList<class KTextEditor::View> l1=l.at(i)->views ();//KTextEditor::Document
    for (uint i1=0;i1<l1.count();i1++) {
      l1.at(i1)->reloadXML();
//       kdDebug(13001)<<"reloading Keysettings for view "<<i<<"/"<<i1<<endl;
    }
  }

  externalTools->actionCollection()->writeShortcutSettings( "Shortcuts", new KConfig("externaltools", false, false, "appdata") );
}

void KateMainWindow::openURL (const QString &name)
{
  m_viewManager->openURL (KURL(name));
}

void KateMainWindow::slotConfigure()
{
  if (!m_viewManager->activeView())
    return;

  KateConfigDialog* dlg = new KateConfigDialog (this, m_viewManager->activeView());
  dlg->exec();

  delete dlg;
}

KURL KateMainWindow::activeDocumentUrl()
{
  // anders: i make this one safe, as it may be called during
  // startup (by the file selector)
  Kate::View *v = m_viewManager->activeView();
  if ( v )
    return v->getDoc()->url();
  return KURL();
}

void KateMainWindow::fileSelected(const KFileItem * /*file*/)
{
  const KFileItemList *list=fileselector->dirOperator()->selectedItems();
  KFileItem *tmp;
  for (KFileItemListIterator it(*list); (tmp = it.current()); ++it)
  {
    m_viewManager->openURL(tmp->url());
    fileselector->dirOperator()->view()->setSelected(tmp,false);
  }
}

// TODO make this work
void KateMainWindow::mSlotFixOpenWithMenu()
{
  //kdDebug(13001)<<"13000"<<"fixing open with menu"<<endl;
  documentOpenWith->popupMenu()->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->getDoc()->url() );
  //kdDebug(13001)<<"13000"<<"url: "<<m_viewManager->activeView()->getDoc()->url().prettyURL()<<"mime type: "<<mime->name()<<endl;
  // some checking goes here...
  KTrader::OfferList offers = KTrader::self()->query(mime->name(), "Type == 'Application'");
  // for each one, insert a menu item...
  for(KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it) {
    if ((*it)->name() == "Kate") continue;
    documentOpenWith->popupMenu()->insertItem( SmallIcon( (*it)->icon() ), (*it)->name() );
  }
  // append "Other..." to call the KDE "open with" dialog.
  documentOpenWith->popupMenu()->insertItem(i18n("&Other..."));
}

void KateMainWindow::slotOpenWithMenuAction(int idx)
{
  KURL::List list;
  list.append( m_viewManager->activeView()->getDoc()->url() );
  QString* appname = new QString( documentOpenWith->popupMenu()->text(idx) );
  if ( appname->compare(i18n("&Other...")) == 0 ) {
    // display "open with" dialog
    KOpenWithDlg* dlg = new KOpenWithDlg(list);
    if (dlg->exec())
      KRun::run(*dlg->service(), list);
    return;
  }
  QString qry = QString("((Type == 'Application') and (Name == '%1'))").arg( appname->latin1() );
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->getDoc()->url() );
  KTrader::OfferList offers = KTrader::self()->query(mime->name(), qry);
  KService::Ptr app = offers.first();
  // some checking here: pop a wacko message it the app wasn't found.
  KRun::run(*app, list);
}

void KateMainWindow::pluginHelp()
{
  kapp->invokeHelp (QString::null, "kate-plugins");
}

void KateMainWindow::setupScripts()
{
  // locate all scripts, local as well as global.
  // The script manager will do the nessecary sanity checking
  QStringList scripts = KGlobal::dirs()->findAllResources("data", QString(kapp->name())+"/scripts/*.desktop", false, true );
  for (QStringList::Iterator it = scripts.begin(); it != scripts.end(); ++it )
    kscript->addScript( *it );
  QStringList l ( kscript->scripts() );
  for (QStringList::Iterator it=l.begin(); it != l.end(); ++it )
    scriptMenu->popupMenu()->insertItem( *it );
}

void KateMainWindow::runScript( int mIId )
{
//  kdDebug(13001) << "Starting script engine..." << endl;
//  kdDebug(13001)<<"runScript( "<<mIId<<" ) ["<<scriptMenu->popupMenu()->text( mIId )<<"]"<<endl;
  kscript->runScript( scriptMenu->popupMenu()->text( mIId ) );
}

void KateMainWindow::slotMail()
{
  KateMailDialog *d = new KateMailDialog(this, this);
  if ( ! d->exec() )
    return;
  QPtrList<Kate::Document> attDocs = d->selectedDocs();
  delete d;
  // Check that all selected files are saved (or shouldn't be)
  QStringList urls; // to atthatch
  Kate::Document *doc;
  QPtrListIterator<Kate::Document> it(attDocs);
  for ( ; it.current(); ++it ) {
    doc = it.current();
    if (!doc) continue;
    if ( doc->url().isEmpty() ) {
      // unsaved document. back out unless it gets saved
      int r = KMessageBox::questionYesNo( this,
              i18n("<p>The current document has not been saved, and "
              "cannot be attached to an email message."
              "<p>Do you want to save it and proceed?"),
              i18n("Cannot Send Unsaved File") );
      if ( r == KMessageBox::Yes ) {
        Kate::View *v = (Kate::View*)doc->views().first();
        int sr = v->saveAs();
        if ( sr == Kate::View::SAVE_OK ) { ;
        }
        else {
          if ( sr != Kate::View::SAVE_CANCEL ) // ERROR or RETRY(?)
            KMessageBox::sorry( this, i18n("The file could not be saved. Please check "
                                        "if you have write permission.") );
          continue;
        }
      }
      else
        continue;
    }
    if ( doc->isModified() ) {
      // warn that document is modified and offer to save it before proceeding.
      int r = KMessageBox::warningYesNoCancel( this,
                i18n("<p>The current file:<br><strong>%1</strong><br>has been "
                "modified. Modifications will not be available in the attachment."
                "<p>Do you want to save it before sending it?").arg(doc->url().prettyURL()),
                i18n("Save Before Sending?") );
      switch ( r ) {
        case KMessageBox::Cancel:
          continue;
        case KMessageBox::Yes:
          doc->save();
          if ( doc->isModified() ) { // read-only docs ends here, if modified. Hmm.
            KMessageBox::sorry( this, i18n("The file could not be saved. Please check "
                                      "if you have write permission.") );
            continue;
          }
          break;
        default:
          break;
      }
    }
    // finally call the mailer
    urls << doc->url().url();
  } // check selected docs done
  if ( ! urls.count() )
    return;
  kapp->invokeMailer( QString::null, // to
                      QString::null, // cc
                      QString::null, // bcc
                      QString::null, // subject
                      QString::null, // body
                      QString::null, // msgfile
                      urls           // urls to atthatch
                      );
}
void KateMainWindow::tipOfTheDay()
{
  KTipDialog::showTip( /*0*/this, QString::null, true );
}

void KateMainWindow::slotFullScreen(bool t)
{
  if (t)
    showFullScreen();
  else
    showNormal();
}

bool KateMainWindow::eventFilter( QObject *o, QEvent *e )
{

  if ( o == greptool && e->type() == QEvent::Show && m_viewManager->activeView() )
  {
    if ( m_viewManager->activeView()->getDoc()->url().isLocalFile() )
    {
      greptool->updateDirName( m_viewManager->activeView()->getDoc()->url().directory() );
      return true;
    }
  }
  if ( ( o == greptool || o == console ) &&
      e->type() == QEvent::Hide && m_viewManager->activeView() )
  {
     m_viewManager->activeView()->setFocus();
     return true;
  }

  return KateMDI::MainWindow::eventFilter( o, e );
}

bool KateMainWindow::event( QEvent *e )
{
  uint type = e->type();
  if ( type == QEvent::WindowActivate && modNotification )
  {
    if ( m_modignore )
    {
      m_modignore = false;
      return KateMDI::MainWindow::event( e );
    }
    showModOnDiskPrompt();
  }
  // Try to disable the modonhd prompt from showing after internal dialogs.
  // TODO make this work better.
  else if ( (type == QEvent::WindowUnblocked || type == QEvent::WindowBlocked) && modNotification)
    m_modignore = true;

  return KateMDI::MainWindow::event( e );
}

bool KateMainWindow::showModOnDiskPrompt()
{
  Kate::Document *doc;

  DocVector list( KateDocManager::self()->documents() );
  uint cnt = 0;
  for( doc = KateDocManager::self()->firstDocument(); doc; doc = KateDocManager::self()->nextDocument() )
  {
    if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDisc )
    {
      list.insert( cnt, doc );
      cnt++;
    }
  }

  if ( cnt )
  {
    list.resize( cnt );
    KateMwModOnHdDialog mhdlg( list, this );
    bool res = mhdlg.exec();

    return res;
  }
  return true;
}

void KateMainWindow::slotDocumentCreated (Kate::Document *doc)
{
  connect(doc,SIGNAL(modStateChanged(Kate::Document *)),this,SLOT(updateCaption(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(updateCaption(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(slotUpdateOpenWith()));

  updateCaption (doc);
}

void KateMainWindow::updateCaption (Kate::Document *doc)
{
  if (!m_viewManager->activeView())
  {
    setCaption ("", false);
    return;
  }

  if (!(m_viewManager->activeView()->getDoc() == doc))
    return;

  QString c;
  if (m_viewManager->activeView()->getDoc()->url().isEmpty() || (!m_viewManager->getShowFullPath()))
  {
    c = m_viewManager->activeView()->getDoc()->docName();
  }
  else
  {
    c = m_viewManager->activeView()->getDoc()->url().prettyURL();
  }

  setCaption( KStringHandler::lsqueeze(c,64), m_viewManager->activeView()->getDoc()->isModified());
}

void KateMainWindow::saveProperties(KConfig *config)
{
  QString grp=config->group();

  saveSession(config, grp);
  m_viewManager->saveViewConfiguration (config, grp);

  config->setGroup(grp);
}

void KateMainWindow::readProperties(KConfig *config)
{
  QString grp=config->group();

  startRestore(config, grp);
  finishRestore ();
  m_viewManager->restoreViewConfiguration (config, grp);

  config->setGroup(grp);
}

void KateMainWindow::saveGlobalProperties( KConfig* sessionConfig )
{
  KateDocManager::self()->saveDocumentList (sessionConfig);

  sessionConfig->setGroup("General");
  sessionConfig->writeEntry ("Last Session", ((KateApp *)kapp)->kateSessionManager()->activeSession()->sessionFileRelative());
}

void KateMainWindow::slotPipeToConsole ()
{
  if (!console)
    return;

  if (KMessageBox::warningYesNo
       (this
        , i18n ("Do you really want to pipe the text to the console? This will execute any contained commands with your user rights.")
        , i18n ("Pipe to Console?")
        , KStdGuiItem::yes(), KStdGuiItem::no(), "Pipe To Console Warning") != KMessageBox::Yes)
    return;

  Kate::View *v = m_viewManager->activeView();

  if (!v)
    return;

  if (v->getDoc()->hasSelection ())
    console->sendInput (v->getDoc()->selection());
  else
    console->sendInput (v->getDoc()->text());
}
// kate: space-indent on; indent-width 2; replace-tabs on;
