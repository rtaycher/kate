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

#include "kategrepdialog.h"

uint KateMainWindow::uniqueID = 0;

KateMainWindow::KateMainWindow(KateDocManager *_m_docManager, KatePluginManager *_m_pluginManager) :
	Kate::MainWindow (),
             DCOPObject ((QString("KateMainWindow%1").arg(uniqueID)).latin1()),ToolViewManager()
{
  m_leftDock=m_rightDock=m_topDock=m_bottomDock=0;

  m_docManager =  _m_docManager;
  m_pluginManager =_m_pluginManager;
  config = kapp->config();

  //m_dockStyle=IDEAlStyle;
  m_dockStyle=ClassicStyle;

  myID = uniqueID;
  uniqueID++;
  
  activeView = 0;

  consoleDock = 0L;
  console = 0L;

  setAcceptDrops(true);

  m_settingsShowToolViews=new KActionMenu( i18n("Tool Views"), actionCollection(),"settings_show_toolviews");

  setupMainWindow();

  setupActions();

  setXMLFile( "kateui.rc" );
  createShellGUI ( true );
  
  m_pluginManager->enableAllPluginsGUI (this);

  // connect settings menu aboutToshow
  QPopupMenu* pm_set = (QPopupMenu*)factory()->container("settings", this);
  connect(pm_set, SIGNAL(aboutToShow()), this, SLOT(settingsMenuAboutToShow()));

  // connect settings menu aboutToshow
  documentMenu = (QPopupMenu*)factory()->container("documents", this);
  connect(documentMenu, SIGNAL(aboutToShow()), this, SLOT(documentMenuAboutToShow()));

  readOptions(config);
    
  setAutoSaveSettings( QString::fromLatin1("MainWindow"), false );
}

KateMainWindow::~KateMainWindow()
{
}

void KateMainWindow::setupMainWindow ()
{
  grep_dlg = new GrepDialog( QDir::homeDirPath(), this, "grepdialog" );
  connect(grep_dlg, SIGNAL(itemSelected(QString,int)), this, SLOT(slotGrepDialogItemSelected(QString,int)));

  mainDock = createDockWidget( "mainDock", 0L );
  

  if (m_dockStyle==IDEAlStyle)
  {
    m_leftDock = createDockWidget("leftDock",SmallIcon("misc"),0L,"Left Dock");
    m_rightDock = createDockWidget("rightDock",SmallIcon("misc"),0L,"Right Dock");
    m_topDock = createDockWidget("topDock",SmallIcon("misc"),0L,"Top Dock");
    m_bottomDock = createDockWidget("bottomDock",SmallIcon("misc"),0L,"Bottom Dock");
  }

  mainDock->setGeometry(100, 100, 100, 100);
  m_viewManager = new KateViewManager (mainDock, m_docManager);
  m_viewManager->setMinimumSize(200,200);
  mainDock->setWidget(m_viewManager);
  
  setMainDockWidget( mainDock );
  setView( mainDock );
  mainDock->setEnableDocking ( KDockWidget::DockNone );
  mainDock->setDockSite( KDockWidget::DockCorner );


  if (m_dockStyle==IDEAlStyle)
  {
    m_leftDock->setWidget(new KateDockContainer(m_leftDock, this, KDockWidget::DockLeft));
    m_rightDock->setWidget(new KateDockContainer(m_rightDock, this, KDockWidget::DockRight));
    m_topDock->setWidget(new KateDockContainer(m_topDock, this, KDockWidget::DockTop));
    m_bottomDock->setWidget(new KateDockContainer(m_bottomDock, this, KDockWidget::DockBottom));
  }


  if (m_dockStyle==IDEAlStyle)
  {
     m_leftDock->manualDock(mainDock, KDockWidget::DockLeft,0);
     m_rightDock->manualDock(mainDock, KDockWidget::DockRight,100);
     m_topDock->manualDock(mainDock, KDockWidget::DockTop,0);
     m_bottomDock->manualDock(mainDock, KDockWidget::DockBottom,100);
     m_bottomDock->undock();
     m_topDock->undock();
  }

  filelist = new KateFileList (m_docManager, m_viewManager, this/*filelistDock*/, "filelist");
  filelistDock=addToolViewWidget(KDockWidget::DockLeft,filelist,SmallIcon("kmultiple"),"File List");

  fileselector = new KateFileSelector( this, m_viewManager, /*fileselectorDock*/ this, "operator");
  fileselectorDock=addToolViewWidget(KDockWidget::DockLeft,fileselector, SmallIcon("fileopen"),"Selector");

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
  KStdAction::openNew( m_viewManager, SLOT( slotDocumentNew() ), actionCollection(), "file_new" );
  KStdAction::open( m_viewManager, SLOT( slotDocumentOpen() ), actionCollection(), "file_open" );

  fileOpenRecent = KStdAction::openRecent (m_viewManager, SLOT(openConstURL (const KURL&)), actionCollection());
  new KAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, m_viewManager, SLOT( slotDocumentSaveAll() ), actionCollection(), "file_save_all" );
  KStdAction::close( m_viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" );
  new KAction( i18n( "Clos&e All" ), 0, m_viewManager, SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );

  KStdAction::mail( this, SLOT(slotMail()), actionCollection() );

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" );

  new KAction(i18n("Find in Files..."), CTRL+SHIFT+Qt::Key_F, this, SLOT(slotFindInFiles()), actionCollection(),"edit_find_in_files" );

  new KAction(i18n("New &View"), 0, this, SLOT(newWindow()), actionCollection(), "view_new_view");
  new KAction( i18n("Split &Vertical"), "view_left_right", CTRL+SHIFT+Key_L, m_viewManager, SLOT( slotSplitViewSpaceVert() ), actionCollection(), "view_split_vert");
  new KAction( i18n("Split &Horizontal"), "view_top_bottom", CTRL+SHIFT+Key_T, m_viewManager, SLOT( slotSplitViewSpaceHoriz() ), actionCollection(), "view_split_horiz");
  closeCurrentViewSpace = new KAction( i18n("Close &Current"), "view_remove", CTRL+SHIFT+Key_R, m_viewManager, SLOT( slotCloseCurrentViewSpace() ), actionCollection(), "view_close_current_space");

  goNext=new KAction(i18n("Next View"),Key_F8,m_viewManager, SLOT(activateNextView()),actionCollection(),"go_next");
  goPrev=new KAction(i18n("Previous View"),SHIFT+Key_F8,m_viewManager, SLOT(activatePrevView()),actionCollection(),"go_prev");

  windowNext = KStdAction::back(m_viewManager, SLOT(slotWindowNext()), actionCollection());
  windowPrev = KStdAction::forward(m_viewManager, SLOT(slotWindowPrev()), actionCollection());

  documentOpenWith = new KActionMenu(i18n("Open W&ith"), actionCollection(), "file_open_with");
  connect(documentOpenWith->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(mSlotFixOpenWithMenu()));
  connect(documentOpenWith->popupMenu(), SIGNAL(activated(int)), this, SLOT(slotOpenWithMenuAction(int)));

  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");



  // toggle dockwidgets
//  settingsShowFilelist = new KToggleAction(i18n("Show File List"), 0, filelistDock, SLOT(changeHideShowState()), actionCollection(), "settings_show_filelist");
//  settingsShowToolViews->insert(settingsShowFilelist);

//  settingsShowFileselector = new KToggleAction(i18n("Show File Selector"), 0, fileselectorDock, SLOT(changeHideShowState()), actionCollection(), "settings_show_fileselector");
  settingsShowConsole = new KToggleAction(i18n("Show Terminal Emulator"), QString::fromLatin1("konsole"), Qt::Key_F7, this, SLOT(slotSettingsShowConsole()), actionCollection(), "settings_show_console");

//  settingsShowToolViews->insert(settingsShowFileselector);
  m_settingsShowToolViews->insert(settingsShowConsole);


  if (m_dockStyle==IDEAlStyle)
  {
	  KActionMenu *settingsShowToolDocks=new KActionMenu( i18n("Tool Docks"), actionCollection(),"settings_show_tooldocks");
  }

  settingsShowToolbar = KStdAction::showToolbar(this, SLOT(slotSettingsShowToolbar()), actionCollection(), "settings_show_toolbar");
  settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");

  // help menu
// Don't call this, KMainWindow does it for us.
//  new KHelpMenu(this, instance()->aboutData(), true, actionCollection());

  // tip of the day :-)
  KStdAction::tipOfDay( this, SLOT( tipOfTheDay() ), actionCollection() );

  if (m_pluginManager->pluginList().count() > 0)
    new KAction(i18n("Contents &Plugins"), 0, this, SLOT(pluginHelp()), actionCollection(), "help_plugins_contents");

  connect(m_viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));

  slotWindowActivated ();
}

bool KateMainWindow::queryClose()
{
  kdDebug(13000)<<"queryClose()"<<endl;
  bool val = false;

  if ( ((KateApp *)kapp)->mainWindows () < 2 )
  {
    saveOptions(config);

    m_viewManager->saveAllDocsAtCloseDown(  );

    if ( (!m_docManager->activeDocument()) || ((!m_viewManager->activeView()->getDoc()->isModified()) && (m_docManager->documents() == 1)))
    {
      if (m_viewManager->activeView()) m_viewManager->deleteLastView ();
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

  if (config->readBoolEntry("Show Console", false))
  {
    slotSettingsShowConsole();
  }

  QSize tmpSize(600, 400);
  resize( config->readSizeEntry( "size", &tmpSize ) );

  m_viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));

  settingsShowToolbar->setChecked(config->readBoolEntry("Show Toolbar", true));
  slotSettingsShowToolbar();
  m_viewManager->setUseOpaqueResize(config->readBoolEntry("Opaque Resize", true));

  fileOpenRecent->setMaxItems( config->readNumEntry("Number of recent files", fileOpenRecent->maxItems() ) );
  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");
  //fileselector->setView(KFile::Default); grr - the file selector reads the config and does this!!

 // readDockConfig();
}

void KateMainWindow::saveOptions(KConfig *config)
{
  config->setGroup("General");

  if (consoleDock && console)
    config->writeEntry("Show Console", console->isVisible());
  else
    config->writeEntry("Show Console", false);

  config->writeEntry("size", size());

  config->writeEntry("Show Full Path in Title", m_viewManager->getShowFullPath());
  config->writeEntry("Show Toolbar", settingsShowToolbar->isChecked());
  config->writeEntry("Opaque Resize", m_viewManager->useOpaqueResize);
  config->writeEntry("Sync Konsole", syncKonsole);

  fileOpenRecent->saveEntries(config, "Recent Files");

  fileselector->writeConfig(config, "fileselector");

  writeDockConfig();

  if (m_viewManager->activeView())
    m_viewManager->activeView()->getDoc()->writeConfig();

  m_viewManager->saveViewSpaceConfig();
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

  if (m_viewManager->viewCount ()  > 1)
  {
    windowNext->setEnabled(true);
    windowPrev->setEnabled(true);
  }
  else
  {
    windowNext->setEnabled(false);
    windowPrev->setEnabled(false);
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

void KateMainWindow::slotGrepDialogItemSelected(QString filename,int linenumber)
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

void KateMainWindow::slotSettingsShowConsole()
{
  if (!consoleDock && !console)
  {
    consoleDock = createDockWidget( "consoleDock", SmallIcon("konsole"), 0L, "Console", "" );
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
    if (m_viewManager->activeView())
      m_viewManager->activeView()->setFocus();
}

void KateMainWindow::settingsMenuAboutToShow()
{
//  settingsShowFilelist->setChecked( filelistDock->isVisible() );
//  settingsShowFileselector->setChecked( fileselectorDock->isVisible() );

  if (consoleDock)
    settingsShowConsole->setChecked( consoleDock->isVisible() );
}

void KateMainWindow::openURL (const QString &name)
{
  m_viewManager->openURL (KURL(name));
}

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
              i18n("<p>The current document has not beed saved, and can "
              "not be atthatched to a email message."
              "<p>Do you want to save it and proceed?"),
              i18n("Can not send unsaved file") );
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
                QString( i18n("<p>The current file:<br><strong>%1</strong><br>has been "
                "modified. Modifications will not be available in the atthatchment."
                "<p>Do you want to save it before sending it?") ).arg(doc->url().prettyURL()),
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


KDockWidget *KateMainWindow::addToolView(KDockWidget::DockPosition pos,const char* name, const QPixmap &icon,const QString& caption)
{
	KDockWidget *dw=createDockWidget( name,  icon, 0L, caption, "");
        dw->setEnableDocking(dw->enableDocking() & ~KDockWidget::DockDesktop);
        dw->setDockWindowType (NET::Tool);
        dw->setDockWindowTransient (this, true);
	if (m_dockStyle==ClassicStyle)
		dw->manualDock ( mainDock, pos, 20 );
	else
	{
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
	
	KToggleAction *showaction= new KToggleAction(i18n("Show %1").arg(caption), 0, dw, SLOT(changeHideShowState()), actionCollection(), name);
	m_settingsShowToolViews->insert(showaction);

	return dw;


}

KDockWidget *KateMainWindow::addToolViewWidget(KDockWidget::DockPosition pos,QWidget *widget, const QPixmap &icon,const QString &caption)
{
	KDockWidget *dw=addToolView(pos,widget->name(),icon,caption);
        dw->setWidget (widget);
	return dw;

}
