/* This file is part of the KDE libraries
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateview.h"
#include "kateviewinternal.h"
#include "kateview.moc"
#include "katedocument.h"
#include "katecmd.h"
#include "katefactory.h"
#include "katehighlight.h"
#include "kateviewdialog.h"
#include "katedialogs.h"
#include "katefiledialog.h"
#include "katecodefoldinghelpers.h"

#include <kurldrag.h>
#include <qfocusdata.h>
#include <kdebug.h>
#include <kapp.h>
#include <kiconloader.h>
#include <qscrollbar.h>
#include <qiodevice.h>
#include <qclipboard.h>
#include <qpopupmenu.h>
#include <kpopupmenu.h>
#include <qkeycode.h>
#include <qintdict.h>
#include <klineeditdlg.h>
#include <qdropsite.h>
#include <qdragobject.h>
#include <kconfig.h>
#include <ksconfig.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qevent.h>
#include <qdir.h>
#include <qvbox.h>
#include <qpaintdevicemetrics.h>

#include <qstyle.h>
#include <kcursor.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kparts/event.h>
#include <kxmlguifactory.h>
#include <dcopclient.h>
#include <qregexp.h>

#include <kaccel.h>

#include "katetextline.h"
#include "kateiconborder.h"
#include "kateexportaction.h"

KateView::KateView(KateDocument *doc, QWidget *parent, const char * name) : Kate::View (doc, parent, name)
    , extension( 0 )
{
  m_editAccels=0;
  setInstance( KateFactory::instance() );

  initCodeCompletionImplementation();

  active = false;
  //myIconBorder = false;
//  iconBorderStatus = KateIconBorder::None;
   iconBorderStatus = KateIconBorder::FoldingMarkers;
  _hasWrap = false;

  myDoc = doc;
  myViewInternal = new KateViewInternal (this,doc);
  myViewInternal->leftBorder = new KateIconBorder(this, myViewInternal);
  myViewInternal->leftBorder->setGeometry(0, 0, myViewInternal->leftBorder->width(), myViewInternal->iconBorderHeight);
  myViewInternal->leftBorder->hide();
  myViewInternal->leftBorder->installEventFilter( this );
  connect(myViewInternal->leftBorder,SIGNAL(toggleRegionVisibility(unsigned int)),
                                            doc->regionTree,SLOT(toggleRegionVisibility(unsigned int)));
  connect(doc->regionTree,SIGNAL(regionVisibilityChangedAt(unsigned int)),
					this,SLOT(slotRegionVisibilityChangedAt(unsigned int)));
//  connect(doc->regionTree,SIGNAL(regionBeginEndAddedRemoved(unsigned int)),
//					this,SLOT(slotRegionBeginEndAddedRemoved(unsigned int)));
    connect(doc,SIGNAL(codeFoldingUpdated()),this,SLOT(slotCodeFoldingChanged()));
  doc->addView( this );

  connect(myViewInternal,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(dropEventPassEmited(QDropEvent *)));

  replacePrompt = 0L;
  rmbMenu = 0L;

  setFocusProxy( myViewInternal );
  myViewInternal->setFocus();
  resize(parent->width() -4, parent->height() -4);

  myViewInternal->installEventFilter( this );

  if (!doc->m_bSingleViewMode)
  {
    setXMLFile( "katepartui.rc" );
  }
  else
  {
    if (doc->m_bReadOnly)
      myDoc->setXMLFile( "katepartreadonlyui.rc" );
    else
      myDoc->setXMLFile( "katepartui.rc" );

    if (doc->m_bBrowserView)
    {
      extension = new KateBrowserExtension( myDoc, this );
    }
  }

  setupActions();
  setupEditKeys();

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( doc, SIGNAL( undoChanged() ), this, SLOT( slotNewUndo() ) );
  connect( doc, SIGNAL( fileNameChanged() ), this, SLOT( slotFileStatusChanged() ) );

  if ( doc->m_bBrowserView )
  {
    connect( this, SIGNAL( dropEventPass(QDropEvent*) ), this, SLOT( slotDropEventPass(QDropEvent*) ) );
  }

  slotUpdate();

  KAccel *m_debugAccels=new KAccel(this,this);
  m_debugAccels->insert("KATE_DUMP_REGION_TREE",i18n("Show the code folding region tree"),"","Ctrl+Shift+Alt+D",myDoc,SLOT(dumpRegionTree()));
  m_debugAccels->setEnabled(true);
  myViewInternal->updateView (KateView::ufDocGeometry);
}


KateView::~KateView()
{
  if (myDoc && !myDoc->m_bSingleViewMode)
    myDoc->removeView( this );

  delete myViewInternal;
  delete myCC_impl;
}

void KateView::slotRegionVisibilityChangedAt(unsigned int)
{
	kdDebug()<<"void KateView::slotRegionVisibilityChangedAt(unsigned int)"<<endl;
	myViewInternal->updateView(KateView::ufFoldingChanged);
}

void KateView::slotCodeFoldingChanged()
{
	myViewInternal->leftBorder->repaint();

}

void KateView::slotRegionBeginEndAddedRemoved(unsigned int line)
{
	kdDebug()<<"void KateView::slotRegionBeginEndAddedRemoved(unsigned int)"<<endl;
//	myViewInternal->repaint();
	if (myDoc->getVirtualLine(line)<=myViewInternal->endLine) // FIXME: performance problem
	myViewInternal->leftBorder->repaint();

}


void KateView::initCodeCompletionImplementation()
{
  myCC_impl=new CodeCompletion_Impl(this);
  connect(myCC_impl,SIGNAL(completionAborted()),this,SIGNAL(completionAborted()));
  connect(myCC_impl,SIGNAL(completionDone()),this,SIGNAL(completionDone()));
  connect(myCC_impl,SIGNAL(argHintHidden()),this,SIGNAL(argHintHidden()));
  connect(myCC_impl,SIGNAL(completionDone(KTextEditor::CompletionEntry)),this,SIGNAL(completionDone(KTextEditor::CompletionEntry)));
  connect(myCC_impl,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)),this,SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString *)));
}

QPoint KateView::cursorCoordinates()
{
  return myViewInternal->cursorCoordinates();
}

void KateView::copy () const
{
  myDoc->copy(myDoc->_configFlags);
}

void KateView::setupEditKeys()
{

  if (m_editAccels) delete m_editAccels;
  m_editAccels=new KAccel(this,this);
  m_editAccels->insert("KATE_CURSOR_LEFT",i18n("Cursor left"),"",Key_Left,this,SLOT(cursorLeft()));
  m_editAccels->insert("KATE_WORD_LEFT",i18n("One word left"),"",CTRL+Key_Left,this,SLOT(wordLeft()));
  m_editAccels->insert("KATE_CURSOR_LEFT_SELECT",i18n("Cursor left + SELECT"),"",SHIFT+Key_Left,this,SLOT(shiftCursorLeft()));
  m_editAccels->insert("KATE_WORD_LEFT_SELECT",i18n("One word left + SELECT"),"",SHIFT+CTRL+Key_Left,this,SLOT(shiftWordLeft()));


  m_editAccels->insert("KATE_CURSOR_RIGHT",i18n("Cursor right"),"",Key_Right,this,SLOT(cursorRight()));
  m_editAccels->insert("KATE_WORD_RIGHT",i18n("One word right"),"",CTRL+Key_Right,this,SLOT(wordRight()));
  m_editAccels->insert("KATE_CURSOR_RIGHT_SELECT",i18n("Cursor right + SELECT"),"",SHIFT+Key_Right,this,SLOT(shiftCursorRight()));
  m_editAccels->insert("KATE_WORD_RIGHT_SELECT",i18n("One word right + SELECT"),"",SHIFT+CTRL+Key_Right,this,SLOT(shiftWordRight()));

  m_editAccels->insert("KATE_CURSOR_HOME",i18n("Home"),"",Key_Home,this,SLOT(home()));
  m_editAccels->insert("KATE_CURSOR_TOP",i18n("Top"),"",CTRL+Key_Home,this,SLOT(top()));
  m_editAccels->insert("KATE_CURSOR_HOME_SELECT",i18n("Home + SELECT"),"",SHIFT+Key_Home,this,SLOT(shiftHome()));
  m_editAccels->insert("KATE_CURSOR_TOP_SELECT",i18n("Top + SELECT"),"",SHIFT+CTRL+Key_Home,this,SLOT(shiftTop()));

  m_editAccels->insert("KATE_CURSOR_END",i18n("End"),"",Key_End,this,SLOT(end()));
  m_editAccels->insert("KATE_CURSOR_BOTTOM",i18n("Bottom"),"",CTRL+Key_End,this,SLOT(bottom()));
  m_editAccels->insert("KATE_CURSOR_END_SELECT",i18n("End + SELECT"),"",SHIFT+Key_End,this,SLOT(shiftEnd()));
  m_editAccels->insert("KATE_CURSOR_BOTTOM_SELECT",i18n("Bottom + SELECT"),"",SHIFT+CTRL+Key_End,this,SLOT(shiftBottom()));

  m_editAccels->insert("KATE_CURSOR_UP",i18n("Cursor up"),"",Key_Up,this,SLOT(up()));
  m_editAccels->insert("KATE_CURSOR_UP_SELECT",i18n("Cursor up + SELECT"),"",SHIFT+Key_Up,this,SLOT(shiftUp()));
  m_editAccels->insert("KATE_SCROLL_UP",i18n("Scroll one line up"),"",CTRL+Key_Up,this,SLOT(scrollUp()));

  m_editAccels->insert("KATE_CURSOR_DOWN",i18n("Cursor down"),"",Key_Down,this,SLOT(down()));
  m_editAccels->insert("KATE_CURSOR_DOWN_SELECT",i18n("Cursor down + SELECT"),"",SHIFT+Key_Down,this,SLOT(shiftDown()));
  m_editAccels->insert("KATE_SCROLL_DOWN",i18n("Scroll one line down"),"",CTRL+Key_Down,this,SLOT(scrollDown()));

  KConfig config("kateeditkeysrc");
  m_editAccels->readSettings(&config);

  if (!(myViewInternal->hasFocus())) m_editAccels->setEnabled(false);
}

void KateView::setupActions()
{
  KActionCollection *ac = this->actionCollection ();

  if (myDoc->m_bSingleViewMode)
  {
    ac = myDoc->actionCollection ();
  }

  if (!myDoc->m_bReadOnly)
  {
    KStdAction::save(this, SLOT(save()), ac);
    editUndo = KStdAction::undo(myDoc, SLOT(undo()), ac);
    editRedo = KStdAction::redo(myDoc, SLOT(redo()), ac);
    KStdAction::cut(this, SLOT(cut()), ac);
    KStdAction::paste(this, SLOT(paste()), ac);
    new KAction(i18n("Apply Word Wrap"), "", 0, myDoc, SLOT(applyWordWrap()), ac, "edit_apply_wordwrap");
    KStdAction::replace(this, SLOT(replace()), ac);
    new KAction(i18n("Editing Co&mmand"), Qt::CTRL+Qt::Key_M, this, SLOT(slotEditCommand()), ac, "edit_cmd");

    // setup Tools menu
    KStdAction::spelling(myDoc, SLOT(spellcheck()), ac);
    new KAction(i18n("&Indent"), "indent", Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              ac, "tools_indent");
    new KAction(i18n("&Unindent"), "unindent", Qt::CTRL+Qt::SHIFT+Qt::Key_I, this, SLOT(unIndent()),
                                ac, "tools_unindent");
    new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   ac, "tools_cleanIndent");
    new KAction(i18n("C&omment"), CTRL+Qt::Key_NumberSign, this, SLOT(comment()),
                               ac, "tools_comment");
    new KAction(i18n("Unco&mment"), CTRL+SHIFT+Qt::Key_NumberSign, this, SLOT(uncomment()),
                                 ac, "tools_uncomment");
  }

  KStdAction::close( this, SLOT(flush()), ac, "file_close" );
  KStdAction::copy(this, SLOT(copy()), ac);

  KStdAction::saveAs(this, SLOT(saveAs()), ac);
  KStdAction::find(this, SLOT(find()), ac);
  KStdAction::findNext(this, SLOT(findAgain()), ac);
  KStdAction::findPrev(this, SLOT(findPrev()), ac, "edit_find_prev");
  KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  new KAction(i18n("&Configure Editor..."), 0, myDoc, SLOT(configDialog()),ac, "set_confdlg");
  setHighlight = myDoc->hlActionMenu (i18n("&Highlight Mode"),ac,"set_highlight");
  myDoc->exportActionMenu (i18n("&Export"),ac,"file_export");
  KStdAction::selectAll(myDoc, SLOT(selectAll()), ac);
  KStdAction::deselect(myDoc, SLOT(clearSelection()), ac);
  new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, this, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, this, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  new KAction(i18n("&Toggle Block Selection"), Key_F4, myDoc, SLOT(toggleBlockSelectionMode()), ac, "set_verticalSelect");
  new KToggleAction(i18n("Show &Icon Border"), Key_F6, this, SLOT(toggleIconBorder()), ac, "view_border");
  new KToggleAction(i18n("Show &Line Numbers"), Key_F11, this, SLOT(setLineNumbersOn(bool)), ac, "view_line_numbers");
  bookmarkMenu = new KActionMenu(i18n("&Bookmarks"), ac, "bookmarks");

  // setup bookmark menu
  bookmarkToggle = new KAction(i18n("Toggle &Bookmark"), Qt::CTRL+Qt::Key_B, this, SLOT(toggleBookmark()), ac, "edit_bookmarkToggle");
  bookmarkClear = new KAction(i18n("Clear Bookmarks"), 0, myDoc, SLOT(clearMarks()), ac, "edit_bookmarksClear");

  // connect bookmarks menu aboutToshow
  connect(bookmarkMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));

  QStringList list;
  setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, ac, "set_eol");
  connect(setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
  list.clear();
  list.append("&Unix");
  list.append("&Windows/Dos");
  list.append("&Macintosh");
  setEndOfLine->setItems(list);
}

void KateView::slotUpdate()
{
  slotNewUndo();
}

void KateView::slotFileStatusChanged()
{
  int eol = getEol();
  eol = eol>=1 ? eol : 0;

    setEndOfLine->setCurrentItem(eol);
}

void KateView::slotNewUndo()
{
  if (myDoc->m_bReadOnly)
    return;

  if (doc()->undoCount() == 0)
  {
    editUndo->setEnabled(false);
  }
  else
  {
    editUndo->setEnabled(true);
  }

  if (doc()->redoCount() == 0)
  {
    editRedo->setEnabled(false);
  }
  else
  {
    editRedo->setEnabled(true);
  }
}

void KateView::slotDropEventPass( QDropEvent * ev )
{
    KURL::List lstDragURLs;
    bool ok = KURLDrag::decode( ev, lstDragURLs );

    KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
    if ( ok && ext )
        emit ext->openURLRequest( lstDragURLs.first() );
}

void KateView::keyPressEvent( QKeyEvent *ev )
{
  int key=KKey(ev).keyCodeQt();
  VConfig c;

  switch(key)
  {
        case Key_PageUp:
            pageUp();
            break;
        case SHIFT+Key_PageUp:
            shiftPageUp();
            break;
        case CTRL+Key_PageUp:
            topOfView();
            break;
        case Key_PageDown:
            pageDown();
            break;
        case SHIFT+Key_PageDown:
            shiftPageDown();
            break;
        case CTRL+Key_PageDown:
            bottomOfView();
            break;
        case Key_Return:
        case Key_Enter:
            doEditCommand(KateView::cmReturn);
            break;
        case Key_Delete:
            keyDelete();
            break;
        case CTRL+Key_Delete:
            shiftWordRight();
            myViewInternal->getVConfig(c);
            myDoc->removeSelectedText();
            myViewInternal->update();
            break;
        case Key_Backspace:
            backspace();
            break;
        case CTRL+Key_Backspace:
            shiftWordLeft();
            myViewInternal->getVConfig(c);
            myDoc->removeSelectedText();
            myViewInternal->update();
            break;
        case Key_Insert:
            toggleInsert();
            break;
        case CTRL+Key_K:
            killLine();
            break;
        default:
            KTextEditor::View::keyPressEvent( ev );
            return;
            break; // never reached ;)
    }
    ev->accept();
}

void KateView::customEvent( QCustomEvent *ev )
{
    if ( KParts::GUIActivateEvent::test( ev ) && static_cast<KParts::GUIActivateEvent *>( ev )->activated() )
    {
        installPopup(static_cast<QPopupMenu *>(factory()->container("rb_popup", this) ) );
        return;
    }

    KTextEditor::View::customEvent( ev );
    return;
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
    if ( !extension || !myDoc )
        return;
    
    emit extension->popupMenu( ev->globalPos(), myDoc->url(),
                               QString::fromLatin1( "text/plain" ) );
    ev->accept();
}

bool KateView::setCursorPosition( uint line, uint col )
{
  setCursorPositionInternal( line, col, tabWidth() );
  return true;
}

bool KateView::setCursorPositionReal( uint line, uint col)
{
  setCursorPositionInternal( line, col, 1 );
  return true;
}

void KateView::cursorPosition( uint *line, uint *col )
{
  if ( line )
    *line = cursorLine();

  if ( col )
    *col = cursorColumn();
}

void KateView::cursorPositionReal( uint *line, uint *col )
{
  if ( line )
    *line = cursorLine();

  if ( col )
    *col = cursorColumnReal();
}

uint KateView::cursorLine() {
  return myViewInternal->getCursor().line;
}

uint KateView::cursorColumn() {
  return myDoc->currentColumn(myViewInternal->getCursor());
}

uint KateView::cursorColumnReal() {
  return myViewInternal->getCursor().col;
}

void KateView::setCursorPositionInternal(int line, int col, int tabwidth)
{
  if ((uint)line > myDoc->lastLine())
    return;

  KateTextCursor cursor;

  TextLine::Ptr textLine = myDoc->getTextLine(line);
  QString line_str = QString(textLine->getText(), textLine->length());

  int z;
  int x = 0;
  for (z = 0; z < (int)line_str.length() && z < col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  cursor.col = x;
  cursor.line = line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myDoc->updateViews();
}

int KateView::tabWidth() {
  return myDoc->tabChars;
}

void KateView::setTabWidth(int w) {
  myDoc->setTabWidth(w);
  myDoc->updateViews();
}

void KateView::setEncoding (QString e) {
  myDoc->setEncoding (e);
  myDoc->updateViews();
}

bool KateView::isLastView() {
  return myDoc->isLastView(1);
}

KateDocument *KateView::doc() {
  return myDoc;
}

bool KateView::isOverwriteMode() const
{
  return ( myDoc->_configFlags & KateDocument::cfOvr );
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    myDoc->setConfigFlags( myDoc->_configFlags ^ KateDocument::cfOvr );
  else
    myDoc->setConfigFlags( myDoc->_configFlags | KateDocument::cfOvr );
}

void KateView::setDynWordWrap (bool b)
{
  if (_hasWrap != b)
    myViewInternal->updateView(KateView::ufDocGeometry);

  _hasWrap = b;
}

bool KateView::dynWordWrap () const
{
  return _hasWrap;
}

void KateView::toggleInsert() {
  myDoc->setConfigFlags(myDoc->_configFlags ^ KateDocument::cfOvr);
  emit newStatus();
}

QString KateView::currentTextLine() {
  TextLine::Ptr textLine = myDoc->getTextLine(myViewInternal->getCursor().line);
  return QString(textLine->getText(), textLine->length());
}

QString KateView::currentWord() {
  return myDoc->getWord(myViewInternal->getCursor());
}

/*
QString KateView::word(int x, int y) {
  KateTextCursor cursor;
  cursor.line = (myViewInternal->yPos + y)/myDoc->viewFont.fontHeight;
  if (cursor.line < 0 || cursor.line > (int)myDoc->lastLine()) return QString();
  cursor.col = myDoc->textPos(myDoc->getTextLine(cursor.line), myViewInternal->xPos + x);
  return myDoc->getWord(cursor);
}
*/

void KateView::insertText(const QString &s)
{
  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->insertText(c.cursor.line, c.cursor.col, s);

//JW BEGIN
      for (int i=0;i<s.length();i++)
	{
		cursorRight();
	}
//JW END
     // updateCursor(c.cursor);

}

bool KateView::canDiscard() {
  int query;

  if (doc()->isModified()) {
    query = KMessageBox::warningYesNoCancel(this,
      i18n("The current Document has been modified.\nWould you like to save it?"));
    switch (query) {
      case KMessageBox::Yes: //yes
        if (save() == SAVE_CANCEL) return false;
        if (doc()->isModified()) {
            query = KMessageBox::warningContinueCancel(this,
               i18n("Could not save the document.\nDiscard it and continue?"),
           QString::null, i18n("&Discard"));
          if (query == KMessageBox::Cancel) return false;
        }
        break;
      case KMessageBox::Cancel: //cancel
        return false;
    }
  }
  return true;
}

void KateView::flush()
{
  if (canDiscard()) myDoc->flush();
}

KateView::saveResult KateView::save() {
  int query = KMessageBox::Yes;
  if (doc()->isModified()) {
    if (!myDoc->url().fileName().isEmpty() && doc()->isReadWrite()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( myDoc->isNewDoc() )
      {
        query = checkOverwrite( myDoc->url() );
        if( query == KMessageBox::Cancel )
          return SAVE_CANCEL;
      }
      if( query == KMessageBox::Yes )
      {
         if( !myDoc->saveAs(myDoc->url()) ) {
       KMessageBox::sorry(this,
        i18n("The file could not be saved. Please check if you have write permission."));
      return SAVE_ERROR;
  }
      }
      else  // Do not overwrite already existing document:
        return saveAs();
    } // New, unnamed document:
    else
      return saveAs();
  }
  return SAVE_OK;
}

/*
 * Check if the given URL already exists. Currently used by both save() and saveAs()
 *
 * Asks the user for permission and returns the message box result and defaults to
 * KMessageBox::Yes in case of doubt
 */
int KateView::checkOverwrite( KURL u )
{
  int query = KMessageBox::Yes;

  if( u.isLocalFile() )
  {
    QFileInfo info;
    QString name( u.path() );
    info.setFile( name );
    if( info.exists() )
      query = KMessageBox::warningYesNoCancel( this,
        i18n( "A Document with this Name already exists.\nDo you want to overwrite it?" ) );
  }
  return query;
}

KateView::saveResult KateView::saveAs() {
  int query;
  KateFileDialog *dialog;
  KateFileDialogData data;

  do {
    query = KMessageBox::Yes;

    dialog = new KateFileDialog (myDoc->url().url(),doc()->encoding(), this, i18n ("Save File"), KateFileDialog::saveDialog);
    data = dialog->exec ();
    delete dialog;
    if (data.url.isEmpty())
      return SAVE_CANCEL;
    query = checkOverwrite( data.url );
  }
  while ((query != KMessageBox::Cancel) && (query != KMessageBox::Yes));

  if( query == KMessageBox::Cancel )
    return SAVE_CANCEL;

  myDoc->setEncoding (data.encoding);
  if( !myDoc->saveAs(data.url) ) {
    KMessageBox::sorry(this,
      i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

void KateView::doCursorCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  if (cmdNum & selectFlag) c.flags |= KateDocument::cfMark;
  cmdNum &= ~selectFlag;
  myViewInternal->doCursorCommand(c, cmdNum);
  myDoc->updateViews();
}

void KateView::doEditCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  myViewInternal->doEditCommand(c, cmdNum);
}

static void kwview_addToStrList(QStringList &list, const QString &str) {
  if (list.count() > 0) {
    if (list.first() == str) return;
    QStringList::Iterator it;
    it = list.find(str);
    if (*it != 0L) list.remove(it);
    if (list.count() >= 16) list.remove(list.fromLast());
  }
  list.prepend(str);
}

void KateView::find() {
  SearchDialog *searchDialog;

  if (!myDoc->hasSelection()) myDoc->_searchFlags &= ~KateDocument::sfSelected;

  searchDialog = new SearchDialog(this, myDoc->searchForList, myDoc->replaceWithList,
  myDoc->_searchFlags & ~KateDocument::sfReplace);

  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
   QString str;
   if (myDoc->hasSelection())
     str = myDoc->selection();

   if (str.isEmpty())
     str = currentWord();

   if (!str.isEmpty())
   {
     str.replace(QRegExp("^\n"), "");
     int pos=str.find("\n");
     if (pos>-1)
       str=str.left(pos);
     searchDialog->setSearchText( str );
   }

  myViewInternal->focusOutEvent(0L);// QT bug ?
  if (searchDialog->exec() == QDialog::Accepted) {
    kwview_addToStrList(myDoc->searchForList, searchDialog->getSearchFor());
    myDoc->_searchFlags = searchDialog->getFlags() | (myDoc->_searchFlags & KateDocument::sfPrompt);
    initSearch(myDoc->s, myDoc->_searchFlags);
    findAgain(myDoc->s);
  }
  delete searchDialog;
}

void KateView::replace() {
  SearchDialog *searchDialog;

  if (!doc()->isReadWrite()) return;

  if (!myDoc->hasSelection()) myDoc->_searchFlags &= ~KateDocument::sfSelected;
  searchDialog = new SearchDialog(this, myDoc->searchForList, myDoc->replaceWithList,
    myDoc->_searchFlags | KateDocument::sfReplace);

  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
   QString str;
   if (myDoc->hasSelection())
     str = myDoc->selection();

   if (str.isEmpty())
     str = currentWord();

   if (!str.isEmpty())
   {
     str.replace(QRegExp("^\n"), "");
     int pos=str.find("\n");
     if (pos>-1)
       str=str.left(pos);
     searchDialog->setSearchText( str );
   }

  myViewInternal->focusOutEvent(0L);// QT bug ?
  if (searchDialog->exec() == QDialog::Accepted) {
//    myDoc->recordReset();
    kwview_addToStrList(myDoc->searchForList, searchDialog->getSearchFor());
    kwview_addToStrList(myDoc->replaceWithList, searchDialog->getReplaceWith());
    myDoc->_searchFlags = searchDialog->getFlags();
    initSearch(myDoc->s, myDoc->_searchFlags);
    replaceAgain();
  }
  delete searchDialog;
}

void KateView::gotoLine()
{
  GotoLineDialog *dlg;

  dlg = new GotoLineDialog(this, myViewInternal->getCursor().line + 1, myDoc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int linenumber )
{
  KateTextCursor cursor;

  cursor.col = 0;
  cursor.line = linenumber;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView(KateView::ufUpdateOnScroll);
  myDoc->updateViews();
 }

void KateView::initSearch(SConfig &, int flags) {

  myDoc->s.flags = flags;
  myDoc->s.setPattern(myDoc->searchForList.first());

  if (!(myDoc->s.flags & KateDocument::sfFromBeginning)) {
    // If we are continuing a backward search, make sure we do not get stuck
    // at an existing match.
    myDoc->s.cursor = myViewInternal->getCursor();
    TextLine::Ptr textLine = myDoc->getTextLine(myDoc->s.cursor.line);
    QString const txt(textLine->getText(),textLine->length());
    const QString searchFor= myDoc->searchForList.first();
    int pos = myDoc->s.cursor.col-searchFor.length()-1;
    if ( pos < 0 ) pos = 0;
    pos= txt.find(searchFor, pos, myDoc->s.flags & KateDocument::sfCaseSensitive);
    if ( myDoc->s.flags & KateDocument::sfBackward )
    {
      if ( pos <= myDoc->s.cursor.col )  myDoc->s.cursor.col= pos-1;
    }
    else
      if ( pos == myDoc->s.cursor.col )  myDoc->s.cursor.col++;
  } else {
    if (!(myDoc->s.flags & KateDocument::sfBackward)) {
      myDoc->s.cursor.col = 0;
      myDoc->s.cursor.line = 0;
    } else {
      myDoc->s.cursor.col = -1;
      myDoc->s.cursor.line = myDoc->lastLine();
    }
    myDoc->s.flags |= KateDocument::sfFinished;
  }
  if (!(myDoc->s.flags & KateDocument::sfBackward)) {
    if (!(myDoc->s.cursor.col || myDoc->s.cursor.line))
      myDoc->s.flags |= KateDocument::sfFinished;
  }
}

void KateView::continueSearch(SConfig &) {

  if (!(myDoc->s.flags & KateDocument::sfBackward)) {
    myDoc->s.cursor.col = 0;
    myDoc->s.cursor.line = 0;
  } else {
    myDoc->s.cursor.col = -1;
    myDoc->s.cursor.line = myDoc->lastLine();
  }
  myDoc->s.flags |= KateDocument::sfFinished;
  myDoc->s.flags &= ~KateDocument::sfAgain;
}

void KateView::findAgain(SConfig &s) {
  int query;
  KateTextCursor cursor;
  QString str;

  QString searchFor = myDoc->searchForList.first();

  if( searchFor.isEmpty() ) {
    find();
    return;
  }

  do {
    query = KMessageBox::Cancel;
    if (myDoc->doSearch(myDoc->s,searchFor)) {
      cursor = myDoc->s.cursor;
      if (!(myDoc->s.flags & KateDocument::sfBackward))
        myDoc->s.cursor.col += myDoc->s.matchedLength;
      myViewInternal->updateCursor(myDoc->s.cursor); //does deselectAll()
      exposeFound(cursor,myDoc->s.matchedLength,(myDoc->s.flags & KateDocument::sfAgain) ? 0 : KateView::ufUpdateOnScroll,false);
    } else {
      if (!(myDoc->s.flags & KateDocument::sfFinished)) {
        // ask for continue
        if (!(myDoc->s.flags & KateDocument::sfBackward)) {
          // forward search
          str = i18n("End of document reached.\n"
                "Continue from the beginning?");
          query = KMessageBox::warningContinueCancel(this,
                           str, i18n("Find"), i18n("Continue"));
        } else {
          // backward search
          str = i18n("Beginning of document reached.\n"
                "Continue from the end?");
          query = KMessageBox::warningContinueCancel(this,
                           str, i18n("Find"), i18n("Continue"));
        }
        continueSearch(s);
      } else {
        // wrapped
        KMessageBox::sorry(this,
          i18n("Search string '%1' not found!").arg(KStringHandler::csqueeze(searchFor)),
          i18n("Find"));
      }
    }
  } while (query == KMessageBox::Continue);
}

void KateView::replaceAgain() {
  if (!doc()->isReadWrite())
    return;

  replaces = 0;
  if (myDoc->s.flags & KateDocument::sfPrompt) {
    doReplaceAction(-1);
  } else {
    doReplaceAction(KateView::srAll);
  }
}

void KateView::doReplaceAction(int result, bool found) {
  int rlen;
  KateTextCursor cursor;
  bool started;

  QString searchFor = myDoc->searchForList.first();
  QString replaceWith = myDoc->replaceWithList.first();
  rlen = replaceWith.length();

  switch (result) {
    case KateView::srYes: //yes
      myDoc->removeText (myDoc->s.cursor.line, myDoc->s.cursor.col, myDoc->s.cursor.line, myDoc->s.cursor.col + myDoc->s.matchedLength);
      myDoc->insertText (myDoc->s.cursor.line, myDoc->s.cursor.col, replaceWith);
      replaces++;

      if (!(myDoc->s.flags & KateDocument::sfBackward))
            myDoc->s.cursor.col += rlen;
          else
          {
            if (myDoc->s.cursor.col > 0)
              myDoc->s.cursor.col--;
            else
            {
              myDoc->s.cursor.line--;

              if (myDoc->s.cursor.line >= 0)
              {
                myDoc->s.cursor.col = myDoc->getTextLine(myDoc->s.cursor.line)->length();
              }
            }
          }

      break;
    case KateView::srNo: //no
      if (!(myDoc->s.flags & KateDocument::sfBackward))
            myDoc->s.cursor.col += myDoc->s.matchedLength;
          else
          {
            if (myDoc->s.cursor.col > 0)
              myDoc->s.cursor.col--;
            else
            {
              myDoc->s.cursor.line--;

              if (myDoc->s.cursor.line >= 0)
              {
                myDoc->s.cursor.col = myDoc->getTextLine(myDoc->s.cursor.line)->length();
              }
            }
          }
     
      break;
    case KateView::srAll: //replace all
      deleteReplacePrompt();
      do {
        started = false;
        while (found || myDoc->doSearch(myDoc->s,searchFor)) {
          if (!started) {
            found = false;
            started = true;
          }
          myDoc->removeText (myDoc->s.cursor.line, myDoc->s.cursor.col, myDoc->s.cursor.line, myDoc->s.cursor.col + myDoc->s.matchedLength);
          myDoc->insertText (myDoc->s.cursor.line, myDoc->s.cursor.col, replaceWith);
          replaces++;

          if (!(myDoc->s.flags & KateDocument::sfBackward))
            myDoc->s.cursor.col += rlen;
          else
          {
            if (myDoc->s.cursor.col > 0)
              myDoc->s.cursor.col--;
            else
            {
              myDoc->s.cursor.line--;

              if (myDoc->s.cursor.line >= 0)
              {
                myDoc->s.cursor.col = myDoc->getTextLine(myDoc->s.cursor.line)->length();
              }
            }
          }

        }
      } while (!askReplaceEnd());
      return;
    case KateView::srCancel: //cancel
      deleteReplacePrompt();
      return;
    default:
      replacePrompt = 0L;
  }

  do {
    if (myDoc->doSearch(myDoc->s,searchFor)) {
      //text found: highlight it, show replace prompt if needed and exit
      cursor = myDoc->s.cursor;
      if (!(myDoc->s.flags & KateDocument::sfBackward)) cursor.col += myDoc->s.matchedLength;
      myViewInternal->updateCursor(cursor); //does deselectAll()
      exposeFound(myDoc->s.cursor,myDoc->s.matchedLength,(myDoc->s.flags & KateDocument::sfAgain) ? 0 : KateView::ufUpdateOnScroll,true);
      if (replacePrompt == 0L) {
        replacePrompt = new ReplacePrompt(this);
        myDoc->setPseudoModal(replacePrompt);//disable();
        connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
        replacePrompt->show(); //this is not modal
      }
      return; //exit if text found
    }
    //nothing found: repeat until user cancels "repeat from beginning" dialog
  } while (!askReplaceEnd());
  deleteReplacePrompt();
}

void KateView::exposeFound(KateTextCursor &cursor, int slen, int flags, bool replace) {
/* FIXME

  int x1, x2, y1, y2, xPos, yPos;

  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->selectLength(cursor,slen,c.flags);

  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);
  x1 = myDoc->textWidth(textLine,cursor.col)        -10;
  x2 = myDoc->textWidth(textLine,cursor.col + slen) +20;
  y1 = myDoc->viewFont.fontHeight*cursor.line       -10;
  y2 = y1 + myDoc->viewFont.fontHeight              +30;

  xPos = myViewInternal->xPos;
  yPos = myViewInternal->yPos;

  if (x1 < 0) x1 = 0;
  if (replace) y2 += 90;

  if (x1 < xPos || x2 > xPos + myViewInternal->width()) {
    xPos = x2 - myViewInternal->width();
  }
  if (y1 < yPos || y2 > yPos + myViewInternal->height()) {
    xPos = x2 - myViewInternal->width();
    yPos = myDoc->viewFont.fontHeight*cursor.line - height()/3;
  }
  myViewInternal->setPos(xPos, yPos);
  myViewInternal->updateView(flags);// | ufPos,xPos,yPos);
  myDoc->updateViews();
*/
}

void KateView::deleteReplacePrompt() {
  myDoc->setPseudoModal(0L);
}

bool KateView::askReplaceEnd() {
  QString str;
  int query;

  myDoc->updateViews();
  if (myDoc->s.flags & KateDocument::sfFinished) {
    // replace finished
    str = i18n("%1 replacement(s) made").arg(replaces);
    KMessageBox::information(this, str, i18n("Replace"));
    return true;
  }

  // ask for continue
  if (!(myDoc->s.flags & KateDocument::sfBackward)) {
    // forward search
    str = i18n("%1 replacement(s) made.\n"
               "End of document reached.\n"
               "Continue from the beginning?").arg(replaces);
    query = KMessageBox::questionYesNo(this, str, i18n("Replace"),
        i18n("Continue"), i18n("Stop"));
  } else {
    // backward search
    str = i18n("%1 replacement(s) made.\n"
                "Beginning of document reached.\n"
                "Continue from the end?").arg(replaces);
    query = KMessageBox::questionYesNo(this, str, i18n("Replace"),
                i18n("Continue"), i18n("Stop"));
  }
  replaces = 0;
  continueSearch(myDoc->s);
  return (query == KMessageBox::No);
}

void KateView::replaceSlot() {
  doReplaceAction(replacePrompt->result(),true);
}

void KateView::installPopup(QPopupMenu *rmb_Menu)
{
  rmbMenu = rmb_Menu;
}

void KateView::readSessionConfig(KConfig *config)
{
  KateTextCursor cursor;

/*FIXME 
  myViewInternal->xPos = config->readNumEntry("XPos");
  myViewInternal->yPos = config->readNumEntry("YPos");
*/
  cursor.col = config->readNumEntry("CursorX");
  cursor.line = config->readNumEntry("CursorY");
  myViewInternal->updateCursor(cursor);
  iconBorderStatus = config->readNumEntry("IconBorderStatus");
  setIconBorder( iconBorderStatus & KateIconBorder::Icons );
  setLineNumbersOn( iconBorderStatus & KateIconBorder::LineNumbers );
}

void KateView::writeSessionConfig(KConfig *config)
{
/*FIXME
  config->writeEntry("XPos",myViewInternal->xPos);
  config->writeEntry("YPos",myViewInternal->yPos);
  config->writeEntry("CursorX",myViewInternal->cursor.col);
  config->writeEntry("CursorY",myViewInternal->cursor.line);
*/

  config->writeEntry("IconBorderStatus", iconBorderStatus );
}

int KateView::getEol() {
  return myDoc->eolMode;
}

void KateView::setEol(int eol) {
  if (!doc()->isReadWrite())
    return;

  myDoc->eolMode = eol;
  myDoc->setModified(true);
}

void KateView::resizeEvent(QResizeEvent *)
{
  myViewInternal->tagAll();
  myViewInternal->updateView(0/*ufNoScroll*/);
}

void KateView::dropEventPassEmited (QDropEvent* e)
{
  emit dropEventPass(e);
}

// Applies a new pattern to the search context.
void SConfig::setPattern(QString &newPattern) {
  bool regExp = (flags & KateDocument::sfRegularExpression);

  m_pattern = newPattern;
  if (regExp) {
    m_regExp.setCaseSensitive(flags & KateDocument::sfCaseSensitive);
    m_regExp.setPattern(m_pattern);
  }
}

void KateView::setActive (bool b)
{
  active = b;
}

bool KateView::isActive ()
{
  return active;
}

void KateView::setFocus ()
{
  QWidget::setFocus ();

  emit gotFocus ((Kate::View *) this);
}

bool KateView::eventFilter (QObject *object, QEvent *event)
{
  if ( object == myViewInternal )
    KCursor::autoHideEventFilter( object, event );

  if ( (event->type() == QEvent::FocusIn) )
  {
    m_editAccels->setEnabled(true);
    emit gotFocus (this);
  }

  if ( (event->type() == QEvent::FocusOut) )
  {
  	m_editAccels->setEnabled(false);
  }

  if ( (event->type() == QEvent::KeyPress) )
    {
        QKeyEvent * ke=(QKeyEvent *)event;

        if ((ke->key()==Qt::Key_Tab) || (ke->key()==Qt::Key_BackTab))
          {
            myViewInternal->keyPressEvent(ke);
            return true;
          }
    }
  if (object == myViewInternal->leftBorder && event->type() == QEvent::Resize)
    updateIconBorder();
  return QWidget::eventFilter (object, event);
}

void KateView::findAgain (bool back)
{
  bool b= (myDoc->_searchFlags & KateDocument::sfBackward) > 0;
  initSearch(myDoc->s, (myDoc->_searchFlags & ((b==back)?~KateDocument::sfBackward:~0) & ~KateDocument::sfFromBeginning)
                | KateDocument::sfPrompt | KateDocument::sfAgain | ((b!=back)?KateDocument::sfBackward : 0) );
  if (myDoc->s.flags & KateDocument::sfReplace)
    replaceAgain();
  else
    KateView::findAgain(myDoc->s);
}

void KateView::slotEditCommand ()
{
  bool ok;
  QString cmd = KLineEditDlg::getText(i18n("Editing Command"), "", &ok, this);

  if (ok)
    myDoc->cmd()->execCmd (cmd, this);
}

void KateView::setIconBorder (bool enable)
{
  if ( enable == iconBorderStatus & KateIconBorder::Icons )
    return; // no change
  if ( enable )
    iconBorderStatus |= KateIconBorder::Icons;
  else
    iconBorderStatus &= ~KateIconBorder::Icons;

  updateIconBorder();
}

void KateView::toggleIconBorder ()
{
  setIconBorder ( ! (iconBorderStatus & KateIconBorder::Icons) );
}

void KateView::setLineNumbersOn(bool enable)
{
  if (enable == iconBorderStatus & KateIconBorder::LineNumbers)
    return; // no change

  if (enable)
    iconBorderStatus |= KateIconBorder::LineNumbers;
  else
    iconBorderStatus &= ~KateIconBorder::LineNumbers;

  updateIconBorder();
}

void KateView::toggleLineNumbersOn()
{
  setLineNumbersOn( ! (iconBorderStatus & KateIconBorder::LineNumbers) );
}

// FIXME anders: move into KateIconBorder class
void KateView::updateIconBorder()
{
  if ( iconBorderStatus != KateIconBorder::None )
  {
    myViewInternal->leftBorder->show();
  }
  else
  {
    myViewInternal->leftBorder->hide();
  }
  myViewInternal->leftBorder->resize(myViewInternal->leftBorder->width(),myViewInternal->leftBorder->height());
  myViewInternal->resize(width()-myViewInternal->leftBorder->width(), myViewInternal->height());
  myViewInternal->move(myViewInternal->leftBorder->width(), 0);
  myViewInternal->updateView(ufLeftBorder);
}

void KateView::gotoMark (KTextEditor::Mark *mark)
{
  KateTextCursor cursor;

  cursor.col = 0;
  cursor.line = mark->line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView(KateView::ufUpdateOnScroll);
  myDoc->updateViews();
}

void KateView::toggleBookmark ()
{
  uint mark = myDoc->mark (cursorLine());

  if (mark&KateDocument::markType01)
    myDoc->removeMark (cursorLine(), KateDocument::markType01);
  else
    myDoc->addMark (cursorLine(), KateDocument::markType01);
}

void KateView::bookmarkMenuAboutToShow()
{
  bookmarkMenu->popupMenu()->clear ();
  bookmarkToggle->plug (bookmarkMenu->popupMenu());
  bookmarkClear->plug (bookmarkMenu->popupMenu());
  bookmarkMenu->popupMenu()->insertSeparator ();

  list = myDoc->marks();
  for (int i=0; (uint) i < list.count(); i++)
  {
    if (list.at(i)->type&KateDocument::markType01)
    {
      QString bText = myDoc->textLine(list.at(i)->line);
      bText.truncate(32);
      bText.append ("...");
      bookmarkMenu->popupMenu()->insertItem ( QString("%1 - \"%2\"").arg(list.at(i)->line).arg(bText), this, SLOT (gotoBookmark(int)), 0, i );
    }
  }
}

void KateView::gotoBookmark (int n)
{
  gotoMark (list.at(n));
}

void KateView::slotIncFontSizes ()
{
  QFont font = myDoc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()+1);
  myDoc->setFont (KateDocument::ViewFont,font);
}

void KateView::slotDecFontSizes ()
{
  QFont font = myDoc->getFont(KateDocument::ViewFont);
  font.setPointSize (font.pointSize()-1);
  myDoc->setFont (KateDocument::ViewFont,font);
}

KateDocument *KateView::document()
{
  return myDoc;
}

KateBrowserExtension::KateBrowserExtension( KateDocument *doc, KateView *view )
: KParts::BrowserExtension( doc, "katepartbrowserextension" )
{
  m_doc = doc;
  m_view = view;
  connect( m_doc, SIGNAL( selectionChanged() ), this, SLOT( slotSelectionChanged() ) );
  emit enableAction( "print", true );
}

void KateBrowserExtension::copy()
{
  m_doc->copy( 0 );
}

void KateBrowserExtension::print()
{
  m_doc->printDialog ();
}

void KateBrowserExtension::slotSelectionChanged()
{
  emit enableAction( "copy", m_doc->hasSelection() );
}
