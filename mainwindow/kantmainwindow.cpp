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

#include <cassert>
#include <fstream>
#include <iostream>
#include <qcheckbox.h>
#include <qiconview.h>
#include <qinputdialog.h>
#include <qlist.h>
#include <kapp.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kedittoolbar.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <kparts/event.h>
#include <kparts/part.h>
#include <kurldrag.h>
#include "../console/kantconsole.h"
#include "../document/kantdocmanager.h"
#include "../kwrite/kwdialog.h"
#include "../pluginmanager/kantpluginmanager.h"
#include "../project/kantprojectmanager.h"
#include "../view/kantviewmanager.h"
#include "kantmenuitem.h"

#define POP_(x) std::cout << #x " = " << flush << x << std::endl

KantMainWindow::KantMainWindow(KantPluginManager *_pluginManager) : KDockMainWindow  (0, "Main Window"), DCOPObject("KantIface" )
{
  pluginManager=_pluginManager;
  config = kapp->config();

  setXMLFile( "kantui.rc" );

  setAcceptDrops(true);

  setupMainWindow();
  setupActions();

  createGUI();

  readProperties(config);

  // connect slot for updating menustatus of "project"
  QPopupMenu* pm_project = (QPopupMenu*)factory()->container("project", this);
  connect(pm_project, SIGNAL(aboutToShow()), this, SLOT(projectMenuAboutToShow()));

  // connect settings menu aboutToshow
  QPopupMenu* pm_set = (QPopupMenu*)factory()->container("settings", this);
  connect(pm_set, SIGNAL(aboutToShow()), this, SLOT(settingsMenuAboutToShow()));

  setUpdatesEnabled(false);
//  QObject *jw1=new QObject(this);
//  KXMLGUIClient *gui=new KXMLGUIClient(this);
  KParts::Plugin::loadPlugins(viewManager,pluginManager->plugins);
        KParts::GUIActivateEvent ev( true );
        QApplication::sendEvent( viewManager, &ev );
 
//        guiFactory()->addClient( gui );

        QList<KParts::Plugin> plugins =KParts:: Plugin::pluginObjects( viewManager );
        QListIterator<KParts::Plugin> pIt( plugins );
        for (; pIt.current(); ++pIt )
            guiFactory()->addClient( pIt.current() );
  setUpdatesEnabled(true);

}

KantMainWindow::~KantMainWindow()
{
}

void KantMainWindow::setupMainWindow ()
{
  docManager = new KantDocManager ();

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
  viewManager = new KantViewManager (mainDock, docManager, sidebar);
  viewManager->setMinimumSize(200,200);
  mainDock->setWidget(viewManager);
  setView( mainDock );
  setMainDockWidget( mainDock );

  mainDock->setEnableDocking ( KDockWidget::DockNone );
  sidebarDock->manualDock ( mainDock, KDockWidget::DockLeft, 20 );
  consoleDock->manualDock ( mainDock, KDockWidget::DockBottom, 20 );

  sidebarDock->setFocusPolicy(QWidget::ClickFocus);
  consoleDock->setFocusPolicy(QWidget::ClickFocus);

  projectManager = new KantProjectManager (docManager, viewManager, statusBar());

  statusBar()->hide();
}

bool KantMainWindow::eventFilter(QObject* o, QEvent* e)
{
  if (e->type() == QEvent::KeyPress)
        {
                QKeyEvent *ke=(QKeyEvent*)e;
                if (ke->key()==goNext->accel())
                        {
                                kdDebug()<<"Jump next view  registered in Konsole";
				slotGoNext();
                                return true;
                        }
		else
	                if (ke->key()==goPrev->accel())
        	                {
                	                kdDebug()<<"Jump prev view  registered in Konsole";
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

  editInsert = new KAction(i18n("&Insert File..."), 0, viewManager, SLOT(slotInsertFile()), actionCollection(), "edit_insertFile");

  gotoLine = KStdAction::gotoLine(viewManager, SLOT(slotGotoLine()), actionCollection());
  toolsSpell = KStdAction::spelling(viewManager, SLOT(slotSpellcheck()), actionCollection());

  new KAction ( i18n("Fi&lter Text..."), "edit_filter", CTRL + Key_Backslash, this,
  				SLOT( slotEditFilter() ), actionCollection(), "edit_filter" );
  viewSplitVert = new KAction( i18n("Split &Vertical"), "view_left_right", CTRL+SHIFT+Key_L, viewManager, SLOT( slotSplitViewSpaceVert() ), actionCollection(), "view_split_vert");
  viewSplitHoriz = new KAction( i18n("Split &Horizontal"), "view_top_bottom", CTRL+SHIFT+Key_T, viewManager, SLOT( slotSplitViewSpaceHoriz() ), actionCollection(), "view_split_horiz");
  closeCurrentViewSpace = new KAction( i18n("Close &Current"), "remove_view", CTRL+SHIFT+Key_R, viewManager, SLOT( slotCloseCurrentViewSpace() ), actionCollection(), "view_close_current_space");

  goNext=new KAction(i18n("Next View"),Key_F8,viewManager, SLOT(activateNextView()),actionCollection(),"go_next");
  goPrev=new KAction(i18n("Previous View"),SHIFT+Key_F8,viewManager, SLOT(activatePrevView()),actionCollection(),"go_prev");

  windowNext = KStdAction::back(viewManager, SLOT(slotWindowNext()), actionCollection());
  windowPrev = KStdAction::forward(viewManager, SLOT(slotWindowPrev()), actionCollection());

  docListMenu = new KActionMenu(i18n("&Document List"), actionCollection(), "doc_list");
  connect(docListMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(docListMenuAboutToShow()));

  documentReload = new KAction(i18n("&Reload"), "reload", Key_F5, viewManager, SLOT(reloadCurrentDoc()), actionCollection(), "document_reload");

  setHighlightConf = new KAction(i18n("Configure Highlighti&ng..."), 0, viewManager, SLOT(slotHlDlg()),actionCollection(), "set_confHighlight");

  setHighlight = new KSelectAction(i18n("&Highlight Mode"), 0, actionCollection(), "set_highlight");
  connect(setHighlight, SIGNAL(activated(int)), viewManager, SLOT(slotSetHl(int)));
  QStringList list;
  for (int z = 0; z < HlManager::self()->highlights(); z++)
       list.append(i18n(HlManager::self()->hlName(z)));
  setHighlight->setItems(list);

  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(slotEditToolbars()), actionCollection(), "set_configure_toolbars");

  // toggle sidebar -anders
  settingsShowSidebar = new KToggleAction(i18n("Show Side&bar"), CTRL+Key_B, this, SLOT(slotSettingsShowSidebar()), actionCollection(), "settings_show_sidebar");
  settingsShowConsole = new KToggleAction(i18n("Show &Console"), CTRL+Key_K, this, SLOT(slotSettingsShowConsole()), actionCollection(), "settings_show_console");
  // allow full path in title -anders
  settingsShowFullPath = new KToggleAction(i18n("Show Full &Path in Title"), 0, this, SLOT(slotSettingsShowFullPath()), actionCollection(), "settings_show_full_path");
  settingsShowToolbar = KStdAction::showToolbar(this, SLOT(slotSettingsShowToolbar()), actionCollection(), "settings_show_toolbar");
  settingsConfigure = KStdAction::preferences(this, SLOT(slotConfigure()), actionCollection(), "settings_configure");
  settingsShowFullScreen = new KToggleAction(i18n("Show &Fullscreen"),0,this,SLOT(slotSettingsShowFullScreen()),actionCollection(),"settings_show_fullscreen");

  // KActions for Project
  projectNew = new KAction(i18n("&New"), 0, projectManager,
  	 SLOT(slotProjectNew()), actionCollection(),"project_new");
  projectOpen = new KAction(i18n("&Open"), 0, projectManager,
  	 SLOT(slotProjectOpen()), actionCollection(),"project_open");
  projectSave = new KAction(i18n("&Save"), 0, projectManager,
  	 SLOT(slotProjectSave()), actionCollection(),"project_save");
  projectSaveAs = new KAction(i18n("&SaveAs"), 0, projectManager,
  	 SLOT(slotProjectSaveAs()), actionCollection(),"project_save_as");
  projectConfigure = new KAction(i18n("&Configure"), 0, projectManager,
  	 SLOT(slotProjectConfigure()), actionCollection(),"project_configure");
  projectCompile = new KAction(i18n("&Compile"), Key_F5, projectManager,
  	 SLOT(slotProjectCompile()), actionCollection(),"project_compile");
  projectRun = new KAction(i18n("&Run"), 0, projectManager,
  	 SLOT(slotProjectRun()), actionCollection(),"project_run");

  // KActions for Plugins
  pluginsAdd = new KAction(i18n("&Add"), 0, this,
  	 SLOT(slotPluginsAdd()), actionCollection(),"plugins_add");
  pluginsDelete = new KAction(i18n("&Delete"), 0, this,
  	 SLOT(slotPluginsDelete()), actionCollection(),"plugins_delete");

  connect(viewManager,SIGNAL(viewChanged()),this,SLOT(slotWindowActivated()));
  connect(viewManager,SIGNAL(statChanged()),this,SLOT(slotCurrentDocChanged()));

  sidebarFocusNext = new KAction(i18n("Next Sidebar &Widget"), CTRL+SHIFT+Key_B, this, SLOT(slotSidebarFocusNext()), actionCollection(), "sidebar_focus_next");

  slotWindowActivated ();
}

bool KantMainWindow::queryClose()
{
  saveProperties(config);
//  viewManager->slotDocumentCloseAll();
  viewManager->saveAllDocsAtCloseDown( config );

  if ( (!docManager->currentDoc()) || ((!viewManager->activeView()->doc()->isModified()) && (docManager->docCount() == 1)))
  {
    if (viewManager->activeView()) viewManager->deleteLastView ();
    return true;
  }

  return false;
}

void KantMainWindow::slotEditToolbars()
{
  KEditToolbar dlg(factory());//(actionCollection());

  if (dlg.exec())
    createGUI();
}

        void
splitString (QString q, char c, QStringList &list)  //  PCP
{

// screw the OnceAndOnlyOnce Principle!

  int pos;
  QString item;

  while ( (pos = q.find(c)) >= 0)
    {
      item = q.left(pos);
      list.append(item);
      q.remove(0,pos+1);
    }
  list.append(q);
}


        static void  //  PCP
slipInNewText (KantView & view, QString pre, QString marked, QString post, bool reselect)
{

  int preDeleteLine = -1, preDeleteCol = -1;
  view.getCursorPosition (&preDeleteLine, &preDeleteCol);
  assert (preDeleteLine > -1);  assert (preDeleteCol > -1);

  //  shoot me for strlen() but it worked better than .length() for some reason...

//  POP_(marked.latin1 ());
  if (strlen (marked.latin1 ()) > 0)  view.keyDelete ();
  int line = -1, col = -1;
  view.getCursorPosition (&line, &col);
  assert (line > -1);  assert (col > -1);
  view.insertText (pre + marked + post);

  //  all this muck to leave the cursor exactly where the user
  //  put it...

  //  Someday we will can all this (unless if it already
  //  is canned and I didn't find it...)

  //  The second part of the if disrespects the display bugs
  //  when we try to reselect. TODO: fix those bugs, and we can
  //  un-break this if...

  //  TODO: fix OnceAndOnlyOnce between this module and plugin_kanthtmltools.cpp

  if (reselect && preDeleteLine == line && -1 == marked.find ('\n'))
    if (preDeleteLine == line && preDeleteCol == col)
        {
        view.setCursorPosition (line, col + pre.length () + marked.length () - 1);

        for (int x (strlen (marked.latin1()));  x--;)
                view.shiftCursorLeft ();
        }
   else
        {
        view.setCursorPosition (line, col += pre.length ());

        for (int x (strlen (marked.latin1()));  x--;)
                view.shiftCursorRight ();
        }

}


/*        static void
slipInHTMLtag (KantView & view, QString text)  //  PCP
{

  //  We must add a <em>heavy</em> elaborate HTML markup system. Not!

   QStringList list;
   splitString (text, ' ', list);
   QString marked (view.markedText ());
   QString pre ("<" + text + ">");
   QString post;
   if (list.count () > 0)  post = "</" + list[0] + ">";
   slipInNewText (view, pre, marked, post, true);

}*/


        static QString  //  PCP
KantPrompt
        (
        char const     * strTitle,
        char const     * strPrompt,
        KantMainWindow * that
        )
{

  bool ok (false);

  //  TODO: Make this a "memory edit" field with a combo box
  //  containing prior entries

  QString text ( QInputDialog::getText
                        (
                        that -> tr( strTitle ),
                        that -> tr( strPrompt ),
                        QString::null,
                        &ok,
                        that
                        ) );

  if (!ok) text = "";
  return text;

}


  /*      void
KantMainWindow::slotEditHTMLtag ()  //  PCP
{

  if (!viewManager)  return;
  KantView * kv (viewManager -> activeView ());
  if (!kv) return;

  QString text ( KantPrompt ( "HTML Tag",
                        "Enter HTML tag contents. We will supply the <, > and closing tag",
                        this
                        ) );

  if ( !text.isEmpty () )
      slipInHTMLtag (*kv, text); // user entered something and pressed ok

}
*/

        static void  //  PCP
slipInFilter (KantView & view, QString text)
{

  QString marked (view.markedText ());

    //  TODO make this a real temp file
    //  (sorry, but I did not expect
    //  a non-POSIX-compliant popen when I got
    //  here. We could also create a KantPopen
    //  that does all the dup & fork mucking
    //  on real bi-pipes)

  std::ofstream out (".scratch.txt");
  out << marked.latin1 ();
  out.close ();
  text += "<.scratch.txt >.scratch2.txt ";

  system (text.latin1 ());
  FILE * fp (fopen (".scratch2.txt", "r"));

  if (fp)
        {
        //fputs (marked.latin1 (), fp);
        QString replacement;

        while (!feof (fp))
                {
                int ch (fgetc (fp));
                if (ch == EOF)  break;
                replacement += char (ch);
                }

        fclose (fp);
        slipInNewText (view, "", replacement, "", false);
        }
  else
        perror ("fopen failed");

}


                 void
KantMainWindow::slotEditFilter ()  //  PCP
{
  if (!viewManager)  return;
  KantView * kv (viewManager -> activeView ());
  if (!kv) return;

  QString text ( KantPrompt ( "Filter",
                        "Enter command to pipe selected text thru",
                        this
                        ) );

  if ( !text.isEmpty () )
      slipInFilter (*kv, text);
}


void KantMainWindow::slotFileQuit()
{

//  viewManager->slotDocumentCloseAll();
  viewManager->saveAllDocsAtCloseDown( config );
  close();
}


void KantMainWindow::readProperties(KConfig *config)
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

  viewManager->fileselector->readConfig(config, "fileselector");
  viewManager->fileselector->setView(KFile::Default);

  sidebar->readConfig( config );

  readDockConfig();
}


void KantMainWindow::saveProperties(KConfig *config)
{
/*  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find(kWrite->doc()) + 1);
  kWrite->writeSessionConfig(config); */
  config->setGroup("General");
  config->writeEntry("Show Sidebar", sidebar->isVisible());
  config->writeEntry("size", size());
  config->writeEntry("Show Full Path in Title", viewManager->getShowFullPath());
  config->writeEntry("Show Toolbar", settingsShowToolbar->isChecked());
  config->writeEntry("Opaque Resize", viewManager->useOpaqueResize);

  fileOpenRecent->saveEntries(config, "Recent Files");

  viewManager->fileselector->saveConfig(config, "fileselector");
  sidebar->saveConfig( config );
  writeDockConfig();
}


void KantMainWindow::saveGlobalProperties(KConfig * /*config*/ )
{
  /*int z;
  char buf[16];
  KWriteDoc *doc;

  config->setGroup("Number");
  config->writeEntry("NumberOfDocuments",docList.count());

  for (z = 1; z <= (int) docList.count(); z++) {
     sprintf(buf,"Document%d",z);
     config->setGroup(buf);
     doc = docList.at(z - 1);
     doc->writeSessionConfig(config);
  } */
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

void KantMainWindow::slotPluginsAdd()
{
}

void KantMainWindow::slotPluginsDelete()
{
}

void KantMainWindow::projectMenuAboutToShow()
{

 // projectCompile->setEnabled(true); PCP - the best thing to do here would be to test for an executable 'builder.sh' in the current directory...

  projectConfigure->setEnabled(false);
  projectRun->setEnabled(false);

  if (projectManager->projectFile->isEmpty())
    projectSave->setEnabled(false);
  else
    projectSave->setEnabled(true);

  if (docManager->docCount () == 0)
   projectSaveAs->setEnabled(false);
  else
    projectSaveAs->setEnabled(true);
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
}

void KantMainWindow::settingsMenuAboutToShow()
{
  settingsShowSidebar->setChecked( sidebarDock->isVisible() );
  settingsShowConsole->setChecked( consoleDock->isVisible() );
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
  KDialogBase* dlg = new KDialogBase(KDialogBase::IconList, "Configure Kant", KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, this, "configdialog");//KantConfigDlg(this);

  QFrame* frGeneral = dlg->addPage(i18n("General"), i18n("General Options"), BarIcon("misc", KIcon::SizeMedium));
  QGridLayout* gridFrG = new QGridLayout(frGeneral);
  gridFrG->setSpacing( 6 );

  // opaque resize of view splitters
  QCheckBox* cb_opaqueResize = new QCheckBox( frGeneral );
  cb_opaqueResize->setText(i18n("Show &Content when resizing views"));
  gridFrG->addMultiCellWidget( cb_opaqueResize, 0, 0, 0, 1 );
  cb_opaqueResize->setChecked(viewManager->useOpaqueResize);

  // reopen files
  QCheckBox* cb_reopenFiles = new QCheckBox( frGeneral );
  cb_reopenFiles->setText(i18n("Reopen &Files at startup"));
  gridFrG->addMultiCellWidget( cb_reopenFiles, 1, 1, 0, 1 );
  config->setGroup("open files");
  cb_reopenFiles->setChecked( config->readBoolEntry("reopen at startup", true) );

  // editor widgets from kwrite/kwdialog
  // color options
  QStringList path;
  path << i18n("Editor") << i18n("Colors");
  QVBox *page = dlg->addVBoxPage(path, i18n("Colors"),
                              BarIcon("colorize", KIcon::SizeMedium) );
  ColorConfig *colorConfig = new ColorConfig(page);
  // some kwrite tabs needs a kwrite as an arg!
  KantView* v = viewManager->activeView();
  KSpellConfig * ksc = 0L;
  IndentConfigTab * indentConfig = 0L;
  SelectConfigTab * selectConfig = 0L;
  EditConfigTab * editConfig = 0L;
  QColor* colors = 0L;
  // the test can go if we are sure we allways have 1 view
  if (v) {
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
  }

  if (dlg->exec()) {
    viewManager->setUseOpaqueResize(cb_opaqueResize->isChecked());
    config->setGroup("open files");
    config->writeEntry("reopen at startup", cb_reopenFiles->isChecked());

    if (viewManager->viewCount()) {
      ksc->writeGlobalSettings();
      colorConfig->getColors( colors );
      config->setGroup("kwrite");
      v->writeConfig( config );
      v->doc()->writeConfig( config );
      QListIterator<KantView> it (viewManager->viewList);
      for (; it.current(); ++it) {
        v = it.current();
	v->applyColors();
        indentConfig->getData( v );
        selectConfig->getData( v );
        editConfig->getData( v );
      }
    }
  }
  delete dlg;
  dlg = 0;
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

void KantMainWindow::reopenDocuments(bool isRestore)
{
  // read the list and loop around it.
  config->setGroup("open files");
  if (config->readBoolEntry("reopen at startup", true) || isRestore )
  {
    QStringList list = config->readListEntry("list");

    for ( int i = list.count() - 1; i > -1; i-- ) {
      QStringList data = config->readListEntry( list[i] );
      // open file
      viewManager->openURL( KURL(data[0]) );
      // restore cursor position
      int dot, line, col;
      dot = data[1].find(".");
      line = data[1].left(dot).toInt();
      col = data[1].mid(dot + 1, 100).toInt();
      if (viewManager->activeView()->numLines() >= line) // HACK--
        viewManager->activeView()->setCursorPosition(line, col);
    }
    config->writeEntry("list", "");
    config->sync();
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
  kdDebug()<<"focusIn"<<endl;
  docManager->checkAllModOnHD();
}
