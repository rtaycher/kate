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

// $Id$

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
#include "katedockcontainer.h"

#include <dcopclient.h>
#include <kinstance.h>
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

#include <ktip.h>

#include <kmenubar.h>

#include "kategrepdialog.h"
//END

uint KateMainWindow::uniqueID = 0;



KateMainWindow::KateMainWindow(KateDocManager *_m_docManager, KatePluginManager *_m_pluginManager, KateProjectManager *projectMan) :
	KParts::DockMainWindow (),
             DCOPObject ((QString("KateMainWindow%1").arg(uniqueID)).latin1())
{
  m_mainWindow = new Kate::MainWindow (this);
  m_toolViewManager = new Kate::ToolViewManager (this);

  m_leftDock=m_rightDock=m_topDock=m_bottomDock=0;

  m_docManager =  _m_docManager;
  m_pluginManager =_m_pluginManager;
  m_projectManager = projectMan;

  m_project = 0;
  m_projectNumber = 0;

  config = kapp->config();

  QString grp=config->group();
  config->setGroup("General");
  manager()->setSplitterOpaqueResize(config->readBoolEntry("Opaque Resize", true));
  m_dockStyle= (config->readEntry("viewMode",DEFAULT_STYLE)=="Modern") ? ModernStyle : ClassicStyle;

  if (config->readBoolEntry("deleteKDockWidgetConfig",false))
  {
	config->writeEntry("deleteKDockWidgetConfig",false);
	config->deleteGroup("dock_setting_default");
	config->deleteGroup("KateDock::leftDock");
	config->deleteGroup("KateDock::rightDock");
	config->deleteGroup("KateDock::bottomDock");
	config->deleteGroup("KateDock::topDock");
	config->sync();
  }

  config->setGroup(grp);

  myID = uniqueID;
  uniqueID++;

  activeView = 0;

  consoleDock = 0L;
  console = 0L;

  setAcceptDrops(true);

  m_settingsShowToolViews=new KActionMenu( i18n("Tool Views"), actionCollection(),"settings_show_toolviews");
  m_settingsShowToolViews->setWhatsThis(i18n("Shows all available tool views and allows showing and hiding of them."));

  setupMainWindow();

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

  readOptions(config);

  if (m_dockStyle==ModernStyle) mainDock->setDockSite( KDockWidget::DockNone );

 if (console)
    console->loadConsoleIfNeeded();

  // call it as last thing, must be sure everything is allready set up ;)
  setAutoSaveSettings( QString::fromLatin1("MainWindow"), true );
}

KateMainWindow::~KateMainWindow()
{
    delete kscript;
}

void KateMainWindow::setupMainWindow ()
{
  grep_dlg = new GrepDialog( QDir::homeDirPath(), this, "grepdialog" );
  connect(grep_dlg, SIGNAL(itemSelected(const QString &,int)), this, SLOT(slotGrepDialogItemSelected(const QString &,int)));

  mainDock = createDockWidget( "mainDock", 0L );

  if (m_dockStyle==ModernStyle)
  {
    m_leftDock = createDockWidget("leftDock",SmallIcon("misc"),0L,"Left Dock");
    m_rightDock = createDockWidget("rightDock",SmallIcon("misc"),0L,"Right Dock");
    m_topDock = createDockWidget("topDock",SmallIcon("misc"),0L,"Top Dock");
    m_bottomDock = createDockWidget("bottomDock",SmallIcon("misc"),0L,"Bottom Dock");
  }

  mainDock->setGeometry(100, 100, 100, 100);
  m_viewManager = new KateViewManager (mainDock, m_docManager);
  mainDock->setWidget(m_viewManager);

  setMainDockWidget( mainDock );
  setView( mainDock );
  mainDock->setEnableDocking ( KDockWidget::DockNone );
  mainDock->setDockSite( KDockWidget::DockCorner );

  if (m_dockStyle==ModernStyle)
  {
    KateDockContainer *tmpDC;
    m_leftDock->setWidget(tmpDC=new KateDockContainer(m_leftDock, this, KDockWidget::DockLeft));
    tmpDC->init();
    m_rightDock->setWidget(tmpDC=new KateDockContainer(m_rightDock, this, KDockWidget::DockRight));
    tmpDC->init();
    m_topDock->setWidget(tmpDC=new KateDockContainer(m_topDock, this, KDockWidget::DockTop));
    tmpDC->init();
    m_bottomDock->setWidget(tmpDC=new KateDockContainer(m_bottomDock, this, KDockWidget::DockBottom));
    tmpDC->init();

     m_leftDock->manualDock(mainDock, KDockWidget::DockLeft,20);
     m_rightDock->manualDock(mainDock, KDockWidget::DockRight,20);
     m_topDock->manualDock(mainDock, KDockWidget::DockTop,20);
     m_bottomDock->manualDock(mainDock, KDockWidget::DockBottom,20);

     m_leftDock->setDockSite( KDockWidget::DockCenter );
     m_rightDock->setDockSite( KDockWidget::DockCenter );
     m_topDock->setDockSite( KDockWidget::DockCenter );
     m_bottomDock->setDockSite( KDockWidget::DockCenter );

     m_topDock->undock();
     m_rightDock->undock();
  }


  filelist = new KateFileList (m_docManager, m_viewManager, this/*filelistDock*/, "filelist");
  filelistDock=addToolViewWidget(KDockWidget::DockLeft,filelist,SmallIcon("kmultiple"), i18n("Files"));

  projectlist = new KateProjectList (m_projectManager, this, this/*filelistDock*/, "projectlist");
  projectlistDock=addToolViewWidget(KDockWidget::DockLeft,projectlist,SmallIcon("view_choose"), i18n("Projects"));

  projectviews = new KateProjectViews (m_projectManager, this, this/*filelistDock*/, "projectviews");
  projectviewsDock =addToolViewWidget(KDockWidget::DockLeft,projectviews,SmallIcon("view_tree"), i18n("Project View"));

  fileselector = new KateFileSelector( this, m_viewManager, /*fileselectorDock*/ this, "operator");
  fileselectorDock=addToolViewWidget(KDockWidget::DockLeft,fileselector, SmallIcon("fileopen"), i18n("Selector"));

  if (kapp->authorize("shell_access"))
  {
     console = new KateConsole (this, "console",viewManager());
     consoleDock = addToolViewWidget(KDockWidget::DockBottom,console, SmallIcon("konsole"), i18n("Terminal"));
  }

  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));
}

bool KateMainWindow::eventFilter(QObject* o, QEvent* e)
{
  if ( e->type() == QEvent::WindowActivate && o == this ) {
    //kdDebug()<<"YAY!!! this KateMainWindow was activated!"<<endl;
    Kate::Document *doc;
    for( doc = m_docManager->firstDocument(); doc; doc = m_docManager->nextDocument() ) {
      doc->isModOnHD();
    }
  }
  /* FIXME this never worked - can i delete it?
  if (e->type() == QEvent::KeyPress)
  {
    QKeyEvent *ke = (QKeyEvent*)e;

    if (ke->key()==goNext->accel())
    {
      kdDebug(13000)<<"Jump next view  registered in Konsole";
      slotGoNext();
      return true;
    }
    else

    if (ke->key()==goPrev->accel())
    {
      kdDebug(13000)<<"Jump prev view  registered in Konsole";
      slotGoPrev();
      return true;
    }
  }
  */

  return QWidget::eventFilter(o,e);
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

  fileOpenRecent = KStdAction::openRecent (m_viewManager, SLOT(openConstURLCheck (const KURL&)), actionCollection());
  fileOpenRecent->setWhatsThis(i18n("This lists files which you have opened recently, and allows you to easily open them again."));

  a=new KAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, m_viewManager, SLOT( slotDocumentSaveAll() ), actionCollection(), "file_save_all" );
  a->setWhatsThis(i18n("Save all open, modified documents to disc."));

  KStdAction::close( m_viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" )->setWhatsThis(i18n("Close the current document."));

  a=new KAction( i18n( "Clos&e All" ), 0, m_viewManager, SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );
  a->setWhatsThis(i18n("Close all open documents."));

  KStdAction::mail( this, SLOT(slotMail()), actionCollection() )->setWhatsThis(i18n("Send one or more of the open documents as email attachments."));

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" )->setWhatsThis(i18n("Close this window"));

  a=new KAction(i18n("Find in Files..."), CTRL+SHIFT+Qt::Key_F, this, SLOT(slotFindInFiles()), actionCollection(),"edit_find_in_files" );
  a->setWhatsThis(i18n("Look up text in a selection of files in a given directory (and below)."));

  a=new KAction(i18n("&New View"), 0, this, SLOT(newWindow()), actionCollection(), "view_new_view");
  a->setWhatsThis(i18n("Create a new Kate view (a new window with the same document list)."));

  a=new KAction( i18n("Split &Vertical"), "view_left_right", CTRL+SHIFT+Key_L, m_viewManager, SLOT( slotSplitViewSpaceVert() ), actionCollection(), "view_split_vert");
  a->setWhatsThis(i18n("Split the currently active view vertically into two views."));

  a=new KAction( i18n("Split &Horizontal"), "view_top_bottom", CTRL+SHIFT+Key_T, m_viewManager, SLOT( slotSplitViewSpaceHoriz() ), actionCollection(), "view_split_horiz");
  a->setWhatsThis(i18n("Split the currently active view horizontally into two views."));

  a=closeCurrentViewSpace = new KAction( i18n("Close &Current"), "view_remove", CTRL+SHIFT+Key_R, m_viewManager, SLOT( slotCloseCurrentViewSpace() ), actionCollection(), "view_close_current_space");
  a->setWhatsThis(i18n("Close the currently active splitted view"));

  connect(new KToggleAction(i18n("Show &Full-Screen"), QString::fromLatin1("window_fullscreen"),0, actionCollection(),
	"view_fullscreen_view"),SIGNAL(toggled(bool)), this,SLOT(slotFullScreen(bool)));

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

  if (m_dockStyle==ModernStyle)
  {
	  KActionMenu *settingsShowToolDocks=new KActionMenu( i18n("Tool Docks"), actionCollection(),"settings_show_tooldocks");
          settingsShowToolDocks->setWhatsThis(i18n("This allows you to show/hide certain tool view dock areas."));

	  settingsShowToolDocks->insert(new KateToggleToolViewAction(i18n("Bottom"),0,m_bottomDock,actionCollection(),this,"settings_show_bottomdock"));
	  settingsShowToolDocks->insert(new KateToggleToolViewAction(i18n("Left"),0,m_leftDock,actionCollection(),this,"settings_show_leftdock"));
  	  settingsShowToolDocks->insert(new KateToggleToolViewAction(i18n("Right"),0,m_rightDock,actionCollection(),this,"settings_show_rightdock"));
    	  settingsShowToolDocks->insert(new KateToggleToolViewAction(i18n("Top"),0,m_topDock,actionCollection(),this,"settings_show_topdock"));
  }

  // project menu
  a = new KAction(i18n("&New Project..."), "filenew", 0, this, SLOT(slotProjectNew()), actionCollection(), "project_new");
  a = new KAction(i18n("&Open Project..."), "fileopen", 0, this, SLOT(slotProjectOpen()), actionCollection(), "project_open");
  saveProject = new KAction(i18n("&Save Project"), "filesave", 0, this, SLOT(slotProjectSave()), actionCollection(), "project_save");
  closeProject = new KAction(i18n("&Close Project"), "fileclose", 0, this, SLOT(slotProjectClose()), actionCollection(), "project_close");

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

bool KateMainWindow::queryClose()
{
  if (m_viewManager->reopening()) return false;
  kdDebug(13000) << "queryClose()" << endl;
  bool val = false;

  if ( ((KateApp *)kapp)->mainWindows () < 2 )
  {
    saveOptions(config);

    m_viewManager->saveAllDocsAtCloseDown();

    if ( !m_docManager->activeDocument() || !m_viewManager->activeView() ||
       ( !m_viewManager->activeView()->getDoc()->isModified() && m_docManager->documents() == 1 ) )
    {
      if( m_viewManager->activeView() )
        m_viewManager->deleteLastView();
      val = true;
    }
  }
  else
    val = true;

  if (val)
  {
    ((KateApp *)kapp)->removeMainWindow (this);

    if( consoleDock && console && consoleDock->isVisible() )
      consoleDock->changeHideShowState();
  }

  return val;
}

void KateMainWindow::saveProperties( KConfig * )
{
  // This gets called in case of a crash. TODO: have a file list saved
  kdDebug(13000)<<"KateMainWindow::saveProperties()"<<endl;
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
  config->setGroup("General");
  syncKonsole =  config->readBoolEntry("Sync Konsole", true);

  m_viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));
  m_viewManager->setUseOpaqueResize(config->readBoolEntry("Opaque Resize", true));

  fileOpenRecent->setMaxItems( config->readNumEntry("Number of recent files", fileOpenRecent->maxItems() ) );
  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");

  filelist->setSortType(config->readNumEntry("Sort Type of File List", KateFileList::sortByID));

  readDockConfig();
}

void KateMainWindow::saveOptions(KConfig *config)
{
  config->setGroup("General");

  if (consoleDock && console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);

  config->writeEntry("Show Full Path in Title", m_viewManager->getShowFullPath());
  config->writeEntry("Opaque Resize", m_viewManager->useOpaqueResize);

  config->writeEntry("Sync Konsole", syncKonsole);

  fileOpenRecent->saveEntries(config, "Recent Files");

  fileselector->writeConfig(config, "fileselector");

  config->writeEntry("Sort Type of File List", filelist->sortType());

  writeDockConfig();

  if (m_viewManager->activeView())
    m_viewManager->activeView()->getDoc()->writeConfig();

  m_viewManager->saveViewSpaceConfig();
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

  if (m_viewManager->activeView() != 0)
  {
    if (console && syncKonsole)
    {
      QString newPath = m_viewManager->activeView()->getDoc()->url().directory();

      if ( newPath != path )
      {
        path = newPath;
        console->cd (path);
      }
    }
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
    if ( (!m_docManager->document(z)->url().isEmpty()) && (m_docManager->document(z)->url().filename() != 0) )
    {
       //File name shouldn't be too long - Maciek
       if (m_docManager->document(z)->url().filename().length() > 200)
         entry=QString("&%1 ").arg(i)+"..."+(m_docManager->document(z)->url().filename()).right(197);
       else
         entry=QString("&%1 ").arg(i)+m_docManager->document(z)->url().filename();
     }
    else
      entry=QString("&%1 ").arg(i)+i18n("Untitled %1").arg(m_docManager->document(z)->documentNumber());

    if (m_docManager->document(z)->isModified())
      entry.append (i18n(" - Modified"));

    documentMenu->insertItem ( entry, m_viewManager, SLOT (activateView (int)), 0,  m_docManager->document(z)->documentNumber());

    if (m_viewManager->activeView())
      documentMenu->setItemChecked( m_docManager->document(z)->documentNumber(), ((Kate::Document *)m_viewManager->activeView()->getDoc())->documentNumber() == m_docManager->document(z)->documentNumber() );

    z++;
    i++;
  }
}

void KateMainWindow::slotFindInFiles ()
{
  QString d = activeDocumentUrl().directory();
  if ( ! d.isEmpty() )
    grep_dlg->setDirName( d );
  grep_dlg->show();
  grep_dlg->raise();
}

void KateMainWindow::slotGrepDialogItemSelected(const QString &filename,int linenumber)
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
  event->accept(QUriDrag::canDecode(event));
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
  KKeyDialog dlg( false, this );
  QPtrList<KXMLGUIClient> clients = guiFactory()->clients();
  for( QPtrListIterator<KXMLGUIClient> it( clients );
       it.current(); ++it ) {
    dlg.insert( (*it)->actionCollection() );
  }
  dlg.configure();
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

void KateMainWindow::restore(bool isRestored)
{ m_viewManager->reopenDocuments(isRestored); }

void KateMainWindow::mSlotFixOpenWithMenu()
{
  //kdDebug()<<"13000"<<"fixing open with menu"<<endl;
  documentOpenWith->popupMenu()->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::findByURL( m_viewManager->activeView()->getDoc()->url() );
  //kdDebug()<<"13000"<<"url: "<<m_viewManager->activeView()->getDoc()->url().prettyURL()<<"mime type: "<<mime->name()<<endl;
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
        kdDebug()<<"runScript( "<<mIId<<" ) ["<<scriptMenu->popupMenu()->text( mIId )<<"]"<<endl;
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
              i18n("<p>The current document has not been saved, and can "
              "not be attached to a email message."
              "<p>Do you want to save it and proceed?"),
              i18n("Cannot Send Unsaved File") );
      if ( r == KMessageBox::Yes ) {
        Kate::View *v = (Kate::View*)doc->views().first();
        int sr = v->saveAs();
        if ( sr == Kate::View::SAVE_OK ) {
          doc->setDocName( doc->url().fileName() );
          m_viewManager->setWindowCaption();
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
      // warn that document is modified and offer to save it before preceeding.
      int r = KMessageBox::warningYesNoCancel( this,
                i18n("<p>The current file:<br><strong>%1</strong><br>has been "
                "modified. Modifications will not be available in the attachment."
                "<p>Do you want to save it before sending it?").arg(doc->url().prettyURL()),
                i18n("Save before sending?") );
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

int KateMainWindow::currentDocumentIfaceNumber()
{
  Kate::View *v = m_viewManager->activeView();
  if ( v )
  {
  kdDebug()<<"currentDocumentIfaceNumber(): returning "<<v->getDoc()->documentNumber()<<endl;
    return v->getDoc()->documentNumber();
  }
  return 0;
}


void KateMainWindow::slotFullScreen(bool t)
{
	if (t) showFullScreen(); else showNormal();
}


KDockWidget *KateMainWindow::addToolView(KDockWidget::DockPosition pos,const char* name, const QPixmap &icon,const QString& caption)
{
	KDockWidget *dw=createDockWidget( name,  icon, 0L, caption, (m_dockStyle==ModernStyle)?caption:"");

	if (m_dockStyle==ClassicStyle)
	{
	        	dw->setDockWindowType (NET::Tool);
        		dw->setDockWindowTransient (this, true);

			//KDockWidget=mainDock->
			KDockWidget *dw1=mainDock->findNearestDockWidget(pos);
			if (dw1)
			dw->manualDock(dw1,KDockWidget::DockCenter,20);
			else
			dw->manualDock ( mainDock, pos, 20 );
	}
	else
	{
	        dw->setEnableDocking(dw->enableDocking() & ~KDockWidget::DockDesktop);
		switch (pos)
		{
			case KDockWidget::DockLeft:  dw->manualDock(m_leftDock,KDockWidget::DockCenter,20);
						break;
			case KDockWidget::DockRight:  dw->manualDock(m_rightDock,KDockWidget::DockCenter,20);
						break;
			case KDockWidget::DockTop:  dw->manualDock(m_topDock,KDockWidget::DockCenter,20);
						break;
			case KDockWidget::DockBottom:  dw->manualDock(m_bottomDock,KDockWidget::DockCenter,20);
						break;
			default:	dw->manualDock(mainDock,pos,20);
						break;
		}
	}

	KToggleAction *showaction= new KateToggleToolViewAction(i18n("Show %1").arg(i18n(caption.utf8())), 0, dw, actionCollection(),this, name);
	m_settingsShowToolViews->insert(showaction);

	return dw;


}

KDockWidget *KateMainWindow::addToolViewWidget(KDockWidget::DockPosition pos,QWidget *widget, const QPixmap &icon,const QString &caption)
{
	KDockWidget *dw=addToolView(pos,QString("DOCK%1").arg(widget->name()).latin1(),icon,caption);
	kapp->sendPostedEvents();
	kapp->syncX();
        dw->setWidget (widget);
	widget->show(); // I'm not sure, if this is a bug in kdockwidget, which I would better fix there
	return dw;

}

bool KateMainWindow::removeToolViewWidget(QWidget *w)
{
	if (w->parent()->qt_cast("KDockWidget"))
	{
		KDockWidget *dw=static_cast<KDockWidget*>(w->parent()->qt_cast("KDockWidget"));
		if (dw->dockManager()==manager())
		{

			dw->undock();
			dw->deleteLater();
			return true;
		} else return false;

	}
	else
	return false;
}

bool KateMainWindow::removeToolView(KDockWidget *dw)
{
	if (dw->dockManager()==manager())
	{

		dw->undock();
		dw->deleteLater();
		return true;
	} else return false;

}

bool KateMainWindow::hideToolView(class KDockWidget*){return false;}
bool KateMainWindow::showToolView(class KDockWidget*){return false;}
bool KateMainWindow::hideToolView(const QString& name){return false;}
bool KateMainWindow::showToolView(const QString& name){return false;}


//-------------------------------------

KateToggleToolViewAction::KateToggleToolViewAction( const QString& text, const KShortcut& cut, KDockWidget *dw,QObject* parent,KateMainWindow *mw, const char* name )
	:KToggleAction(text,cut,parent,name),m_dw(dw),m_mw(mw)
{
	connect(this,SIGNAL(toggled(bool)),this,SLOT(slotToggled(bool)));
	connect(m_dw->dockManager(),SIGNAL(change()),this,SLOT(anDWChanged()));
	connect(m_dw,SIGNAL(destroyed()),this,SLOT(slotWidgetDestroyed()));
	setChecked(m_dw->mayBeHide());
}


KateToggleToolViewAction::~KateToggleToolViewAction(){;}

void KateToggleToolViewAction::anDWChanged()
{
	if (isChecked() && m_dw->mayBeShow()) setChecked(false);
	else if ((!isChecked()) && m_dw->mayBeHide()) setChecked(true);
	else if (isChecked() && (m_dw->parentDockTabGroup() &&
		((static_cast<KDockWidget*>(m_dw->parentDockTabGroup()->
			parent()->qt_cast("KDockWidget")))->mayBeShow()))) setChecked(false);
}


void KateToggleToolViewAction::slotToggled(bool t)
{
  m_mw->mainDock->setDockSite( KDockWidget::DockCorner );

  if ((!t) && m_dw->mayBeHide() ) m_dw->undock();
  else
    if ( t && m_dw->mayBeShow() ) m_mw->makeDockVisible(m_dw);

  if (m_mw->dockStyle()==KateMainWindow::ModernStyle) m_mw->mainDock->setDockSite( KDockWidget::DockNone );
}

void KateToggleToolViewAction::slotWidgetDestroyed()

{
	unplugAll();
	deleteLater();
}

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
  QString fileName = KFileDialog::getOpenFileName (QString::null, QString ("*.kateproject|") + i18n("Kate Project Files"), this, i18n("Open Kate Project"));

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
    activateProject (project);

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
