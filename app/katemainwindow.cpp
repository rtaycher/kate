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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katemainwindow.h"
#include "katemainwindow.moc"

#include "kateconfigdialog.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
//#include "katefileselector.h"
#include "katefilelist.h"
#include "kategrepdialog.h"
#include "katemailfilesdialog.h"
#include "katemainwindowiface.h"
#include "kateexternaltools.h"
#include "katesavemodifieddialog.h"
#include "katemwmodonhddialog.h"
#include "katesession.h"
#include "katetabwidget.h"


#include "../interfaces/mainwindow.h"

#include <kaboutapplication.h>
#include <dcopclient.h>
#include <kinstance.h>
#include <kaboutdata.h>
#include <kaction.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kdiroperator.h>
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
#include <kmenu.h>
#include <ksimpleconfig.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <ktrader.h>
#include <kuniqueapplication.h>
#include <kdesktopfile.h>
#include <khelpmenu.h>
#include <kmultitabbar.h>
#include <ktip.h>
#include <kmenubar.h>
#include <kstringhandler.h>
#include <qlayout.h>
//Added by qt3to4:
#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QList>
#include <QDesktopWidget>

#include <assert.h>
#include <unistd.h>
#include <ktoolinvocation.h>
#include <kmenu.h>
#include <kauthorized.h>
//END

uint KateMainWindow::uniqueID = 1;

KateMainWindow::KateMainWindow (KConfig *sconfig, const QString &sgroup)
  : KateMDI::MainWindow (0,(QString("__KateMainWindow#%1").arg(uniqueID)).toLatin1())
{
  // first the very important id
  myID = uniqueID;
  uniqueID++;

  m_modignore = false;

  console = 0;
  greptool = 0;

  // here we go, set some usable default sizes
  if (!initialGeometrySet())
  {
    int scnum = QApplication::desktop()->screenNumber(parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);

    QSize size;

    // try to load size
    if (sconfig)
    {
      sconfig->setGroup (sgroup);
      size.setWidth (sconfig->readEntry( QString::fromLatin1("Width %1").arg(desk.width()), 0 ));
      size.setHeight (sconfig->readEntry( QString::fromLatin1("Height %1").arg(desk.height()), 0 ));
    }

    // if thats fails, try to reuse size
    if (size.isEmpty())
    {
      // first try to reuse size known from current or last created main window ;=)
      if (KateApp::self()->mainWindows () > 0)
      {
        KateMainWindow *win = KateApp::self()->activeMainWindow ();

        if (!win)
          win = KateApp::self()->mainWindow (KateApp::self()->mainWindows ()-1);

        size = win->size();
      }
      else // now fallback to hard defaults ;)
      {
        // first try global app config
		KGlobal::config()->setGroup ("MainWindow");
        size.setWidth (KGlobal::config()->readEntry( QString::fromLatin1("Width %1").arg(desk.width()), 0 ));
        size.setHeight (KGlobal::config()->readEntry( QString::fromLatin1("Height %1").arg(desk.height()), 0 ));

        if (size.isEmpty())
          size = QSize (qMin (700, desk.width()), qMin(480, desk.height()));
      }

      resize (size);
    }
  }

  // start session restore if needed
  startRestore (sconfig, sgroup);

  m_mainWindow = new Kate::MainWindow (this);

  m_dcop = new KateMainWindowDCOPIface (this);

  // setup the most important widgets
  setupMainWindow();

  // setup the actions
  setupActions();

  setStandardToolBarMenuEnabled( true );
  setXMLFile( "kateui.rc" );
  createShellGUI ( true );

  kDebug()<<"****************************************************************************"<<sconfig<<endl;
  KatePluginManager::self()->enableAllPluginsGUI (this,sconfig);

#ifdef _GNUC__
#warning fixme later
#endif
  /* if ( KateApp::self()->authorize("shell_access") )
    KTextEditor::Document::registerCommand(KateExternalToolsCommand::self());
*/
  // connect documents menu aboutToshow
  documentMenu = (QMenu*)factory()->container("documents", this);
  connect(documentMenu, SIGNAL(aboutToShow()), this, SLOT(documentMenuAboutToShow()));

  documentsGroup = new QActionGroup(documentMenu);
  documentsGroup->setExclusive(true);
  connect(documentsGroup, SIGNAL(triggered(QAction *)), this, SLOT(activateDocumentFromDocMenu(QAction *)));

  // caption update
  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
    slotDocumentCreated (KateDocManager::self()->document(i));

  connect(KateDocManager::self(),SIGNAL(documentCreated(KTextEditor::Document *)),this,SLOT(slotDocumentCreated(KTextEditor::Document *)));

  readOptions();

  if (sconfig)
    m_viewManager->restoreViewConfiguration (sconfig, sgroup);

  finishRestore ();

  setAcceptDrops(true);
}

KateMainWindow::~KateMainWindow()
{
  // first, save our fallback window size ;)
		KGlobal::config()->setGroup ("MainWindow");
  saveWindowSize (KGlobal::config());

  // save other options ;=)
  saveOptions();

  KateApp::self()->removeMainWindow (this);

  KatePluginManager::self()->disableAllPluginsGUI (this);

  delete m_dcop;
}

void KateMainWindow::setupMainWindow ()
{
  setToolViewStyle( KMultiTabBar::KDEV3ICON );

  m_tabWidget = new KateTabWidget (centralWidget());

  m_viewManager = new KateViewManager (this);

  KateMDI::ToolView *ft = createToolView("kate_filelist", KMultiTabBar::Left, SmallIcon("kmultiple"), i18n("Documents"));
  filelist = new KateFileList (this, m_viewManager, ft);
  //filelist->readConfig(KateApp::self()->config(), "Filelist");

#if 0
  KateMDI::ToolView *t = createToolView("kate_fileselector", KMultiTabBar::Left, SmallIcon("fileopen"), i18n("Filesystem Browser"));
  fileselector = new KateFileSelector( this, m_viewManager, t, "operator");
  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));
#endif
#if 0
  // ONLY ALLOW SHELL ACCESS IF ALLOWED ;)
  if (KAuthorized::authorize("shell_access"))
  {
    t = createToolView("kate_greptool", KMultiTabBar::Bottom, SmallIcon("filefind"), i18n("Find in Files") );
    greptool = new GrepTool( t );
    greptool->setObjectName( "greptool" );
    connect(greptool, SIGNAL(itemSelected(const QString &,int)), this, SLOT(slotGrepToolItemSelected(const QString &,int)));
    connect(t,SIGNAL(toolVisibleChanged(bool)),this, SLOT(updateGrepDir (bool)));
    // WARNING HACK - anders: showing the greptool seems to make the menu accels work
    greptool->show();

    t = createToolView("kate_console", KMultiTabBar::Bottom, SmallIcon("konsole"), i18n("Terminal"));
    console = new KateConsole (this, t);
  }
#endif
  // make per default the filelist visible, if we are in session restore, katemdi will skip this ;)
  showToolView (ft);
}

void KateMainWindow::setupActions()
{
  KAction *a;

  KStdAction::openNew( m_viewManager, SLOT( slotDocumentNew() ), actionCollection(), "file_new" )->setWhatsThis(i18n("Create a new document"));
  KStdAction::open( m_viewManager, SLOT( slotDocumentOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Open an existing document for editing"));

  fileOpenRecent = KStdAction::openRecent (m_viewManager, SLOT(openURL (const KUrl&)), actionCollection());
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=new KAction( KIcon("save_all"), i18n("Save A&ll"), actionCollection(), "file_save_all" );
  a->setShortcut( Qt::CTRL+Qt::Key_L );
  connect( a, SIGNAL( triggered() ), KateDocManager::self(), SLOT( saveAll() ) );
  a->setWhatsThis(i18n("Save all open, modified documents to disk."));

  KStdAction::close( m_viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" )->setWhatsThis(i18n("Close the current document."));

  a=new KAction( i18n( "Clos&e All" ), actionCollection(), "file_close_all" );
  connect( a, SIGNAL( triggered() ), this, SLOT( slotDocumentCloseAll() ) );
  a->setWhatsThis(i18n("Close all open documents."));

  KStdAction::mail( this, SLOT(slotMail()), actionCollection() )->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" )->setWhatsThis(i18n("Close this window"));

  a=new KAction( KIcon("window_new"), i18n("&New Window"), actionCollection(), "view_new_view" );
  connect( a, SIGNAL( triggered() ), this, SLOT( newWindow() ) );
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  if ( KAuthorized::authorize("shell_access") )
  {
    externalTools = new KateExternalToolsMenuAction( i18n("External Tools"), actionCollection(), "tools_external", this );
    externalTools->setWhatsThis( i18n("Launch external helper applications") );
  }

  KToggleAction* showFullScreenAction = KStdAction::fullScreen( 0, 0, actionCollection(),this);
  connect( showFullScreenAction,SIGNAL(toggled(bool)), this,SLOT(slotFullScreen(bool)));

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), actionCollection(), "file_open_with");
  documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
  connect(documentOpenWith->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotOpenWithMenuAction(int)));

  a=KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  KAction* settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");
  settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

#if 0
  // pipe to terminal action
  if (KateApp::self()->authorize("shell_access"))
    new KAction(i18n("&Pipe to Console"), "pipe", 0, console, SLOT(slotPipeToConsole()), actionCollection(), "tools_pipe_to_terminal");
#endif

  // tip of the day :-)
  KStdAction::tipOfDay( this, SLOT( tipOfTheDay() ), actionCollection() )->setWhatsThis(i18n("This shows useful tips on the use of this application."));

  if (KatePluginManager::self()->pluginList().count() > 0)
  {
    a=new KAction( i18n("&Plugins Handbook"), actionCollection(), "help_plugins_contents" );
    connect( a, SIGNAL( triggered() ), this, SLOT( pluginHelp() ) );
    a->setWhatsThis(i18n("This shows help files for various available plugins."));
  }

  a=new KAction( i18n("&About Editor Component"), actionCollection(), "help_about_editor" );
  connect( a, SIGNAL( triggered() ), this, SLOT( aboutEditor() ) );
  a->setGlobalShortcutAllowed(true);

  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));
  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotUpdateOpenWith()));

  slotWindowActivated ();

  // session actions
  a = new KAction( KIcon("filenew"), i18nc("Menu entry Session->New", "&New"), actionCollection(), "sessions_new" );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionNew() ) );
  a = new KAction( KIcon("fileopen"), i18n("&Open..."), actionCollection(), "sessions_open" );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionOpen() ) );
  a = new KAction( KIcon("filesave"), i18n("&Save"), actionCollection(), "sessions_save" );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSave() ) );
  a = new KAction( KIcon("filesaveas"), i18n("Save &As..."), actionCollection(), "sessions_save_as" );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSaveAs() ) );
  a = new KAction( KIcon("view_choose"), i18n("&Manage..."), actionCollection(), "sessions_manage" );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionManage() ) );

  // quick open menu ;)
  new KateSessionsAction (i18n("&Quick Open"), actionCollection(), "sessions_list");
}

KateTabWidget *KateMainWindow::tabWidget ()
{
  return m_tabWidget;
}

void KateMainWindow::slotDocumentCloseAll() {
  if (queryClose_internal())
    KateDocManager::self()->closeAllDocuments(false);
}

bool KateMainWindow::queryClose_internal() {
   uint documentCount=KateDocManager::self()->documents();

  if ( ! showModOnDiskPrompt() )
    return false;

  QList<KTextEditor::Document*> modifiedDocuments=KateDocManager::self()->modifiedDocumentList();
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
  if (KateApp::self()->sessionSaving())
  {
    return queryClose_internal ();
  }

  // normal closing of window
  // allow to close all windows until the last without restrictions
  if ( KateApp::self()->mainWindows () > 1 )
    return true;

  // last one: check if we can close all documents, try run
  // and save docs if we really close down !
  if ( queryClose_internal () )
  {
    KateApp::self()->sessionManager()->saveActiveSession(true, true);

    // detach the dcopClient
    KateApp::self()->dcopClient()->detach();

    return true;
  }

  return false;
}

void KateMainWindow::newWindow ()
{
  KateApp::self()->newMainWindow ();
}

void KateMainWindow::slotEditToolbars()
{
  saveMainWindowSettings( KGlobal::config(), "MainWindow" );
  KEditToolbar dlg( factory() );

  connect( &dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()) );
  dlg.exec();
}

void KateMainWindow::slotNewToolbarConfig()
{
  applyMainWindowSettings( KGlobal::config(), "MainWindow" );
}

void KateMainWindow::slotFileQuit()
{
  KateApp::self()->shutdownKate (this);
}

void KateMainWindow::readOptions ()
{
  KConfig *config = KGlobal::config();

  config->setGroup("General");
  syncKonsole =  config->readEntry("Sync Konsole", QVariant(true)).toBool();
  modNotification = config->readEntry("Modified Notification", QVariant(false)).toBool();
  KateDocManager::self()->setSaveMetaInfos(config->readEntry("Save Meta Infos", QVariant(true)).toBool());
  KateDocManager::self()->setDaysMetaInfos(config->readEntry("Days Meta Infos", 30));

  m_viewManager->setShowFullPath(config->readEntry("Show Full Path in Title", QVariant(false)).toBool());

  fileOpenRecent->loadEntries(config, "Recent Files");

  //fileselector->readConfig(config, "fileselector");
}

void KateMainWindow::saveOptions ()
{
  KConfig *config = KGlobal::config();

  config->setGroup("General");

#if 0
  if (console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);
#endif

  config->writeEntry("Save Meta Infos", KateDocManager::self()->getSaveMetaInfos());

  config->writeEntry("Days Meta Infos", KateDocManager::self()->getDaysMetaInfos());

  config->writeEntry("Show Full Path in Title", m_viewManager->getShowFullPath());

  config->writeEntry("Sync Konsole", syncKonsole);

  fileOpenRecent->saveEntries(config, "Recent Files");

  //fileselector->writeConfig(config, "fileselector");

  filelist->writeConfig(config, "Filelist");
}

void KateMainWindow::slotWindowActivated ()
{
  if (m_viewManager->activeView())
  {
#if 0
    if (console && syncKonsole)
    {
      static QString path;
      QString newPath = m_viewManager->activeView()->document()->url().directory();

      if ( newPath != path )
      {
        path = newPath;
        console->cd (KUrl( path ));
      }
    }
#endif
    updateCaption ((KTextEditor::Document *)m_viewManager->activeView()->document());
  }

  // update proxy
  centralWidget()->setFocusProxy (m_viewManager->activeView());
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
  // removes all actions from documentMenu and documentsGroup
  qDeleteAll( documentsGroup->actions() );

  Q3ListViewItem * item = filelist->firstChild();
  while( item ) {
    KTextEditor::Document *doc = ((KateFileListItem *)item)->document();
    // would it be saner to use the screen width as a limit that some random number??
    QAction *action = new QAction(
                                  KStringHandler::rsqueeze(doc->documentName(), 150),
                                  documentsGroup );
    action->setCheckable(true);
    if(m_viewManager->activeView() && doc == m_viewManager->activeView()->document())
      action->setChecked(true);
    action->setData(QVariant::fromValue((void *)doc));
    documentMenu->addAction(action);

    item = item->nextSibling();
  }

}

void KateMainWindow::activateDocumentFromDocMenu (QAction *action)
{
  KTextEditor::Document *doc = (KTextEditor::Document *)action->data().value<void *>();
  if (doc)
    m_viewManager->activateView (doc);
}

void KateMainWindow::slotGrepToolItemSelected(const QString &filename,int linenumber)
{
  KUrl fileURL;
  fileURL.setPath( filename );
  m_viewManager->openURL( fileURL );
  if ( m_viewManager->activeView() == 0 ) return;

  if (m_viewManager->activeView())
    m_viewManager->activeView()->setCursorPosition( KTextEditor::Cursor (linenumber, 0) );

  raise();
  activateWindow();
}

void KateMainWindow::dragEnterEvent( QDragEnterEvent *event )
{
  if (!event->mimeData()) return;
  event->setAccepted(KUrl::List::canDecode(event->mimeData()));
}

void KateMainWindow::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void KateMainWindow::slotDropEvent( QDropEvent * event )
{
  if (event->mimeData()==0) return;
  KUrl::List textlist=KUrl::List::fromMimeData(event->mimeData());

  for (KUrl::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
  {
    m_viewManager->openURL (*i);
  }
}

void KateMainWindow::editKeys()
{
  KKeyDialog dlg ( KKeyChooser::AllActions, KKeyChooser::LetterShortcutsDisallowed, this );

  QList<KXMLGUIClient*> clients = guiFactory()->clients();

  foreach(KXMLGUIClient *client, clients)
    dlg.insert ( client->actionCollection(), client->instance()->aboutData()->programName() );

  dlg.insert( externalTools->actionCollection(), i18n("External Tools") );

  dlg.configure();

  QList<KTextEditor::Document*>  l=KateDocManager::self()->documentList();
  for (int i=0;i<l.count();i++) {
//     kDebug(13001)<<"reloading Keysettings for document "<<i<<endl;
    l.at(i)->reloadXML();
    QList<KDocument::View *> l1=l.at(i)->views ();//KTextEditor::Document
    for (int i1=0;i1<l1.count();i1++) {
      l1.at(i1)->reloadXML();
//       kDebug(13001)<<"reloading Keysettings for view "<<i<<"/"<<i1<<endl;
    }
  }

  externalTools->actionCollection()->writeSettings( new KConfig("externaltools", false, false, "appdata") );
}

void KateMainWindow::openURL (const QString &name)
{
  m_viewManager->openURL (KUrl(name));
}

void KateMainWindow::slotConfigure()
{
  if (!m_viewManager->activeView())
    return;

  KateConfigDialog* dlg = new KateConfigDialog (this, m_viewManager->activeView());
  dlg->exec();

  delete dlg;
}

KUrl KateMainWindow::activeDocumentUrl()
{
  // anders: i make this one safe, as it may be called during
  // startup (by the file selector)
  KTextEditor::View *v = m_viewManager->activeView();
  if ( v )
    return v->document()->url();
  return KUrl();
}

#if 0
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
#endif

// TODO make this work
void KateMainWindow::mSlotFixOpenWithMenu()
{
  //kDebug(13001)<<"13000"<<"fixing open with menu"<<endl;
  documentOpenWith->popupMenu()->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->document()->url() );
  //kDebug(13001)<<"13000"<<"url: "<<m_viewManager->activeView()->document()->url().prettyURL()<<"mime type: "<<mime->name()<<endl;
  // some checking goes here...
  KTrader::OfferList offers = KTrader::self()->query(mime->name(), "Type == 'Application'");
  // for each one, insert a menu item...
  for(KTrader::OfferList::Iterator it = offers.begin(); it != offers.end(); ++it) {
    if ((*it)->name() == "Kate") continue;
    documentOpenWith->popupMenu()->addAction( KIcon((*it)->icon()), (*it)->name() );
  }
  // append "Other..." to call the KDE "open with" dialog.
  documentOpenWith->popupMenu()->addAction(i18n("&Other..."));
}

void KateMainWindow::slotOpenWithMenuAction(int idx)
{
  KUrl::List list;
  list.append( m_viewManager->activeView()->document()->url() );

  QString appname = documentOpenWith->popupMenu()->actions().at(idx)->text();
  appname = appname.remove('&'); //Remove a possible accelerator ... otherwise the application might not get found.
  if ( appname.compare(i18n("Other...")) == 0 ) {
    // display "open with" dialog
    KOpenWithDlg* dlg = new KOpenWithDlg(list);
    if (dlg->exec())
      KRun::run(*dlg->service(), list);
    return;
  }
  QString qry = QString("((Type == 'Application') and (Name == '%1'))").arg( appname );
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->document()->url() );
  KTrader::OfferList offers = KTrader::self()->query(mime->name(), qry);

  if (!offers.isEmpty()) {
    KService::Ptr app = offers.first();
    KRun::run(*app, list);
  }
  else
    KMessageBox::error(this, i18n("Application '%1' not found!", appname), i18n("Application not found!"));
}

void KateMainWindow::pluginHelp()
{
  KToolInvocation::invokeHelp (QString(), "kate-plugins");
}

void KateMainWindow::aboutEditor()
{
	KAboutApplication ad(KateDocManager::self()->editor()->aboutData(),this);
	ad.exec();
}

void KateMainWindow::slotMail()
{
  KateMailDialog *d = new KateMailDialog(this, this);
  if ( ! d->exec() )
    return;
  QList<KTextEditor::Document *> attDocs = d->selectedDocs();
  delete d;
  // Check that all selected files are saved (or shouldn't be)
  QStringList urls; // to atthatch
  KTextEditor::Document *doc;
  QList<KTextEditor::Document *>::iterator it = attDocs.begin();
  for ( ; *it; ++it ) {
    doc = *it;
    if (!doc) continue;
    if ( doc->url().isEmpty() ) {
      // unsaved document. back out unless it gets saved
      int r = KMessageBox::questionYesNo( this,
              i18n("<p>The current document has not been saved, and "
              "cannot be attached to an email message."
              "<p>Do you want to save it and proceed?"),
              i18n("Cannot Send Unsaved File"),KStdGuiItem::saveAs(),KStdGuiItem::cancel() );
      if ( r == KMessageBox::Yes ) {
        bool sr = doc->documentSaveAs();
       /* if ( sr == KTextEditor::View::SAVE_OK ) { ;
        }
        else {*/
          if ( !sr  ) // ERROR or RETRY(?)
         {   KMessageBox::sorry( this, i18n("The file could not be saved. Please check "
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
                "<p>Do you want to save it before sending it?", doc->url().prettyURL()),
                i18n("Save Before Sending?"), KStdGuiItem::save(), i18n("Do Not Save") );
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
  KToolInvocation::invokeMailer( QString(), // to
                      QString(), // cc
                      QString(), // bcc
                      QString(), // subject
                      QString(), // body
                      QString(), // msgfile
                      urls           // urls to atthatch
                      );
}
void KateMainWindow::tipOfTheDay()
{
  KTipDialog::showTip( /*0*/this, QString(), true );
}

void KateMainWindow::slotFullScreen(bool t)
{
  if (t)
    showFullScreen();
  else
    showNormal();
}

void KateMainWindow::updateGrepDir (bool visible)
{
  // grepdlg gets hidden
  if (!visible)
    return;

  if ( m_viewManager->activeView() )
  {
    if ( m_viewManager->activeView()->document()->url().isLocalFile() )
    {
      greptool->updateDirName( m_viewManager->activeView()->document()->url().directory() );
    }
  }
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
  KTextEditor::Document *doc;

  DocVector list( KateDocManager::self()->documents() );
  uint cnt = 0;
  foreach( doc,KateDocManager::self()->documentList())
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

void KateMainWindow::slotDocumentCreated (KTextEditor::Document *doc)
{
  connect(doc,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(updateCaption(KTextEditor::Document *)));
  connect(doc,SIGNAL(documentNameChanged(KTextEditor::Document *)),this,SLOT(updateCaption(KTextEditor::Document *)));
  connect(doc,SIGNAL(documentNameChanged(KTextEditor::Document *)),this,SLOT(slotUpdateOpenWith()));

  updateCaption (doc);
}

void KateMainWindow::updateCaption (KTextEditor::Document *doc)
{
  if (!m_viewManager->activeView())
  {
    setCaption ("", false);
    return;
  }

  // block signals from inactive docs
  if (!((KTextEditor::Document*)m_viewManager->activeView()->document() == doc))
    return;

  QString c;
  if (m_viewManager->activeView()->document()->url().isEmpty() || (!m_viewManager->getShowFullPath()))
  {
    c = ((KTextEditor::Document*)m_viewManager->activeView()->document())->documentName();
  }
  else
  {
    c = m_viewManager->activeView()->document()->url().prettyURL();
  }

  QString sessName = KateApp::self()->sessionManager()->activeSession()->sessionName();
  if ( !sessName.isEmpty() )
    sessName = QString("%1: ").arg( sessName );

  setCaption( sessName + KStringHandler::lsqueeze(c,64),
              m_viewManager->activeView()->document()->isModified());
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
  sessionConfig->writeEntry ("Last Session", KateApp::self()->sessionManager()->activeSession()->sessionFileRelative());
}

// kate: space-indent on; indent-width 2; replace-tabs on;
