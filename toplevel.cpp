/*
  $Id$

    Copyright (C) 1998, 1999 Jochen Wilhelmy
                             digisnap@cs.tu-berlin.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <qkeycode.h>
#include <qtabdialog.h>
#include <qdropsite.h>
#include <qdragobject.h>
#include <qvbox.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kapp.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kiconloader.h>
#include <kurl.h>
#include <kstdaccel.h>
#include <kconfig.h>
#include <kwin.h>
#include <kcmdlineargs.h>
#include <kdialogbase.h>

#include "kwdialog.h"
#include "highlight.h"
#include "kwrite_factory.h"

#include "toplevel.h"
#include "toplevel.moc"

// StatusBar field IDs
#define ID_LINE_COLUMN 1
#define ID_INS_OVR 2
#define ID_MODIFIED 3
#define ID_GENERAL 4

const int toolUndo = 1;
const int toolRedo = 2;
const int toolCut = 3;
const int toolPaste = 4;

//command categories
const int ctFileCommands = 10;
//file commands
const int cmNew             = 1;
const int cmOpen            = 2;
const int cmInsert          = 3;
const int cmSave            = 4;
const int cmSaveAs          = 5;
const int cmPrint           = 6;
const int cmNewWindow       = 7;
const int cmNewView         = 8;
const int cmClose           = 9;



QList<KWriteDoc> docList; //documents
//HlManager hlManager; //highlight manager
//KGuiCmdManager cmdMngr; //manager for user -> gui commands


TopLevel::TopLevel (KWriteDoc *doc, const QString &path)
  : KTMainWindow("KWrite") {

  setMinimumSize(180,120);

  recentFiles.setAutoDelete(TRUE);

  statusbarTimer = new QTimer(this);
  connect(statusbarTimer,SIGNAL(timeout()),this,SLOT(timeout()));

//  connect(kapp,SIGNAL(kdisplayPaletteChanged()),this,SLOT(set_colors()));

  if (!doc) {
    doc = new KWriteDoc(HlManager::self(), path); //new doc with default path
    docList.append(doc);
  }
  setupEditWidget(doc);
  setupMenuBar();
  setupToolBar();
  setupStatusBar();

//  readConfig();

  setAcceptDrops(true);
}

TopLevel::~TopLevel() {

//  delete file;
//  delete edit;
//  delete help;
//  delete options;
//  delete recentpopup;
//  delete toolbar;
  if (kWrite->isLastView()) docList.remove(kWrite->doc());
}

void TopLevel::init() {

  hideToolBar = !hideToolBar;
  toggleToolBar();
  hideStatusBar = !hideStatusBar;
  toggleStatusBar();
  showPath = !showPath;
  togglePath();
  newCurPos();
  newStatus();
  newUndo();

  show();
}

bool TopLevel::queryClose() {
  if (!kWrite->isLastView()) return true;
  return kWrite->canDiscard();
//  writeConfig();
}

bool TopLevel::queryExit() {
  writeConfig();
  kapp->config()->sync();

  return true;
}

void TopLevel::loadURL(const KURL &url, int flags) {
  kWrite->loadURL(url,flags);
}


void TopLevel::setupEditWidget(KWriteDoc *doc) {
  kWrite = new KWrite(doc, this, 0, false);

  connect(kWrite,SIGNAL(newCurPos()),this,SLOT(newCurPos()));
  connect(kWrite,SIGNAL(newStatus()),this,SLOT(newStatus()));
  connect(kWrite,SIGNAL(statusMsg(const QString &)),this,SLOT(statusMsg(const QString &)));
  connect(kWrite,SIGNAL(fileChanged()),this,SLOT(newCaption()));
  connect(kWrite,SIGNAL(newUndo()),this,SLOT(newUndo()));
  connect(kWrite->view(),SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));

  setView(kWrite,FALSE);
}

void TopLevel::setupMenuBar() {
  KMenuBar *menubar;
  KGuiCmdPopup *find, *bookmarks;
  QPopupMenu *help, *popup;
  QPixmap pixmap;
  int z;

  KGuiCmdDispatcher *dispatcher = kWrite->dispatcher();
  //  dispatcher = new KGuiCmdDispatcher(this, KGuiCmdManager::self());
  //  dispatcher->connectCategory(ctCursorCommands, kWrite, SLOT(doCursorCommand(int)));
  //  dispatcher->connectCategory(ctEditCommands, kWrite, SLOT(doEditCommand(int)));
  dispatcher->connectCategory(ctBookmarkCommands, kWrite, SLOT(doBookmarkCommand(int)));
  //  dispatcher->connectCategory(ctStateCommands, kWrite, SLOT(doStateCommand(int)));

  file =        new KGuiCmdPopup(dispatcher);
  edit =        new KGuiCmdPopup(dispatcher);
  find =        new KGuiCmdPopup(dispatcher);
  bookmarks =   new KGuiCmdPopup(dispatcher);
  options =     new KGuiCmdPopup(dispatcher);
  help =        new QPopupMenu();
  recentPopup = new QPopupMenu();

//    int addCommand(int catNum, int cmdNum, QPixmap &pixmap,
//      const QObject *receiver, const char *member, int id = -1, int index = -1);

  pixmap = BarIcon("filenew");
  file->addCommand(ctFileCommands, cmNew, pixmap, kWrite, SLOT(newDoc()));
  pixmap = BarIcon("fileopen");
  file->addCommand(ctFileCommands, cmOpen, pixmap, kWrite, SLOT(open()));
  menuInsert = file->addCommand(ctFileCommands, cmInsert, kWrite, SLOT(insertFile()));
  file->insertItem(i18n("Open &Recent"), recentPopup);
  connect(recentPopup, SIGNAL(activated(int)), SLOT(openRecent(int)));
  file->insertSeparator ();
  pixmap = BarIcon("filesave");
  menuSave = file->addCommand(ctFileCommands, cmSave, pixmap, kWrite, SLOT(save()));
  file->addCommand(ctFileCommands, cmSaveAs, kWrite, SLOT(saveAs()));
  file->insertSeparator ();
  pixmap = BarIcon("fileprint");
  file->addCommand(ctFileCommands, cmPrint, pixmap, this, SLOT(printDlg()));
  file->insertSeparator ();
  file->addCommand(ctFileCommands, cmNewWindow, this, SLOT(newWindow()));
  file->addCommand(ctFileCommands, cmNewView, this, SLOT(newView()));
  file->insertSeparator ();
  pixmap = BarIcon("exit");
  file->addCommand(ctFileCommands, cmClose, pixmap, this, SLOT(closeWindow()));

/*
  file->insertItem(i18n("&New..."),kWrite,SLOT(newDoc()),keys.openNew());
  file->insertItem(i18n("&Open..."),kWrite,SLOT(open()),keys.open());
  menuInsert = file->insertItem(i18n("&Insert..."),kWrite,SLOT(insertFile()));
  file->insertItem(i18n("Open Recen&t"), recentPopup);
  connect(recentPopup,SIGNAL(activated(int)),SLOT(openRecent(int)));
  file->insertSeparator ();
  menuSave = file->insertItem(i18n("&Save"),kWrite,SLOT(save()),keys.save());
  file->insertItem(i18n("S&ave as..."),kWrite,SLOT(saveAs()));
  file->insertSeparator ();
  file->insertItem(i18n("&Print..."), kWrite,SLOT(print()),keys.print());
  file->insertSeparator ();
//  file->insertItem (i18n("&Mail..."),this,SLOT(mail()));
//  file->insertSeparator ();
  file->insertItem (i18n("New &Window"),this,SLOT(newWindow()));
  file->insertItem (i18n("New &View"),this,SLOT(newView()));
  file->insertSeparator ();
  file->insertItem(i18n("&Close"),this,SLOT(closeWindow()),keys.close());
//  file->insertItem (i18n("E&xit"),this,SLOT(quitEditor()),keys.quit());
*/

  pixmap = BarIcon("undo");
  menuUndo = edit->addCommand(ctEditCommands, cmUndo, pixmap);
  pixmap = BarIcon("redo");
  menuRedo = edit->addCommand(ctEditCommands, cmRedo, pixmap);
  menuUndoHist = edit->insertItem(i18n("Undo/Redo &History..."),kWrite,SLOT(undoHistory()));
  edit->insertSeparator();
  pixmap = BarIcon("editcut");
  menuCut = edit->addCommand(ctEditCommands, cmCut, pixmap);
  pixmap = BarIcon("editcopy");
  edit->addCommand(ctEditCommands, cmCopy, pixmap);
  pixmap = BarIcon("editpaste");
  menuPaste = edit->addCommand(ctEditCommands, cmPaste, pixmap);
//  edit->insertSeparator();
//  pixmap = BarIcon("search");
//  edit->addCommand(ctFindCommands, cmFind, pixmap, kWrite, SLOT(search()));
//  edit->addCommand(ctFindCommands, cmReplace, kWrite, SLOT(replace()));
//  edit->addCommand(ctFindCommands, cmFindAgain, kWrite, SLOT(searchAgain()));
//  edit->addCommand(ctFindCommands, cmGotoLine, kWrite, SLOT(gotoLine()));
  edit->insertSeparator();
  edit->addCommand(ctEditCommands, cmIndent);
  edit->addCommand(ctEditCommands, cmUnindent);
  edit->addCommand(ctEditCommands, cmCleanIndent);
  edit->insertSeparator();
  edit->addCommand(ctEditCommands, cmComment);
  edit->addCommand(ctEditCommands, cmUncomment);
  edit->insertSeparator();
  edit->addCommand(ctEditCommands, cmSelectAll);
  edit->addCommand(ctEditCommands, cmDeselectAll);
  edit->addCommand(ctEditCommands, cmInvertSelection);
  edit->insertSeparator();
  pixmap = BarIcon("spellcheck");
  menuSpell = edit->insertItem(pixmap, i18n("Check Spe&lling..."), kWrite,SLOT(spellcheck()));


  // find dialog
  pixmap = BarIcon("find");
  find->addCommand(ctFindCommands, cmFind, pixmap, kWrite, SLOT(find()));
  find->addCommand(ctFindCommands, cmReplace, kWrite, SLOT(replace()));
  find->addCommand(ctFindCommands, cmFindAgain, kWrite, SLOT(findAgain()));
  find->addCommand(ctFindCommands, cmGotoLine, kWrite, SLOT(gotoLine()));

/*
  menuCut = edit->insertItem(i18n("C&ut"),kWrite,SLOT(cut()),keys.cut());
  edit->insertItem(i18n("&Copy"),kWrite,SLOT(copy()),keys.copy());
  menuPaste = edit->insertItem(i18n("&Paste"),kWrite,SLOT(paste()),keys.paste());
  edit->insertSeparator();
  edit->insertItem(i18n("C&ut"),kWrite,SLOT(cut()), keys.cut());
  edit->insertItem(i18n("&Copy"),kWrite,SLOT(copy()), keys.copy());
  edit->insertItem(i18n("&Paste"),kWrite,SLOT(paste()), keys.paste());
  edit->insertSeparator();
  edit->insertItem(i18n("&Find..."),kWrite,SLOT(search()),keys.find());
  menuReplace = edit->insertItem(i18n("&Replace..."),kWrite,SLOT(replace()),keys.replace());
  edit->insertItem(i18n("Find &Again"),kWrite,SLOT(searchAgain()),Key_F3);
  edit->insertItem(i18n("&Goto Line..."),kWrite,SLOT(gotoLine()),CTRL+Key_G);
  edit->insertSeparator();
  menuUndo = edit->insertItem(i18n("U&ndo"),kWrite,SLOT(undo()),keys.undo());
  menuRedo = edit->insertItem(i18n("R&edo"),kWrite,SLOT(redo()),CTRL+Key_Y);
  menuUndoHist = edit->insertItem(i18n("Undo/Redo &History..."),kWrite,SLOT(undoHistory()));
  edit->insertSeparator();
  menuIndent = edit->insertItem(i18n("&Indent"),kWrite,SLOT(indent()),CTRL+Key_I);
  menuUnindent = edit->insertItem(i18n("Uninden&t"),kWrite,SLOT(unIndent()),CTRL+Key_U);
  menuCleanIndent = edit->insertItem(i18n("C&lean Indentation"),kWrite,SLOT(cleanIndent()));
  edit->insertSeparator();
//  edit->insertItem(i18n("Format..."),kWrite,SLOT(format()));
//  edit->insertSeparator();
  edit->insertItem(i18n("&Select All"),kWrite,SLOT(selectAll()));
  edit->insertItem(i18n("&Deselect All"),kWrite,SLOT(deselectAll()));
  edit->insertItem(i18n("In&vert Selection"),kWrite,SLOT(invertSelection()));
  edit->insertSeparator();
  menuSpell = edit->insertItem(i18n("Spe&llcheck..."), kWrite,SLOT(spellcheck()));
//  edit->insertSeparator();
//  edit->insertItem(i18n("Insert &Date"),this,SLOT(insertDate()));
//  edit->insertItem(i18n("Insert &Time"),this,SLOT(insertTime()));
*/

//  bookmarks->insertItem(i18n("&Set Bookmark..."),kWrite,SLOT(setBookmark()),ALT+Key_S);
//  bookmarks->insertItem(i18n("&Add Bookmark"),kWrite,SLOT(addBookmark()));
//  bookmarks->insertItem(i18n("&Clear Bookmarks"),kWrite,SLOT(clearBookmarks()),ALT+Key_C);
  kWrite->installBMPopup(bookmarks);

  //highlight selector
  hlPopup = new QPopupMenu();
  hlPopup->setCheckable(true);
  for (z = 0; z < HlManager::self()->highlights(); z++) {
    hlPopup->insertItem(i18n(HlManager::self()->hlName(z)),z);
  }
  connect(hlPopup,SIGNAL(activated(int)),kWrite,SLOT(setHl(int)));

  // end of line selector
  eolPopup = new QPopupMenu();
  eolPopup->setCheckable(true);
  eolPopup->insertItem("Unix", eolUnix);
  eolPopup->insertItem("Macintosh", eolMacintosh);
  eolPopup->insertItem("Windows/Dos", eolDos);
  connect(eolPopup,SIGNAL(activated(int)),kWrite,SLOT(setEol(int)));

  options->setCheckable(TRUE);
  options->insertItem(i18n("Set Highlight"),hlPopup);
  connect(hlPopup,SIGNAL(aboutToShow()),this,SLOT(showHighlight()));
  options->insertItem(i18n("&Defaults..."),kWrite,SLOT(hlDef()));
  options->insertItem(i18n("&Highlight..."),kWrite,SLOT(hlDlg()));
//  indentID = options->insertItem(i18n("Auto &Indent"),this,SLOT(toggle_indent_mode()));
//  options->insertSeparator();
//  options->insertItem(i18n("&Options..."),kWrite,SLOT(optDlg()));
  options->insertItem(i18n("Colo&rs..."),kWrite,SLOT(colDlg()));
  options->insertSeparator();
  options->insertItem(i18n("&Configure..."), this, SLOT(configure()));
  options->insertItem(i18n("&Keys..."), this, SLOT(keys()));
  options->insertItem(i18n("End Of Line"),eolPopup);
  connect(eolPopup,SIGNAL(aboutToShow()),this,SLOT(showEol()));
//  options->insertItem(i18n("&Keys..."), this, SLOT(keyDlg()));
  options->insertSeparator();
  menuVertical = options->addCommand(ctStateCommands, cmToggleVertical);
    //Item(i18n("&Vertical Selections"),kWrite,SLOT(toggleVertical()),Key_F5);
  menuShowTB = options->insertItem(i18n("Show &Toolbar"),this,SLOT(toggleToolBar()));
  menuShowSB = options->insertItem(i18n("Show &Statusbar"),this,SLOT(toggleStatusBar()));
  menuShowPath = options->insertItem(i18n("Show &Path"),this,SLOT(togglePath()));
  options->insertItem(i18n("Save Config"),this,SLOT(writeConfig()));
//  options->insertSeparator();
//  options->insertItem(i18n("Save Options"),this,SLOT(save_options()));

  help = helpMenu( i18n("KWrite %1\n\nCopyright 1998, 1999\nJochen Wilhelmy\ndigisnap@cs.tu-berlin.de").arg(KWRITE_VERSION),false);

  //right mouse button popup
  popup = new QPopupMenu();

  pixmap = BarIcon("fileopen");
  popup->insertItem(QIconSet(pixmap), i18n("&Open..."),kWrite,SLOT(open()));
  pixmap = BarIcon("filefloppy");
  popup->insertItem(QIconSet(pixmap), i18n("&Save"),kWrite,SLOT(save()));
  popup->insertItem(i18n("S&ave as..."),kWrite,SLOT(saveAs()));
  popup->insertSeparator();
/*  pixmap = BarIcon("undo");
  popup->insertItem(QIconSet(pixmap), i18n("&Undo"),kWrite,SLOT(undo()));
  pixmap = BarIcon("redo");
  popup->insertItem(QIconSet(pixmap), i18n("R&edo"),kWrite,SLOT(redo()));
  popup->insertSeparator();*/
  pixmap = BarIcon("editcut");
  popup->insertItem(QIconSet(pixmap), i18n("C&ut"),kWrite,SLOT(cut()));
  pixmap = BarIcon("editcopy");
  popup->insertItem(QIconSet(pixmap), i18n("&Copy"),kWrite,SLOT(copy()));
  pixmap = BarIcon("editpaste");
  popup->insertItem(QIconSet(pixmap), i18n("&Paste"),kWrite,SLOT(paste()));
  kWrite->installRBPopup(popup);

  menubar = menuBar();
  menubar->insertItem(i18n("&File"), file);
  menubar->insertItem(i18n("&Edit"), edit);
  menubar->insertItem(i18n("&Search"), find);
  menubar->insertItem(i18n("&Bookmarks"), bookmarks);
  menubar->insertItem(i18n("&Options"), options);
  menubar->insertSeparator();
  menubar->insertItem(i18n("&Help"), help);
}

void TopLevel::setupToolBar(){
  KToolBar *toolbar;

  toolbar = toolBar();//new KToolBar(this);

  toolbar->insertButton("filenew",0,SIGNAL(clicked()),
    kWrite,SLOT(newDoc()),TRUE,i18n("New"));

  toolbar->insertButton("fileopen",0,SIGNAL(clicked()),
    kWrite,SLOT(open()),TRUE,i18n("Open"));

  toolbar->insertButton("filesave",0,SIGNAL(clicked()),
    kWrite,SLOT(save()),TRUE,i18n("Save"));

  toolbar->insertSeparator();

  toolbar->insertButton("editcut",toolCut,SIGNAL(clicked()),
    kWrite,SLOT(cut()),TRUE,i18n("Cut"));

  toolbar->insertButton("editcopy",0,SIGNAL(clicked()),
    kWrite,SLOT(copy()),TRUE,i18n("Copy"));

  toolbar->insertButton("editpaste",toolPaste,SIGNAL(clicked()),
    kWrite,SLOT(paste()),TRUE,i18n("Paste"));

  toolbar->insertSeparator();

  toolbar->insertButton("undo",toolUndo,SIGNAL(clicked()),
    kWrite,SLOT(undo()),TRUE,i18n("Undo"));

  toolbar->insertButton("redo",toolRedo,SIGNAL(clicked()),
    kWrite,SLOT(redo()),TRUE,i18n("Redo"));

  toolbar->insertSeparator();

  toolbar->insertButton("fileprint", 0, SIGNAL(clicked()),
    this, SLOT(printNow()), TRUE, i18n("Print Document"));

/*
  pixmap = BarIcon("send");
  toolbar->insertButton(pixmap, 0,
                      SIGNAL(clicked()), this,
                      SLOT(mail()), TRUE, i18n("Mail Document"));

*/
  toolbar->insertSeparator();
  toolbar->insertButton("help",0,SIGNAL(clicked()),
    this,SLOT(helpSelected()),TRUE,i18n("Help"));

  //  toolbar->setBarPos(KToolBar::Top);
}

void TopLevel::setupStatusBar()
{
    KStatusBar *statusbar;
    statusbar = statusBar();
    statusbar->insertItem(" Line:000000 Col: 000 ", ID_LINE_COLUMN);
    statusbar->insertItem(" XXX ", ID_INS_OVR);
    statusbar->insertFixedItem(" * ", ID_MODIFIED);
    statusbar->insertItem("", ID_GENERAL, 1);
    statusbar->setItemAlignment( ID_GENERAL, AlignLeft );
}


void TopLevel::openRecent(int id) {
  if (kWrite->canDiscard()) kWrite->loadURL(recentPopup->text(id));
}

void TopLevel::newWindow() {

  TopLevel *t = new TopLevel(0L, kWrite->doc()->url().fileName());
  t->readConfig();
  t->init();
//  t->kWrite->doc()->inheritFileName(kWrite->doc());
}

void TopLevel::newView() {

  TopLevel *t = new TopLevel(kWrite->doc());
  t->readConfig();
  t->kWrite->copySettings(kWrite);
  t->init();
}


void TopLevel::closeWindow() {
  close();
}


void TopLevel::quitEditor() {

//  writeConfig();
  kapp->quit();
}

void TopLevel::configure() {
  KWin kwin;
  // I read that no widgets should be created on the stack
  KDialogBase *kd = new KDialogBase(KDialogBase::IconList,
				    i18n("Configure KWrite"),
				    KDialogBase::Ok | KDialogBase::Cancel |
				    KDialogBase::Help ,
				    KDialogBase::Ok, this, "tabdialog");

  // indent options
  QVBox *page=kd->addVBoxPage(i18n("Indent"), QString::null, 
			      BarIcon("rightjust", KIcon::SizeMedium) );
  IndentConfigTab *indentConfig = new IndentConfigTab(page, kWrite);

  // select options
  page=kd->addVBoxPage(i18n("Select"), QString::null, 
		       BarIcon("misc") );
  SelectConfigTab *selectConfig = new SelectConfigTab(page, kWrite);

  // edit options
  page=kd->addVBoxPage(i18n("Edit"), QString::null,
		       BarIcon("kwrite", KIcon::SizeMedium ) );
  EditConfigTab *editConfig = new EditConfigTab(page, kWrite);

  // spell checker
  page = kd->addVBoxPage( i18n("Spelling"), i18n("Spell checker behavior"),
			  BarIcon("spellcheck", KIcon::SizeMedium) );
  KSpellConfig *ksc = new KSpellConfig(page, 0L, kWrite->ksConfig(), false );
  // keys
  //this still lacks layout management, so the tabdialog does not
  //make it fit
//  KGuiCmdConfigTab *keys = new KGuiCmdConfigTab(qtd, &cmdMngr);
//  qtd->addTab(keys, i18n("Keys"));

  // Is there a _right_ way to do this?
  // yes: don´t do it :)
//  qtd->setMinimumSize (ksc.sizeHint().width() + qtd->sizeHint().width(),
//          ksc.sizeHint().height() + qtd->sizeHint().height());
  kwin.setIcons(kd->winId(), kapp->icon(), kapp->miniIcon());

  if (kd->exec()) {
    // indent options
    indentConfig->getData(kWrite);
    // select options
    selectConfig->getData(kWrite);
    // edit options
    editConfig->getData(kWrite);
    // spell checker
    ksc->writeGlobalSettings();
    kWrite->setKSConfig(*ksc);
    // keys
//    cmdMngr.changeAccels();
//    cmdMngr.writeConfig(kapp->config());
//  } else {
//    // cancel keys
//    cmdMngr.restoreAccels();
  }

  delete kd;
}

void TopLevel::keys() {

  KDialogBase *dlg = new KDialogBase(this, "keys", true,
    i18n("Configure Keybindings"), KDialogBase::Ok | KDialogBase::Cancel,
    KDialogBase::Ok);

  // keys
  //this still lacks layout management, so the tabdialog does not
  //make it fit
  KGuiCmdConfigTab *keys = new KGuiCmdConfigTab(dlg, KGuiCmdManager::self());
  keys->resize(450, 290);
  dlg->setMainWidget(keys);
  dlg->resize(450, 315);

  if (dlg->exec()) {
    // change keys
    KGuiCmdManager::self()->changeAccels();
    KGuiCmdManager::self()->writeConfig(kapp->config());
  } else {
    // cancel keys
    KGuiCmdManager::self()->restoreAccels();
  }

  delete dlg;
}



void TopLevel::toggleToolBar() {

  options->setItemChecked(menuShowTB,hideToolBar);
  if (hideToolBar) {
    hideToolBar = FALSE;
    enableToolBar(KToolBar::Show);
  } else {
    hideToolBar = TRUE;
    enableToolBar(KToolBar::Hide);
  }
}
/*
void TopLevel::keyDlg() {
  QDialog *dlg;

//  cmdMngr.saveAccels();
  dlg = new KGuiCmdConfig(&cmdMngr, this);
  dlg->setCaption(i18n("Key Bindings"));
  if (dlg->exec() == QDialog::Accepted) {
    cmdMngr.changeAccels();
    cmdMngr.writeConfig(kapp->config());
  } else cmdMngr.restoreAccels();
  delete dlg;
}
*/
void TopLevel::toggleStatusBar() {

  options->setItemChecked(menuShowSB, hideStatusBar);
  if (hideStatusBar) {
    hideStatusBar = FALSE;
    enableStatusBar(KStatusBar::Show);
  } else {
    hideStatusBar = TRUE;
    enableStatusBar(KStatusBar::Hide);
  }
}

void TopLevel::togglePath() {
  showPath = !showPath;
  options->setItemChecked(menuShowPath, showPath);
  newCaption();
}


void TopLevel::print(bool dialog) {
  QString title = kWrite->doc()->url().fileName();
  if (!showPath) {
    int pos = title.findRev('/');
    if (pos != -1) {
      title = title.right(title.length() - pos - 1);
    }
  }

  KTextPrintConfig::print(this, kapp->config(), dialog, title,
    kWrite->numLines(), this, SLOT(doPrint(KTextPrint &)));
}

void TopLevel::doPrint(KTextPrint &printer) {
  KWriteDoc *doc = kWrite->doc();

  int z, numAttribs;
  Attribute *a;
  int line, attr, nextAttr, oldZ;
  TextLine *textLine;
  const QChar *s;

//  printer.setTitle(kWriteDoc->fileName());
  printer.setTabWidth(doc->tabWidth());

  numAttribs = doc->numAttribs();
  a = doc->attribs();
  for (z = 0; z < numAttribs; z++) {
    printer.defineColor(z, a[z].col.red(), a[z].col.green(), a[z].col.blue());
  }

  printer.begin();

  line = 0;
  attr = -1;
  while (true) {
    textLine = doc->getTextLine(line);
    s = textLine->getText();
//    printer.print(s, textLine->length());
    oldZ = 0;
    for (z = 0; z < textLine->length(); z++) {
      nextAttr = textLine->getAttr(z);
      if (nextAttr != attr) {
        attr = nextAttr;
        printer.print(&s[oldZ], z - oldZ);
        printer.setColor(attr);
        int fontStyle = 0;
        if (a[attr].font.bold()) fontStyle |= KTextPrint::Bold;
        if (a[attr].font.italic()) fontStyle |= KTextPrint::Italics;
        printer.setFontStyle(fontStyle);
        oldZ = z;
      }
    }
    printer.print(&s[oldZ], z - oldZ);

    line++;
    if (line == doc->numLines()) break;
    printer.newLine();
  }

  printer.end();
}

void TopLevel::printNow() {
  print(false);
}

void TopLevel::printDlg() {
  print(true);
}



void TopLevel::helpSelected() {
  kapp->invokeHelp( );
}

void TopLevel::newCurPos() {
  statusBar()->changeItem(i18n(" Line: %1 Col: %2 ")
    .arg(KGlobal::locale()->formatNumber(kWrite->currentLine() + 1, 0))
    .arg(KGlobal::locale()->formatNumber(kWrite->currentColumn() + 1, 0)),
    ID_LINE_COLUMN);
}

void TopLevel::newStatus() {
  int config;
  bool readOnly;

  newCaption();

  readOnly = kWrite->isReadOnly();

  config = kWrite->config();
  options->setItemChecked(menuVertical,config & cfVerticalSelect);

  if (readOnly)
    statusBar()->changeItem(i18n(" R/O "),ID_INS_OVR);
  else
    statusBar()->changeItem(config & cfOvr ? i18n(" OVR ") : i18n(" INS "),ID_INS_OVR);

  statusBar()->changeItem(kWrite->isModified() ? " * " : "",ID_MODIFIED);

  file->setItemEnabled(menuInsert,!readOnly);
  file->setItemEnabled(menuSave,!readOnly);

  edit->setItemEnabled(menuIndent,!readOnly);
  edit->setItemEnabled(menuUnindent,!readOnly);
  edit->setItemEnabled(menuCleanIndent,!readOnly);
  edit->setItemEnabled(menuComment,!readOnly);
  edit->setItemEnabled(menuUncomment,!readOnly);
  edit->setItemEnabled(menuSpell,!readOnly);
  edit->setItemEnabled(menuCut,!readOnly);
  edit->setItemEnabled(menuPaste,!readOnly);
  edit->setItemEnabled(menuReplace,!readOnly);

  toolBar()->setItemEnabled(toolCut,!readOnly);
  toolBar()->setItemEnabled(toolPaste,!readOnly);

  newUndo();
}

void TopLevel::statusMsg(const QString &msg) {
  statusbarTimer->stop();
  statusBar()->changeItem(" " + msg, ID_GENERAL);
  statusbarTimer->start(10000, true); //single shot
}

void TopLevel::timeout() {
  statusBar()->changeItem("", ID_GENERAL);
}

void TopLevel::newCaption()
{
  if (kWrite->doc()->url().fileName().isEmpty()) {
    setCaption(i18n("Untitled"),kWrite->isModified());
  } else {
    //set caption
    if (showPath)
      setCaption(kWrite->doc()->url().prettyURL(),kWrite->isModified());
    else
      setCaption(kWrite->doc()->url().fileName(),kWrite->isModified());

    //set recent files popup menu
    QString url = kWrite->doc()->url().url();
    for ( uint z = 0; z < recentPopup->count(); z++ )
      if (url == recentPopup->text(recentPopup->idAt(z)))
        recentPopup->removeItemAt(z);
    recentPopup->insertItem(url, 0, 0);
    if (recentPopup->count() > 5)
      recentPopup->removeItemAt(5);
    for (uint z = 0; z < 5; z++)
      recentPopup->setId(z, z);
  }
}

void TopLevel::newUndo() {
  int state, uType, rType;
  QString t;

  state = kWrite->undoState();

  edit->setItemEnabled(menuUndoHist,(state & 1 || state & 2));

  t = KGuiCmdManager::self()->getCommand(ctEditCommands, cmUndo)->getName();
  if (state & 1) {
    uType = kWrite->nextUndoType();
    edit->setItemEnabled(menuUndo,true);
    toolBar()->setItemEnabled(toolUndo,true);

    t += ' ';
    t += i18n(kWrite->undoTypeName(uType));
  } else {
    edit->setItemEnabled(menuUndo,false);
    toolBar()->setItemEnabled(toolUndo,false);
  }
  edit->setText(t, menuUndo);

  t = KGuiCmdManager::self()->getCommand(ctEditCommands, cmRedo)->getName();
  if (state & 2) {
    rType = kWrite->nextRedoType();
    edit->setItemEnabled(menuRedo,true);
    toolBar()->setItemEnabled(toolRedo,true);

    t += ' ';
    t += i18n(kWrite->undoTypeName(rType));
  } else {
    edit->setItemEnabled(menuRedo,false);
    toolBar()->setItemEnabled(toolRedo,false);
  }
  edit->setText(t, menuRedo);
}

void TopLevel::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept(QUriDrag::canDecode(event));
}

void TopLevel::dropEvent( QDropEvent *event )
{
  slotDropEvent(event);
}

void TopLevel::slotDropEvent( QDropEvent *event )
{
  QStrList  urls;

  if (QUriDrag::decode(event, urls)) {
    kdDebug() << "TopLevel:Handling QUriDrag..." << endl;
    char *s;
    for (s = urls.first(); s != 0L; s = urls.next()) {
      // Load the first file in this window
      if (s == urls.getFirst() && !kWrite->isModified() && !kWrite->isReadOnly()) {
        loadURL(s);
      } else {
        TopLevel *t = new TopLevel();
        t->readConfig();
        t->loadURL(s);
        t->init();
      }
    }
  }
}

void TopLevel::showHighlight() {
  int hl = kWrite->getHl();

  for (int index = 0; index < (int) hlPopup->count(); index++)
    hlPopup->setItemChecked(index, hl == index);
}

void TopLevel::showEol() {
  int eol = kWrite->getEol();

  for (int index = 0; index < (int) eolPopup->count(); index++)
    eolPopup->setItemChecked(index, eol == index);
}

//common config
void TopLevel::readConfig(KConfig *config) {
  int z;
  char name[16];
  QString s;

  hideToolBar = config->readNumEntry("HideToolBar");
  hideStatusBar = config->readNumEntry("HideStatusBar");
  showPath = config->readNumEntry("ShowPath");

  for (z = 0; z < 5; z++) {
    sprintf(name, "Recent%d", z + 1);
    s = config->readEntry(name);
    if (!s.isEmpty()) recentPopup->insertItem(s);
  }
}

void TopLevel::writeConfig(KConfig *config) {
  int z;
  char name[16];

  config->writeEntry("HideToolBar",hideToolBar);
  config->writeEntry("HideStatusBar",hideStatusBar);
  config->writeEntry("ShowPath",showPath);

  for (z = 0; z < (int) recentPopup->count(); z++) {
    sprintf(name, "Recent%d", z + 1);
    config->writeEntry(name, recentPopup->text(recentPopup->idAt(z)));
  }
}

//config file
void TopLevel::readConfig() {
  KConfig *config;
  int w, h;

  config = kapp->config();

  config->setGroup("General Options");
  w = config->readNumEntry("Width", 550);
  h = config->readNumEntry("Height", 400);
  resize(w, h);

  readConfig(config);
//  hideToolBar = config->readNumEntry("HideToolBar");
//  hideStatusBar = config->readNumEntry("HideStatusBar");

  kWrite->readConfig(config);
  kWrite->doc()->readConfig(config);
}

void TopLevel::writeConfig() {
  KConfig *config;

  config = kapp->config();

  config->setGroup("General Options");
  config->writeEntry("Width", width());
  config->writeEntry("Height", height());

  writeConfig(config);
//  config->writeEntry("HideToolBar",hideToolBar);
//  config->writeEntry("HideStatusBar",hideStatusBar);

  kWrite->writeConfig(config);
  kWrite->doc()->writeConfig(config);
}

// session management
void TopLevel::restore(KConfig *config, int n) {

  if (kWrite->isLastView() && kWrite->doc()->url().fileName().isEmpty()) { //in this case first view
    loadURL(kWrite->doc()->url().fileName(), lfNoAutoHl);
  }
  readPropertiesInternal(config, n);
  init();
//  show();
}

void TopLevel::readProperties(KConfig *config) {

  readConfig(config);
  kWrite->readSessionConfig(config);
}

void TopLevel::saveProperties(KConfig *config) {

  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find(kWrite->doc()) + 1);
  kWrite->writeSessionConfig(config);
#warning fix session management
#if 0
  setUnsavedData(kWrite->isModified());
#endif
}

void TopLevel::saveData(KConfig *config) { //save documents
  int z;
  char buf[16];
  KWriteDoc *doc;

  config->setGroup("Number");
  config->writeEntry("NumberOfDocuments",docList.count());

  for (z = 1; z <= (int) docList.count(); z++) {
     sprintf(buf,"Document%d",z);
     config->setGroup(buf);
     doc = docList.at(z - 1);
     doc->writeSessionConfig(config);
  }
}




//restore session
void restore() {
  KConfig *config;
  int docs, windows, z;
  char buf[16];
  KWriteDoc *doc;
  TopLevel *t;

  config = kapp->sessionConfig();
  if (!config) return;

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (z = 1; z <= docs; z++) {
     sprintf(buf,"Document%d",z);
     config->setGroup(buf);
     doc = new KWriteDoc(HlManager::self());
     doc->readSessionConfig(config);
     docList.append(doc);
  }

  for (z = 1; z <= windows; z++) {
    sprintf(buf,"%d",z);
    config->setGroup(buf);
    t = new TopLevel(docList.at(config->readNumEntry("DocumentNumber") - 1));
    t->restore(config,z);
  }
}

static KCmdLineOptions options[] =
{
  { "+[URL]",   I18N_NOOP("Document to open."), 0 },
  { 0, 0, 0}
};

int main(int argc, char **argv)
{
  KCmdLineArgs::init( argc, argv, KWriteFactory::aboutData() );
  KCmdLineArgs::addCmdLineOptions( options );

  KGuiCmdApp *a = new KGuiCmdApp;
  //KApplication a(argc,argv);

  //list that contains all documents
  docList.setAutoDelete(false);

  //init commands
  KWrite::addCursorCommands(*KGuiCmdManager::self());

  KGuiCmdManager::self()->addCategory(ctFileCommands, I18N_NOOP("File Commands"));
  KGuiCmdManager::self()->addCommand(cmNew,             I18N_NOOP("&New..."    ));
  KGuiCmdManager::self()->addCommand(cmOpen,            I18N_NOOP("&Open..."   ), Qt::CTRL+Qt::Key_O, Qt::Key_F17);
  KGuiCmdManager::self()->addCommand(cmInsert,          I18N_NOOP("&Insert..." ));
  KGuiCmdManager::self()->addCommand(cmSave,            I18N_NOOP("&Save"      ), Qt::CTRL+Qt::Key_S);
  KGuiCmdManager::self()->addCommand(cmSaveAs,          I18N_NOOP("Save &As..."));
  KGuiCmdManager::self()->addCommand(cmPrint,           I18N_NOOP("&Print..."  ), Qt::CTRL+Qt::Key_P);
  KGuiCmdManager::self()->addCommand(cmNewWindow,       I18N_NOOP("New &Window"));
  KGuiCmdManager::self()->addCommand(cmNewView,         I18N_NOOP("New &View"  ));
  KGuiCmdManager::self()->addCommand(cmClose,           I18N_NOOP("&Close"     ), Qt::CTRL+Qt::Key_W, Qt::Key_Escape);
//  cmdMngr.addCommand(cmClose,           "&Quit"      );

  KWrite::addEditCommands(*KGuiCmdManager::self());
  KWrite::addFindCommands(*KGuiCmdManager::self());
  KWrite::addBookmarkCommands(*KGuiCmdManager::self());
  KWrite::addStateCommands(*KGuiCmdManager::self());


  //todo: insert reading of kde-global keybinding file here
  KGuiCmdManager::self()->makeDefault(); //make keybindings default
  KGuiCmdManager::self()->readConfig(kapp->config());

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (kapp->isRestored()) {
    restore();
  } else {
    TopLevel *t = new TopLevel();
    t->readConfig();
    if ( args->count() == 1 ) t->loadURL( args->url(0) ,lfNewFile);
    t->init();
  }
  int r = a->exec();

  args->clear();

  return r;
}
