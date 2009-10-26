/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>

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
#include "katesavemodifieddialog.h"
#include "katemwmodonhddialog.h"
#include "katesession.h"
#include "katemainwindowadaptor.h"
#include "kateviewdocumentproxymodel.h"
#include "kateviewspace.h"
//#include "modeltest.h"

#include <kate/mainwindow.h>

#include <KAboutApplicationDialog>
#include <KAction>
#include <kdebug.h>
#include <KEditToolBar>
#include <KGlobal>
#include <KShortcutsDialog>
#include <KLocale>
#include <KMessageBox>
#include <KMimeType>
#include <KOpenWithDialog>
#include <KMenu>
#include <KConfig>
#include <kstandardaction.h>
#include <KMimeTypeTrader>
#include <KMultiTabBar>
#include <KTipDialog>
#include <KStringHandler>
#include <KToolInvocation>
#include <KAuthorized>
#include <KRun>
#include <KRecentFilesAction>
#include <KToggleFullScreenAction>
#include <KAboutData>
#include <kwindowsystem.h>

#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QList>
#include <QDesktopWidget>

#include <assert.h>
#include <unistd.h>
//END

uint KateMainWindow::uniqueID = 1;
KateMwModOnHdDialog *KateMainWindow::s_modOnHdDialog=0;

KateContainerStackedLayout::KateContainerStackedLayout(QWidget* parent)
  : QStackedLayout(parent)
{}

QSize KateContainerStackedLayout::sizeHint() const
{
  if (currentWidget())
    return currentWidget()->sizeHint();
  return QStackedLayout::sizeHint();
}

QSize KateContainerStackedLayout::minimumSize() const
{
  if (currentWidget())
    return currentWidget()->minimumSize();
  return QStackedLayout::minimumSize();
}


KateMainWindow::KateMainWindow (KConfig *sconfig, const QString &sgroup)
    : KateMDI::MainWindow (0)
{
  setObjectName((QString("__KateMainWindow#%1").arg(uniqueID)).toLatin1());
  // first the very important id
  myID = uniqueID;
  uniqueID++;

  new KateMainWindowAdaptor( this );
  m_dbusObjectPath = "/MainWindow/" + QString::number( myID );
  QDBusConnection::sessionBus().registerObject( m_dbusObjectPath, this );

  m_modignore = false;

  // here we go, set some usable default sizes
  if (!initialGeometrySet())
  {
    int scnum = QApplication::desktop()->screenNumber(parentWidget());
    QRect desk = QApplication::desktop()->screenGeometry(scnum);

    QSize size;

    // try to load size
    if (sconfig)
    {
      KConfigGroup cg( sconfig, sgroup );
      size.setWidth (cg.readEntry( QString::fromLatin1("Width %1").arg(desk.width()), 0 ));
      size.setHeight (cg.readEntry( QString::fromLatin1("Height %1").arg(desk.height()), 0 ));
    }

    // if thats fails, try to reuse size
    if (size.isEmpty())
    {
      // first try to reuse size known from current or last created main window ;=)
      if (KateApp::self()->mainWindows () > 0)
      {
        KateMainWindow *win = KateApp::self()->activeMainWindow ();

        if (!win)
          win = KateApp::self()->mainWindow (KateApp::self()->mainWindows () - 1);

        size = win->size();
      }
      else // now fallback to hard defaults ;)
      {
        // first try global app config
        KConfigGroup cg( KGlobal::config(), "MainWindow" );
        size.setWidth (cg.readEntry( QString::fromLatin1("Width %1").arg(desk.width()), 0 ));
        size.setHeight (cg.readEntry( QString::fromLatin1("Height %1").arg(desk.height()), 0 ));

        if (size.isEmpty())
          size = QSize (qMin (700, desk.width()), qMin(480, desk.height()));
      }

      resize (size);
    }
  }

  // start session restore if needed
  startRestore (sconfig, sgroup);

  m_mainWindow = new Kate::MainWindow (this);

  // setup most important actions first, needed by setupMainWindow
  setupImportantActions ();

  // setup the most important widgets
  setupMainWindow();

  // setup the actions
  setupActions();

  setStandardToolBarMenuEnabled( true );
  setXMLFile( "kateui.rc" );
  createShellGUI ( true );

  kDebug() << "****************************************************************************" << sconfig;

  // register mainwindow in app
  KateApp::self()->addMainWindow (this);

  // enable plugin guis
  KatePluginManager::self()->enableAllPluginsGUI (this, sconfig);

  // connect documents menu aboutToshow
  documentMenu = (QMenu*)factory()->container("go", this);
  connect(documentMenu, SIGNAL(aboutToShow()), this, SLOT(documentMenuAboutToShow()));

  documentsGroup = new QActionGroup(documentMenu);
  documentsGroup->setExclusive(true);
  connect(documentsGroup, SIGNAL(triggered(QAction *)), this, SLOT(activateDocumentFromDocMenu(QAction *)));

  // caption update
  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
    slotDocumentCreated (KateDocManager::self()->document(i));

  connect(KateDocManager::self(), SIGNAL(documentCreated(KTextEditor::Document *)), this, SLOT(slotDocumentCreated(KTextEditor::Document *)));

  readOptions();

  if (sconfig)
  {
    int id = KateApp::self()->mainWindowID (this);
    m_documentModel->readSessionConfig( sconfig,
        QString("DocumentModel:MainWindow:%1").arg(id) );
    // sync session config - or do we need an extra
    // KateFileList::readSessionConfig() here too?
    m_fileList->setSortRole(m_documentModel->sortRole());

    m_viewManager->restoreViewConfiguration (KConfigGroup(sconfig, sgroup) );
  }

  finishRestore ();

  fileOpenRecent->loadEntries( KConfigGroup(sconfig, "Recent Files" ) );

  setAcceptDrops(true);

  connect(KateSessionManager::self(), SIGNAL(sessionChanged()), this, SLOT(updateCaption()));
}

KateMainWindow::~KateMainWindow()
{
  // first, save our fallback window size ;)
  saveWindowSize (KConfigGroup(KGlobal::config(), "MainWindow"));

  // save other options ;=)
  saveOptions();

  // unregister mainwindow in app
  KateApp::self()->removeMainWindow (this);

  // disable all plugin guis, delete all pluginViews
  KatePluginManager::self()->disableAllPluginsGUI (this);
}

void KateMainWindow::setupImportantActions ()
{
  // settings
  m_paShowStatusBar = KStandardAction::showStatusbar(this, SLOT(toggleShowStatusBar()), actionCollection());
  m_paShowStatusBar->setWhatsThis(i18n("Use this command to show or hide the view's statusbar"));

  m_paShowPath = new KToggleAction( i18n("Sho&w Path"), this );
  actionCollection()->addAction( "settings_show_full_path", m_paShowPath );
  connect( m_paShowPath, SIGNAL(toggled(bool)), this, SLOT(updateCaption()) );
  m_paShowPath->setWhatsThis(i18n("Show the complete document path in the window caption"));
}

void KateMainWindow::setupMainWindow ()
{
  setToolViewStyle( KMultiTabBar::KDEV3ICON );

  m_viewManager = new KateViewManager (centralWidget(), this);
  ((QBoxLayout*)(centralWidget()->layout()))->setStretchFactor(m_viewManager,100);
  m_horizontalViewBarContainer=new QWidget(centralWidget());
  m_containerstack = new KateContainerStackedLayout(m_horizontalViewBarContainer);


  KateMDI::ToolView *ft = createToolView("kate_filelist", KMultiTabBar::Left, SmallIcon("document-multiple"), i18n("Documents"));
  m_fileList = new KateFileList(ft, actionCollection());
  m_documentModel = new KateViewDocumentProxyModel(this);
  m_documentModel->setSourceModel(KateDocManager::self());
//  new ModelTest(m_documentModel,this);
  m_fileList->setModel(m_documentModel);
  m_fileList->setSelectionModel(m_documentModel->selection());
  m_fileList->setDragEnabled(true);
  m_fileList->setDragDropMode(QAbstractItemView::InternalMove);
  m_fileList->setDropIndicatorShown(true);
#ifdef __GNUC__
#warning I do not like it, it looks like a hack, search for a better way, but for now it should work. (Even on windows most lisviews, except exploder are single click) (jowenn)
#endif
  if (!style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick, 0, m_fileList))
  {
    kDebug() << "HACK:***********************CONNECTING CLICKED***************************";
    connect(m_fileList, SIGNAL(clicked(const QModelIndex&)), m_documentModel, SLOT(opened(const QModelIndex&)));
    connect(m_fileList, SIGNAL(clicked(const QModelIndex&)), m_viewManager, SLOT(activateDocument(const QModelIndex &)));
  }
  connect(m_fileList, SIGNAL(activated(const QModelIndex&)), m_documentModel, SLOT(opened(const QModelIndex&)));
  connect(m_fileList, SIGNAL(activated(const QModelIndex&)), m_viewManager, SLOT(activateDocument(const QModelIndex &)));
  connect(m_fileList, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showFileListPopup(const QPoint&)));
  connect(m_fileList, SIGNAL(closeDocument(KTextEditor::Document*)),m_viewManager,SLOT(slotDocumentClose(KTextEditor::Document*)));
  connect(m_fileList, SIGNAL(closeOtherDocument(KTextEditor::Document*)),this,SLOT(slotDocumentCloseOther(KTextEditor::Document*)));
  //filelist = new KateFileList (this, m_viewManager, ft);
//   m_fileList->readConfig(KConfigGroup(KGlobal::config(), "FileList"));

#if 0
  KateMDI::ToolView *t = createToolView("kate_fileselector", KMultiTabBar::Left, SmallIcon("document-open"), i18n("Filesystem Browser"));
  fileselector = new KateFileSelector( this, m_viewManager, t, "operator");
  connect(fileselector->dirOperator(), SIGNAL(fileSelected(const KFileItem&)), this, SLOT(fileSelected(const KFileItem&)));
#endif

  // make per default the filelist visible, if we are in session restore, katemdi will skip this ;)
  showToolView (ft);
}

void KateMainWindow::setupActions()
{
  KAction *a;

  actionCollection()->addAction( KStandardAction::New, "file_new", m_viewManager, SLOT( slotDocumentNew() ) )
  ->setWhatsThis(i18n("Create a new document"));
  actionCollection()->addAction( KStandardAction::Open, "file_open", m_viewManager, SLOT( slotDocumentOpen() ) )
  ->setWhatsThis(i18n("Open an existing document for editing"));

  fileOpenRecent = KStandardAction::openRecent (m_viewManager, SLOT(openUrl (const KUrl&)), this);
  actionCollection()->addAction(fileOpenRecent->objectName(), fileOpenRecent);
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a = actionCollection()->addAction( "file_save_all" );
  a->setIcon( KIcon("document-save-all") );
  a->setText( i18n("Save A&ll") );
  a->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_L) );
  connect( a, SIGNAL( triggered() ), KateDocManager::self(), SLOT( saveAll() ) );
  a->setWhatsThis(i18n("Save all open, modified documents to disk."));

  a = actionCollection()->addAction( "file_reload_all" );
  a->setText( i18n("&Reload All") );
  connect( a, SIGNAL( triggered() ), KateDocManager::self(), SLOT( reloadAll() ) );
  a->setWhatsThis(i18n("Reload all open documents."));

  a = actionCollection()->addAction( "file_close_orphaned" );
  a->setText( i18n("Close Orphaned") );
  connect( a, SIGNAL( triggered() ), KateDocManager::self(), SLOT( closeOrphaned() ) );
  a->setWhatsThis(i18n("Close all documents in the file list that could not be reopened, because they are not accessible anymore."));
  
  actionCollection()->addAction( KStandardAction::Close, "file_close", m_viewManager, SLOT( slotDocumentClose() ) )
  ->setWhatsThis(i18n("Close the current document."));

  a = actionCollection()->addAction( "file_close_other" );
  a->setText( i18n( "Close Other" ) );
  connect( a, SIGNAL( triggered() ), this, SLOT( slotDocumentCloseOther() ) );
  a->setWhatsThis(i18n("Close other open documents."));

  a = actionCollection()->addAction( "file_close_all" );
  a->setText( i18n( "Clos&e All" ) );
  connect( a, SIGNAL( triggered() ), this, SLOT( slotDocumentCloseAll() ) );
  a->setWhatsThis(i18n("Close all open documents."));

  actionCollection()->addAction( KStandardAction::Quit, "file_quit", this, SLOT( slotFileQuit() ) )
  ->setWhatsThis(i18n("Close this window"));

  a = actionCollection()->addAction( "view_new_view" );
  a->setIcon( KIcon("window-new") );
  a->setText( i18n("&New Window") );
  connect( a, SIGNAL( triggered() ), this, SLOT( newWindow() ) );
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  KToggleAction* showFullScreenAction = KStandardAction::fullScreen( 0, 0, this, this);
  actionCollection()->addAction( showFullScreenAction->objectName(), showFullScreenAction );
  connect( showFullScreenAction, SIGNAL(toggled(bool)), this, SLOT(slotFullScreen(bool)));

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), this);
  actionCollection()->addAction("file_open_with", documentOpenWith);
  documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
  connect(documentOpenWith->menu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->menu(), SIGNAL(triggered(QAction*)), this, SLOT(slotOpenWithMenuAction(QAction*)));

  a = KStandardAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a = KStandardAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection());
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));

  QAction* settingsConfigure = KStandardAction::preferences(this, SLOT(slotConfigure()), actionCollection());
  settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

  // tip of the day :-)
  actionCollection()->addAction( KStandardAction::TipofDay, this, SLOT( tipOfTheDay() ) )
  ->setWhatsThis(i18n("This shows useful tips on the use of this application."));

  if (KatePluginManager::self()->pluginList().count() > 0)
  {
    a = actionCollection()->addAction( "help_plugins_contents" );
    a->setText( i18n("&Plugins Handbook") );
    connect( a, SIGNAL( triggered() ), this, SLOT( pluginHelp() ) );
    a->setWhatsThis(i18n("This shows help files for various available plugins."));
  }

  a = actionCollection()->addAction( "help_about_editor" );
  a->setText( i18n("&About Editor Component") );
  connect( a, SIGNAL( triggered() ), this, SLOT( aboutEditor() ) );

  connect(m_viewManager, SIGNAL(viewChanged()), m_mainWindow, SIGNAL(viewChanged()));
  connect(m_viewManager, SIGNAL(viewCreated(KTextEditor::View *)), m_mainWindow, SIGNAL(viewCreated(KTextEditor::View *)));
  connect(m_viewManager, SIGNAL(viewChanged()), this, SLOT(slotWindowActivated()));
  connect(m_viewManager, SIGNAL(viewChanged()), this, SLOT(slotUpdateOpenWith()));
  connect(m_viewManager, SIGNAL(viewChanged()), this, SLOT(slotUpdateHorizontalViewBar()));

  slotWindowActivated ();

  // session actions
  a = actionCollection()->addAction( "sessions_new" );
  a->setIcon( KIcon("document-new") );
  a->setText( i18nc("Menu entry Session->New", "&New") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionNew() ) );
  a = actionCollection()->addAction( "sessions_open" );
  a->setIcon( KIcon("document-open") );
  a->setText( i18n("&Open...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionOpen() ) );
  a = actionCollection()->addAction( "sessions_save" );
  a->setIcon( KIcon("document-save") );
  a->setText( i18n("&Save") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSave() ) );
  a = actionCollection()->addAction( "sessions_save_as" );
  a->setIcon( KIcon("document-save-as") );
  a->setText( i18n("Save &As...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSaveAs() ) );
  a = actionCollection()->addAction( "sessions_save_default" );
  a->setIcon( KIcon("document-save-as") );
  a->setText( i18n("Save as &Default...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionSaveAsDefault() ) );
  a = actionCollection()->addAction( "sessions_manage" );
  a->setIcon( KIcon("view-choose") );
  a->setText( i18n("&Manage...") );
  connect( a, SIGNAL( triggered() ), KateSessionManager::self(), SLOT( sessionManage() ) );

  // quick open menu ;)
  a = new KateSessionsAction (i18n("&Quick Open"), this);
  actionCollection()->addAction("sessions_list", a);
}

void KateMainWindow::slotDocumentCloseAll()
{
  if (queryClose_internal())
    KateDocManager::self()->closeAllDocuments(false);
}


void KateMainWindow::slotDocumentCloseOther(KTextEditor::Document *document)
{
  if (queryClose_internal(document))
    KateDocManager::self()->closeOtherDocuments(document);
}

void KateMainWindow::slotDocumentCloseOther()
{
  if (queryClose_internal(m_viewManager->activeView()->document()))
    KateDocManager::self()->closeOtherDocuments(m_viewManager->activeView()->document());
}

bool KateMainWindow::queryClose_internal(KTextEditor::Document* doc)
{
  uint documentCount = KateDocManager::self()->documents();

  if ( ! showModOnDiskPrompt() )
    return false;

  QList<KTextEditor::Document*> modifiedDocuments = KateDocManager::self()->modifiedDocumentList(doc);
  bool shutdown = (modifiedDocuments.count() == 0);

  if (!shutdown)
  {
    shutdown = KateSaveModifiedDialog::queryClose(this, modifiedDocuments);
  }

  if ( KateDocManager::self()->documents() > documentCount )
  {
    KMessageBox::information (this,
                              i18n ("New file opened while trying to close Kate, closing aborted."),
                              i18n ("Closing Aborted"));
    shutdown = false;
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
    KateApp::self()->sessionManager()->saveActiveSession(true);
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
  saveMainWindowSettings(KConfigGroup(KGlobal::config(), "MainWindow"));
  KEditToolBar dlg( factory() );

  connect( &dlg, SIGNAL(newToolbarConfig()), this, SLOT(slotNewToolbarConfig()) );
  dlg.exec();
}

void KateMainWindow::slotNewToolbarConfig()
{
  applyMainWindowSettings(KConfigGroup(KGlobal::config(), "MainWindow"));
}

void KateMainWindow::slotFileQuit()
{
  // delay real shutdown outside of this, as we are inside menu action handling
  QTimer::singleShot(0, this, SLOT(slotFileQuitDelayed()));
}

void KateMainWindow::slotFileQuitDelayed()
{
  KateApp::self()->shutdownKate (this);
}

void KateMainWindow::readOptions ()
{
  KSharedConfig::Ptr config = KGlobal::config();

  const KConfigGroup generalGroup(config, "General");
  modNotification = generalGroup.readEntry("Modified Notification", false);
  KateDocManager::self()->setSaveMetaInfos(generalGroup.readEntry("Save Meta Infos", true));
  KateDocManager::self()->setDaysMetaInfos(generalGroup.readEntry("Days Meta Infos", 30));

  m_paShowPath->setChecked (generalGroup.readEntry("Show Full Path in Title", false));
  m_paShowStatusBar->setChecked (generalGroup.readEntry("Show Status Bar", true));

  // emit signal to hide/show statusbars
  toggleShowStatusBar ();
}

void KateMainWindow::saveOptions ()
{
  KSharedConfig::Ptr config = KGlobal::config();

  KConfigGroup generalGroup(config, "General");

  generalGroup.writeEntry("Save Meta Infos", KateDocManager::self()->getSaveMetaInfos());

  generalGroup.writeEntry("Days Meta Infos", KateDocManager::self()->getDaysMetaInfos());

  generalGroup.writeEntry("Show Full Path in Title", m_paShowPath->isChecked());
  generalGroup.writeEntry("Show Status Bar", m_paShowStatusBar->isChecked());

#ifdef __GNUC__
#warning PORTME
#endif
  //filelist->writeConfig(config.data(), "Filelist");
}

void KateMainWindow::toggleShowStatusBar ()
{
  emit statusBarToggled ();
}

bool KateMainWindow::showStatusBar ()
{
  return m_paShowStatusBar->isChecked ();
}

void KateMainWindow::slotWindowActivated ()
{
  if (m_viewManager->activeView())
  {
    m_documentModel->opened(modelIndexForDocument(m_viewManager->activeView()->document()));
    updateCaption (m_viewManager->activeView()->document());
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

class KateRowColumn
{
  public:
    KateRowColumn(): m_row(-1), m_column(-1)
    {}
    KateRowColumn(int row, int column): m_row(row), m_column(column)
    {}
    ~KateRowColumn()
    {}
    int column()
    {
      return m_column;
    }
    int row()
    {
      return m_row;
    }
    bool isValid()
    {
      return ( (m_row >= 0) && (m_column >= 0));
    }
  private:
    int m_row;
    int m_column;
};

Q_DECLARE_METATYPE(KateRowColumn)


void KateMainWindow::documentMenuAboutToShow()
{
  qRegisterMetaType<KTextEditor::Document*>("KTextEditor::Document*");
  qDeleteAll( documentsGroup->actions() );
  int rows = m_fileList->model()->rowCount(QModelIndex());
  QAbstractItemModel *model = m_fileList->model();
  for (int row = 0;row < rows;row++)
  {
    QModelIndex index = model->index(row, 0, QModelIndex());
    Q_ASSERT(index.isValid());
    KTextEditor::Document *doc = index.data(KateDocManager::DocumentRole).value<KTextEditor::Document*>();
    const QString name = KStringHandler::rsqueeze(doc->documentName(), 150);
    QAction *action = new QAction(doc->isModified() ?
                                  i18nc("'document name [*]', [*] means modified", "%1 [*]", name) : name,
                                  documentsGroup );
    action->setCheckable(true);
    if(m_viewManager->activeView() && doc == m_viewManager->activeView()->document())
      action->setChecked(true);
    action->setData(QVariant::fromValue(KateRowColumn(index.row(), index.column())));
    documentMenu->addAction(action);
  }
}

void KateMainWindow::activateDocumentFromDocMenu (QAction *action)
{
  KateRowColumn rowCol = action->data().value<KateRowColumn>();
  if (!rowCol.isValid()) return;
  QModelIndex index = m_documentModel->index(rowCol.row(), rowCol.column());
  if (index.isValid())
  {
    KTextEditor::Document *doc = index.data(KateDocManager::DocumentRole).value<KTextEditor::Document*>();
    if (doc)
      m_viewManager->activateView (doc);
    m_documentModel->opened(index);
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
  if (event->mimeData() == 0) return;
  KUrl::List textlist = KUrl::List::fromMimeData(event->mimeData());

  // Try to get the KTextEditor::View that sent this, and activate it, so that the file opens in the
  // view where it was dropped
  KTextEditor::View *kVsender = qobject_cast<KTextEditor::View *>(QObject::sender());
  if (kVsender != 0) {
    QWidget *parent = kVsender->parentWidget();
    if (parent != 0) {
      KateViewSpace* vs = qobject_cast<KateViewSpace *>(parent->parentWidget());
      if (vs != 0) m_viewManager->setActiveSpace(vs);
    }
  }
  
  for (KUrl::List::Iterator i = textlist.begin(); i != textlist.end(); ++i)
  {
    m_viewManager->openUrl (*i);
  }
}

void KateMainWindow::editKeys()
{
  KShortcutsDialog dlg ( KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this );

  QList<KXMLGUIClient*> clients = guiFactory()->clients();

  foreach(KXMLGUIClient *client, clients) {
#ifdef __GNUC__
#warning there appear to be invalid clients after session switching    
#endif
//     kDebug(13001)<<"adding client to shortcut editor";
//     kDebug(13001)<<client;
//     kDebug(13001)<<client->actionCollection();
//     kDebug(13001)<<client->componentData().aboutData();
//     kDebug(13001)<<client->componentData().aboutData()->programName();
    dlg.addCollection ( client->actionCollection(), client->componentData().aboutData()->programName() );
  }
  /*
    dlg.insert( externalTools->actionCollection(), i18n("External Tools") );
  */
  dlg.configure();

  QList<KTextEditor::Document*>  l = KateDocManager::self()->documentList();
  for (int i = 0;i < l.count();i++)
  {
//     kDebug(13001)<<"reloading Keysettings for document "<<i;
    l.at(i)->reloadXML();
    QList<KTextEditor::View *> l1 = l.at(i)->views ();
    for (int i1 = 0;i1 < l1.count();i1++)
    {
      l1.at(i1)->reloadXML();
//       kDebug(13001)<<"reloading Keysettings for view "<<i<<"/"<<i1;
    }
  }

  //externalTools->actionCollection()->writeSettings( new KConfig("externaltools", false, false, "appdata") );
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

  m_viewManager->reactivateActiveView(); // gui (toolbars...) needs to be updated, because
                                         // of possible changes that the configure dialog
                                         // could have done on it, specially for plugins.
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
void KateMainWindow::fileSelected(const KFileItem & /*file*/)
{
  const KFileItemList list = fileselector->dirOperator()->selectedItems();

  foreach (const KFileItem tmp, list )
  {
    m_viewManager->openUrl(tmp.url());
    fileselector->dirOperator()->view()->setSelected(tmp, false);
  }
}
#endif

void KateMainWindow::mSlotFixOpenWithMenu()
{
  KMenu *menu = documentOpenWith->menu();
  menu->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::mimeType(m_viewManager->activeView()->document()->mimeType());
  //kDebug(13001) << "mime type: " << mime->name();

  QAction *a = 0;
  KService::List offers = KMimeTypeTrader::self()->query(mime->name(), "Application");
  // for each one, insert a menu item...
  for(KService::List::Iterator it = offers.begin(); it != offers.end(); ++it)
  {
    KService::Ptr service = *it;
    if (service->name() == "Kate") continue;
    a = menu->addAction(KIcon(service->icon()), service->name());
    a->setData(service->entryPath());
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
  if (openWith.isEmpty())
  {
    // display "open with" dialog
    KOpenWithDialog dlg(list);
    if (dlg.exec())
      KRun::run(*dlg.service(), list, this);
    return;
  }

  KService::Ptr app = KService::serviceByDesktopPath(openWith);
  if (app)
  {
    KRun::run(*app, list, this);
  }
  else
  {
    KMessageBox::error(this, i18n("Application '%1' not found.", openWith), i18n("Application not found"));
  }
}

void KateMainWindow::pluginHelp()
{
  KToolInvocation::invokeHelp (QString(), "kate-plugins");
}

void KateMainWindow::aboutEditor()
{
  KAboutApplicationDialog ad(KateDocManager::self()->editor()->aboutData(),this);
  ad.exec();
}

void KateMainWindow::tipOfTheDay()
{
  KTipDialog::showTip( /*0*/this, QString(), true );
}

void KateMainWindow::slotFullScreen(bool t)
{
  KToggleFullScreenAction::setFullScreen(this, t);
}

bool KateMainWindow::event( QEvent *e )
{
/*  uint type = e->type();
  if ( type == QEvent::WindowActivate && modNotification )
  {
    showModOnDiskPrompt();
  }*/
  return KateMDI::MainWindow::event( e );
}

bool KateMainWindow::showModOnDiskPrompt()
{
  KTextEditor::Document *doc;

  DocVector list;
  list.reserve( KateDocManager::self()->documents() );
  foreach( doc, KateDocManager::self()->documentList())
  {
    if ( KateDocManager::self()->documentInfo( doc )->modifiedOnDisc )
    {
      list.append( doc );
    }
  }

  if ( !list.isEmpty() && !m_modignore )
  {
    KateMwModOnHdDialog mhdlg( list, this );
    m_modignore = true;
    bool res = mhdlg.exec();
    m_modignore = false;

    return res;
  }
  return true;
}

void KateMainWindow::slotDocumentCreated (KTextEditor::Document *doc)
{
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(slotDocModified(KTextEditor::Document *)));
  connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(updateCaption(KTextEditor::Document *)));
  connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document *)), this, SLOT(updateCaption(KTextEditor::Document *)));
  connect(doc, SIGNAL(documentNameChanged(KTextEditor::Document *)), this, SLOT(slotUpdateOpenWith()));

  updateCaption (doc);
}

void KateMainWindow::updateCaption ()
{
  if (m_viewManager->activeView())
    updateCaption(m_viewManager->activeView()->document());
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
  if (m_viewManager->activeView()->document()->url().isEmpty() || (!m_paShowPath || !m_paShowPath->isChecked()))
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

  setCaption( sessName + KStringHandler::lsqueeze(c, 64),
              m_viewManager->activeView()->document()->isModified());
}

void KateMainWindow::saveProperties(KConfigGroup& config)
{
  saveSession(config);

  // store all plugin view states
  int id = KateApp::self()->mainWindowID (this);
  foreach(const KatePluginInfo &item, KatePluginManager::self()->pluginList())
  {
      if (item.plugin && pluginViews().value(item.plugin)) {
          pluginViews().value(item.plugin)->writeSessionConfig (config.config(),
              QString("Plugin:%1:MainWindow:%2").arg(item.saveName()).arg(id) );
      }
  }

  m_documentModel->writeSessionConfig( config.config(),
      QString("DocumentModel:MainWindow:%1").arg(id) );

  fileOpenRecent->saveEntries( KConfigGroup(config.config(), "Recent Files" ) );
  m_viewManager->saveViewConfiguration (config);
}

void KateMainWindow::readProperties(const KConfigGroup& config)
{
  // TODO startRestore should take a const KConfigBase*, or even just a const KConfigGroup&,
  // but this propagates down to interfaces/kate/plugin.h so all plugins have to be ported
  KConfigBase* configBase = const_cast<KConfig *>(config.config());
  startRestore(configBase, config.name());

  // perhaps enable plugin guis
  KatePluginManager::self()->enableAllPluginsGUI (this, configBase);

  finishRestore ();

  int id = KateApp::self()->mainWindowID (this);
  m_documentModel->readSessionConfig( config.config(),
      QString("DocumentModel:MainWindow:%1").arg(id) );

  fileOpenRecent->loadEntries( KConfigGroup(config.config(), "Recent Files" ) );
  m_viewManager->restoreViewConfiguration (config);
}

void KateMainWindow::saveGlobalProperties( KConfig* sessionConfig )
{
  KateDocManager::self()->saveDocumentList (sessionConfig);

  KConfigGroup cg( sessionConfig, "General");
  cg.writeEntry ("Last Session", KateApp::self()->sessionManager()->activeSession()->sessionFileRelative());
}

void KateMainWindow::showFileListPopup(const QPoint& pos)
{
  if (m_fileList->selectionModel()->selection().count() == 0) return;
  QMenu *menu = (QMenu*) (factory()->container("filelist_popup", m_viewManager->mainWindow()));
  if (menu) menu->exec(m_fileList->viewport()->mapToGlobal(pos));
}


static QModelIndex modelIndexForDocumentRec(const QModelIndex &index, QAbstractItemModel *model)
{
  QAbstractProxyModel *m = qobject_cast<QAbstractProxyModel*>(model);
  if (m == 0) return index;
  return m->mapFromSource(modelIndexForDocumentRec(index, m->sourceModel()));
}

QModelIndex KateMainWindow::modelIndexForDocument(KTextEditor::Document *document)
{
  KTextEditor::Document *tmp = m_documentModel->selection()->currentIndex().data(KateDocManager::DocumentRole).value<KTextEditor::Document*>();
  if (tmp == document) return m_documentModel->selection()->currentIndex();
  else return modelIndexForDocumentRec(KateDocManager::self()->indexForDocument(document), m_documentModel);
}


void KateMainWindow::slotDocModified(KTextEditor::Document *document)
{
  if (document->isModified()) m_documentModel->modified(modelIndexForDocument(document));
}

void KateMainWindow::saveWindowConfig(const KConfigGroup &_config)
{
  KConfigGroup config( _config );
  saveMainWindowSettings(config);
  saveWindowSize(config);
  config.writeEntry("WindowState", int(((KParts::MainWindow*)this)->windowState()));
  config.sync();
}

void KateMainWindow::restoreWindowConfig(const KConfigGroup &config)
{
  applyMainWindowSettings(config);
  restoreWindowSize(config);
  setWindowState( QFlags<Qt::WindowState>(config.readEntry("WindowState", int(Qt::WindowActive))) );
}

void KateMainWindow::slotUpdateHorizontalViewBar()
{
  kDebug()<<"slotUpdateHorizontalViewBar()"<<endl;
  KTextEditor::View *view=m_viewManager->activeView();
  BarState bs=m_viewBarMapping[view];
  if (bs.bar() && bs.state()) {
    m_containerstack->setCurrentWidget(bs.bar());
    m_containerstack->currentWidget()->show();
    m_horizontalViewBarContainer->show();
  } else {
    QWidget *wid=m_containerstack->currentWidget();
    if (wid) wid->hide();
    kDebug()<<wid<<"hiding container"<<endl;
    m_horizontalViewBarContainer->hide();
  }
}

void KateMainWindow::queueModifiedOnDisc(KTextEditor::Document *doc)
{
  if (!modNotification) return;

  if (s_modOnHdDialog==0) {
    DocVector list;
    list.reserve( KateDocManager::self()->documents() );
    list.append(doc);
    
    s_modOnHdDialog= new KateMwModOnHdDialog( list, this );
    m_modignore = true;
    KWindowSystem::setOnAllDesktops( s_modOnHdDialog->winId(), true);
    bool res = s_modOnHdDialog->exec();
    delete s_modOnHdDialog;
    m_modignore = false;
    return;
  } else {
    s_modOnHdDialog->addDocument(doc);
    //if (qApp->activeWindow()) {
//     s_modOnHdDialog->raise();
//   #ifdef Q_WS_X11
//   KWindowSystem::activateWindow (s_modOnHdDialog->winId ());
//   KWindowSystem::raiseWindow (s_modOnHdDialog->winId ());
//   KWindowSystem::setCurrentDesktop(
//   #endif
    return;
  }
}


// kate: space-indent on; indent-width 2; replace-tabs on;
