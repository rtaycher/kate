/***************************************************************************
                          kantmainwindow.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantmainwindow.h"
#include "kantmainwindow.moc"

#include "../sidebar/kantsidebar.h"
#include "../console/kantconsole.h"
#include "../document/kantdocument.h"
#include "../document/kantdocmanager.h"
#include "../pluginmanager/kantpluginmanager.h"
#include "../pluginmanager/kantconfigplugindialogpage.h"
#include "../view/kantviewmanager.h"
#include "../app/kantapp.h"
#include "../fileselector/kantfileselector.h"
#include "../filelist/kantfilelist.h"
#include "../factory/kantfactory.h"
#include "kantmenuitem.h"

#include "../view/kantviewdialog.h"

#include <cassert>
#include <qcheckbox.h>
#include <qiconview.h>
#include <qinputdialog.h>
#include <qlayout.h>
#include <qlayout.h>
#include <qlist.h>
#include <qsplitter.h>
#include <qsplitter.h>
#include <qvbox.h>
#include <qwhatsthis.h>
#include <qwidgetstack.h>

#include <dcopclient.h>
#include <kinstance.h>
#include <kaction.h>
#include <kapp.h>
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
#include <kio/netaccess.h>
#include <kkeydialog.h>
#include <klistbox.h>
#include <klocale.h>
#include <kparts/event.h>
#include <kparts/part.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstddirs.h>
#include <kuniqueapp.h>
#include <kurldrag.h>


#define POP_(x) kdDebug(13000) << #x " = " << flush << x << endl

KantViewManagerIface *KantMyPluginIface::viewManagerIface ()
{
  return ((KantViewManagerIface *)((KantMainWindow *)parent())->viewManager);
}

KantDocManagerIface *KantMyPluginIface::docManagerIface ()
{
  return ((KantDocManagerIface *)((KantMainWindow *)parent())->docManager);
}

KantMainWindow::KantMainWindow(KantDocManager *_docManager, KantPluginManager *_pluginManager) :
	KDockMainWindow (0, "Main Window"),
             DCOPObject ("KantIface" )
{
  tagSidebar=QString("sidebar");
  docManager =  _docManager;
  pluginManager =_pluginManager;
  config = KantFactory::instance()->config();

  setXMLFile( "kantui.rc" );

  setAcceptDrops(true);

  setupMainWindow();
  setupActions();

  createGUI();

  readOptions(config);

  // connect settings menu aboutToshow
  QPopupMenu* pm_set = (QPopupMenu*)factory()->container("settings", this);
  connect(pm_set, SIGNAL(aboutToShow()), this, SLOT(settingsMenuAboutToShow()));

  pluginManager->loadAllEnabledPlugins (pluginIface);
}

KantMainWindow::~KantMainWindow()
{
}

void KantMainWindow::setupMainWindow ()
{
  sidebarDock =  createDockWidget( "sidebarDock", 0 );
  sidebar = new KantSidebar (sidebarDock);
  sidebar->setMinimumSize(100,100);
  sidebarDock->setWidget( sidebar );

  consoleDock = createDockWidget( "consoleDock", 0 );
  console = new KantConsole (consoleDock, "console");
  console->installEventFilter( this );
  console->setMinimumSize(50,50);
  consoleDock->setWidget( console );

  mainDock = createDockWidget( "mainDock", 0 );
  mainDock->setGeometry(100, 100, 100, 100);
  viewManager = new KantViewManager (mainDock, docManager);
  viewManager->setMinimumSize(200,200);
  mainDock->setWidget(viewManager);
  setView( mainDock );
  setMainDockWidget( mainDock );

  mainDock->setEnableDocking ( KDockWidget::DockNone );
  sidebarDock->manualDock ( mainDock, KDockWidget::DockLeft, 20 );
  consoleDock->manualDock ( mainDock, KDockWidget::DockBottom, 20 );

  filelist = new KantFileList (docManager, viewManager, 0L, "filelist");
  sidebar->addWidget (filelist, i18n("Filelist"));

  fileselector = new KantFileSelector(0L, "operator");
  fileselector->dirOperator()->setView(KFile::Simple);
  sidebar->addWidget (fileselector, i18n("Fileselector"));
  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileViewItem*)),this,SLOT(fileSelected(const KFileViewItem*)));

  pluginIface = new KantMyPluginIface (this);

  statusBar()->hide();
}

bool KantMainWindow::eventFilter(QObject* o, QEvent* e)
{
  if (e->type() == QEvent::KeyPress)
        {
                QKeyEvent *ke=(QKeyEvent*)e;
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

void KantMainWindow::setupActions()
{
  KStdAction::openNew( viewManager, SLOT( slotDocumentNew() ), actionCollection(), "file_new" );
  KStdAction::open( viewManager, SLOT( slotDocumentOpen() ), actionCollection(), "file_open" );

  fileOpenRecent = KStdAction::openRecent (viewManager, SLOT(openConstURL (const KURL&)), actionCollection());

  fileSave = KStdAction::save( viewManager, SLOT( slotDocumentSave() ), actionCollection(), "file_save" );
  fileSaveAll = new KAction( i18n("Save A&ll"),"save_all", CTRL+Key_L, viewManager, SLOT( slotDocumentSaveAll() ), actionCollection(), "file_save_all" );
  fileSaveAs = KStdAction::saveAs( viewManager, SLOT( slotDocumentSaveAs() ), actionCollection(), "file_save_as" );
  filePrint = KStdAction::print(viewManager, SLOT(printDlg()), actionCollection());
  fileClose = KStdAction::close( viewManager, SLOT( slotDocumentClose() ), actionCollection(), "file_close" );
  fileCloseAll = new KAction( i18n( "&Close All" ), 0, viewManager, SLOT( slotDocumentCloseAll() ), actionCollection(), "file_close_all" );

  new KAction(i18n("New &Window"), 0, this, SLOT(newWindow()), actionCollection(), "file_newWindow");

  KStdAction::quit( this, SLOT( slotFileQuit() ), actionCollection(), "file_quit" );

  editUndo = KStdAction::undo(viewManager, SLOT(slotUndo()), actionCollection());
  editRedo = KStdAction::redo(viewManager, SLOT(slotRedo()), actionCollection());
  editUndoHist = new KAction(i18n("Undo/Redo &History..."), 0, viewManager, SLOT(slotUndoHistory()), actionCollection(), "edit_undoHistory");
  editCut = KStdAction::cut(viewManager, SLOT(slotCut()), actionCollection());
  editCopy = KStdAction::copy(viewManager, SLOT(slotCopy()), actionCollection()) ;
  editPaste = KStdAction::paste(viewManager, SLOT(slotPaste()), actionCollection());

  editSelectAll = KStdAction::selectAll(viewManager, SLOT(slotSelectAll()), actionCollection());
  editDeselectAll =new KAction(i18n("&Deselect All"), 0, viewManager, SLOT(slotDeselectAll()), actionCollection(), "edit_deselectAll");
  editInvertSelection = new KAction(i18n("Invert &Selection"), 0, viewManager, SLOT(slotInvertSelection()), actionCollection(), "edit_invertSelection");

  editFind = KStdAction::find(viewManager, SLOT(slotFind()), actionCollection());
  editFindNext = KStdAction::findNext(viewManager, SLOT(slotFindAgain()), actionCollection());
  editFindPrev = KStdAction::findPrev(viewManager, SLOT(slotFindAgainB()), actionCollection(), "edit_find_prev");
  editReplace = KStdAction::replace(viewManager, SLOT(slotReplace()), actionCollection());

  editIndent = new KAction(i18n("&Indent"), "indent", CTRL+Key_I, viewManager, SLOT(slotIndent()), actionCollection(), "edit_indent");
  editUnIndent = new KAction(i18n("&Unindent"), "unindent", CTRL+SHIFT+Key_I, viewManager, SLOT(slotUnIndent()), actionCollection(), "edit_unindent");

  editInsert = new KAction(i18n("I&nsert File..."), 0, viewManager, SLOT(slotInsertFile()), actionCollection(), "edit_insertFile");

  gotoLine = KStdAction::gotoLine(viewManager, SLOT(slotGotoLine()), actionCollection());

  bookmarkAdd = new KAction(i18n("&Add Marker"), Qt::CTRL+Qt::Key_M, viewManager, SLOT(addBookmark()), actionCollection(), "edit_bookmarkAdd");
  bookmarkSet = new KAction(i18n("&Set Marker..."), 0, viewManager, SLOT(setBookmark()), actionCollection(), "edit_bookmarkSet");
  bookmarkClear = new KAction(i18n("&Clear Markers"), 0, viewManager, SLOT(clearBookmarks()), actionCollection(), "edit_bookmarksClear");
  bookmarkMenu = new KActionMenu(i18n("&List of Markers"), actionCollection(), "edit_bookmarkList");
  connect(bookmarkMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));

  toolsSpell = KStdAction::spelling(viewManager, SLOT(slotSpellcheck()), actionCollection());

  viewSplitVert = new KAction( i18n("Split &Vertical"), "view_left_right", CTRL+SHIFT+Key_L, viewManager, SLOT( slotSplitViewSpaceVert() ), actionCollection(), "view_split_vert");
  viewSplitHoriz = new KAction( i18n("Split &Horizontal"), "view_top_bottom", CTRL+SHIFT+Key_T, viewManager, SLOT( slotSplitViewSpaceHoriz() ), actionCollection(), "view_split_horiz");
  closeCurrentViewSpace = new KAction( i18n("Close &Current"), "view_remove", CTRL+SHIFT+Key_R, viewManager, SLOT( slotCloseCurrentViewSpace() ), actionCollection(), "view_close_current_space");

  goNext=new KAction(i18n("Next View"),Key_F8,viewManager, SLOT(activateNextView()),actionCollection(),"go_next");
  goPrev=new KAction(i18n("Previous View"),SHIFT+Key_F8,viewManager, SLOT(activatePrevView()),actionCollection(),"go_prev");

  windowNext = KStdAction::back(viewManager, SLOT(slotWindowNext()), actionCollection());
  windowPrev = KStdAction::forward(viewManager, SLOT(slotWindowPrev()), actionCollection());

  docListMenu = new KActionMenu(i18n("&Document List"), actionCollection(), "doc_list");
  connect(docListMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(docListMenuAboutToShow()));

  setEndOfLine = new KSelectAction(i18n("&End Of Line"), 0, actionCollection(), "set_eol");
  connect(setEndOfLine, SIGNAL(activated(int)), viewManager, SLOT(setEol(int)));
  connect(setEndOfLine->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(setEOLMenuAboutToShow()));
  QStringList list;
  list.append("&Unix");
  list.append("&Macintosh");
  list.append("&Windows/Dos");
  setEndOfLine->setItems(list);

  documentReload = new KAction(i18n("&Reload"), "reload", Key_F4, viewManager, SLOT(reloadCurrentDoc()), actionCollection(), "document_reload");

  setHighlightConf = new KAction(i18n("Configure Highlighti&ng..."), 0, this, SLOT(slotHlConfigure()),actionCollection(), "set_confHighlight");

  setHighlight = new KSelectAction(i18n("&Highlight Mode"), 0, actionCollection(), "set_highlight");
  connect(setHighlight, SIGNAL(activated(int)), viewManager, SLOT(slotSetHl(int)));
  connect(setHighlight->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(setHighlightMenuAboutToShow()));

  list.clear();
  for (int z = 0; z < HlManager::self()->highlights(); z++)
       list.append(i18n(HlManager::self()->hlName(z)));
  setHighlight->setItems(list);

  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");

  // toggle sidebar -anders
  settingsShowSidebar = new KToggleAction(i18n("Show Side&bar"), CTRL+Key_B, this, SLOT(slotSettingsShowSidebar()), actionCollection(), "settings_show_sidebar");
  settingsShowConsole = new KToggleAction(i18n("Show &Console"), CTRL+Key_T, this, SLOT(slotSettingsShowConsole()), actionCollection(), "settings_show_console");
  // allow full path in title -anders
  settingsShowFullPath = new KToggleAction(i18n("Show Full &Path in Title"), 0, this, SLOT(slotSettingsShowFullPath()), actionCollection(), "settings_show_full_path");
  settingsShowToolbar = KStdAction::showToolbar(this, SLOT(slotSettingsShowToolbar()), actionCollection(), "settings_show_toolbar");
  settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");
  settingsShowFullScreen = new KToggleAction(i18n("Show &Fullscreen"),0,this,SLOT(slotSettingsShowFullScreen()),actionCollection(),"settings_show_fullscreen");

  connect(viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));
  connect(viewManager,SIGNAL(statChanged()),this,SLOT(slotCurrentDocChanged()));

  sidebarFocusNext = new KAction(i18n("Next Sidebar &Widget"), CTRL+SHIFT+Key_B, this, SLOT(slotSidebarFocusNext()), actionCollection(), "sidebar_focus_next");

  slotWindowActivated ();
}

bool KantMainWindow::queryClose()
{
  bool val = false;

  if ( ((KantApp *)kapp)->mainWindowsCount () < 2 )
  {
    saveOptions(config);

    viewManager->saveAllDocsAtCloseDown(  );

    if ( (!docManager->currentDoc()) || ((!viewManager->activeView()->doc()->isModified()) && (docManager->docCount() == 1)))
    {
      if (viewManager->activeView()) viewManager->deleteLastView ();
      val = true;
    }
  }
  else
    val = true;

  if (val)
  {
    ((KantApp *)kapp)->removeMainWindow (this);
    if (consoleDock->isVisible())
      consoleDock->changeHideShowState();
  }

  return val;
}
// anders: attempt to get session manager to restore kant.
void KantMainWindow::saveProperties(KConfig* cfg)
{
  cfg->writeEntry("hello", "world");
}

void KantMainWindow::newWindow ()
{
  ((KantApp *)kapp)->newMainWindow ();
}

void KantMainWindow::slotEditToolbars()
{
  KEditToolbar dlg(factory());

  if (dlg.exec())
    createGUI();
}

void KantMainWindow::slotFileQuit()
{

  viewManager->saveAllDocsAtCloseDown(  );

  close();
}

void KantMainWindow::readOptions(KConfig *config)
{
 /* readConfig(config);
  kWrite->readSessionConfig(config);*/
  config->setGroup("General");
  sidebarDock->resize( config->readSizeEntry("Sidebar:size", new QSize(150, height())) );
  settingsShowSidebar->setChecked( config->readBoolEntry("Show Sidebar", true) );
  resize( config->readSizeEntry( "size", new QSize(600, 400) ) );
  viewManager->setShowFullPath(config->readBoolEntry("Show Full Path in Title", false));
  settingsShowFullPath->setChecked(viewManager->getShowFullPath());
  settingsShowToolbar->setChecked(config->readBoolEntry("Show Toolbar", true));
  slotSettingsShowToolbar();
  viewManager->setUseOpaqueResize(config->readBoolEntry("Opaque Resize", true));

  fileOpenRecent->loadEntries(config, "Recent Files");

  fileselector->readConfig(config, "fileselector");
  fileselector->setView(KFile::Default);

  sidebar->readConfig( config );

  readDockConfig();
}

void KantMainWindow::saveOptions(KConfig *config)
{
  config->setGroup("General");
  config->writeEntry("Show Sidebar", sidebar->isVisible());
  config->writeEntry("size", size());
  config->writeEntry("Show Full Path in Title", viewManager->getShowFullPath());
  config->writeEntry("Show Toolbar", settingsShowToolbar->isChecked());
  config->writeEntry("Opaque Resize", viewManager->useOpaqueResize);

  fileOpenRecent->saveEntries(config, "Recent Files");

  fileselector->saveConfig(config, "fileselector");
  sidebar->saveConfig( config );
  writeDockConfig();

  if (viewManager->activeView())
    KantFactory::instance()->config()->sync();

  // Do not uncomment this unless you wish to see how
  // view configuration looks in $KDEHOME/share/config/kantsessionrc
  // It does not crash :) and I'm getting closer to finish.
  viewManager->saveViewSpaceConfig();
}

void KantMainWindow::slotWindowActivated ()
{
  if (viewManager->activeView() == 0)
  {
    fileSave->setEnabled(false);
    fileSaveAs->setEnabled(false);
    filePrint->setEnabled(false);
    fileClose->setEnabled(false);
    editUndo->setEnabled(false);
    editRedo->setEnabled(false);
    editUndoHist->setEnabled(false);
    editCut->setEnabled(false);
    editCopy->setEnabled(false);
    editPaste->setEnabled(false);
    editSelectAll->setEnabled(false);
    editDeselectAll->setEnabled(false);
    editInvertSelection->setEnabled(false);
    editFind->setEnabled(false);
    editFindNext->setEnabled(false);
    editReplace->setEnabled(false);
    editInsert->setEnabled(false);
    toolsSpell->setEnabled(false);
    setHighlightConf->setEnabled(false);
    setHighlight->setEnabled(false);
    gotoLine->setEnabled(false);
  }

  if (viewManager->activeView() != 0)
  {
    if (console)
      console->cd (viewManager->activeView()->doc()->url());

    fileSave->setEnabled(viewManager->activeView()->doc()->isModified());
    fileSaveAs->setEnabled(true);
    filePrint->setEnabled(true);
    fileClose->setEnabled(true);
    editCut->setEnabled(true);
    editCopy->setEnabled(true);
    editPaste->setEnabled(true);
    editSelectAll->setEnabled(true);
    editDeselectAll->setEnabled(true);
    editInvertSelection->setEnabled(true);
    editFind->setEnabled(true);
    editFindNext->setEnabled(true);
    editReplace->setEnabled(true);
    editInsert->setEnabled(true);
    toolsSpell->setEnabled(true);
    setHighlightConf->setEnabled(true);
    setHighlight->setEnabled(true);
    gotoLine->setEnabled(true);
  }

  if (viewManager->viewCount () == 0 )
  {
    fileCloseAll->setEnabled(false);
    windowNext->setEnabled(false);
    windowPrev->setEnabled(false);
    viewSplitVert->setEnabled(false);
    viewSplitHoriz->setEnabled(false);
    docListMenu->setEnabled(false);
  }
  else
  {
    fileCloseAll->setEnabled(true);
    docListMenu->setEnabled(true);

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

    viewSplitVert->setEnabled(true);
    viewSplitHoriz->setEnabled(true);
  }

  if (viewManager->viewSpaceCount() == 1)
    closeCurrentViewSpace->setEnabled(false);
  else
    closeCurrentViewSpace->setEnabled(true);
}

void KantMainWindow::slotCurrentDocChanged()
{
  if (!viewManager->activeView())
    return;

  fileSave->setEnabled( viewManager->activeView()->doc()->isModified() );

  if (!(viewManager->activeView()->undoState() & 1))
  {
    editUndo->setEnabled(false);
  }
  else
  {
    editUndo->setEnabled(true);
  }

  if (!(viewManager->activeView()->undoState() & 2))
  {
    editRedo->setEnabled(false);
  }
  else
  {
    editRedo->setEnabled(true);
  }

  if (!(viewManager->activeView()->undoState() & 1) && !(viewManager->activeView()->undoState() & 2))
  {
    editUndoHist->setEnabled(false);
  }
  else
  {
    editUndoHist->setEnabled(true);
  }
}

void KantMainWindow::docListMenuAboutToShow()
{
  docListMenu->popupMenu()->clear();

  if (docManager->docCount() == 0) return;

  int z=0;
  int i=1;
  int id = 0;
  docListMenu->popupMenu()->polish(); // adjust system settings
  QFont fMod = docListMenu->popupMenu()->font();
  fMod.setBold( TRUE );
  QFont fUnMod = docListMenu->popupMenu()->font();

  QString Entry;
  while ( z<docManager->docCount() )
  {
    if ( (!docManager->nthDoc(z)->url().isEmpty()) && (docManager->nthDoc(z)->url().filename() != 0) )
	{
		Entry=QString("&%1 ").arg(i)+docManager->nthDoc(z)->url().filename();
	}
	else
	{
		Entry=QString("&%1 ").arg(i)+i18n("Untitled %1").arg(docManager->nthDoc(z)->docID());
	}
    id=docListMenu->popupMenu()->insertItem(new KantMenuItem(Entry,
			docManager->nthDoc(z)->isModified() ? fMod : fUnMod,
                        docManager->nthDoc(z)->isModified() ? SmallIcon("modified") : SmallIcon("null")) );
    docListMenu->popupMenu()->connectItem(id, viewManager, SLOT( activateView ( int ) ) );

    docListMenu->popupMenu()->setItemParameter( id, docManager->nthDoc(z)->docID() );

    if (viewManager->activeView())
      docListMenu->popupMenu()->setItemChecked( id, ((KantDocument *)viewManager->activeView()->doc())->docID() == docManager->nthDoc(z)->docID() );

    z++;
    i++;
  }
}

void KantMainWindow::bookmarkMenuAboutToShow()
{
   bookmarkMenu->popupMenu()->clear();
   viewManager->activeView()->doUpdateBookmarks();
   QList<KAction> l =  viewManager->activeView()->bmActions();
   QListIterator<KAction> it(l);
   for (; it.current(); ++it)
     it.current()->plug( bookmarkMenu->popupMenu() );
}

void KantMainWindow::setHighlightMenuAboutToShow()
{
   setHighlight->setCurrentItem( viewManager->activeView()->getHl() );
}

void KantMainWindow::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(QUriDrag::canDecode(event));
}

void KantMainWindow::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void KantMainWindow::slotDropEvent( QDropEvent * event )
{
  KURL::List textlist;
  if (!KURLDrag::decode(event, textlist)) return;

  for (KURL::List::Iterator i=textlist.begin(); i != textlist.end(); ++i)
  {
    viewManager->openURL (*i);
  }
}

void KantMainWindow::editKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KantMainWindow::slotSettingsShowSidebar()
{
  sidebarDock->changeHideShowState();
}

void KantMainWindow::slotSettingsShowConsole()
{
  consoleDock->changeHideShowState();
  // Anders: focus at show
  if( consoleDock->isVisible() )
    console->setFocus();
  else
    viewManager->activeView()->setFocus();
}

void KantMainWindow::settingsMenuAboutToShow()
{
  settingsShowSidebar->setChecked( sidebarDock->isVisible() );
  settingsShowConsole->setChecked( consoleDock->isVisible() );
}

void KantMainWindow::setEOLMenuAboutToShow()
{
  int eol = viewManager->activeView()->getEol()-1;
  eol = eol>=0? eol: 0;
  setEndOfLine->setCurrentItem( eol );
}

void KantMainWindow::openURL (const QString &name)
{
  viewManager->openURL (KURL(name));
}

void KantMainWindow::ShowErrorMessage (const QString & strFileName, int nLine, const QString & strMessage)
{
 // TODO put the error delivery stuff here instead of after the piper
        POP_(strFileName.latin1());
        POP_(nLine);
        POP_(strMessage.latin1());
}

void KantMainWindow::slotSettingsShowFullPath()
{
  viewManager->setShowFullPath( settingsShowFullPath->isChecked() );
}

void KantMainWindow::slotSettingsShowToolbar()
{
  if (settingsShowToolbar->isChecked())
    toolBar()->show();
  else
    toolBar()->hide();
}

void KantMainWindow::slotConfigure()
{
  KantView* v = 0L;
  v = viewManager->activeView();

  if (!v) return;

  KDialogBase* dlg = new KDialogBase(KDialogBase::IconList, i18n("Configure Kant"), KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, this, "configdialog");//KantConfigDlg(this);

  QFrame* frGeneral = dlg->addPage(i18n("General"), i18n("General Options"), BarIcon("misc", KIcon::SizeMedium));
  QGridLayout* gridFrG = new QGridLayout(frGeneral);
  gridFrG->setSpacing( 6 );

  // opaque resize of view splitters
  QCheckBox* cb_opaqueResize = new QCheckBox( frGeneral );
  cb_opaqueResize->setText(i18n("Show &Content when resizing views"));
  gridFrG->addMultiCellWidget( cb_opaqueResize, 0, 0, 0, 1 );
  cb_opaqueResize->setChecked(viewManager->useOpaqueResize);
  QWhatsThis::add(cb_opaqueResize, i18n("If this is disabled, resizing views will display a <i>rubberband</i> to show the new sizes untill you release the mouse button."));

  // reopen files
  QCheckBox* cb_reopenFiles = new QCheckBox( frGeneral );
  cb_reopenFiles->setText(i18n("Reopen &Files at startup"));
  gridFrG->addMultiCellWidget( cb_reopenFiles, 1, 1, 0, 1 );
  config->setGroup("open files");
  cb_reopenFiles->setChecked( config->readBoolEntry("reopen at startup", true) );
  QWhatsThis::add(cb_reopenFiles, i18n("If this is enabled Kant will attempt to reopen files that was open when you closed last time. Cursor position will be recovered if possible. Non-existing files will not be opened."));

  // restore view config
  QCheckBox* cb_restoreVC = new QCheckBox( frGeneral );
  cb_restoreVC->setText(i18n("Restore &View Configuration"));
  gridFrG->addMultiCellWidget( cb_restoreVC, 2, 2, 0, 1 );
  config->setGroup("General");
  cb_restoreVC->setChecked( config->readBoolEntry("restore views", true) );
  QWhatsThis::add(cb_restoreVC, i18n("Check this if you want all your views restored each time you open Kant"));

  QCheckBox *cb_singleInstance = new QCheckBox(frGeneral);
  cb_singleInstance->setText(i18n("Restrict to single instance"));
  gridFrG->addMultiCellWidget(cb_singleInstance,3,3,0,1);
  config->setGroup("startup");
  cb_singleInstance->setChecked(config->readBoolEntry("singleinstance",true));

  config->setGroup("General");

  // editor widgets from kwrite/kwdialog
  // color options
  QStringList path;
  path << i18n("Editor") << i18n("Colors");
  QVBox *page = dlg->addVBoxPage(path, i18n("Colors"),
                              BarIcon("colorize", KIcon::SizeMedium) );
  ColorConfig *colorConfig = new ColorConfig(page);
  // some kwrite tabs needs a kwrite as an arg!

  KSpellConfig * ksc = 0L;
  IndentConfigTab * indentConfig = 0L;
  SelectConfigTab * selectConfig = 0L;
  EditConfigTab * editConfig = 0L;
  QColor* colors = 0L;

  // indent options
  page=dlg->addVBoxPage(i18n("Indent"), i18n("Indent Options"),
                       BarIcon("rightjust", KIcon::SizeMedium) );
  indentConfig = new IndentConfigTab(page, v);

  // select options
  page=dlg->addVBoxPage(i18n("Select"), QString::null,
                       BarIcon("misc") );
  selectConfig = new SelectConfigTab(page, v);

  // edit options
  page=dlg->addVBoxPage(i18n("Edit"), QString::null,
                       BarIcon("edit", KIcon::SizeMedium ) );
  editConfig = new EditConfigTab(page, v);

  // spell checker
  page = dlg->addVBoxPage( i18n("Spelling"), i18n("Spell checker behavior"),
                          BarIcon("spellcheck", KIcon::SizeMedium) );
  ksc = new KSpellConfig(page, 0L, v->ksConfig(), false );
  colors = v->getColors();
  colorConfig->setColors( colors );

  page=dlg->addVBoxPage(i18n("Plugins"),i18n("Configure plugins"),
                          BarIcon("misc",KIcon::SizeMedium));
  (void)new KantConfigPluginPage(page);

  if (dlg->exec())
  {
    viewManager->setUseOpaqueResize(cb_opaqueResize->isChecked());
    config->setGroup("startup");
    config->writeEntry("singleinstance",cb_singleInstance->isChecked());
    config->setGroup("open files");
    config->writeEntry("reopen at startup", cb_reopenFiles->isChecked());

    config->setGroup("General");
    config->writeEntry("restore views", cb_restoreVC->isChecked());

    ksc->writeGlobalSettings();
    colorConfig->getColors( colors );
    config->setGroup("kwrite");
    v->writeConfig( config );
    v->doc()->writeConfig( config );
    v->applyColors();
    config->sync();

    // all docs need to reread config.

    QListIterator<KantDocument> dit (docManager->docList);
    for (; dit.current(); ++dit)
    {
      dit.current()->readConfig( config );
    }

    QListIterator<KantView> it (viewManager->viewList);
    for (; it.current(); ++it)
    {
      v = it.current();
      indentConfig->getData( v );
      selectConfig->getData( v );
      editConfig->getData( v );
    }

    // repeat some calls: kwrite has a bad design.
    config->setGroup("kwrite");
    v->writeConfig( config );
    v->doc()->writeConfig( config );
    config->sync();
  }

  delete dlg;
  dlg = 0;
}

void KantMainWindow::slotHlConfigure()
{
  if (viewManager->activeView())
    viewManager->activeView()->hlDlg();
}

//Set focus to next input element
void KantMainWindow::slotGoNext()
{
  QFocusEvent::setReason(QFocusEvent::Tab);
  /*res= */focusNextPrevChild(true); //TRUE == NEXT , FALSE = PREV
  QFocusEvent::resetReason();
}

//Set focus to previous input element
void KantMainWindow::slotGoPrev()
{
  QFocusEvent::setReason(QFocusEvent::Tab);
  /*res= */focusNextPrevChild(false); //TRUE == NEXT , FALSE = PREV
  QFocusEvent::resetReason();
}


void KantMainWindow::slotSettingsShowFullScreen()
{
	if (settingsShowFullScreen->isChecked())
	{
		showFullScreen();
	}
	else
	{
		showNormal();
	}
}

void KantMainWindow::slotSidebarFocusNext()
{
   if (! sidebarDock->isVisible()) {
     slotSettingsShowSidebar();
     return;
   }
   sidebar->focusNextWidget();
}

void KantMainWindow::focusInEvent(QFocusEvent*  /* e */)
{
  kdDebug(13000)<<"focusIn"<<endl;
  docManager->checkAllModOnHD();
}

KURL KantMainWindow::currentDocUrl()
{
  return viewManager->activeView()->doc()->url();
}

void KantMainWindow::fileSelected(const KFileViewItem *file)
{
  KURL u(file->urlString());
  viewManager->openURL( u );
}

QStringList KantMainWindow::containerTags() const
{
  kdDebug(13000)<<"This is KanMainWindow::containerTags()"<<endl;
  QStringList tmp = KDockMainWindow::containerTags();

  tmp<<tagSidebar;
  return tmp;
}    

QWidget *KantMainWindow::createContainer( QWidget *parent, int index,
          const QDomElement &element, int &id )
  {
    kdDebug(13000)<<"************************This is KantMainWindow::createContainer " << ((parent) ? parent->className() : "Null ") <<endl;
    if (element.tagName().lower()==tagSidebar)
      {
        kdDebug(13000)<<"****** A sidebar should be created";
        KDockWidget *tmp=createDockWidget("TMPDOCK",0);
        tmp->manualDock ( mainDock, KDockWidget::DockRight, 20 );
	tmp->setWidget(new KListBox(tmp));
	return tmp;
      }
    return KDockMainWindow::createContainer(parent,index,element,id);
  }

void KantMainWindow::restore(bool isRestored)
{ viewManager->reopenDocuments(isRestored); }
