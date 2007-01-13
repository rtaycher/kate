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
#include "katemailfilesdialog.h"
#include "kateexternaltools.h"
#include "katesavemodifieddialog.h"
#include "katemwmodonhddialog.h"
#include "katesession.h"
#include "katetabwidget.h"
#include "katemainwindowadaptor.h"
#include "kateviewdocumentproxymodel.h"

#include "../interfaces/mainwindow.h"

#include <KAboutApplication>
#include <KInstance>
#include <KAboutData>
#include <KAction>
#include <KActionCollection>
#include <KCmdLineArgs>
#include <kdebug.h>
#include <KDirOperator>
#include <KEditToolbar>
#include <KFileDialog>
#include <KGlobalAccel>
#include <KGlobal>
#include <KGlobalSettings>
#include <KIconLoader>
#include <KKeyDialog>
#include <KLocale>
#include <KMessageBox>
#include <KMimeType>
#include <KOpenWithDialog>
#include <KMenu>
#include <KRecentFilesAction>
#include <KSimpleConfig>
#include <KStatusBar>
#include <kstandardaction.h>
#include <KStandardDirs>
#include <KToggleFullScreenAction>
#include <KMimeTypeTrader>
#include <KUniqueApplication>
#include <KDesktopFile>
#include <KHelpMenu>
#include <KMultiTabBar>
#include <KTipDialog>
#include <KMenuBar>
#include <KStringHandler>
#include <KToolInvocation>
#include <KAuthorized>

#include <QLayout>
#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QList>
#include <QDesktopWidget>
#include <QListView>

#include <assert.h>
#include <unistd.h>
//END

uint KateMainWindow::uniqueID = 1;

KateMainWindow::KateMainWindow (KConfig *sconfig, const QString &sgroup)
  : KateMDI::MainWindow (0)
{
  setObjectName((QString("__KateMainWindow#%1").arg(uniqueID)).toLatin1());
  // first the very important id
  myID = uniqueID;
  uniqueID++;

  new KateMainWindowAdaptor( this );
  m_dbusObjectPath = "/KateMainWindow_" + QString::number( myID );
  QDBusConnection::sessionBus().registerObject( m_dbusObjectPath, this );

  m_modignore = false;

  console = 0;

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

  // setup the most important widgets
  setupMainWindow();

  // setup the actions
  setupActions();

  setStandardToolBarMenuEnabled( true );
  setXMLFile( "kateui.rc" );
  createShellGUI ( true );

  kDebug()<<"****************************************************************************"<<sconfig<<endl;
  KatePluginManager::self()->enableAllPluginsGUI (this,sconfig);

  if ( KAuthorized::authorize("shell_access") )
  {
    KTextEditor::CommandInterface* cmdIface =
        qobject_cast<KTextEditor::CommandInterface*>( KateDocManager::self()->editor() );
    if( cmdIface )
      cmdIface->registerCommand( KateExternalToolsCommand::self() );
  }

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

}

void KateMainWindow::setupMainWindow ()
{
  setToolViewStyle( KMultiTabBar::KDEV3ICON );

  m_tabWidget = new KateTabWidget (centralWidget());

  m_viewManager = new KateViewManager (this);

  KateMDI::ToolView *ft = createToolView("kate_filelist", KMultiTabBar::Left, SmallIcon("kmultiple"), i18n("Documents"));
  m_fileList=new KateFileList(ft,actionCollection());
  m_pM=new KateViewDocumentProxyModel(this);
  m_pM->setSourceModel(KateDocManager::self());
  m_fileList->setModel(m_pM);
  m_fileList->setSelectionModel(m_pM->selection());
  m_fileList->setDragEnabled(true);
  m_fileList->setDragDropMode(QAbstractItemView::InternalMove);
  m_fileList->setDropIndicatorShown(true);
#ifdef __GNUC__
#warning I do not like it, it looks like a hack, search for a better way, but for now it should work. (Even on windows most lisviews, except exploder are single click) (jowenn)
#endif
  if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, m_fileList)) {
    kDebug()<<"HACK:***********************CONNECTING CLICKED***************************"<<endl;
    connect(m_fileList,SIGNAL(clicked(const QModelIndex&)),m_pM,SLOT(opened(const QModelIndex&)));
    connect(m_fileList,SIGNAL(clicked(const QModelIndex&)),m_viewManager,SLOT(activateDocument(const QModelIndex &)));
  }
  connect(m_fileList,SIGNAL(activated(const QModelIndex&)),m_pM,SLOT(opened(const QModelIndex&)));
  connect(m_fileList,SIGNAL(activated(const QModelIndex&)),m_viewManager,SLOT(activateDocument(const QModelIndex &)));
  connect(m_fileList,SIGNAL(customContextMenuRequested(const QPoint &)),this,SLOT(showFileListPopup(const QPoint&)));
  m_fileList->setContextMenuPolicy(Qt::CustomContextMenu);
  //filelist = new KateFileList (this, m_viewManager, ft);
  //filelist->readConfig(KateApp::self()->config(), "Filelist");

#if 0
  KateMDI::ToolView *t = createToolView("kate_fileselector", KMultiTabBar::Left, SmallIcon("fileopen"), i18n("Filesystem Browser"));
  fileselector = new KateFileSelector( this, m_viewManager, t, "operator");
  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));
#endif

  // make per default the filelist visible, if we are in session restore, katemdi will skip this ;)
  showToolView (ft);
}

void KateMainWindow::setupActions()
{
  QAction *a;

  actionCollection()->addAction( KStandardAction::New, "file_new", m_viewManager, SLOT( slotDocumentNew() ) )
    ->setWhatsThis(i18n("Create a new document"));
  actionCollection()->addAction( KStandardAction::Open, "file_open", m_viewManager, SLOT( slotDocumentOpen() ) )
    ->setWhatsThis(i18n("Open an existing document for editing"));

  fileOpenRecent = KStandardAction::openRecent (m_viewManager, SLOT(openUrl (const KUrl&)), this);
  actionCollection()->addAction(fileOpenRecent->objectName(), fileOpenRecent);
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=actionCollection()->addAction( "file_save_all" );
  a->setIcon( KIcon("save_all") );
  a->setText( i18n("Save A&ll") );
  a->setShortcut( QKeySequence(Qt::CTRL+Qt::Key_L) );
  connect( a, SIGNAL( triggered() ), KateDocManager::self(), SLOT( saveAll() ) );
  a->setWhatsThis(i18n("Save all open, modified documents to disk."));

  actionCollection()->addAction( KStandardAction::Close, "file_close", m_viewManager, SLOT( slotDocumentClose() ) )
      ->setWhatsThis(i18n("Close the current document."));

  a=actionCollection()->addAction( "file_close_all" );
  a->setText( i18n( "Clos&e All" ) );
  connect( a, SIGNAL( triggered() ), this, SLOT( slotDocumentCloseAll() ) );
  a->setWhatsThis(i18n("Close all open documents."));

  actionCollection()->addAction( KStandardAction::Mail, this, SLOT(slotMail()) )
      ->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));

  actionCollection()->addAction( KStandardAction::Quit, "file_quit", this, SLOT( slotFileQuit() ) )
      ->setWhatsThis(i18n("Close this window"));

  a=actionCollection()->addAction( "view_new_view" );
  a->setIcon( KIcon("window_new") );
  a->setText( i18n("&New Window") );
  connect( a, SIGNAL( triggered() ), this, SLOT( newWindow() ) );
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  if ( KAuthorized::authorize("shell_access") )
  {
    externalTools = new KateExternalToolsMenuAction( i18n("External Tools"), this, this );
    actionCollection()->addAction( "tools_external", externalTools );
    externalTools->setWhatsThis( i18n("Launch external helper applications") );
  }

  KToggleAction* showFullScreenAction = KStandardAction::fullScreen( 0, 0, this, this);
  actionCollection()->addAction( showFullScreenAction->objectName(), showFullScreenAction );
  connect( showFullScreenAction,SIGNAL(toggled(bool)), this,SLOT(slotFullScreen(bool)));

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), this);
  actionCollection()->addAction("file_open_with", documentOpenWith);
  documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
  connect(documentOpenWith->menu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotOpenWithMenuAction(QAction*)));

  a=actionCollection()->addAction(KStandardAction::KeyBindings, this, SLOT(editKeys()));
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=actionCollection()->addAction(KStandardAction::ConfigureToolbars, "set_configure_toolbars",
                                  this, SLOT(slotEditToolbars()));
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  QAction* settingsConfigure = actionCollection()->addAction(KStandardAction::Preferences, "settings_configure",
                                                             this, SLOT(slotConfigure()));
  settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

#if 0
  // pipe to terminal action
  if (KateApp::self()->authorize("shell_access")) {
      a=actionCollection()->addAction("tools_pipe_to_terminal");
      a->setIcon(KIcon("pipe"));
      a->setText(i18n("&Pipe to Console"));
      connect(a, SIGNAL(triggered(QAction*)), console, SLOT(slotPipeToConsole()));
  }
#endif

  // tip of the day :-)
  actionCollection()->addAction( KStandardAction::TipofDay, this, SLOT( tipOfTheDay() ) )
      ->setWhatsThis(i18n("This shows useful tips on the use of this application."));

  if (KatePluginManager::self()->pluginList().count() > 0)
  {
    a=actionCollection()->addAction( "help_plugins_contents" );
    a->setText( i18n("&Plugins Handbook") );
    connect( a, SIGNAL( triggered() ), this, SLOT( pluginHelp() ) );
    a->setWhatsThis(i18n("This shows help files for various available plugins."));
  }

  a=actionCollection()->addAction( "help_about_editor" );
  a->setText( i18n("&About Editor Component") );
  connect( a, SIGNAL( triggered() ), this, SLOT( aboutEditor() ) );
  qobject_cast<KAction*>( a )->setGlobalShortcutAllowed(true);

  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));
  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotUpdateOpenWith()));

  slotWindowActivated ();

  // session actions
  a = actionCollection()->addAction( "sessions_new" );
  a->setIcon( KIcon("filenew") );
  a->setText( i18nc("Menu entry Session->New", "&New") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionNew() ) );
  a = actionCollection()->addAction( "sessions_open" );
  a->setIcon( KIcon("fileopen") );
  a->setText( i18n("&Open...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionOpen() ) );
  a = actionCollection()->addAction( "sessions_save" );
  a->setIcon( KIcon("filesave") );
  a->setText( i18n("&Save") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSave() ) );
  a = actionCollection()->addAction( "sessions_save_as" );
  a->setIcon( KIcon("filesaveas") );
  a->setText( i18n("Save &As...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSaveAs() ) );
  a = actionCollection()->addAction( "sessions_manage" );
  a->setIcon( KIcon("view_choose") );
  a->setText( i18n("&Manage...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionManage() ) );

  // quick open menu ;)
  a = new KateSessionsAction (i18n("&Quick Open"), this);
  actionCollection()->addAction("sessions_list", a);
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
#ifdef __GNUC__
#warning "kde4: dbus port"
#endif
#if 0
    // detach the dcopClient
    KateApp::self()->dcopClient()->detach();
#endif
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
#ifdef __GNUC__
  #warning PORTME
#endif
  //filelist->writeConfig(config, "Filelist");
}

void KateMainWindow::slotWindowActivated ()
{
  if (m_viewManager->activeView())
  {
    m_pM->opened(modelIndexForDocument(m_viewManager->activeView()->document()));
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

class KateRowColumn {
  public:
    KateRowColumn():m_row(-1),m_column(-1){}
    KateRowColumn(int row,int column):m_row(row),m_column(column){}
    ~KateRowColumn() {}
    int column() {return m_column;}
    int row() {return m_row;}
    bool isValid() {return ( (m_row>=0) && (m_column>=0));}
  private:
    int m_row;
    int m_column;
};

Q_DECLARE_METATYPE(KateRowColumn)


void KateMainWindow::documentMenuAboutToShow()
{
  qRegisterMetaType<KTextEditor::Document*>("KTextEditor::Document*");
  qDeleteAll( documentsGroup->actions() );
  int rows=m_fileList->model()->rowCount(QModelIndex());
  QAbstractItemModel *model=m_fileList->model();
  for (int row=0;row<rows;row++) {
    QModelIndex index=model->index(row,0,QModelIndex());
    Q_ASSERT(index.isValid());
    KTextEditor::Document *doc=index.data(KateDocManager::DocumentRole).value<KTextEditor::Document*>();
    const QString name = KStringHandler::rsqueeze(doc->documentName(), 150);
    QAction *action = new QAction(doc->isModified() ?
        i18nc("'document name [*]', [*] means modified", "%1 [*]", name) : name,
        documentsGroup );
    action->setCheckable(true);
    if(m_viewManager->activeView() && doc == m_viewManager->activeView()->document())
      action->setChecked(true);
    action->setData(QVariant::fromValue(KateRowColumn(index.row(),index.column())));
    documentMenu->addAction(action);
  }
}

void KateMainWindow::activateDocumentFromDocMenu (QAction *action)
{
  KateRowColumn rowCol=action->data().value<KateRowColumn>();
  if (!rowCol.isValid()) return;
  QModelIndex index=m_pM->index(rowCol.row(),rowCol.column());
  if (index.isValid()) {
    KTextEditor::Document *doc = index.data(KateDocManager::DocumentRole).value<KTextEditor::Document*>();
    if (doc)
      m_viewManager->activateView (doc);
    m_pM->opened(index);
  }
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
    m_viewManager->openUrl (*i);
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

void KateMainWindow::openUrl (const QString &name)
{
  m_viewManager->openUrl (KUrl(name));
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
    m_viewManager->openUrl(tmp->url());
    fileselector->dirOperator()->view()->setSelected(tmp,false);
  }
}
#endif

void KateMainWindow::mSlotFixOpenWithMenu()
{
  KMenu *menu = documentOpenWith->menu();
  menu->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::mimeType(m_viewManager->activeView()->document()->mimeType());
  //kDebug(13001) << "mime type: " << mime->name() << endl;

  QAction *a = 0;
  KService::List offers = KMimeTypeTrader::self()->query(mime->name(), "Application");
  // for each one, insert a menu item...
  for(KService::List::Iterator it = offers.begin(); it != offers.end(); ++it) {
    KService::Ptr service = *it;
    if (service->name() == "Kate") continue;
    a = menu->addAction(KIcon(service->icon()), service->name());
    a->setData(service->desktopEntryPath());
  }
  // append "Other..." to call the KDE "open with" dialog.
  a = documentOpenWith->menu()->addAction(i18n("&Other..."));
  a->setData(QString());
}

void KateMainWindow::slotOpenWithMenuAction(QAction* a)
{
  KUrl::List list;
  list.append( m_viewManager->activeView()->document()->url() );

  const QString openWith = a->data().toString();
  if (openWith.isEmpty()) {
    // display "open with" dialog
    KOpenWithDialog dlg(list);
    if (dlg.exec())
      KRun::run(*dlg.service(), list, this);
    return;
  }

  KService::Ptr app = KService::serviceByDesktopPath(openWith);
  if (app) {
    KRun::run(*app, list, this);
  } else {
    KMessageBox::error(this, i18n("Application '%1' not found!", openWith), i18n("Application not found!"));
  }
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
  {
    delete d;
    return;
  }
  QList<KTextEditor::Document *> attDocs = d->selectedDocs();
  delete d;
  // Check that all selected files are saved (or shouldn't be)
  QStringList urls; // to atthatch
  KTextEditor::Document *doc;
  for ( QList<KTextEditor::Document *>::iterator it = attDocs.begin();
        it != attDocs.end(); ++it ) {
    doc = *it;
    if (!doc) continue;
    if ( doc->url().isEmpty() ) {
      // unsaved document. back out unless it gets saved
      int r = KMessageBox::questionYesNo( this,
              i18n("<p>The current document has not been saved, and "
              "cannot be attached to an email message."
              "<p>Do you want to save it and proceed?"),
              i18n("Cannot Send Unsaved File"),KStandardGuiItem::saveAs(),KStandardGuiItem::cancel() );
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
                "<p>Do you want to save it before sending it?", doc->url().prettyUrl()),
                i18n("Save Before Sending?"), KStandardGuiItem::save(), KGuiItem(i18n("Do Not Save")) );
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
  connect(doc,SIGNAL(modifiedChanged(KTextEditor::Document *)),this,SLOT(slotDocModified(KTextEditor::Document *)));
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
    c = m_viewManager->activeView()->document()->url().prettyUrl();
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

void KateMainWindow::showFileListPopup(const QPoint& pos) {
  if (m_fileList->selectionModel()->selection().count()==0) return;
  QMenu *menu = (QMenu*) (factory()->container("filelist_popup", m_viewManager->mainWindow()));
  if (menu) menu->exec(m_fileList->viewport()->mapToGlobal(pos));
}


static QModelIndex modelIndexForDocumentRec(const QModelIndex &index, QAbstractItemModel *model) {
    QAbstractProxyModel *m=qobject_cast<QAbstractProxyModel*>(model);
    if (m==0) return index;
    return m->mapFromSource(modelIndexForDocumentRec(index,m->sourceModel()));
}

QModelIndex KateMainWindow::modelIndexForDocument(KTextEditor::Document *document) {
  KTextEditor::Document *tmp=m_pM->selection()->currentIndex().data(KateDocManager::DocumentRole).value<KTextEditor::Document*>();
  if (tmp==document) return m_pM->selection()->currentIndex(); else return modelIndexForDocumentRec(KateDocManager::self()->indexForDocument(document),m_pM);
}


void KateMainWindow::slotDocModified(KTextEditor::Document *document) {
  if (document->isModified()) m_pM->modified(modelIndexForDocument(document));
}

// kate: space-indent on; indent-width 2; replace-tabs on;
