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

#include <qdropsite.h>
#include <qdragobject.h>
#include <qvbox.h>

#include <kiconloader.h>
#include <kaboutdata.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kapp.h>
#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kwin.h>
#include <kcmdlineargs.h>
#include <kdialogbase.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kdebug.h>

#include "ktextprint.h"
#include "kwdoc.h"
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


QList<KWriteDoc> docList; //documents


TopLevel::TopLevel (KWriteDoc *doc, const QString &path)
  : KMainWindow(0)
{
  setMinimumSize(180,120);

  statusbarTimer = new QTimer(this);
  connect(statusbarTimer,SIGNAL(timeout()),this,SLOT(timeout()));

  if (!doc) {
    doc = new KWriteDoc(HlManager::self(), path); //new doc with default path
    docList.append(doc);
  }
  setupEditWidget(doc);
  setupActions();
  setupStatusBar();

  setAcceptDrops(true);
}


TopLevel::~TopLevel()
{
  if (kWrite->isLastView()) docList.remove(kWrite->doc());
}


void TopLevel::init()
{
  setHighlight->setCurrentItem(kWrite->getHl());
  setShowPath->setChecked(showPath);
  newCurPos();
  newStatus();

  show();
}


void TopLevel::loadURL(const KURL &url, int flags)
{
  kWrite->loadURL(url,flags);
}


bool TopLevel::queryClose()
{
  if (!kWrite->isLastView()) return true;
  return kWrite->canDiscard();
//  writeConfig();
}


bool TopLevel::queryExit()
{
  writeConfig();
  kapp->config()->sync();

  return true;
}


void TopLevel::setupEditWidget(KWriteDoc *doc)
{
  kWrite = new KWrite(doc, this, 0, false);

  connect(kWrite,SIGNAL(newCurPos()),this,SLOT(newCurPos()));
  connect(kWrite,SIGNAL(newStatus()),this,SLOT(newStatus()));
  connect(kWrite,SIGNAL(statusMsg(const QString &)),this,SLOT(statusMsg(const QString &)));
  connect(kWrite,SIGNAL(fileChanged()),this,SLOT(newCaption()));
  connect(kWrite,SIGNAL(newUndo()),this,SLOT(newUndo()));
  connect(kWrite->view(),SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(slotDropEvent(QDropEvent *)));
  connect(kWrite->doc(),SIGNAL(highlightChanged()),this,SLOT(slotHighlightChanged()));
  connect(kWrite, SIGNAL( enableUI( bool ) ), this, SLOT( slotEnableActions( bool ) ) );

  setCentralWidget(kWrite);
}


void TopLevel::setupActions()
{
  // setup File menu
  KStdAction::openNew(kWrite, SLOT(newDoc()), actionCollection());
  KStdAction::open(kWrite, SLOT(open()), actionCollection());
  fileRecent = KStdAction::openRecent(this, SLOT(openRecent(const KURL&)),
                                      actionCollection());
  fileSave = KStdAction::save(kWrite, SLOT(save()), actionCollection());
  KStdAction::saveAs(kWrite, SLOT(saveAs()), actionCollection());
  KStdAction::print(this, SLOT(printDlg()), actionCollection());
  new KAction(i18n("New &Window"), 0, this, SLOT(newWindow()),
              actionCollection(), "file_newWindow");
  new KAction(i18n("New &View"), 0, this, SLOT(newView()),
              actionCollection(), "file_newView");
  KStdAction::close(this, SLOT(close()), actionCollection());

  // setup edit menu
  editUndo = KStdAction::undo(kWrite, SLOT(undo()), actionCollection());
  editRedo = KStdAction::redo(kWrite, SLOT(redo()), actionCollection());
  editUndoHist = new KAction(i18n("Undo/Redo &History..."), 0, kWrite, SLOT(undoHistory()),
                             actionCollection(), "edit_undoHistory");
  editCut = KStdAction::cut(kWrite, SLOT(cut()), actionCollection());
  editPaste = KStdAction::copy(kWrite, SLOT(copy()), actionCollection());
  editReplace = KStdAction::paste(kWrite, SLOT(paste()), actionCollection());
  KStdAction::selectAll(kWrite, SLOT(selectAll()), actionCollection());
  new KAction(i18n("&Deselect All"), 0, kWrite, SLOT(deselectAll()),
              actionCollection(), "edit_deselectAll");
  new KAction(i18n("Invert &Selection"), 0, kWrite, SLOT(invertSelection()),
              actionCollection(), "edit_invertSelection");
  KStdAction::find(kWrite, SLOT(find()), actionCollection());
  KStdAction::findNext(kWrite, SLOT(findAgain()), actionCollection());
  KStdAction::replace(kWrite, SLOT(replace()), actionCollection());
  editInsert = new KAction(i18n("&Insert File..."), 0, kWrite, SLOT(insertFile()),
                           actionCollection(), "edit_insertFile");

  // setup Go menu
  KStdAction::gotoLine(kWrite, SLOT(gotoLine()), actionCollection());
  new KAction(i18n("&Add Marker..."), Qt::CTRL+Qt::Key_M, kWrite, SLOT(addBookmark()),
              actionCollection(), "go_addMarker");
  new KAction(i18n("&Set Marker..."), 0, kWrite, SLOT(setBookmark()),
              actionCollection(), "go_setMarker");
  new KAction(i18n("&Clear Markers..."), 0, kWrite, SLOT(clearBookmarks()),
              actionCollection(), "go_clearMarkers");

  // setup Tools menu
  toolsSpell = KStdAction::spelling(kWrite, SLOT(spellcheck()), actionCollection());
  toolsIndent = new KAction(i18n("&Intend"), Qt::CTRL+Qt::Key_I, kWrite, SLOT(indent()),
                            actionCollection(), "tools_indent");
  toolsUnindent = new KAction(i18n("&Unintend"), Qt::CTRL+Qt::Key_U, kWrite, SLOT(unIndent()),
                              actionCollection(), "tools_unindent");
  toolsCleanIndent = new KAction(i18n("&Clean Indentation"), 0, kWrite, SLOT(cleanIndent()),
                                 actionCollection(), "tools_cleanIndent");
  toolsComment = new KAction(i18n("C&omment"), 0, kWrite, SLOT(comment()),
                             actionCollection(), "tools_comment");
  toolsUncomment = new KAction(i18n("Unco&mment"), 0, kWrite, SLOT(uncomment()),
                               actionCollection(), "tools_uncomment");

  // setup Settings menu
  KStdAction::showToolbar(this, SLOT(toggleToolbar()), actionCollection());
  KStdAction::showStatusbar(this, SLOT(toggleStatusbar()), actionCollection());
  setShowPath = new KToggleAction(i18n("Sho&w Path"), 0, this, SLOT(togglePath()),
                    actionCollection(), "set_showPath");
  KStdAction::keyBindings(this, SLOT(editKeys()), actionCollection());
  KStdAction::configureToolbars(this, SLOT(editToolbars()), actionCollection());
  KStdAction::preferences(this, SLOT(configure()), actionCollection());
  setVerticalSelection = new KToggleAction(i18n("&Vertical Selection"), 0, kWrite, SLOT(toggleVertical()),
                             actionCollection(), "set_verticalSelect");

  setHighlight = new KSelectAction(i18n("&Highlight Mode"), 0, actionCollection(), "set_highlight");
  connect(setHighlight, SIGNAL(activated(int)), kWrite, SLOT(setHl(int)));
  QStringList list;
  for (int z = 0; z < HlManager::self()->highlights(); z++)
    list.append(i18n(HlManager::self()->hlName(z)));
  setHighlight->setItems(list);

  setEndOfLine = new KSelectAction(i18n("&End Of Line"), 0, actionCollection(), "set_eol");
  connect(setEndOfLine, SIGNAL(activated(int)), kWrite, SLOT(setEol(int)));
  list.clear();
  list.append("&Unix");
  list.append("&Macintosh");
  list.append("&Windows/Dos");
  setEndOfLine->setItems(list);


  // keyboard actions...
  KAccel *acc=new KAccel(this);
  KAction* act;
  act = new KAction(i18n("Left"), Qt::Key_Left, kWrite, SLOT(cursorLeft()), actionCollection(), "cursor_left");
  act->plugAccel(acc);
  act = new KAction(i18n("Left + Select"), Qt::SHIFT+Qt::Key_Left, kWrite, SLOT(shiftCursorLeft()), actionCollection(), "cursor_left_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Right"), Qt::Key_Right, kWrite, SLOT(cursorRight()), actionCollection(), "cursor_right");
  act->plugAccel(acc);
  act = new KAction(i18n("Right + Select"), Qt::SHIFT+Qt::Key_Right, kWrite, SLOT(shiftCursorRight()), actionCollection(), "cursor_right_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Word Left"), Qt::CTRL+Qt::Key_Left, kWrite, SLOT(wordLeft()), actionCollection(), "word_left");
  act->plugAccel(acc);
  act = new KAction(i18n("Word Left + Select"), Qt::SHIFT+Qt::CTRL+Qt::Key_Left, kWrite, SLOT(shiftWordLeft()), actionCollection(), "word_left_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Word Right"), Qt::CTRL+Qt::Key_Right, kWrite, SLOT(wordRight()), actionCollection(), "word_right");
  act->plugAccel(acc);
  act = new KAction(i18n("Word Right + Select"), Qt::SHIFT+Qt::CTRL+Qt::Key_Right, kWrite, SLOT(shiftWordRight()), actionCollection(), "word_right_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Home"), Qt::Key_Home, kWrite, SLOT(home()), actionCollection(), "cursor_home");
  act->plugAccel(acc);
  act = new KAction(i18n("Home + Select"), Qt::SHIFT+Qt::Key_Home, kWrite, SLOT(shiftHome()), actionCollection(), "cursor_home_select");
  act->plugAccel(acc);
  act = new KAction(i18n("End"), Qt::Key_End, kWrite, SLOT(end()), actionCollection(), "cursor_end");
  act->plugAccel(acc);
  act = new KAction(i18n("End + Select"), Qt::SHIFT+Qt::Key_End, kWrite, SLOT(shiftEnd()), actionCollection(), "cursor_end_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Up"), Qt::Key_Up, kWrite, SLOT(up()), actionCollection(), "cursor_up");
  act->plugAccel(acc);
  act = new KAction(i18n("Up + Select"), Qt::SHIFT+Qt::Key_Up, kWrite, SLOT(shiftUp()), actionCollection(), "cursor_up_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Down"), Qt::Key_Down, kWrite, SLOT(down()), actionCollection(), "cursor_down");
  act->plugAccel(acc);
  act = new KAction(i18n("Down + Select"), Qt::SHIFT+Qt::Key_Down, kWrite, SLOT(shiftDown()), actionCollection(), "cursor_down_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Scroll Up"), Qt::CTRL+Qt::Key_Up, kWrite, SLOT(scrollUp()), actionCollection(), "scroll_up");
  act->plugAccel(acc);
  act = new KAction(i18n("Scroll Down"), Qt::CTRL+Qt::Key_Down, kWrite, SLOT(scrollDown()), actionCollection(), "scroll_down");
  act->plugAccel(acc);
  act = new KAction(i18n("Top Of View"), Qt::CTRL+Qt::Key_PageUp, kWrite, SLOT(topOfView()), actionCollection(), "top_of_view");
  act->plugAccel(acc);
  act = new KAction(i18n("Bottom Of View"), Qt::CTRL+Qt::Key_PageDown, kWrite, SLOT(bottomOfView()), actionCollection(), "bottom_of_view");
  act->plugAccel(acc);
  act = new KAction(i18n("Page Up"), Qt::Key_PageUp, kWrite, SLOT(pageUp()), actionCollection(), "page_up");
  act->plugAccel(acc);
  act = new KAction(i18n("Page Up + Select"), Qt::SHIFT+Qt::Key_PageUp, kWrite, SLOT(shiftPageUp()), actionCollection(), "page_up_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Page Down"), Qt::Key_PageDown, kWrite, SLOT(pageDown()), actionCollection(), "page_down");
  act->plugAccel(acc);
  act = new KAction(i18n("Page Down + Select"), Qt::SHIFT+Qt::Key_PageDown, kWrite, SLOT(shiftPageDown()), actionCollection(), "page_down_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Top"), Qt::CTRL+Qt::Key_Home, kWrite, SLOT(top()), actionCollection(), "top");
  act->plugAccel(acc);
  act = new KAction(i18n("Top + Select"), Qt::SHIFT+Qt::CTRL+Qt::Key_Home, kWrite, SLOT(shiftTop()), actionCollection(), "top_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Bottom"), Qt::CTRL+Qt::Key_End, kWrite, SLOT(bottom()), actionCollection(), "bottom");
  act->plugAccel(acc);
  act = new KAction(i18n("Bottom + Select"), Qt::SHIFT+Qt::CTRL+Qt::Key_End, kWrite, SLOT(shiftBottom()), actionCollection(), "bottom_select");
  act->plugAccel(acc);
  act = new KAction(i18n("Return"), Qt::Key_Return, kWrite, SLOT(keyReturn()), actionCollection(), "return");
  act->plugAccel(acc);
  // ugly trick, KAction only supports one keybinding
  act = new KAction(i18n("New Line"), Qt::Key_Enter, kWrite, SLOT(keyReturn()), actionCollection(), "new_line");
  act->plugAccel(acc);
  act = new KAction(i18n("Delete"), Qt::Key_Delete, kWrite, SLOT(keyDelete()), actionCollection(), "delete");
  act->plugAccel(acc);
  act = new KAction(i18n("Backspace"), Qt::Key_Backspace, kWrite, SLOT(backspace()), actionCollection(), "backspace");
  act->plugAccel(acc);
  act = new KAction(i18n("Kill Line"), Qt::CTRL+Qt::Key_K, kWrite, SLOT(killLine()), actionCollection(), "kill_line");
  act->plugAccel(acc);
  act = new KAction(i18n("Insert Mode"), Qt::Key_Insert, kWrite, SLOT(toggleInsert()), actionCollection(), "insert_mode");
  act->plugAccel(acc);

  createGUI();

  kWrite->installRBPopup(static_cast<QPopupMenu *>(factory()->container("rb_popup", this)));
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


void TopLevel::openRecent(const KURL& url)
{
  if (kWrite->canDiscard())
    kWrite->loadURL(url);
}


void TopLevel::newWindow()
{
  TopLevel *t = new TopLevel(0L, kWrite->doc()->url().fileName());
  t->readConfig();
  t->init();
//  t->kWrite->doc()->inheritFileName(kWrite->doc());
}


void TopLevel::newView()
{
  TopLevel *t = new TopLevel(kWrite->doc());
  t->readConfig();
  t->kWrite->copySettings(kWrite);
  t->init();
}

void TopLevel::configure()
{
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
  }

  delete kd;
}


void TopLevel::togglePath()
{
  showPath = !showPath;
  newCaption();
}


void TopLevel::toggleToolbar()
{
  QToolBar *bar = toolBar("mainToolBar");
  if(bar->isVisible())
    bar->hide();
  else
    bar->show();
}


void TopLevel::toggleStatusbar()
{
  if (statusBar()->isVisible())
    statusBar()->hide();
  else
    statusBar()->show();
}


void TopLevel::editKeys()
{
  KKeyDialog::configureKeys(actionCollection(), xmlFile());
}


void TopLevel::editToolbars()
{
  KEditToolbar *dlg = new KEditToolbar(actionCollection());

  if (dlg->exec())
    createGUI();

  kWrite->installRBPopup(static_cast<QPopupMenu *>(factory()->container("rb_popup", this)));

  delete dlg;
}


void TopLevel::print(bool dialog)
{
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


void TopLevel::doPrint(KTextPrint &printer)
{
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


void TopLevel::printNow()
{
  print(false);
}


void TopLevel::printDlg()
{
  print(true);
}


void TopLevel::newCurPos()
{
  statusBar()->changeItem(i18n(" Line: %1 Col: %2 ")
    .arg(KGlobal::locale()->formatNumber(kWrite->currentLine() + 1, 0))
    .arg(KGlobal::locale()->formatNumber(kWrite->currentColumn() + 1, 0)),
    ID_LINE_COLUMN);
}


void TopLevel::newStatus()
{
  newCaption();

  bool readOnly = kWrite->isReadOnly();
  int config = kWrite->config();

  setVerticalSelection->setChecked(config & cfVerticalSelect);

  if (readOnly)
    statusBar()->changeItem(i18n(" R/O "),ID_INS_OVR);
  else
    statusBar()->changeItem(config & cfOvr ? i18n(" OVR ") : i18n(" INS "),ID_INS_OVR);

  statusBar()->changeItem(kWrite->isModified() ? " * " : "",ID_MODIFIED);

  fileSave->setEnabled(!readOnly);
  editInsert->setEnabled(!readOnly);
  editCut->setEnabled(!readOnly);
  editPaste->setEnabled(!readOnly);
  editReplace->setEnabled(!readOnly);
  toolsIndent->setEnabled(!readOnly);
  toolsUnindent->setEnabled(!readOnly);
  toolsCleanIndent->setEnabled(!readOnly);
  toolsComment->setEnabled(!readOnly);
  toolsUncomment->setEnabled(!readOnly);
  toolsSpell->setEnabled(!readOnly);

  newUndo();
}


void TopLevel::statusMsg(const QString &msg)
{
  statusbarTimer->stop();
  statusBar()->changeItem(" " + msg, ID_GENERAL);
  statusbarTimer->start(10000, true); //single shot
}


void TopLevel::timeout() {
  statusBar()->changeItem("", ID_GENERAL);
}


void TopLevel::newCaption()
{
  setEndOfLine->setCurrentItem(kWrite->getEol());

  if (kWrite->doc()->url().isEmpty()) {
    setCaption(i18n("Untitled"),kWrite->isModified());
  } else {
    //set caption
    if (showPath)
      setCaption(kWrite->doc()->url().prettyURL(),kWrite->isModified());
    else
      setCaption(kWrite->doc()->url().fileName(),kWrite->isModified());

    //set recent files popup menu
    fileRecent->addURL(kWrite->doc()->url());
  }
}


void TopLevel::newUndo()
{
  int state = kWrite->undoState();

  editUndoHist->setEnabled(state & 1 || state & 2);

  QString t = i18n("Und&o");   // it would be nicer to fetch the original string
  if (state & 1) {
    editUndo->setEnabled(true);
    t += ' ';
    t += i18n(kWrite->undoTypeName(kWrite->nextUndoType()));
  } else {
    editUndo->setEnabled(false);
  }
  // icons disappear when text is altered, seems to be a bug in KToolbarButton
  //editUndo->setText(t);

  t = i18n("Re&do");   // it would be nicer to fetch the original string
  if (state & 2) {
    editRedo->setEnabled(true);
    t += ' ';
    t += i18n(kWrite->undoTypeName(kWrite->nextRedoType()));
  } else {
    editRedo->setEnabled(false);
  }
  // icons disappear when text is altered, seems to be a bug in KToolbarButton
  //editRedo->setText(t);
}


void TopLevel::slotHighlightChanged()
{
  setHighlight->setCurrentItem(kWrite->getHl());
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

void TopLevel::slotEnableActions( bool enable )
{
    QValueList<KAction *> actions = actionCollection()->actions();
    QValueList<KAction *>::ConstIterator it = actions.begin();
    QValueList<KAction *>::ConstIterator end = actions.end();
    for (; it != end; ++it )
        (*it)->setEnabled( enable );
}

//common config
void TopLevel::readConfig(KConfig *config)
{
  showPath = config->readBoolEntry("ShowPath");
  fileRecent->loadEntries(config);
}


void TopLevel::writeConfig(KConfig *config)
{
  config->writeEntry("ShowPath",showPath);

  fileRecent->saveEntries(config);
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

  kWrite->readConfig(config);
  kWrite->doc()->readConfig(config);
}


void TopLevel::writeConfig()
{
  KConfig *config;

  config = kapp->config();

  config->setGroup("General Options");
  config->writeEntry("Width", width());
  config->writeEntry("Height", height());

  writeConfig(config);

  kWrite->writeConfig(config);
  kWrite->doc()->writeConfig(config);
}

// session management
void TopLevel::restore(KConfig *config, int n)
{
  if (kWrite->isLastView() && !kWrite->doc()->url().isEmpty()) { //in this case first view
    loadURL(kWrite->doc()->url(), lfNewFile );
  }
  readPropertiesInternal(config, n);
  init();
//  show();
}


void TopLevel::readProperties(KConfig *config)
{
  readConfig(config);
  kWrite->readSessionConfig(config);
}


void TopLevel::saveProperties(KConfig *config)
{
  writeConfig(config);
  config->writeEntry("DocumentNumber",docList.find(kWrite->doc()) + 1);
  kWrite->writeSessionConfig(config);
}


void TopLevel::saveGlobalProperties(KConfig *config) //save documents
{
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
void restore()
{
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
  KAboutData aboutData( "kwrite", I18N_NOOP( "KWrite" ),
                       KWRITE_VERSION,
                       I18N_NOOP( "Advanced Texteditor" ),
                       KAboutData::License_GPL,
                       "(c) 2000, Jochen Wilhelmy" );
  aboutData.addAuthor( "Jochen Wilhemly", I18N_NOOP( "Author" ), "digisnap@cs.tu-berlin.de" );
  aboutData.addAuthor("Michael Koch",I18N_NOOP("Port to KParts"), "koch@kde.org");
  aboutData.addAuthor("Glen Parker",I18N_NOOP("Undo History, Kspell integration"), "glenebob@nwlink.com");

  KCmdLineArgs::init( argc, argv, &aboutData );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication *a = new KApplication();

  //list that contains all documents
  docList.setAutoDelete(false);

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if (kapp->isRestored()) {
    restore();
  } else {
    TopLevel *t = new TopLevel();
    t->readConfig();
    if ( args->count() == 1 )
      t->loadURL(args->url(0), lfNewFile);
    t->init();
  }
  int r = a->exec();

  args->clear();

  return r;
}
