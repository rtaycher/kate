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
#include "kateprojectlist.h"
#include "kateprojectviews.h"
#include "katefileselector.h"
#include "katefilelist.h"
#include "katemailfilesdialog.h"
#include "katemainwindowiface.h"

#include <kmdichildview.h>
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

#include <qlayout.h>

#include <ktip.h>

#include <kmenubar.h>

#include "kategrepdialog.h"

#include <assert.h>
#include <unistd.h>
//END

uint KateMainWindow::uniqueID = 1;
KMdi::MdiMode KateMainWindow::defaultMode=KMdi::UndefinedMode;
static bool sessionClosingAccepted=false;
static bool sessionClosingStarted=false;
static bool sessionClosingStarted2=false;

KateMainWindow::KateMainWindow(KateDocManager *_m_docManager, KatePluginManager *_m_pluginManager,
	KateProjectManager *projectMan,KMdi::MdiMode guiMode) :
	KMdiMainFrm (0,(QString("__KateMainWindow#%1").arg(uniqueID)).latin1(),guiMode)
{
  // first the very important id
  myID = uniqueID;
  uniqueID++;

  // init some vars
  m_docManager =  _m_docManager;
  m_pluginManager =_m_pluginManager;
  m_projectManager = projectMan;

  m_project = 0;
  m_projectNumber = 0;

  activeView = 0;

  console = 0L;

  // now the config
  config = kapp->config();

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

  m_mainWindow = new Kate::MainWindow (this);
  m_toolViewManager = new Kate::ToolViewManager (this);
  setStandardMDIMenuEnabled(false);
  setManagedDockPositionModeEnabled(true);

  m_dcop = new KateMainWindowDCOPIface (this);

  // setup the most important widgets
  setupMainWindow();
  
  // setup the actions
  setupActions();
  projectlist->setupActions();

  setStandardToolBarMenuEnabled( true );
  setXMLFile( "kateui.rc" );
  createShellGUI ( true );

  m_pluginManager->enableAllPluginsGUI (this);

  // connect settings menu aboutToshow
  documentMenu = (QPopupMenu*)factory()->container("documents", this);
  connect(documentMenu, SIGNAL(aboutToShow()), this, SLOT(documentMenuAboutToShow()));

  connect(m_projectManager->projectManager(),SIGNAL(projectDeleted(uint)),this,SLOT(projectDeleted(uint)));

  // caption update
  for (uint i = 0; i < m_docManager->documents(); i++)
    slotDocumentCreated (m_docManager->document(i));

  connect(m_docManager,SIGNAL(documentCreated(Kate::Document *)),this,SLOT(slotDocumentCreated(Kate::Document *)));

  readOptions(config);

  if (console)
    console->loadConsoleIfNeeded();

  setAcceptDrops(true);
}

KateMainWindow::~KateMainWindow()
{
  m_pluginManager->disableAllPluginsGUI (this);

  delete m_dcop;
  delete kscript;
}

void KateMainWindow::setupMainWindow ()
{
  greptool = new GrepTool( QDir::homeDirPath(), this, "greptool" );
  greptool->installEventFilter( this );
  connect(greptool, SIGNAL(itemSelected(const QString &,int)), this, SLOT(slotGrepToolItemSelected(const QString &,int)));

  KMdiChildView* pMDICover = new KMdiChildView("MainDock");
  pMDICover->setName("MainDock");

  //mainDock->setGeometry(100, 100, 100, 100);
  QBoxLayout *ml=new QHBoxLayout(pMDICover);
  ml->setAutoAdd(true);
  m_viewManager = new KateViewManager (pMDICover, m_docManager,this);
  addWindow(pMDICover);
  m_viewManager->show();
  pMDICover->show();

  filelist = new KateFileList (m_docManager, m_viewManager, this/*filelistDock*/, "filelist");
  addToolView(KDockWidget::DockLeft,filelist,SmallIcon("kmultiple"), i18n("Files"));

  QVBox *prBox = new QVBox (this,"projects");
  addToolView(KDockWidget::DockLeft,prBox,SmallIcon("view_tree"), i18n("Projects"));
  projectlist = new KateProjectList (m_projectManager, this, prBox/*filelistDock*/, "projectlist");
  projectviews = new KateProjectViews (m_projectManager, this, prBox/*filelistDock*/, "projectviews");
  prBox->setStretchFactor(projectviews, 2);
  prBox->show ();
  projectlist->show ();
  projectviews->show ();

  fileselector = new KateFileSelector( this, m_viewManager, /*fileselectorDock*/ this, "operator");
  addToolView(KDockWidget::DockLeft,fileselector, SmallIcon("fileopen"), i18n("Selector"));

  // TEST
  addToolView( KDockWidget::DockBottom, greptool, SmallIcon("filefind"), i18n("Find in Files") );
  if (kapp->authorize("shell_access"))
  {
     console = new KateConsole (this, "console",viewManager());
     console->installEventFilter( this );
     addToolView(KDockWidget::DockBottom,console, SmallIcon("konsole"), i18n("Terminal"));
  }

  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));
}

void KateMainWindow::setupActions()
{
  KAction *a;

  kscript = new KScriptManager(this, "scriptmanager");
  scriptMenu = new KActionMenu( i18n("KDE Scri&pts"), actionCollection(), "scripts");
  scriptMenu->setWhatsThis(i18n("This shows all available scripts and allows them to be executed."));
  setupScripts();
  connect( scriptMenu->popupMenu(), SIGNAL(activated( int)), this, SLOT(runScript( int )) );

  KStdAction::openNew( m_viewManager, SLOT( slotDocumentNew() ), actionCollection(), "file_new" )->setWhatsThis(i18n("Create a new document"));
  KStdAction::open( m_viewManager, SLOT( slotDocumentOpen() ), actionCollection(), "file_open" )->setWhatsThis(i18n("Open an existing document for editing"));

  fileOpenRecent = KStdAction::openRecent (m_viewManager, SLOT(openURL (const KURL&)), actionCollection());
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=new KAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, m_viewManager, SLOT( slotDocumentSaveAll() ), actionCollection(), "file_save_all" );
  a->setWhatsThis(i18n("Save all open, modified documents to disc."));

  KStdAction::close( m_viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" )->setWhatsThis(i18n("Close the current document."));

  a=new KAction( i18n( "Clos&e All" ), 0, m_viewManager, SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );
  a->setWhatsThis(i18n("Close all open documents."));

  KStdAction::mail( this, SLOT(slotMail()), actionCollection() )->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" )->setWhatsThis(i18n("Close this window"));

  a=new KAction(i18n("&New Window"), "window_new", 0, this, SLOT(newWindow()), actionCollection(), "view_new_view");
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  a=new KAction( i18n("Split &Vertical"), "view_left_right", CTRL+SHIFT+Key_L, m_viewManager, SLOT( slotSplitViewSpaceVert() ), actionCollection(), "view_split_vert");
  a->setWhatsThis(i18n("Split the currently active view vertically into two views."));

  a=new KAction( i18n("Split &Horizontal"), "view_top_bottom", CTRL+SHIFT+Key_T, m_viewManager, SLOT( slotSplitViewSpaceHoriz() ), actionCollection(), "view_split_horiz");
  a->setWhatsThis(i18n("Split the currently active view horizontally into two views."));

  a=closeCurrentViewSpace = new KAction( i18n("Close &Current View"), "view_remove", CTRL+SHIFT+Key_R, m_viewManager, SLOT( slotCloseCurrentViewSpace() ), actionCollection(), "view_close_current_space");
  a->setWhatsThis(i18n("Close the currently active splitted view"));

  showFullScreenAction = KStdAction::fullScreen( 0, 0, actionCollection(),this);
  connect( showFullScreenAction,SIGNAL(toggled(bool)), this,SLOT(slotFullScreen(bool)));

  goNext=new KAction(i18n("Next View"),Key_F8,m_viewManager, SLOT(activateNextView()),actionCollection(),"go_next");
  goNext->setWhatsThis(i18n("Make the next split view the active one."));

  goPrev=new KAction(i18n("Previous View"),SHIFT+Key_F8,m_viewManager, SLOT(activatePrevView()),actionCollection(),"go_prev");
  goPrev->setWhatsThis(i18n("Make the previous split view the active one."));

  windowNext = KStdAction::back(m_viewManager, SLOT(slotWindowNext()), actionCollection());
  windowPrev = KStdAction::forward(m_viewManager, SLOT(slotWindowPrev()), actionCollection());

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), actionCollection(), "file_open_with");
  documentOpenWith->setWhatsThis(i18n("Open the current document using another application registered for its file type, or an application of your choice."));
  connect(documentOpenWith->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotOpenWithMenuAction(int)));

  a=KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  a->setWhatsThis(i18n("Configure the application's keyboard shortcut assignments."));

  a=KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");
  a->setWhatsThis(i18n("Configure which items should appear in the toolbar(s)."));


  // project menu
  a = new KAction(i18n("&New Project..."), "filenew", 0, this, SLOT(slotProjectNew()), actionCollection(), "project_new");
  a = new KAction(i18n("&Open Project..."), "fileopen", 0, this, SLOT(slotProjectOpen()), actionCollection(), "project_open");
  saveProject = new KAction(i18n("&Save Project"), "filesave", 0, this, SLOT(slotProjectSave()), actionCollection(), "project_save");
  closeProject = new KAction(i18n("&Close Project"), "fileclose", 0, this, SLOT(slotProjectClose()), actionCollection(), "project_close");

  recentProjects = new KRecentFilesAction (i18n("Open &Recent"), KShortcut(), this, SLOT(openConstURLProject (const KURL&)),actionCollection(), "project_open_recent");

  settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");
  settingsConfigure->setWhatsThis(i18n("Configure various aspects of this application and the editing component."));

  // tip of the day :-)
  KStdAction::tipOfDay( this, SLOT( tipOfTheDay() ), actionCollection() )->setWhatsThis(i18n("This shows useful tips on the use of this application."));

  if (m_pluginManager->pluginList().count() > 0)
  {
    a=new KAction(i18n("Contents &Plugins"), 0, this, SLOT(pluginHelp()), actionCollection(), "help_plugins_contents");
    a->setWhatsThis(i18n("This shows help files for various available plugins."));
  }

  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));
  connect(m_docManager,SIGNAL(documentChanged()),this,SLOT(slotDocumentChanged()));

  slotWindowActivated ();
  slotDocumentChanged();
}

/**
 * queryClose(), take care that after the last mainwindow the stuff is closed
 */
bool KateMainWindow::queryClose()
{

  kdDebug(13000)<<"QUERY CLOSE ********************"<<endl;
  bool val = false;

  if (!kapp->sessionSaving()) {
    if ( ((KateApp *)kapp)->mainWindows () < 2 )
    {
      // store the stuff
      KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

      m_projectManager->saveProjectList (scfg);

      saveOptions(config);

      if (m_projectManager->closeAll ())
      {
        // first ask if something has changed

	val=m_docManager->queryCloseDocuments(this);
	if (!val)
        {
            delete scfg;
            return false;
        }
        m_docManager->saveDocumentList (scfg);
        saveWindowConfiguration (scfg);

        //m_docManager->closeAllDocuments();

        if ( !m_docManager->activeDocument() || !m_viewManager->activeView() ||
           ( !m_viewManager->activeView()->getDoc()->isModified() && m_docManager->documents() == 1 ) )
        {
           if( m_viewManager->activeView() )
             m_viewManager->deleteLastView();
           val = true;
        }
     }
     scfg->sync();
     delete scfg;
    }
    else
      val = true;
  }
  else
  {
	val=sessionClosingAccepted;
	if (!sessionClosingStarted) {
		val=sessionClosingStarted=sessionClosingAccepted=m_docManager->queryCloseDocuments(this);
		if (val) sessionClosingStarted2=true;
	}
	if (!val) sessionClosingStarted=false;

  }


  if (val)
    ((KateApp *)kapp)->removeMainWindow (this);

  return val;
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
  close ();
}

void KateMainWindow::readOptions(KConfig *config)
{
  applyMainWindowSettings(config, "Kate Main Window");

  config->setGroup("General");
  syncKonsole =  config->readBoolEntry("Sync Konsole", true);
  modNotification = config->readBoolEntry("Modified Notification", false);

  m_viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));

  fileOpenRecent->setMaxItems( config->readNumEntry("Number of recent files", fileOpenRecent->maxItems() ) );
  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");

  filelist->setSortType(config->readNumEntry("Sort Type of File List", KateFileList::sortByID));

  recentProjects->loadEntries (config, "Recent Projects");
}

void KateMainWindow::saveOptions(KConfig *config)
{
  saveMainWindowSettings(config, "Kate Main Window");

  config->setGroup("General");

  if (console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);

  config->writeEntry("Show Full Path in Title", m_viewManager->getShowFullPath());

  config->writeEntry("Sync Konsole", syncKonsole);

  fileOpenRecent->saveEntries(config, "Recent Files");

  fileselector->writeConfig(config, "fileselector");

  config->writeEntry("Sort Type of File List", filelist->sortType());

  recentProjects->saveEntries (config, "Recent Projects");
}

void KateMainWindow::slotDocumentChanged()
{
  if (m_docManager->documents()  > 1)
  {
    windowNext->setEnabled(true);
    windowPrev->setEnabled(true);
  }
  else
  {
    windowNext->setEnabled(false);
    windowPrev->setEnabled(false);
  }
}

void KateMainWindow::slotWindowActivated ()
{
  static QString path;

  if (m_viewManager->activeView())
  {
    documentOpenWith->setEnabled (!m_viewManager->activeView()->document()->url().isEmpty());

    if (console && syncKonsole)
    {
      QString newPath = m_viewManager->activeView()->getDoc()->url().directory();

      if ( newPath != path )
      {
        path = newPath;
        console->cd (path);
      }
    }

    updateCaption (m_viewManager->activeView()->getDoc());
  }
  else
  {
    documentOpenWith->setEnabled (false);
  }

  if (m_viewManager->viewSpaceCount() == 1)
    closeCurrentViewSpace->setEnabled(false);
  else
    closeCurrentViewSpace->setEnabled(true);
}

void KateMainWindow::documentMenuAboutToShow()
{
  documentMenu->clear ();
  windowNext->plug (documentMenu);
  windowPrev->plug (documentMenu);
  documentMenu->insertSeparator ();

  uint z=0;
  int i=1;

  QString entry;
  while ( z<m_docManager->documents() )
  {
    //File name shouldn't be too long - Maciek
     entry = m_docManager->document(z)->docName();
     if (entry.length() > 200)
       entry = "..." + entry.right(197);
  
    if (m_docManager->document(z)->isModified())
      entry.append (i18n(" - Modified"));

    documentMenu->insertItem ( entry, m_viewManager, SLOT (activateView (int)), 0,  m_docManager->document(z)->documentNumber());

    if (m_viewManager->activeView())
      documentMenu->setItemChecked( m_docManager->document(z)->documentNumber(), ((Kate::Document *)m_viewManager->activeView()->getDoc())->documentNumber() == m_docManager->document(z)->documentNumber() );

    z++;
    i++;
  }
}

void KateMainWindow::slotGrepToolItemSelected(const QString &filename,int linenumber)
{
  KURL fileURL;
  fileURL.setPath( filename );
  m_viewManager->openURL( fileURL );
  if ( m_viewManager->activeView() == 0 ) return;
  m_viewManager->activeView()->gotoLineNumber( linenumber );
  this->raise();
  this->setActiveWindow();
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

  dlg.configure();

  QPtrList<Kate::Document>  l=m_docManager->documentList();
  for (uint i=0;i<l.count();i++) {
	kdDebug(13001)<<"reloading Keysettings for document "<<i<<endl;
	l.at(i)->reloadXML();
	QPtrList<class KTextEditor::View> l1=l.at(i)->views ();//KTextEditor::Document
	for (uint i1=0;i1<l1.count();i1++) {
		l1.at(i1)->reloadXML();
		kdDebug(13001)<<"reloading Keysettings for view "<<i<<"/"<<i1<<endl;

	}

  }
}

void KateMainWindow::openURL (const QString &name)
{
  m_viewManager->openURL (KURL(name));
}

void KateMainWindow::slotConfigure()
{
  KateConfigDialog* dlg = new KateConfigDialog (this, "configdialog");
  dlg->exec();
  delete dlg;
}

//Set focus to next input element
void KateMainWindow::slotGoNext()
{
  QFocusEvent::setReason(QFocusEvent::Tab);
  /*res= */focusNextPrevChild(true); //TRUE == NEXT , FALSE = PREV
  QFocusEvent::resetReason();
}

//Set focus to previous input element
void KateMainWindow::slotGoPrev()
{
  QFocusEvent::setReason(QFocusEvent::Tab);
  /*res= */focusNextPrevChild(false); //TRUE == NEXT , FALSE = PREV
  QFocusEvent::resetReason();
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

void KateMainWindow::fileSelected(const KFileItem *file)
{
  m_viewManager->openURL( file->url() );
}

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
	//kdDebug(13000) << "Starting script engine..." << endl;
        kdDebug(13001)<<"runScript( "<<mIId<<" ) ["<<scriptMenu->popupMenu()->text( mIId )<<"]"<<endl;
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
  if ( o == greptool && e->type() == QEvent::Show && activeView )
  {
    if ( activeView->getDoc()->url().isLocalFile() )
    {
      QString dir ( activeView->getDoc()->url().directory() );
      greptool->setDirName( dir );
         return true;
    }
  }
  if ( ( o == greptool || o == console ) &&
      e->type() == QEvent::Hide && activeView )
  {
     activeView->setFocus();
     return true;
  }
  return KMdiMainFrm::eventFilter( o, e );
}

KMdiToolViewAccessor *KateMainWindow::addToolView(KDockWidget::DockPosition position, QWidget *widget, const QPixmap &icon, const QString &sname, const QString &tabToolTip, const QString &tabCaption)
{
  widget->setIcon(icon);
  widget->setCaption(sname);
  
  return addToolWindow(widget, position, getMainDockWidget(), 25, tabToolTip, tabCaption);
}

bool KateMainWindow::removeToolView(QWidget *w)
{
  deleteToolWindow (w);
  return true;
}

bool KateMainWindow::removeToolView(KMdiToolViewAccessor *accessor)
{
  deleteToolWindow (accessor);
  return true;
}

bool KateMainWindow::showToolView(QWidget *){return false;}
bool KateMainWindow::showToolView(KMdiToolViewAccessor *){return false;}

bool KateMainWindow::hideToolView(QWidget *){return false;}
bool KateMainWindow::hideToolView(KMdiToolViewAccessor *){return false;}

void KateMainWindow::slotProjectNew ()
{
  ProjectInfo *info = m_projectManager->newProjectDialog (this);

  if (info)
  {
    createProject (info->type, info->name, info->fileName);
    delete info;
  }
}

void KateMainWindow::slotProjectOpen ()
{
  QString fileName = KFileDialog::getOpenFileName (QString::null, QString ("*.kateproject|") + i18n("Kate Project Files") + QString (" (*.kateproject)"), this, i18n("Open Kate Project"));

  if (!fileName.isEmpty())
    openProject (fileName);
}

void KateMainWindow::slotProjectSave ()
{
  if (m_project)
    m_project->save ();
}

void KateMainWindow::slotProjectClose ()
{
  if (m_project)
  {
    m_projectManager->close (m_project);
  }
}

void KateMainWindow::activateProject (Kate::Project *project)
{
  kdDebug(13001)<<"activating project "<<project<<endl;
  if (m_project)
    m_projectManager->disableProjectGUI (m_project, this);

  if (project)
    m_projectManager->enableProjectGUI (project, this);

  m_project = project;

  if (project)
  {
    m_projectManager->setCurrentProject (project);
    m_projectNumber = project->projectNumber ();
  }
  else
    m_projectNumber = 0;

  emit m_mainWindow->projectChanged ();
}

Kate::Project *KateMainWindow::createProject (const QString &type, const QString &name, const QString &filename)
{
  Kate::Project *project = m_projectManager->create (type, name, filename);

  if (project)
    activateProject (project);

  return project;
}

Kate::Project *KateMainWindow::openProject (const QString &filename)
{
  Kate::Project *project = m_projectManager->open (filename);

  if (project)
  {
    recentProjects->addURL ( KURL(filename) );
    activateProject (project);
  }

  return project;
}

void KateMainWindow::projectDeleted (uint projectNumber)
{
  if (projectNumber == m_projectNumber)
  {
    if (m_projectManager->projects() > 0)
      activateProject (m_projectManager->project(m_projectManager->projects()-1));
    else
      activateProject (0);
  }
}

void KateMainWindow::slotDocumentCreated (Kate::Document *doc)
{
  connect(doc,SIGNAL(modStateChanged(Kate::Document *)),this,SLOT(updateCaption(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(updateCaption(Kate::Document *)));

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

  // update the sync action
  fileselector->kateViewChanged();

  QString c;
  if (m_viewManager->activeView()->getDoc()->url().isEmpty() || (!m_viewManager->getShowFullPath()))
  {
    c = m_viewManager->activeView()->getDoc()->docName();

    //File name shouldn't be too long - Maciek
    if (c.length() > 64)
      c = c.left(64) + "...";
  }
  else
  {
    c = m_viewManager->activeView()->getDoc()->url().prettyURL();

    //File name shouldn't be too long - Maciek
    if (c.length() > 64)
      c = "..." + c.right(64);
  }

  setCaption( c, m_viewManager->activeView()->getDoc()->isModified());
}

void KateMainWindow::openConstURLProject (const KURL&url)
{
  openProject (url.path());
}



void KateMainWindow::saveProperties(KConfig *config) {
	kdDebug(13000)<<"KateMainWindow::saveProperties()********************************************"<<endl
		      <<config->group()<<endl
		      <<"****************************************************************************"<<endl;
//	return;
	saveWindowConfiguration(config);

#warning FIXME, should be done only once
        m_docManager->saveDocumentList (config);
        m_projectManager->saveProjectList (config);
//	m_docManager->closeAllDocuments();

}

void KateMainWindow::readProperties(KConfig *config) {
	restoreWindowConfiguration(config);
}

void KateMainWindow::saveWindowConfiguration (KConfig *config)
{
  assert(config);

  kdDebug(13000)<<"preparing session saving"<<endl;
  QString grp=config->group();
  QString dockGrp;

  if (kapp->sessionSaving()) dockGrp=grp+"-Docking";
	else dockGrp="MainWindow0-Docking";
/*  if (config->readNumEntry("GUIMode",KMdi::UndefinedMode)!=mdiMode()) {
        config->writeEntry("GUIMode",mdiMode());
        config->deleteGroup("MainWindow0-Docking");
  }*/

  kdDebug(13000)<<"Before write dock config"<<endl;
  writeDockConfig(config,dockGrp);
  kdDebug(13000)<<"After write dock config"<<endl;


  if (kapp->sessionSaving()) dockGrp=grp+"-View Configuration";
	else dockGrp="MainWindow0-View Configuration";

  m_viewManager->saveViewConfiguration (config,dockGrp);
  kdDebug(13000)<<"After saving view configuration"<<endl;
  config->setGroup(grp);

}

void KateMainWindow::restoreWindowConfiguration (KConfig *config)
{
  QString grp=config->group();
  QString dockGrp;

  if (kapp->isRestored()) dockGrp=grp+"-Docking";
	else dockGrp="MainWindow0-Docking";

  if (config->hasGroup(dockGrp))
        readDockConfig(config,dockGrp);

  if (kapp->isRestored()) dockGrp=grp+"-View Configuration";
	else dockGrp="MainWindow0-View Configuration";

  m_viewManager->restoreViewConfiguration (config,dockGrp);
  config->setGroup(grp);
}


void KateMainWindow::saveGlobalProperties( KConfig* sessionConfig ) {
	kdDebug(13001)<<"saveGlobalProperties**********************"<<endl;

     //session saving
     kdDebug(13001)<<"QUERY CLOSE: session management"<<endl;
	if (!sessionClosingStarted2)
		sessionClosingAccepted=m_docManager->queryCloseDocuments(this);
	sessionClosingStarted=true;

	saveOptions(sessionConfig);
}

