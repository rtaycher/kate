/***************************************************************************
                          katemainwindow.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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

#include "kategrepdialog.h"

uint KateMainWindow::uniqueID = 0;

KateMainWindow::KateMainWindow(KateDocManager *_docManager, KatePluginManager *_pluginManager) :
	Kate::MainWindow (),
             DCOPObject ((QString("KateMainWindow%1").arg(uniqueID)).latin1())
{
  docManager =  _docManager;
  pluginManager =_pluginManager;
  config = kapp->config();

  myID = uniqueID;
  uniqueID++;
  
  activeView = 0;

  consoleDock = 0L;
  console = 0L;

  setAcceptDrops(true);

  setupMainWindow();

  setupActions();

  setXMLFile( "kateui.rc" );

  guiFactory()->addClient (this);
  
  //createGUI();

  pluginManager->enableAllPluginsGUI (this);

  // connect settings menu aboutToshow
  QPopupMenu* pm_set = (QPopupMenu*)factory()->container("settings", this);
  connect(pm_set, SIGNAL(aboutToShow()), this, SLOT(settingsMenuAboutToShow()));

  // connect settings menu aboutToshow
  documentMenu = (QPopupMenu*)factory()->container("documents", this);
  connect(documentMenu, SIGNAL(aboutToShow()), this, SLOT(documentMenuAboutToShow()));

  readOptions(config);

  if (((KateApp *)kapp)->_isSDI)
  {
    filelistDock->undock();
    fileselectorDock->undock();
  }

  setAutoSaveSettings( QString::fromLatin1("MainWindow"), false );
  statusBar()->hide();
}

KateMainWindow::~KateMainWindow()
{
}

void KateMainWindow::setupMainWindow ()
{
  grep_dlg = new GrepDialog( QDir::homeDirPath(), this, "grepdialog" );
  connect(grep_dlg, SIGNAL(itemSelected(QString,int)), this, SLOT(slotGrepDialogItemSelected(QString,int)));

  mainDock = createDockWidget( "mainDock", 0L );
  filelistDock =  createDockWidget( "filelistDock",  SmallIcon("kmultiple"), 0L, "Open Files", "");
  fileselectorDock = createDockWidget( "fileselectorDock", SmallIcon("fileopen"), 0L, "Selector", "");

  mainDock->setGeometry(100, 100, 100, 100);
  viewManager = new KateViewManager (mainDock, docManager);
  viewManager->setMinimumSize(200,200);
  mainDock->setWidget(viewManager);

  setMainDockWidget( mainDock );
  setView( mainDock );

  filelist = new KateFileList (docManager, viewManager, filelistDock, "filelist");
  filelistDock->setWidget (filelist);

  fileselector = new KateFileSelector( this, viewManager, fileselectorDock, "operator");
  fileselector->dirOperator()->setView(KFile::Simple);
  fileselectorDock->setWidget (fileselector);

  filelistDock->setDockWindowType (NET::Tool);
  fileselectorDock->setDockWindowType (NET::Tool);
  filelistDock->setDockWindowTransient (this, true);
  fileselectorDock->setDockWindowTransient (this, true);

  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));

  mainDock->setEnableDocking ( KDockWidget::DockNone );
  mainDock->setDockSite( KDockWidget::DockCorner );

  filelistDock->manualDock ( mainDock, KDockWidget::DockLeft, 20 );
  fileselectorDock ->manualDock(filelistDock, KDockWidget::DockCenter);

  statusBar()->hide();
}

bool KateMainWindow::eventFilter(QObject* o, QEvent* e)
{
  if ( e->type() == QEvent::WindowActivate && o == this ) {
    //kdDebug()<<"YAY!!! this KateMainWindow was activated!"<<endl;
    Kate::Document *doc;
    for( doc = docManager->firstDoc(); doc; doc = docManager->nextDoc() ) {
      doc->isModOnHD();
    }
  }
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

  return QWidget::eventFilter(o,e);
}

void KateMainWindow::setupActions()
{
  kscript = new KScriptManager(this, "scriptmanager");
  scriptMenu = new KSelectAction(i18n("KDE Scripts"),0,this,SLOT(runScript()),actionCollection(),"scripts");
  setupScripts();
  scriptMenu->clear();
  scriptMenu->setItems(kscript->scripts());
  KStdAction::openNew( viewManager, SLOT( slotDocumentNew() ), actionCollection(), "file_new" );
  KStdAction::open( viewManager, SLOT( slotDocumentOpen() ), actionCollection(), "file_open" );

  fileOpenRecent = KStdAction::openRecent (viewManager, SLOT(openConstURL_delayed1 (const KURL&)), actionCollection());
  new KAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, viewManager, SLOT( slotDocumentSaveAll() ), actionCollection(), "file_save_all" );
  KStdAction::print(viewManager, SLOT(printDlg()), actionCollection());
  KStdAction::close( viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" );
  new KAction( i18n( "Clos&e All" ), 0, viewManager, SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );

  new KAction(i18n("New &Window"), 0, this, SLOT(newWindow()), actionCollection(), "file_newWindow");

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" );

  new KAction(i18n("Find in Files..."), CTRL+SHIFT+Qt::Key_F, this, SLOT(slotFindInFiles()), actionCollection(),"edit_find_in_files" );

  KStdAction::spelling(viewManager, SLOT(slotSpellcheck()), actionCollection());

  new KAction( i18n("Split &Vertical"), "view_left_right", CTRL+SHIFT+Key_L, viewManager, SLOT( slotSplitViewSpaceVert() ), actionCollection(), "view_split_vert");
  new KAction( i18n("Split &Horizontal"), "view_top_bottom", CTRL+SHIFT+Key_T, viewManager, SLOT( slotSplitViewSpaceHoriz() ), actionCollection(), "view_split_horiz");
  closeCurrentViewSpace = new KAction( i18n("Close &Current"), "view_remove", CTRL+SHIFT+Key_R, viewManager, SLOT( slotCloseCurrentViewSpace() ), actionCollection(), "view_close_current_space");

  viewBorder =  new KToggleAction(i18n("Show &Icon Border"), Key_F6, viewManager, SLOT(toggleIconBorder()), actionCollection(), "view_border");
  viewLineNumbers =  new KToggleAction(i18n("Show &Line Numbers"), Key_F9, viewManager, SLOT(toggleLineNumbers()), actionCollection(), "view_line_numbers");

  goNext=new KAction(i18n("Next View"),Key_F8,viewManager, SLOT(activateNextView()),actionCollection(),"go_next");
  goPrev=new KAction(i18n("Previous View"),SHIFT+Key_F8,viewManager, SLOT(activatePrevView()),actionCollection(),"go_prev");

  windowNext = KStdAction::back(viewManager, SLOT(slotWindowNext()), actionCollection());
  windowPrev = KStdAction::forward(viewManager, SLOT(slotWindowPrev()), actionCollection());

  setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, actionCollection(), "set_eol");
  connect(setEndOfLine, SIGNAL(activated(int)), viewManager, SLOT(setEol(int)));
  connect(setEndOfLine->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(setEOLMenuAboutToShow()));
  QStringList list;
  list.append("&Unix");
  list.append("&Windows/Dos");
  list.append("&Macintosh");
  setEndOfLine->setItems(list);

  documentReload = new KAction(i18n("Reloa&d"), "reload", Key_F5, viewManager, SLOT(reloadCurrentDoc()), actionCollection(), "file_reload");

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), actionCollection(), "file_open_with");
  connect(documentOpenWith->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotOpenWithMenuAction(int)));

  new KAction(i18n("&Toggle Block Selection"), Key_F4, viewManager, SLOT(toggleVertical()),
                                             actionCollection(), "set_verticalSelect");

  if (pluginManager->myPluginList.count() > 0)
    new KAction(i18n("Contents &Plugins"), 0, this, SLOT(pluginHelp()), actionCollection(), "help_plugins_contents");

  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");

  // toggle dockwidgets
  settingsShowFilelist = new KToggleAction(i18n("Show File List"), 0, filelistDock, SLOT(changeHideShowState()), actionCollection(), "settings_show_filelist");
  settingsShowFileselector = new KToggleAction(i18n("Show File Selector"), 0, fileselectorDock, SLOT(changeHideShowState()), actionCollection(), "settings_show_fileselector");
  settingsShowConsole = new KToggleAction(i18n("Show Terminal Emulator"), QString::fromLatin1("konsole"), Qt::Key_F7, this, SLOT(slotSettingsShowConsole()), actionCollection(), "settings_show_console");

  // (now moved to config dialog) show full path in title -anders
  // settingsShowFullPath = new KToggleAction(i18n("Show Full &Path in Title"), 0, this, SLOT(slotSettingsShowFullPath()), actionCollection(), "settings_show_full_path");
  settingsShowToolbar = KStdAction::showToolbar(this, SLOT(slotSettingsShowToolbar()), actionCollection(), "settings_show_toolbar");
  settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");

  setHighlight = docManager->docList.at(0)->hlActionMenu (i18n("&Highlight Mode"), actionCollection(), "set_highlight");
  exportAs = docManager->docList.at(0)->exportActionMenu (i18n("E&xport"), actionCollection(),"file_export");

  connect(viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));

  slotWindowActivated ();
}

bool KateMainWindow::queryClose()
{
  kdDebug(13000)<<"queryClose()"<<endl;
  bool val = false;

  if ( ((KateApp *)kapp)->mainWindowsCount () < 2 )
  {
    saveOptions(config);

    viewManager->saveAllDocsAtCloseDown(  );

    if ( (!docManager->currentDoc()) || ((!viewManager->activeView()->getDoc()->isModified()) && (docManager->docCount() == 1)))
    {
      if (viewManager->activeView()) viewManager->deleteLastView ();
      val = true;
    }
  }
  else
    val = true;

  if (val)
  {
    ((KateApp *)kapp)->removeMainWindow (this);
    if (consoleDock && console)
      if (consoleDock->isVisible())
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
  KEditToolbar dlg(factory());

  if (dlg.exec())
    createGUI();
}

void KateMainWindow::slotFileQuit()
{
  queryClose ();
}

void KateMainWindow::readOptions(KConfig *config)
{
  config->setGroup("General");
  syncKonsole =  config->readBoolEntry("Sync Konsole", true);

  if (config->readBoolEntry("Show Console", false))
  {
    slotSettingsShowConsole();
  }

  if (!((KateApp *)kapp)->_isSDI)
    {
      QSize tmpSize(600, 400);
      resize( config->readSizeEntry( "size", &tmpSize ) );
    }
  viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));
  //settingsShowFullPath->setChecked(viewManager->getShowFullPath());
  settingsShowToolbar->setChecked(config->readBoolEntry("Show Toolbar", true));
  slotSettingsShowToolbar();
  viewManager->setUseOpaqueResize(config->readBoolEntry("Opaque Resize", true));

  fileOpenRecent->setMaxItems( config->readNumEntry("Number of recent files", fileOpenRecent->maxItems() ) );
  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");
  fileselector->setView(KFile::Default);

  if (!((KateApp *)kapp)->_isSDI)
    readDockConfig();
}

void KateMainWindow::saveOptions(KConfig *config)
{
  config->setGroup("General");

  if (consoleDock && console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);

  if (!((KateApp *)kapp)->_isSDI)
    config->writeEntry("size", size());

  config->writeEntry("Show Full Path in Title", viewManager->getShowFullPath());
  config->writeEntry("Show Toolbar", settingsShowToolbar->isChecked());
  config->writeEntry("Opaque Resize", viewManager->useOpaqueResize);
  config->writeEntry("Sync Konsole", syncKonsole);

  fileOpenRecent->saveEntries(config, "Recent Files");

  fileselector->writeConfig(config, "fileselector");

  if (!((KateApp *)kapp)->_isSDI)
    writeDockConfig();

  if (viewManager->activeView())
    viewManager->activeView()->getDoc()->writeConfig();

  viewManager->saveViewSpaceConfig();
}

void KateMainWindow::slotWindowActivated ()
{
  static QString path;
  
  if (viewManager->activeView() != 0)
  {                         
    if (console && syncKonsole)
    {
      QString newPath = viewManager->activeView()->getDoc()->url().directory();

      if ( newPath != path )
      {
        path = newPath;
        console->cd (path);
      }
    }

    viewBorder->setChecked(viewManager->activeView()->iconBorder());
    viewLineNumbers->setChecked(viewManager->activeView()->lineNumbersOn());
    setHighlight->updateMenu (viewManager->activeView()->getDoc());
    exportAs->updateMenu (viewManager->activeView()->getDoc());
  }

  if (viewManager->viewCount ()  > 1)
  {
    windowNext->setEnabled(true);
    windowPrev->setEnabled(true);
  }
  else
  {
    windowNext->setEnabled(false);
    windowPrev->setEnabled(false);
  }

  if (viewManager->viewSpaceCount() == 1)
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
  scriptMenu->plug (documentMenu);
  setHighlight->plug (documentMenu);
  setHighlight->updateMenu (viewManager->activeView()->getDoc());

  setEndOfLine->plug (documentMenu);
  documentMenu->insertSeparator ();

  uint z=0;
  int i=1;

  QString entry;
  while ( z<docManager->docCount() )
  {
    if ( (!docManager->nthDoc(z)->url().isEmpty()) && (docManager->nthDoc(z)->url().filename() != 0) )
    {
       //File name shouldn't be too long - Maciek
       if (docManager->nthDoc(z)->url().filename().length() > 200)
         entry=QString("&%1 ").arg(i)+"..."+(docManager->nthDoc(z)->url().filename()).right(197);
       else
         entry=QString("&%1 ").arg(i)+docManager->nthDoc(z)->url().filename();
     }
    else
      entry=QString("&%1 ").arg(i)+i18n("Untitled %1").arg(docManager->nthDoc(z)->documentNumber());

    if (docManager->nthDoc(z)->isModified())
      entry.append (i18n(" - Modified"));

    documentMenu->insertItem ( entry, viewManager, SLOT (activateView (int)), 0,  docManager->nthDoc(z)->documentNumber());

    if (viewManager->activeView())
      documentMenu->setItemChecked( docManager->nthDoc(z)->documentNumber(), ((Kate::Document *)viewManager->activeView()->getDoc())->documentNumber() == docManager->nthDoc(z)->documentNumber() );

    z++;
    i++;
  }
}

void KateMainWindow::slotFindInFiles ()
{
  QString d = currentDocUrl().directory();
  if ( ! d.isEmpty() )
    grep_dlg->setDirName( d );
  grep_dlg->show();
  grep_dlg->raise();
}

void KateMainWindow::slotGrepDialogItemSelected(QString filename,int linenumber)
{
  KURL fileURL;
  fileURL.setPath( filename );
  viewManager->openURL( fileURL );
  if ( viewManager->activeView() == 0 ) return;
  viewManager->activeView()->gotoLineNumber( linenumber );
  this->raise();
  this->setActiveWindow();
}

void KateMainWindow::gotoBookmark (int n)
{
  viewManager->gotoMark (list.at(n));
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
    viewManager->openURL (*i);
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

void KateMainWindow::slotSettingsShowConsole()
{
  if (!consoleDock && !console)
  {
    consoleDock = createDockWidget( "consoleDock", 0, 0L, "Console", "" );
    console = new KateConsole (consoleDock, "console");
    console->installEventFilter( this );
    console->setMinimumSize(50,50);
    consoleDock->setWidget( console );
    consoleDock->manualDock ( mainDock, KDockWidget::DockBottom, 20 );
    consoleDock->changeHideShowState();
    consoleDock->setDockWindowType (NET::Tool);
    consoleDock->setDockWindowTransient (this, true);
  }

  consoleDock->changeHideShowState();

  if( consoleDock->isVisible() )
    console->setFocus();
  else
    if (viewManager->activeView())
      viewManager->activeView()->setFocus();
}

void KateMainWindow::settingsMenuAboutToShow()
{
  settingsShowFilelist->setChecked( filelistDock->isVisible() );
  settingsShowFileselector->setChecked( fileselectorDock->isVisible() );

  if (consoleDock)
    settingsShowConsole->setChecked( consoleDock->isVisible() );
}

void KateMainWindow::setEOLMenuAboutToShow()
{
  int eol = viewManager->activeView()->getEol();
  eol = eol>=0? eol: 0;
  setEndOfLine->setCurrentItem( eol );
}

void KateMainWindow::openURL (const QString &name)
{
  viewManager->openURL (KURL(name));
}

/* FIXME anders: remove later
void KateMainWindow::slotSettingsShowFullPath()
{
  viewManager->setShowFullPath( settingsShowFullPath->isChecked() );
}
*/
void KateMainWindow::slotSettingsShowToolbar()
{
  if (settingsShowToolbar->isChecked())
    toolBar()->show();
  else
    toolBar()->hide();
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

KURL KateMainWindow::currentDocUrl()
{
  return viewManager->activeView()->getDoc()->url();
}

void KateMainWindow::fileSelected(const KFileItem *file)
{
  viewManager->openURL( file->url() );
}

void KateMainWindow::restore(bool isRestored)
{ viewManager->reopenDocuments(isRestored); }

void KateMainWindow::mSlotFixOpenWithMenu()
{
  //kdDebug()<<"13000"<<"fixing open with menu"<<endl;
  documentOpenWith->popupMenu()->clear();
  // get a list of appropriate services.
  KMimeType::Ptr mime = KMimeType::findByURL( viewManager->activeView()->getDoc()->url() );
  //kdDebug()<<"13000"<<"url: "<<viewManager->activeView()->getDoc()->url().prettyURL()<<"mime type: "<<mime->name()<<endl;
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
  list.append( viewManager->activeView()->getDoc()->url() );
  QString* appname = new QString( documentOpenWith->popupMenu()->text(idx) );
  if ( appname->compare(i18n("&Other...")) == 0 ) {
    // display "open with" dialog
    KOpenWithDlg* dlg = new KOpenWithDlg(list);
    if (dlg->exec())
      KRun::run(*dlg->service(), list);
    return;
  }
  QString qry = QString("((Type == 'Application') and (Name == '%1'))").arg( appname->latin1() );
  KMimeType::Ptr mime = KMimeType::findByURL( viewManager->activeView()->getDoc()->url() );
  KTrader::OfferList offers = KTrader::self()->query(mime->name(), qry);
  KService::Ptr app = offers.first();
  // some checking here: pop a wacko message it the app wasn't found.
  KRun::run(*app, list);
}

Kate::ViewManager *KateMainWindow::getViewManager ()
{
  return ((Kate::ViewManager *)viewManager);
}

Kate::DocManager *KateMainWindow::getDocManager ()
{
  return ((Kate::DocManager *)docManager);
}

KDockWidget *KateMainWindow::getMainDock ()
{
  return mainDock;
}

void KateMainWindow::pluginHelp()
{
  kapp->invokeHelp (QString::null, "kate-plugins");
}

void KateMainWindow::setupScripts()
{
	// scan $KDEDIR/share/applications/kate/scripts/ for desktop files
	// and populate the list
	QString localpath = QString(kapp->name()) + "/scripts/";
	QDir d(locate("data", localpath));
	const QFileInfoList *fileList = d.entryInfoList("*.desktop");
	QFileInfoListIterator it (*fileList);
	QFileInfo *fi;
	while (( fi=it.current()))
	{
		if (KDesktopFile::isDesktopFile(fi->absFilePath()))
		{
			kscript->addScript(fi->absFilePath());
		}
		++it;
	}
}

void KateMainWindow::runScript()
{
	kdDebug(13000) << "Starting script engine..." << endl;
	kscript->runScript(scriptMenu->currentText());
}
