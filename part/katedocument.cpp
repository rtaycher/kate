/***************************************************************************
                          katedocument.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
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

#include "katedocument.h"
#include "katedocument.moc"

#include "katefactory.h"
#include "kateviewdialog.h"
#include "katedialogs.h"
#include "katebuffer.h"
#include "katetextline.h"
#include "katecmd.h"
#include "kateglobal.h"

#include <qfileinfo.h>
#include <qfile.h>
#include <qfocusdata.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include <qpaintdevicemetrics.h>
#include <qiodevice.h>
#include <qclipboard.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <qptrstack.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kurldrag.h>
#include <kprinter.h>
#include <kapp.h>
#include <kpopupmenu.h>
#include <klineeditdlg.h>
#include <kconfig.h>
#include <ksconfig.h>
#include <kcursor.h>
#include <kcharsets.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kparts/event.h>
#include <kiconloader.h>
#include <kxmlguifactory.h>
#include <dcopclient.h>
#include <kwin.h>
#include <kdialogbase.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kglobalsettings.h>

class KateUndo
{
  friend class KateUndoGroup;

  public:
    KateUndo (KateDocument *doc, uint type, uint line, uint col, uint len,  QString text);
    ~KateUndo ();

  private:
    void undo ();
    void redo ();

  public:
    enum types
    {
      editInsertText,
      editRemoveText,
      editWrapLine,
      editUnWrapLine,
      editInsertLine,
      editRemoveLine
    };

  private:
    KateDocument *myDoc;
    uint type;
    uint line;
    uint col;
    uint len;
    QString text;
};

class KateUndoGroup
{
  public:
    KateUndoGroup (KateDocument *doc);
    ~KateUndoGroup ();

    void undo ();
    void redo ();

    void addItem (KateUndo *undo);

  private:
    KateDocument *myDoc;
    QPtrList<KateUndo> items;
};

QStringList KateDocument::searchForList = QStringList();
QStringList KateDocument::replaceWithList = QStringList();

KateUndo::KateUndo (KateDocument *doc, uint type, uint line, uint col, uint len, QString text)
{
  this->myDoc = doc;
  this->type = type;
  this->line = line;
  this->col = col;
  this->len = len;
  this->text = text;
}

KateUndo::~KateUndo ()
{
}

void KateUndo::undo ()
{
  if (type == KateUndo::editInsertText)
  {
    myDoc->editRemoveText (line, col, len);
  }
  else if (type == KateUndo::editRemoveText)
  {
    myDoc->editInsertText (line, col, text);
  }
  else if (type == KateUndo::editWrapLine)
  {
    myDoc->editUnWrapLine (line, col);
  }
  else if (type == KateUndo::editUnWrapLine)
  {
    myDoc->editWrapLine (line, col);
  }
  else if (type == KateUndo::editInsertLine)
  {
    myDoc->editRemoveLine (line);
  }
  else if (type == KateUndo::editRemoveLine)
  {
    myDoc->editInsertLine (line, text);
  }
}

void KateUndo::redo ()
{
  if (type == KateUndo::editRemoveText)
  {
    myDoc->editRemoveText (line, col, len);
  }
  else if (type == KateUndo::editInsertText)
  {
    myDoc->editInsertText (line, col, text);
  }
  else if (type == KateUndo::editUnWrapLine)
  {
    myDoc->editUnWrapLine (line, col);
  }
  else if (type == KateUndo::editWrapLine)
  {
    myDoc->editWrapLine (line, col);
  }
  else if (type == KateUndo::editRemoveLine)
  {
    myDoc->editRemoveLine (line);
  }
  else if (type == KateUndo::editInsertLine)
  {
    myDoc->editInsertLine (line, text);
  }
}

KateUndoGroup::KateUndoGroup (KateDocument *doc)
{
  myDoc = doc;
}

KateUndoGroup::~KateUndoGroup ()
{
}

void KateUndoGroup::undo ()
{
  if (items.count() == 0)
    return;

  bool b = myDoc->editStart (false);

  for (int pos=(int)items.count()-1; pos >= 0; pos--)
  {
    items.at(pos)->undo();
  }

  if (b)
    myDoc->editEnd ();
}

void KateUndoGroup::redo ()
{
  if (items.count() == 0)
    return;

  bool b = myDoc->editStart (false);

  for (uint pos=0; pos < items.count(); pos++)
  {
    items.at(pos)->redo();
  }

  if (b)
    myDoc->editEnd ();
}

void KateUndoGroup::addItem (KateUndo *undo)
{
  items.append (undo);
}

//
// KateDocument Constructor
//
KateDocument::KateDocument(bool bSingleViewMode, bool bBrowserView,
                                           QWidget *parentWidget, const char *widgetName,
                                           QObject *, const char *)
  : Kate::Document (), viewFont(), printFont(),hlManager(HlManager::self ())
{
  hlSetByUser = false;
  PreHighlightedTill=0;
  RequestPreHighlightTill=0;
  setInstance( KateFactory::instance() );

  editIsRunning = false;
  editCurrentUndo = 0L;
  editWithUndo = false;

  pseudoModal = 0L;
  blockSelect = false;

  myAttribs = 0L;
  myAttribsLen = 0;

  m_bSingleViewMode=bSingleViewMode;
  m_bBrowserView = bBrowserView;

  myMarks.setAutoDelete (true);

  printer = new KPrinter();

  selectStart.line = -1;
  selectStart.col = -1;
  selectEnd.line = -1;
  selectEnd.col = -1;
  selectAnchor.line = -1;
  selectAnchor.col = -1;

  // some defaults
  _configFlags = KateDocument::cfAutoIndent | KateDocument::cfBackspaceIndents
    | KateDocument::cfTabIndents | KateDocument::cfKeepIndentProfile
    | KateDocument::cfRemoveSpaces
    | KateDocument::cfDelOnInput | KateDocument::cfMouseAutoCopy | KateDocument::cfWrapCursor
    | KateDocument::cfShowTabs | KateDocument::cfSmartHome;

  _searchFlags = 0;

  //KSpell initial values
  kspell.kspell = 0;
  kspell.ksc = new KSpellConfig; //default KSpellConfig to start
  kspell.kspellon = false;

  myEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());
  maxLength = -1;

  setFont (ViewFont,KGlobalSettings::fixedFont());
  setFont (PrintFont,KGlobalSettings::fixedFont());

  myDocName = QString ("");
  fileInfo = new QFileInfo ();

  myCmd = new KateCmd (this);

  connect(this,SIGNAL(modifiedChanged ()),this,SLOT(slotModChanged ()));

  buffer = new KateBuffer;
  connect(buffer, SIGNAL(linesChanged(int)), this, SLOT(slotBufferChanged()));
  connect(buffer, SIGNAL(needHighlight(uint,uint)),this,SLOT(slotBufferHighlight(uint,uint)));

  colors[0] = KGlobalSettings::baseColor();
  colors[1] = KGlobalSettings::highlightColor();

  m_highlight = 0L;
  tabChars = 8;

  newDocGeometry = false;
  readOnly = false;
  newDoc = false;

  modified = false;

  clear();

  internalSetHlMode(0); //calls updateFontData()
  // if the user changes the highlight with the dialog, notify the doc
  connect(hlManager,SIGNAL(changed()),SLOT(internalHlChanged()));

  newDocGeometry = false;

  readConfig();

  if ( m_bSingleViewMode )
  {
    KTextEditor::View *view = createView( parentWidget, widgetName );
    view->show();
    setWidget( view );
  }
}

//
// KateDocument Destructor
//
KateDocument::~KateDocument()
{
  if ( !m_bSingleViewMode )
  {
    myViews.setAutoDelete( true );
    myViews.clear();
    myViews.setAutoDelete( false );
  }

  if (kspell.kspell)
  {
    kspell.kspell->setAutoDelete(true);
    kspell.kspell->cleanUp(); // need a way to wait for this to complete
  }

  if (kspell.ksc)
    delete kspell.ksc;

  m_highlight->release();
  myMarks.clear ();

  delete printer;
  delete [] myAttribs;
  delete buffer;
}

//
// KTextEditor::Document stuff
//

KTextEditor::View *KateDocument::createView( QWidget *parent, const char *name )
{
  return new KateView( this, parent, name);
}

QPtrList<KTextEditor::View> KateDocument::views () const
{
  return _views;
};

//
// KTextEditor::EditInterface stuff
//

QString KateDocument::text() const
{
  QString s;

  for (uint i=0; i < buffer->count(); i++)
  {
    TextLine::Ptr textLine = buffer->line(i);
    s.append (textLine->getString());
    if ( (i < (buffer->count()-1)) )
      s.append('\n');
  }

  return s;
}

QString KateDocument::text ( uint startLine, uint startCol, uint endLine, uint endCol ) const
{
  QString s;

  for (uint i=startLine; i < buffer->count(); i++)
  {
    TextLine::Ptr textLine = buffer->line(i);

    if (i == startLine)
      s.append(textLine->getString().mid (startCol, textLine->length()-startCol));
    else if (i == endLine)
      s.append(textLine->getString().mid (0, endCol));
    else
      s.append(textLine->getString());

    if ( i < endLine )
      s.append('\n');
  }

  return s;
}

QString KateDocument::textLine( uint line ) const
{
  TextLine::Ptr l = getTextLine( line );

  if ( !l )
    return QString();

  return l->getString();
}

bool KateDocument::setText(const QString &s)
{
  clear();
  return insertText (0, 0, s);
}

bool KateDocument::clear()
{
  KateTextCursor cursor;
  KateView *view;

  setPseudoModal(0L);

  cursor.col = cursor.line = 0;
  for (view = myViews.first(); view != 0L; view = myViews.next() ) {
    view->updateCursor(cursor);
    view->tagAll();
  }

  eolMode = KateDocument::eolUnix;

  buffer->clear();
  clearMarks ();
  longestLine = buffer->line(0);

  clearUndo();
  clearRedo();

  maxLength = 0;

  setModified(false);

  return true;
}

bool KateDocument::insertText( uint line, uint col, const QString &s )
{
  uint insertPos = col;
  uint len = s.length();
  QChar ch;
  QString buf;
  uint endLine, endCol, startLine, startCol;

  startLine = line;
  startCol = col;
  endLine = line;
  endCol = col;

  if (s.isEmpty())
    return true;

  bool b = editStart ();

  for (uint pos = 0; pos < len; pos++)
  {
    ch = s[pos];

    if (ch == '\n')
    {
      editInsertText (line, insertPos, buf);
      editWrapLine (line, insertPos + buf.length());

      line++;
      endLine++;
      insertPos = 0;
      buf.truncate(0);
    }
    else
      buf += ch; // append char to buffer
  }

  editInsertText (line, insertPos, buf);

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  TextLine::Ptr l, tl;
  uint deletePos = 0;
  uint endPos = 0;
  uint line = 0;

  l = getTextLine(startLine);

  if (!l)
    return false;

  bool b = editStart ();

  if (startLine == endLine)
  {
    editRemoveText (startLine, startCol, endCol-startCol);
  }
  else if ((startLine+1) == endLine)
  {
    editRemoveText (startLine, startCol, l->length()-startCol);
    editRemoveText (startLine+1, 0, endCol);
    editUnWrapLine (startLine, startCol);
  }
  else
  {
    for (line = startLine; line <= endLine; line++)
    {
      if ((line > startLine) && (line < endLine))
      {
        deletePos = 0;

        editRemoveText (startLine, deletePos, l->length()-startCol);
        editUnWrapLine (startLine, deletePos);
      }
      else
      {
        if (line == startLine)
        {
          deletePos = startCol;
          endPos = l->length();
        }
         else
        {
          deletePos = 0;
          endPos = endCol;
        }

        l->replace (deletePos, endPos-deletePos, 0, 0);
        editRemoveText (startLine, deletePos, endPos-deletePos);
      }
    }
  }

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::insertLine( uint l, const QString &str )
{
  if (l > buffer->count())
    return false;

  bool b = editStart ();

  editInsertLine (l, str);

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::removeLine( uint line )
{
  bool b = editStart ();
  bool end = editRemoveLine (line);

  if (b)
    editEnd ();

  return end;
}

uint KateDocument::length() const
{
  return text().length();
}

uint KateDocument::numLines() const
{
  return buffer->count();
}

int KateDocument::lineLength ( uint line ) const
{
  TextLine::Ptr l = getTextLine( line );

  if ( !l )
    return -1;

  return l->length();
}

//
// KTextEditor::EditInterface internal stuff
//

//
// Starts an edit session with (or without) undo, update of view disabled during session
//
bool KateDocument::editStart (bool withUndo)
{
  if (editIsRunning)
    return false;

  editIsRunning = true;
  editWithUndo = withUndo;

  editTagLineStart = 0xffffff;
  editTagLineEnd = 0;

  if (editWithUndo)
    editCurrentUndo = new KateUndoGroup (this);
  else
    editCurrentUndo = 0L;

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);
    v->cursorCacheChanged = false;
    v->cursorCache = v->myViewInternal->cursor;
  }

  return true;
}

//
// End edit session and update Views
//
void KateDocument::editEnd ()
{
  if (!editIsRunning)
    return;

  if (editTagLineStart <= editTagLineEnd)
    updateLines(editTagLineStart, editTagLineEnd);

  if (editWithUndo && editCurrentUndo)
  {
    undoItems.append (editCurrentUndo);
    editCurrentUndo = 0L;
    emit undoChanged ();
  }

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);

     if (v->cursorCacheChanged)
      v->updateCursor (v->cursorCache);
  }

  setModified(true);
  emit textChanged ();

  updateViews();

  editIsRunning = false;
}

void KateDocument::editAddUndo (KateUndo *undo)
{
  if (!undo)
    return;

  if (editIsRunning && editWithUndo && editCurrentUndo)
    editCurrentUndo->addItem (undo);
  else
    delete undo;
}

void KateDocument::editTagLine (uint line)
{
  if (line < editTagLineStart)
    editTagLineStart = line;

  if (line > editTagLineEnd)
    editTagLineEnd = line;
}

void KateDocument::editInsertTagLine (uint line)
{
  if (line <= editTagLineStart)
    editTagLineStart++;

  if (line <= editTagLineEnd)
    editTagLineEnd++;
}

void KateDocument::editRemoveTagLine (uint line)
{
  if ((line < editTagLineStart) && (editTagLineStart > 0))
    editTagLineStart--;

  if ((line < editTagLineEnd) && (editTagLineEnd > 0))
    editTagLineEnd--;
}

bool KateDocument::editInsertText ( uint line, uint col, const QString &s )
{
  TextLine::Ptr l;

  l = getTextLine(line);

  if (!l)
    return false;

  bool b = editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editInsertText, line, col, s.length(), s));

  l->replace(col, 0, s.unicode(), s.length());

  buffer->changeLine(line);
  updateMaxLength(l);
  editTagLine (line);

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::editRemoveText ( uint line, uint col, uint len )
{
  TextLine::Ptr l;
  KateView *view;
  uint cLine, cCol;

  l = getTextLine(line);

  if (!l)
    return false;

  bool b = editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editRemoveText, line, col, len, l->getString().mid(col, len)));

  l->replace(col, len, 0L, 0);

  buffer->changeLine(line);
  updateMaxLength(l);

  editTagLine(line);

  newDocGeometry = true;

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);

    cLine = v->cursorCache.line;
    cCol = v->cursorCache.col;

    if ( (cLine == line) && (cCol > col) )
    {
      if ((cCol - len) >= col)
      {
        if ((cCol - len) > 0)
          cCol = cCol-len;
        else
          cCol = 0;
      }
      else
        cCol = col;

      v->cursorCache.line = line;
      v->cursorCache.col = cCol;
      v->cursorCacheChanged = true;
    }
  }

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::editWrapLine ( uint line, uint col )
{
  TextLine::Ptr l, tl;
  KateView *view;

  l = getTextLine(line);
  tl = new TextLine();

  if (!l || !tl)
    return false;

  bool b = editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editWrapLine, line, col, 0, 0));

  l->wrap (tl, col);

  buffer->insertLine (line+1, tl);
  buffer->changeLine(line);

  updateMaxLength(l);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
      if (myMarks.at(z)->line > line+1)
      {
        myMarks.at(z)->line = myMarks.at(z)->line+1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  editInsertTagLine (line);
  editTagLine(line);
  editTagLine(line+1);

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);
    view->insLine(line+1);
  }

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::editUnWrapLine ( uint line, uint col )
{
  TextLine::Ptr l, tl;
  KateView *view;
  uint cLine, cCol;

  l = getTextLine(line);
  tl = getTextLine(line+1);

  if (!l || !tl)
    return false;

  bool b = editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editUnWrapLine, line, col, 0, 0));

  l->unWrap (col, tl, tl->length());
  l->setContext (tl->getContext(), tl->getContextLength());

  if (longestLine == tl)
    longestLine = 0L;

  buffer->changeLine(line);
  buffer->removeLine(line+1);

  updateMaxLength(l);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
      if (myMarks.at(z)->line > line)
      {
        if (myMarks.at(z)->line == line+1)
          myMarks.remove(z);
        else
          myMarks.at(z)->line = myMarks.at(z)->line-1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  editRemoveTagLine (line);
  editTagLine(line);
  editTagLine(line+1);

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);
    view->delLine(line+1);

    cLine = view->cursorCache.line;
    cCol = view->cursorCache.col;

    if ( (cLine == (line+1)) || ((cLine == line) && (cCol >= col)) )
    {
      cCol = col;

      view->cursorCache.line = line;
      view->cursorCache.col = cCol;
      view->cursorCacheChanged = true;
    }
  }

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::editInsertLine ( uint line, const QString &s )
{
  KateView *view;

  bool b = editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editInsertLine, line, 0, s.length(), s));

  TextLine::Ptr TL=new TextLine();
  TL->append(s.unicode(),s.length());
  buffer->insertLine(line,TL);
  buffer->changeLine(line);

  updateMaxLength(TL);

  editInsertTagLine (line);
  editTagLine(line);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
       if (myMarks.at(z)->line >= line)
      {
        myMarks.at(z)->line = myMarks.at(z)->line+1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);
    view->insLine(line);
  }

  if (b)
    editEnd ();

  return true;
}

bool KateDocument::editRemoveLine ( uint line )
{
  KateView *view;
  uint cLine, cCol;

  if (numLines() == 1)
    return false;

  bool b = editStart ();

  editAddUndo (new KateUndo (this, KateUndo::editRemoveLine, line, 0, getTextLine (line)->getString().length(), getTextLine (line)->getString()));

  if (longestLine == getTextLine (line))
    longestLine = 0L;

  buffer->removeLine(line);

  editRemoveTagLine (line);

  if (!myMarks.isEmpty())
  {
    bool b = false;

    for (uint z=0; z<myMarks.count(); z++)
    {
      if (myMarks.at(z)->line >= line)
      {
        if (myMarks.at(z)->line == line)
          myMarks.remove(z);
        else
          myMarks.at(z)->line = myMarks.at(z)->line-1;
        b = true;
      }
    }

    if (b)
      emit marksChanged ();
  }

  newDocGeometry = true;
  for (uint z2 = 0; z2 < myViews.count(); z2++)
  {
    view = myViews.at(z2);
    view->delLine(line);

    cLine = view->cursorCache.line;
    cCol = view->cursorCache.col;

    if ( (cLine == line) )
    {
      if (line < lastLine())
        view->cursorCache.line = line;
      else
        view->cursorCache.line = line-1;


      cCol = 0;
      view->cursorCache.col = cCol;
      view->cursorCacheChanged = true;
    }
  }

  if (b)
    editEnd();

  return true;
}

//
// KTextEditor::SelectionInterface stuff
//

bool KateDocument::setSelection ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  int oldStartL, oldEndL;

  oldStartL = selectStart.line;
  oldEndL = selectEnd.line;

  if (startLine < endLine)
  {
    selectStart.line = startLine;
    selectStart.col = startCol;
    selectEnd.line = endLine;
    selectEnd.col = endCol;
  }
  else if (startLine > endLine)
  {
    selectStart.line = endLine;
    selectStart.col = endCol;
    selectEnd.line = startLine;
    selectEnd.col = startCol;
  }
  else if (startCol < endCol)
  {
    selectStart.line = startLine;
    selectStart.col = startCol;
    selectEnd.line = endLine;
    selectEnd.col = endCol;
  }
  else if (startCol >= endCol)
  {
    selectStart.line = endLine;
    selectStart.col = endCol;
    selectEnd.line = startLine;
    selectEnd.col = startCol;
  }

  int endL, startL;
  if (oldEndL > selectEnd.line)
    endL = oldEndL;
  else
    endL = selectEnd.line;

  if (oldStartL < selectStart.line)
    startL = oldStartL;
  else
    startL = selectStart.line;

  tagLines (startL, endL);
  updateViews ();

  emit selectionChanged ();

  return true;
}

bool KateDocument::clearSelection ()
{
  tagLines(selectStart.line,selectEnd.line);

  selectStart.line = -1;
  selectStart.col = -1;
  selectEnd.line = -1;
  selectEnd.col = -1;
  selectAnchor.line = -1;
  selectAnchor.col = -1;

  updateViews ();

  emit selectionChanged();

  return true;
}

bool KateDocument::hasSelection() const
{
  return ((selectStart.col != selectEnd.col) || (selectEnd.line != selectStart.line));
}

QString KateDocument::selection() const
{
  TextLine::Ptr textLine;
  QString s;

  for (int z=selectStart.line; z <= selectEnd.line; z++)
  {
      textLine = getTextLine(z);
      if (!textLine)
        break;

      if (!blockSelect)
      {
        if ((z > selectStart.line) && (z < selectEnd.line))
          s.append (textLine->getString());
  else
  {
    if ((z == selectStart.line) && (z == selectEnd.line))
      s.append (textLine->getString().mid(selectStart.col, selectEnd.col-selectStart.col));
    else if ((z == selectStart.line))
      s.append (textLine->getString().mid(selectStart.col, textLine->length()-selectStart.col));
    else if ((z == selectEnd.line))
      s.append (textLine->getString().mid(0, selectEnd.col));
  }
      }
      else
      {
        s.append (textLine->getString().mid(selectStart.col, selectEnd.col-selectStart.col));
      }

      if (z < selectEnd.line)
  s.append (QChar('\n'));
    }

  return s;
}

bool KateDocument::removeSelectedText ()
{
  TextLine::Ptr textLine = 0L;
  int delLen, delStart, delLine;

  if (!hasSelection())
    return false;

  bool b = editStart ();

  for (uint z = 0; z < myViews.count(); z++)
  {
    KateView *v = myViews.at(z);
    if (selectStart.line <= v->cursorCache.line <= selectEnd.line)
    {
      v->cursorCache.line = selectStart.line;
      v->cursorCache.col = selectStart.col;
      v->cursorCacheChanged = true;
    }
  }

  int sl = selectStart.line;
  int el = selectEnd.line;
  int sc = selectStart.col;
  int ec = selectEnd.col;

  for (int z=el; z >= sl; z--)
  {
    textLine = getTextLine(z);
    if (!textLine)
      break;

    delLine = 0;
    delStart = 0;
    delLen = 0;

    if (!blockSelect)
    {
      if ((z > sl) && (z < el))
        delLine = 1;
      else
      {
        if ((z == sl) && (z == el))
        {
          delStart = sc;
          delLen = ec-sc;
        }
        else if ((z == sl))
        {
          delStart = sc;
          delLen = textLine->length()-sc;

          if (sl < el)
            delLen++;
        }
        else if ((z == el))
        {
          delStart = 0;
          delLen = ec;
        }
      }
    }
    else
    {
      delStart = sc;
      delLen = ec-sc;

      if (delStart >= (int)textLine->length())
      {
        delStart = 0;
        delLen = 0;
      }
      else if (delLen+delStart > (int)textLine->length())
        delLen = textLine->length()-delStart;
    }

    if (delLine == 1)
      editRemoveLine (z);
    else if (delStart+delLen > (int)textLine->length())
    {
      editRemoveText (z, delStart, textLine->length()-delStart);
      editUnWrapLine (z, delStart);
    }
    else
      editRemoveText (z, delStart, delLen);
  }

  if (b)
    editEnd ();

  updateLines(sl, el);
  clearSelection();

  return true;
}

bool KateDocument::selectAll()
{
  return setSelection (0, 0, lastLine(), getTextLine(lastLine())->length());
}

//
// KTextEditor::BlockSelectionInterface stuff
//

bool KateDocument::blockSelectionMode ()
{
  return blockSelect;
}

bool KateDocument::setBlockSelectionMode (bool on)
{
  if (on != blockSelect)
  {
    blockSelect = on;
    setSelection (selectStart.line, selectStart.col, selectEnd.line, selectEnd.col);
  }

  return true;
}

bool KateDocument::toggleBlockSelectionMode ()
{
  setBlockSelectionMode (!blockSelect);
  return true;
}

//
// KTextEditor::UndoInterface stuff
//

uint KateDocument::undoCount () const
{
  return undoItems.count ();
}

uint KateDocument::redoCount () const
{
  return redoItems.count ();
}

uint KateDocument::undoSteps () const
{
  return myUndoSteps;
}

void KateDocument::setUndoSteps(uint steps)
{
  myUndoSteps = steps;

  emit undoChanged ();
}

void KateDocument::undo()
{
  undoItems.last()->undo();
  redoItems.append (undoItems.last());
  undoItems.removeLast ();

  emit undoChanged ();
}

void KateDocument::redo()
{
  redoItems.last()->redo();
  undoItems.append (redoItems.last());
  redoItems.removeLast ();

  emit undoChanged ();
}

void KateDocument::clearUndo()
{
  undoItems.setAutoDelete (true);
  undoItems.clear ();
  undoItems.setAutoDelete (false);

  emit undoChanged ();
}

void KateDocument::clearRedo()
{
  redoItems.setAutoDelete (true);
  redoItems.clear ();
  redoItems.setAutoDelete (false);

  emit undoChanged ();
}

//
// KTextEditor::CursorInterface stuff
//

KTextEditor::Cursor *KateDocument::createCursor ( )
{
  return new KateCursor (this);
}

QPtrList<KTextEditor::Cursor> KateDocument::cursors () const
{
  return myCursors;
}

//
// KTextEditor::SearchInterface stuff
//

bool KateDocument::searchText (unsigned int startLine, unsigned int startCol, const QString &text, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool casesensitive, bool backwards)
{
  uint line, col;
  uint searchEnd;
  TextLine::Ptr textLine;
  uint foundAt, myMatchLen;
  bool found;

  if (text.isEmpty())
    return false;

  line = startLine;
  col = startCol;

  if (!backwards)
  {
    searchEnd = lastLine();

    while (line <= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, false);

      if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      col = 0;
      line++;
    }
  }
  else
  {
    // backward search
    searchEnd = 0;

    while (line >= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, text, &foundAt, &myMatchLen, casesensitive, true);

        if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      if (line >= 1)
        col = getTextLine(line-1)->length();

      line--;
    }
  }

  return false;
}

bool KateDocument::searchText (unsigned int startLine, unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards)
{
  uint line, col;
  uint searchEnd;
  TextLine::Ptr textLine;
  uint foundAt, myMatchLen;
  bool found;

  if (regexp.isEmpty() || !regexp.isValid())
    return false;

  line = startLine;
  col = startCol;

  if (!backwards)
  {
    searchEnd = lastLine();

    while (line <= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, false);

      if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      col = 0;
      line++;
    }
  }
  else
  {
    // backward search
    searchEnd = 0;

    while (line >= searchEnd)
    {
      textLine = getTextLine(line);

      found = false;
      found = textLine->searchText (col, regexp, &foundAt, &myMatchLen, true);

        if (found)
      {
        (*foundAtLine) = line;
        (*foundAtCol) = foundAt;
        (*matchLen) = myMatchLen;
        return true;
      }

      if (line >= 1)
        col = getTextLine(line-1)->length();

      line--;
    }
  }

  return false;
}

//
// KTextEditor::HighlightingInterface stuff
//

uint KateDocument::hlMode ()
{
  return hlManager->findHl(m_highlight);
}

bool KateDocument::setHlMode (uint mode)
{
  if (internalSetHlMode (mode))
  {
    setDontChangeHlOnSave();
    updateViews();
    return true;
  }

  return false;
}

bool KateDocument::internalSetHlMode (uint mode)
{
  Highlight *h;

  h = hlManager->getHl(mode);
  if (h == m_highlight) {
    updateLines();
  } else {
    if (m_highlight != 0L) m_highlight->release();
    h->use();
    m_highlight = h;
    makeAttribs();
  }
  PreHighlightedTill=0;
  RequestPreHighlightTill=0;

  emit(hlChanged());

  return true;
}

uint KateDocument::hlModeCount ()
{
  return HlManager::self()->highlights();
}

QString KateDocument::hlModeName (uint mode)
{
  return HlManager::self()->hlName (mode);
}

QString KateDocument::hlModeSectionName (uint mode)
{
  return HlManager::self()->hlSection (mode);
}

void KateDocument::setDontChangeHlOnSave()
{
  hlSetByUser = true;
}

//
// KTextEditor::ConfigInterface stuff
//

void KateDocument::readConfig(KConfig *config)
{
  _searchFlags = config->readNumEntry("SearchFlags", KateDocument::sfPrompt);
  _configFlags = config->readNumEntry("ConfigFlags", _configFlags) & ~KateDocument::cfMark;

  myWordWrap = config->readBoolEntry("Word Wrap On", false);
  myWordWrapAt = config->readNumEntry("Word Wrap At", 80);
  if (myWordWrap)
    wrapText (myWordWrapAt);

  setTabWidth(config->readNumEntry("TabWidth", 8));
  setUndoSteps(config->readNumEntry("UndoSteps", 50));
  setFont (ViewFont,config->readFontEntry("Font", &viewFont.myFont));
  setFont (PrintFont,config->readFontEntry("PrintFont", &printFont.myFont));

  colors[0] = config->readColorEntry("Color Background", &colors[0]);
  colors[1] = config->readColorEntry("Color Selected", &colors[1]);
}

void KateDocument::writeConfig(KConfig *config)
{
  config->writeEntry("SearchFlags",_searchFlags);
  config->writeEntry("ConfigFlags",_configFlags);

  config->writeEntry("Word Wrap On", myWordWrap);
  config->writeEntry("Word Wrap At", myWordWrapAt);
  config->writeEntry("TabWidth", tabChars);
  config->writeEntry("Font", viewFont.myFont);
  config->writeEntry("PrintFont", printFont.myFont);
  config->writeEntry("Color Background", colors[0]);
  config->writeEntry("Color Selected", colors[1]);
}

void KateDocument::readConfig()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Document");
  readConfig (config);
  config->sync();
}

void KateDocument::writeConfig()
{
  KConfig *config = KateFactory::instance()->config();
  config->setGroup("Kate Document");
  writeConfig (config);
  config->sync();
}

void KateDocument::readSessionConfig(KConfig *config)
{
  m_url = config->readEntry("URL"); // ### doesn't this break the encoding? (Simon)
  internalSetHlMode(hlManager->nameFind(config->readEntry("Highlight")));
  // anders: restore bookmarks if possible
  QValueList<int> l = config->readIntListEntry("Bookmarks");
  if ( l.count() ) {
    for (uint i=0; i < l.count(); i++) {
      if ( (int)numLines() < l[i] ) break;
      setMark( l[i], KateDocument::markType01 );
    }
  }
}

void KateDocument::writeSessionConfig(KConfig *config)
{
  config->writeEntry("URL", m_url.url() ); // ### encoding?? (Simon)
  config->writeEntry("Highlight", m_highlight->name());
  // anders: save bookmarks
  QValueList<int> ml;
  for (uint i=0; i < myMarks.count(); i++) {
    if ( myMarks.at(i)->type == 1) // only save bookmarks
     ml << myMarks.at(i)->line;
  }
  if (ml.count() )
    config->writeEntry("Bookmarks", ml);
}

void KateDocument::configDialog()
{
  KWin kwin;

  KDialogBase *kd = new KDialogBase(KDialogBase::IconList,
                                    i18n("Configure Kate Editorpart"),
                                    KDialogBase::Ok | KDialogBase::Cancel |
                                    KDialogBase::Help ,
                                    KDialogBase::Ok, kapp->mainWidget());

  // color options
  QVBox *page=kd->addVBoxPage(i18n("Colors"), QString::null,
                              BarIcon("colorize", KIcon::SizeMedium) );
  ColorConfig *colorConfig = new ColorConfig(page);
  QColor* colors = colors;
  colorConfig->setColors(this->colors);

  page = kd->addVBoxPage(i18n("Fonts"), i18n("Fonts Settings"),
                              BarIcon("fonts", KIcon::SizeMedium) );
  FontConfig *fontConfig = new FontConfig(page);
  fontConfig->setFont (getFont(ViewFont));

  //PRINTING / Font options
  page = kd->addVBoxPage(i18n("Printing"),i18n("Print settings"),
			    BarIcon("fonts",KIcon::SizeMedium));
  FontConfig *printFontConfig = new FontConfig(page);
  printFontConfig->setFont(getFont(PrintFont));


  // indent options
  page=kd->addVBoxPage(i18n("Indent"), QString::null,
                       BarIcon("rightjust", KIcon::SizeMedium) );
  IndentConfigTab *indentConfig = new IndentConfigTab(page, this);

  // select options
  page=kd->addVBoxPage(i18n("Select"), QString::null,
                       BarIcon("misc") );
  SelectConfigTab *selectConfig = new SelectConfigTab(page, this);

  // edit options
  page=kd->addVBoxPage(i18n("Edit"), QString::null,
                       BarIcon("edit", KIcon::SizeMedium ) );
  EditConfigTab *editConfig = new EditConfigTab(page, this);

  // spell checker
  page = kd->addVBoxPage( i18n("Spelling"), i18n("Spell checker behavior"),
                          BarIcon("spellcheck", KIcon::SizeMedium) );


  KSpellConfig *ksc = new KSpellConfig(page, 0L, ksConfig(), false );

  kwin.setIcons(kd->winId(), kapp->icon(), kapp->miniIcon());

  HighlightDialogPage *hlPage;
  HlManager *hlManager;
  HlDataList hlDataList;
  ItemStyleList defaultStyleList;

  hlManager = HlManager::self();

  defaultStyleList.setAutoDelete(true);
  hlManager->getDefaults(defaultStyleList);

  hlDataList.setAutoDelete(true);
  //this gets the data from the KConfig object
  hlManager->getHlDataList(hlDataList);

  page=kd->addVBoxPage(i18n("Highlighting"),i18n("Highlighting configuration"),
                        BarIcon("edit",KIcon::SizeMedium));
  hlPage = new HighlightDialogPage(hlManager, &defaultStyleList, &hlDataList, 0, page);

 if (kd->exec()) {
    // color options
    colorConfig->getColors(colors);
    setFont (ViewFont,fontConfig->getFont());
    setFont (PrintFont,printFontConfig->getFont());
    tagAll();
    updateViews();
    // indent options
    indentConfig->getData(this);
    // select options
    selectConfig->getData(this);
    // edit options
    editConfig->getData(this);
    // spell checker
    ksc->writeGlobalSettings();
    setKSConfig(*ksc);
    hlManager->setHlDataList(hlDataList);
    hlManager->setDefaults(defaultStyleList);
    hlPage->saveData();
  }

  delete kd;
}

uint KateDocument::mark (uint line)
{
  if (myMarks.isEmpty())
    return 0;

  if (line > lastLine())
    return 0;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
      return myMarks.at(z)->type;

  return 0;
}

void KateDocument::setMark (uint line, uint markType)
{
  if (line > lastLine())
    return;

  bool b = false;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.at(z)->type=markType;
      b = true;
    }

  if (!b)
  {
    KTextEditor::Mark *mark = new KTextEditor::Mark;
    mark->line = line;
    mark->type = markType;
    myMarks.append (mark);
  }

  emit marksChanged ();

  tagLines (line,line);
  updateViews ();
}

void KateDocument::clearMark (uint line)
{
  if (myMarks.isEmpty())
    return;

  if (line > lastLine())
    return;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.remove(z);
      emit marksChanged ();

      tagLines (line,line);
      updateViews ();
    }
}

void KateDocument::addMark (uint line, uint markType)
{
  if (line > lastLine())
    return;

  bool b = false;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.at(z)->type=myMarks.at(z)->type | markType;
      b = true;
    }

  if (!b)
  {
    KTextEditor::Mark *mark = new KTextEditor::Mark;
    mark->line = line;
    mark->type = markType;
    myMarks.append (mark);
  }

  emit marksChanged ();

  tagLines (line,line);
  updateViews ();
}

void KateDocument::removeMark (uint line, uint markType)
{
  if (myMarks.isEmpty())
    return;

  if (line > lastLine())
    return;

  for (uint z=0; z<myMarks.count(); z++)
    if (myMarks.at(z)->line == line)
    {
      myMarks.at(z)->type=myMarks.at(z)->type & ~markType;
      if (myMarks.at(z)->type == 0)
        myMarks.remove(z);

      emit marksChanged ();
    }

  tagLines (line,line);
  updateViews ();
}

QPtrList<KTextEditor::Mark> KateDocument::marks ()
{
  return myMarks;
}

void KateDocument::clearMarks ()
{
  if (myMarks.isEmpty())
    return;

  while (myMarks.count() > 0)
  {
    tagLines (myMarks.at (0)->line, myMarks.at (0)->line);
    myMarks.remove((uint)0);
  }

  emit marksChanged ();
  updateViews ();
}

//
// KTextEditor::PrintInterface stuff
//

bool KateDocument::printDialog ()
{
   if ( printer->setup( kapp->mainWidget() ) )
   {
     QPainter paint( printer );
     QPaintDeviceMetrics pdm( printer );

     uint y = 0;
     uint lineCount = 0;
     while (  lineCount <= lastLine()  )
     {
        if (y+printFont.fontHeight >= (uint)pdm.height() )
       {
         printer->newPage();
         y=0;
       }

       paintTextLine ( paint, lineCount, y, 0, pdm.width(), false,PrintFont );
       y += printFont.fontHeight;

       lineCount++;
     }

     return true;
  }

  return false;
}

bool KateDocument::print ()
{
  return printDialog ();
}

//
// KParts::ReadWrite stuff
//

bool KateDocument::openFile()
{
  fileInfo->setFile (m_file);
  setMTime();

  if (!fileInfo->exists() || !fileInfo->isReadable())
    return false;

  clear();
  buffer->insertFile(0, m_file, KGlobal::charsets()->codecForName(myEncoding));

  setMTime();

  if (myWordWrap)
    wrapText (myWordWrapAt);

  int hl = hlManager->wildcardFind( m_file );

  if (hl == -1)
  {
    // fill the detection buffer with the contents of the text
    const int HOWMANY = 1024;
    QByteArray buf(HOWMANY);
    int bufpos = 0, len;
    for (uint i=0; i < buffer->count(); i++)
    {
      TextLine::Ptr textLine = buffer->line(i);
      len = textLine->length();
      if (bufpos + len > HOWMANY) len = HOWMANY - bufpos;
      memcpy(&buf[bufpos], textLine->getText(), len);
      bufpos += len;
      if (bufpos >= HOWMANY) break;
    }

    hl = hlManager->mimeFind( buf, m_file );
  }

  internalSetHlMode(hl);

  updateLines();
  updateViews();

  emit fileNameChanged();

  return true;
}

bool KateDocument::saveFile()
{
  QFile f( m_file );
  if ( !f.open( IO_WriteOnly ) )
    return false; // Error

  QTextStream stream(&f);

  stream.setEncoding(QTextStream::RawUnicode); // disable Unicode headers
  stream.setCodec(KGlobal::charsets()->codecForName(myEncoding)); // this line sets the mapper to the correct codec

  int maxLine = numLines();
  int line = 0;
  while(true)
  {
    stream << getTextLine(line)->getString();
    line++;
    if (line >= maxLine) break;

    if (eolMode == KateDocument::eolUnix) stream << "\n";
    else if (eolMode == KateDocument::eolDos) stream << "\r\n";
    else if (eolMode == KateDocument::eolMacintosh) stream << '\r';
  };
  f.close();

  fileInfo->setFile (m_file);
  setMTime();

  if (!hlSetByUser)
  {
  int hl = hlManager->wildcardFind( m_file );

  if (hl == -1)
  {
    // fill the detection buffer with the contents of the text
    const int HOWMANY = 1024;
    QByteArray buf(HOWMANY);
    int bufpos = 0, len;
    for (uint i=0; i < buffer->count(); i++)
    {
      TextLine::Ptr textLine = buffer->line(i);
      len = textLine->length();
      if (bufpos + len > HOWMANY) len = HOWMANY - bufpos;
      memcpy(&buf[bufpos], textLine->getText(), len);
      bufpos += len;
      if (bufpos >= HOWMANY) break;
    }

    hl = hlManager->mimeFind( buf, m_file );
  }

  internalSetHlMode(hl);
  }
  emit fileNameChanged ();

  return (f.status() == IO_Ok);
}

void KateDocument::setReadWrite( bool rw )
{
  KTextEditor::View *view;

  if (rw == readOnly)
  {
    readOnly = !rw;
    for (view = myViews.first(); view != 0L; view = myViews.next() ) {
      emit static_cast<KateView *>( view )->newStatus();
    }
  }
}

bool KateDocument::isReadWrite() const
{
  return !readOnly;
}

void KateDocument::setModified(bool m) {
  KTextEditor::View *view;

  if (m != modified) {
    modified = m;
    for (view = myViews.first(); view != 0L; view = myViews.next() ) {
      emit static_cast<KateView *>( view )->newStatus();
    }
    emit modifiedChanged ();
  }
}

bool KateDocument::isModified() const {
  return modified;
}

//
// Kate specific stuff ;)
//

void KateDocument::setFont (WhichFont wf,QFont font)
{
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;
  //kdDebug()<<"Kate:: setFont"<<endl;
  int oldwidth=fs->myFontMetrics.width('W');  //Quick & Dirty Hack (by JoWenn) //Remove in KDE 3.0
  fs->myFont = font;
  fs->myFontBold = QFont (font);
  fs->myFontBold.setBold (true);

  fs->myFontItalic = QFont (font);
  fs->myFontItalic.setItalic (true);

  fs->myFontBI = QFont (font);
  fs->myFontBI.setBold (true);
  fs->myFontBI.setItalic (true);

  fs->myFontMetrics = QFontMetrics (fs->myFont);
  fs->myFontMetricsBold = QFontMetrics (fs->myFontBold);
  fs->myFontMetricsItalic = QFontMetrics (fs->myFontItalic);
  fs->myFontMetricsBI = QFontMetrics (fs->myFontBI);
  int newwidth=fs->myFontMetrics.width('W'); //Quick & Dirty Hack (by JoWenn)  //Remove in KDE 3.0
  maxLength=maxLength*(float)newwidth/(float)oldwidth; //Quick & Dirty Hack (by JoWenn)  //Remove in KDE 3.0

  fs->updateFontData(tabChars);
  if (wf==ViewFont)
  {
    updateFontData();
    updateViews(); //Quick & Dirty Hack (by JoWenn) //Remove in KDE 3.0
  }
}

uint  KateDocument::needPreHighlight(uint till)
{
  uint max=numLines()-1;

  if (till>max)
    {
      till=max;
    }
  if (PreHighlightedTill>=till)
    return 0;

  uint tmp=RequestPreHighlightTill;
  if (RequestPreHighlightTill<till)
    {
      RequestPreHighlightTill=till;
      if (tmp<=PreHighlightedTill) QTimer::singleShot(10,this,SLOT(doPreHighlight()));
    }
  return RequestPreHighlightTill;
}

void KateDocument::doPreHighlight()
{
  uint from = PreHighlightedTill;
  uint till = PreHighlightedTill+200;
  uint max = numLines()-1;
  if (till > max)
    {
      till = max;
    }
  PreHighlightedTill = till;
  updateLines(from,till);
  emit preHighlightChanged(PreHighlightedTill);
  if (PreHighlightedTill<RequestPreHighlightTill)
    QTimer::singleShot(10,this,SLOT(doPreHighlight()));
}

TextLine::Ptr KateDocument::getTextLine(int line) const
{
  // This is a hack to get this stuff working.
  return buffer->line(line);
}

int KateDocument::textLength(int line) {
  TextLine::Ptr textLine = getTextLine(line);
  if (!textLine) return 0;
  return textLine->length();
}

void KateDocument::setTabWidth(int chars) {
  if (tabChars == chars) return;
  if (chars < 1) chars = 1;
  if (chars > 16) chars = 16;
  tabChars = chars;
  printFont.updateFontData(tabChars);
  viewFont.updateFontData(tabChars);
  updateFontData();

  maxLength = -1;
  for (uint i=0; i < buffer->count(); i++)
  {
    TextLine::Ptr textLine = buffer->line(i);
    int len = textWidth(textLine,textLine->length());
    if (len > maxLength) {
      maxLength = len;
      longestLine = textLine;
    }
  }
}

void KateDocument::setNewDoc( bool m )
{
  if ( m != newDoc )
  {
    newDoc = m;
  }
}

bool KateDocument::isNewDoc() const {
  return newDoc;
}

//  Spellchecking methods

void KateDocument::spellcheck()
{
  if (!isReadWrite())
    return;

  kspell.kspell= new KSpell (kapp->mainWidget(), "KateView: Spellcheck", this,
                      SLOT (spellcheck2 (KSpell *)));

  connect (kspell.kspell, SIGNAL(death()),
          this, SLOT(spellCleanDone()));

  connect (kspell.kspell, SIGNAL (progress (unsigned int)),
          this, SIGNAL (spellcheck_progress (unsigned int)) );
  connect (kspell.kspell, SIGNAL (misspelling (QString , QStringList *, unsigned)),
          this, SLOT (misspelling (QString, QStringList *, unsigned)));
  connect (kspell.kspell, SIGNAL (corrected (QString, QString, unsigned)),
          this, SLOT (corrected (QString, QString, unsigned)));
  connect (kspell.kspell, SIGNAL (done(const QString&)),
          this, SLOT (spellResult (const QString&)));
}

void KateDocument::spellcheck2(KSpell *)
{
  setReadWrite (false);

  // this is a hack, setPseudoModal() has been hacked to recognize 0x01
  // as special (never tries to delete it)
  // this should either get improved (with a #define or something),
  // or kspell should provide access to the spell widget.
  setPseudoModal((QWidget*)0x01);

  kspell.spell_tmptext = text();

  kspell.kspellon = TRUE;
  kspell.kspellMispellCount = 0;
  kspell.kspellReplaceCount = 0;
  kspell.kspellPristine = !isModified();

  kspell.kspell->setProgressResolution (1);

  kspell.kspell->check(kspell.spell_tmptext);
}

void KateDocument::misspelling (QString origword, QStringList *, unsigned pos)
{
  uint line;
  uint cnt;

  // Find pos  -- CHANGEME: store the last found pos's cursor
  //   and do these searched relative to that to
  //   (significantly) increase the speed of the spellcheck

  for (cnt = 0, line = 0 ; line <= lastLine() && cnt <= pos ; line++)
    cnt += getTextLine(line)->length()+1;

  // Highlight the mispelled word
  KateTextCursor cursor;
  line--;
  cursor.col = pos - (cnt - getTextLine(line)->length()) + 1;
  cursor.line = line;
//  deselectAll(); // shouldn't the spell check be allowed within selected text?
  kspell.kspellMispellCount++;

  KateView *view;
  VConfig c;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->myViewInternal->updateCursor(cursor); //this does deselectAll() if no persistent selections
    view->myViewInternal->getVConfig(c);
  }

  selectLength(cursor,origword.length(),c.flags);
  updateViews();
}

void KateDocument::corrected (QString originalword, QString newword, unsigned pos)
{
  uint line;
  uint cnt=0;

  if(newword != originalword)
  {
      // Find pos
      for (line = 0 ; line <= lastLine() && cnt <= pos ; line++)
        cnt += getTextLine(line)->length() + 1;

      // Highlight the mispelled word
      KateTextCursor cursor;
      line--;
      cursor.col = pos - (cnt-getTextLine(line)->length()) + 1;
      cursor.line = line;

      KateView *view;
      VConfig c;
       for (view = myViews.first(); view != 0L; view = myViews.next() )
      {
        view->myViewInternal->updateCursor(cursor); //this does deselectAll() if no persistent selections
        view->myViewInternal->getVConfig(c);
      }

      selectLength(cursor, newword.length(),c.flags);

      removeText (s.cursor.line, s.cursor.col, s.cursor.line, s.cursor.col + originalword.length());
      insertText (s.cursor.line, s.cursor.col, newword);

      kspell.kspellReplaceCount++;
    }

}

void KateDocument::spellResult (const QString &)
{
  clearSelection ();

  // we know if the check was cancelled
  // we can safely use the undo mechanism to backout changes
  // in case of a cancel, because we force the entire spell check
  // into one group (record)
  if (kspell.kspell->dlgResult() == 0) {
    if (kspell.kspellReplaceCount)
    {
      undo();
      // clear the redo list, so the cancelled spell check can't be redo'ed <- say that word ;-)
      clearRedo();
      // make sure the modified flag is turned back off
      // if we started with a clean buffer
      if (kspell.kspellPristine)
        setModified(false);
    }
  }

  setPseudoModal(0L);
  setReadWrite (true);

  updateViews();

  kspell.kspell->cleanUp();
}

void KateDocument::spellCleanDone ()
{
  KSpell::spellStatus status = kspell.kspell->status();
  kspell.spell_tmptext = "";
  delete kspell.kspell;

  kspell.kspell = 0;
  kspell.kspellon = FALSE;

  if (status == KSpell::Error)
  {
     KMessageBox::sorry(kapp->mainWidget(), i18n("ISpell could not be started.\n"
     "Please make sure you have ISpell properly configured and in your PATH."));
  }
  else if (status == KSpell::Crashed)
  {
     setPseudoModal(0L);
     setReadWrite (true);

     updateViews();
     KMessageBox::sorry(kapp->mainWidget(), i18n("ISpell seems to have crashed."));
  }
  else
  {
     emit spellcheck_done();
  }
}

void KateDocument::makeAttribs()
{
  hlManager->makeAttribs(this, m_highlight);
  updateFontData();
  updateLines();
}

void KateDocument::updateFontData() {
  KateView *view;

  for (view = myViews.first(); view != 0L; view = myViews.next() ) {
    view->myViewInternal->drawBuffer->resize(view->width(),viewFont.fontHeight);
    view->tagAll();
    view->updateCursor();
  }
}

void KateDocument::internalHlChanged() { //slot
  makeAttribs();
  updateViews();
}

void KateDocument::addView(KTextEditor::View *view) {
  myViews.append( static_cast<KateView *>( view ) );
  _views.append( view );
}

void KateDocument::removeView(KTextEditor::View *view) {
  myViews.removeRef( static_cast<KateView *>( view ) );
  _views.removeRef( view  );
}

void KateDocument::addCursor(KTextEditor::Cursor *cursor) {
  myCursors.append( cursor );
}

void KateDocument::removeCursor(KTextEditor::Cursor *cursor) {
  myCursors.removeRef( cursor  );
}

bool KateDocument::ownedView(KateView *view) {
  // do we own the given view?
  return (myViews.containsRef(view) > 0);
}

bool KateDocument::isLastView(int numViews) {
  return ((int) myViews.count() == numViews);
}

int KateDocument::charWidth(const TextLine::Ptr &textLine, int cursorX,WhichFont wf) {
  QChar ch = textLine->getChar(cursorX);
  Attribute *a = attribute(textLine->getAttr(cursorX));
  int x;
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  if (ch == '\t')
    x = fs->m_tabWidth - (textWidth(textLine, cursorX) % fs->m_tabWidth);
  else if (a->bold && a->italic)
    x = fs->myFontMetricsBI.width(ch);
  else if (a->bold)
    x = fs->myFontMetricsBold.width(ch);
  else if (a->italic)
    x = fs->myFontMetricsItalic.width(ch);
  else
    x = fs->myFontMetrics.width(ch);

  return x;
}

int KateDocument::charWidth(KateTextCursor &cursor) {
  if (cursor.col < 0)
     cursor.col = 0;
  if (cursor.line < 0)
     cursor.line = 0;
  if (cursor.line >= (int) numLines())
     cursor.line = lastLine();
  return charWidth(getTextLine(cursor.line),cursor.col);
}

uint KateDocument::textWidth(const TextLine::Ptr &textLine, int cursorX,WhichFont wf)
{
  int x;
  int z;
  QChar ch;
  Attribute *a;
  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  x = 0;
  for (z = 0; z < cursorX; z++) {
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth - (x % fs->m_tabWidth);
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);
  }
  return x;
}

uint KateDocument::textWidth(KateTextCursor &cursor)
{
  if (cursor.col < 0)
     cursor.col = 0;
  if (cursor.line < 0)
     cursor.line = 0;
  if (cursor.line >= (int)numLines())
     cursor.line = lastLine();
  return textWidth(getTextLine(cursor.line),cursor.col);
}

uint KateDocument::textWidth(bool wrapCursor, KateTextCursor &cursor, int xPos,WhichFont wf)
{
  int len;
  int x, oldX;
  int z;
  QChar ch;
  Attribute *a;

  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  if (cursor.line < 0) cursor.line = 0;
  if (cursor.line > (int)lastLine()) cursor.line = lastLine();
  TextLine::Ptr textLine = getTextLine(cursor.line);
  len = textLine->length();

  x = oldX = z = 0;
  while (x < xPos && (!wrapCursor || z < len)) {
    oldX = x;
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth - (x % fs->m_tabWidth);
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
    x = oldX;
  }
  cursor.col = z;
  return x;
}

uint KateDocument::textPos(const TextLine::Ptr &textLine, int xPos,WhichFont wf) {
  int x, oldX;
  int z;
  QChar ch;
  Attribute *a;

  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  x = oldX = z = 0;
  while (x < xPos) { // && z < len) {
    oldX = x;
    ch = textLine->getChar(z);
    a = attribute(textLine->getAttr(z));

    if (ch == '\t')
      x += fs->m_tabWidth - (x % fs->m_tabWidth);
    else if (a->bold && a->italic)
      x += fs->myFontMetricsBI.width(ch);
    else if (a->bold)
      x += fs->myFontMetricsBold.width(ch);
    else if (a->italic)
      x += fs->myFontMetricsItalic.width(ch);
    else
      x += fs->myFontMetrics.width(ch);

    z++;
  }
  if (xPos - oldX < x - xPos && z > 0) {
    z--;
   // newXPos = oldX;
  }// else newXPos = x;
  return z;
}

uint KateDocument::textWidth() {
  return (int) maxLength + 8;
}

uint KateDocument::textHeight(WhichFont wf) {
  return numLines()*((wf==ViewFont)?viewFont.fontHeight:printFont.fontHeight);
}

uint KateDocument::currentColumn(KateTextCursor &cursor)
{
  TextLine::Ptr t = getTextLine(cursor.line);

  if (t)
    return t->cursorX(cursor.col,tabChars);
  else
    return 0;
}

bool KateDocument::insertChars ( int line, int col, const QString &chars, KateView *view )
{
  int z, pos, l;
  bool onlySpaces;
  QChar ch;
  QString buf;

  TextLine::Ptr textLine = getTextLine(line);

  pos = 0;
  onlySpaces = true;
  for (z = 0; z < (int) chars.length(); z++) {
    ch = chars[z];
    if (ch == '\t' && _configFlags & KateDocument::cfReplaceTabs) {
      l = tabChars - (textLine->cursorX(col, tabChars) % tabChars);
      while (l > 0) {
        buf.insert(pos, ' ');
        pos++;
        l--;
      }
    } else if (ch.isPrint() || ch == '\t') {
      buf.insert(pos, ch);
      pos++;
      if (ch != ' ') onlySpaces = false;
      if (_configFlags & KateDocument::cfAutoBrackets) {
        if (ch == '(') buf.insert(pos, ')');
        if (ch == '[') buf.insert(pos, ']');
        if (ch == '{') buf.insert(pos, '}');
      }
    }
  }
  //pos = cursor increment

  //return false if nothing has to be inserted
  if (buf.isEmpty()) return false;

  bool b = editStart ();

  if (_configFlags & KateDocument::cfDelOnInput)
  {
    if (hasSelection())
    {
      removeSelectedText();
      line = view->cursorCache.line;
      col = view->cursorCache.col;
    }
  }

  if (_configFlags & KateDocument::cfOvr)
  {
    if ((col+buf.length()) <= textLine->length())
      removeText (line, col, line, col+buf.length());
    else
      removeText (line, col, line, textLine->length());
  }

  insertText (line, col, buf);
  col += pos;

  if (b)
    editEnd ();

  KateTextCursor c;
  c.line = line;
  c.col = col;

  view->updateCursor(c);

/*
  if (myWordWrap && myWordWrapAt > 0) {
    int line;
    const QChar *s;
//    int pos;
    KateTextCursor actionCursor;

    line = c.cursor.line;
    do {
      textLine = getTextLine(line);
      s = textLine->getText();
      l = textLine->length();
      for (z = myWordWrapAt; z < l; z++) if (!s[z].isSpace()) break; //search for text to wrap
      if (z >= l) break; // nothing more to wrap
      pos = myWordWrapAt;
      for (; z >= 0; z--) { //find wrap position
        if (s[z].isSpace()) {
          pos = z + 1;
          break;
        }
      }
      //pos = wrap position

      if (line == c.cursor.line && pos <= c.cursor.col) {
        //wrap cursor
        c.cursor.line++;
        c.cursor.col -= pos;
      }

      if (line == lastLine() || (getTextLine(line+1)->length() == 0) ) {
        //at end of doc: create new line
        actionCursor.col = pos;
        actionCursor.line = line;
        recordAction(KateUndo::newLine,actionCursor);
      } else {
        //wrap
        actionCursor.line = line + 1;
        if (!s[l - 1].isSpace()) { //add space in next line if necessary
          actionCursor.col = 0;
          recordInsert(actionCursor, " ");
        }
        actionCursor.col = textLine->length() - pos;
        recordAction(KateUndo::wordWrap, actionCursor);
      }
      line++;
    } while (true);
  } */

  return true;
}

QString tabString(int pos, int tabChars)
{
  QString s;
  while (pos >= tabChars) {
    s += '\t';
    pos -= tabChars;
  }
  while (pos > 0) {
    s += ' ';
    pos--;
  }
  return s;
}

void KateDocument::newLine(VConfig &c)
{

  if (!(_configFlags & KateDocument::cfAutoIndent)) {
    insertText (c.cursor.line, c.cursor.col, "\n");
    c.cursor.line++;
    c.cursor.col = 0;
  } else {
    TextLine::Ptr textLine = getTextLine(c.cursor.line);
    int pos = textLine->firstChar();
    if (c.cursor.col < pos) c.cursor.col = pos; // place cursor on first char if before

    int y = c.cursor.line;
    while ((y > 0) && (pos < 0)) { // search a not empty text line
      textLine = getTextLine(--y);
      pos = textLine->firstChar();
    }
    insertText (c.cursor.line, c.cursor.col, "\n");
    c.cursor.line++;
    c.cursor.col = 0;
    if (pos > 0) {
      pos = textLine->cursorX(pos, tabChars);
//      if (getTextLine(c.cursor.line)->length() > 0) {
        QString s = tabString(pos, (_configFlags & KateDocument::cfSpaceIndent) ? 0xffffff : tabChars);
        insertText (c.cursor.line, c.cursor.col, s);
        pos = s.length();
//      }
//      recordInsert(c.cursor, QString(textLine->getText(), pos));
      c.cursor.col = pos;
    }
  }

  c.view->updateCursor(c.cursor);
}

void KateDocument::killLine(VConfig &c)
{
  removeLine (c.cursor.line);
}

void KateDocument::backspace(uint line, uint col)
{
  if ((col == 0) && (line == 0))
    return;

  if (col > 0)
  {
    if (!(_configFlags & KateDocument::cfBackspaceIndents))
    {
      // ordinary backspace
      //c.cursor.col--;
      removeText(line, col-1, line, col);
    }
    else
    {
      // backspace indents: erase to next indent position
      int l = 1; // del one char

      TextLine::Ptr textLine = getTextLine(line);
      int pos = textLine->firstChar();
      if (pos < 0 || pos >= (int)col)
      {
        // only spaces on left side of cursor
        // search a line with less spaces
        uint y = line;
        while (y > 0)
        {
          textLine = getTextLine(--y);
          pos = textLine->firstChar();

          if (pos >= 0 && pos < (int)col)
          {
            l = col - pos; // del more chars
            break;
          }
        }
      }
      // break effectively jumps here
      //c.cursor.col -= l;
      removeText(line, col-1, line, col);
    }
  }
  else
  {
    // col == 0: wrap to previous line
    if ((line - 1) >= 0)
      removeText (line-1, getTextLine(line-1)->length(), line, 0);
  }
}

void KateDocument::del(VConfig &c)
{
  if (c.cursor.col < (int) getTextLine(c.cursor.line)->length())
  {
    removeText(c.cursor.line, c.cursor.col, c.cursor.line, c.cursor.col+1);
  }
  else
  {
    removeText(c.cursor.line, c.cursor.col, c.cursor.line+1, 0);
  }
}

void KateDocument::cut(VConfig &)
{
  if (!hasSelection())
    return;

  copy(_configFlags);
  removeSelectedText();
}

void KateDocument::copy(int )
{
  if (!hasSelection()) return;

  if (_configFlags & KateDocument::cfSingleSelection)
    disconnect(QApplication::clipboard(), SIGNAL(dataChanged()), this, 0);

  QApplication::clipboard()->setText(selection ());

  if (_configFlags & KateDocument::cfSingleSelection)
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));
}

void KateDocument::paste (VConfig &c)
{
  QString s = QApplication::clipboard()->text();

  if (!s.isEmpty())
  {
    if (!blockSelect)
      insertText(c.cursor.line, c.cursor.col, s);
    else
    {
      insertText(c.cursor.line, c.cursor.col, s);
    }
  }
}

void KateDocument::selectTo(VConfig &c, KateTextCursor &cursor, int )
{
  if (selectAnchor.line == -1)
  {
    selectAnchor.line = c.cursor.line;
    selectAnchor.col = c.cursor.col;
  }

  setSelection (selectAnchor.line, selectAnchor.col, cursor.line, cursor.col);
}

void KateDocument::selectWord(KateTextCursor &cursor, int flags) {
  int start, end, len;

  TextLine::Ptr textLine = getTextLine(cursor.line);
  len = textLine->length();
  start = end = cursor.col;
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  if (end <= start) return;
  if (!(flags & KateDocument::cfKeepSelection)) clearSelection ();

  setSelection (cursor.line, start, cursor.line, end);
}

void KateDocument::selectLength(KateTextCursor &cursor, int length, int flags) {
  int start, end;

  TextLine::Ptr textLine = getTextLine(cursor.line);
  start = cursor.col;
  end = start + length;
  if (end <= start) return;
  if (!(flags & KateDocument::cfKeepSelection)) clearSelection ();

  setSelection (cursor.line, start, cursor.line, end);
}

void KateDocument::doIndent(VConfig &c, int change)
{
  c.cursor.col = 0;

  if (!hasSelection()) {
    // single line
    optimizeLeadingSpace(c.cursor.line, _configFlags, change);
  } else {
    // entire selection
    TextLine::Ptr textLine;
    int line, z;
    QChar ch;

    if (_configFlags & KateDocument::cfKeepIndentProfile && change < 0) {
      // unindent so that the existing indent profile doesnt get screwed
      // if any line we may unindent is already full left, don't do anything
      for (line = selectStart.line; line <= selectEnd.line; line++) {
        textLine = getTextLine(line);
        if (lineSelected(line) || lineHasSelected(line)) {
          for (z = 0; z < tabChars; z++) {
            ch = textLine->getChar(z);
            if (ch == '\t') break;
            if (ch != ' ') {
              change = 0;
              goto jumpOut;
            }
          }
        }
      }
      jumpOut:;
    }

    for (line = selectStart.line; line <= selectEnd.line; line++) {
      if (lineSelected(line) || lineHasSelected(line)) {
        optimizeLeadingSpace(line, _configFlags, change);
      }
    }
  }
}

/*
  Optimize the leading whitespace for a single line.
  If change is > 0, it adds indentation units (tabChars)
  if change is == 0, it only optimizes
  If change is < 0, it removes indentation units
  This will be used to indent, unindent, and optimal-fill a line.
  If excess space is removed depends on the flag cfKeepExtraSpaces
  which has to be set by the user
*/
void KateDocument::optimizeLeadingSpace(int line, int flags, int change) {
  int len;
  int chars, space, okLen;
  QChar ch;
  int extra;
  QString s;
  KateTextCursor cursor;

  TextLine::Ptr textLine = getTextLine(line);
  len = textLine->length();
  space = 0; // length of space at the beginning of the textline
  okLen = 0; // length of space which does not have to be replaced
  for (chars = 0; chars < len; chars++) {
    ch = textLine->getChar(chars);
    if (ch == ' ') {
      space++;
      if (flags & KateDocument::cfSpaceIndent && okLen == chars) okLen++;
    } else if (ch == '\t') {
      space += tabChars - space % tabChars;
      if (!(flags & KateDocument::cfSpaceIndent) && okLen == chars) okLen++;
    } else break;
  }

  space += change*tabChars; // modify space width
  // if line contains only spaces it will be cleared
  if (space < 0 || chars == len) space = 0;

  extra = space % tabChars; // extra spaces which dont fit the indentation pattern
  if (flags & KateDocument::cfKeepExtraSpaces) chars -= extra;

  if (flags & KateDocument::cfSpaceIndent) {
    space -= extra;
    ch = ' ';
  } else {
    space /= tabChars;
    ch = '\t';
  }

  // dont replace chars which are already ok
  cursor.col = QMIN(okLen, QMIN(chars, space));
  chars -= cursor.col;
  space -= cursor.col;
  if (chars == 0 && space == 0) return; //nothing to do

  s.fill(ch, space);

//printf("chars %d insert %d cursor.col %d\n", chars, insert, cursor.col);
  cursor.line = line;
  removeText (cursor.line, cursor.col, cursor.line, cursor.col+chars);
  insertText(cursor.line, cursor.col, s);
}

/*
  Remove a given string at the begining
  of the current line.
*/
bool KateDocument::removeStringFromBegining(int line, QString &str)
{
  TextLine* textline = getTextLine(line);

  if(textline->startingWith(str))
  {
    // Get string lenght
    int length = str.length();

    // Remove some chars
    removeText (line, 0, line, length);

    return true;
  }

  return false;
}

/*
  Remove a given string at the end
  of the current line.
*/
bool KateDocument::removeStringFromEnd(int line, QString &str)
{
  TextLine* textline = getTextLine(line);

  if(textline->endingWith(str))
  {
    // Get string lenght
    int length = str.length();

    // Remove some chars
    removeText (line, 0, line, length);

    return true;
  }

  return false;
}

/*
  Add to the current line a comment line mark at
  the begining.
*/
void KateDocument::addStartLineCommentToSingleLine(int line)
{
  QString commentLineMark = m_highlight->getCommentSingleLineStart() + " ";
  insertText (line, 0, commentLineMark);
}

/*
  Remove from the current line a comment line mark at
  the begining if there is one.
*/
bool KateDocument::removeStartLineCommentFromSingleLine(int line)
{
  QString shortCommentMark = m_highlight->getCommentSingleLineStart();
  QString longCommentMark = shortCommentMark + " ";

  // Try to remove the long comment mark first
  bool removed = (removeStringFromBegining(line, longCommentMark)
                  || removeStringFromBegining(line, shortCommentMark));

  return removed;
}

/*
  Add to the current line a start comment mark at the
 begining and a stop comment mark at the end.
*/
void KateDocument::addStartStopCommentToSingleLine(int line)
{
  QString startCommentMark = m_highlight->getCommentStart() + " ";
  QString stopCommentMark = " " + m_highlight->getCommentEnd();

  // Add the start comment mark
  insertText (line, 0, startCommentMark);

  // Go to the end of the line
  TextLine* textline = getTextLine(line);
  int col = textline->length();

  // Add the stop comment mark
  insertText (line, col, stopCommentMark);
}

/*
  Remove from the current line a start comment mark at
  the begining and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSingleLine(int line)
{
  QString shortStartCommentMark = m_highlight->getCommentStart();
  QString longStartCommentMark = shortStartCommentMark + " ";
  QString shortStopCommentMark = m_highlight->getCommentEnd();
  QString longStopCommentMark = " " + shortStopCommentMark;

  // Try to remove the long start comment mark first
  bool removedStart = (removeStringFromBegining(line, longStartCommentMark)
                       || removeStringFromBegining(line, shortStartCommentMark));

  // Try to remove the long stop comment mark first
  bool removedStop = (removeStringFromEnd(line, longStopCommentMark)
                      || removeStringFromEnd(line, shortStopCommentMark));

  return (removedStart || removedStop);
}

/*
  Add to the current selection a start comment
 mark at the begining and a stop comment mark
 at the end.
*/
void KateDocument::addStartStopCommentToSelection()
{
  QString startComment = m_highlight->getCommentStart();
  QString endComment = m_highlight->getCommentEnd();

  int sl = selectStart.line;
  int el = selectEnd.line;
  int sc = selectStart.col;
  int ec = selectEnd.col;

  insertText (el, ec, endComment);
  insertText (sl, sc, startComment);
}

/*
  Add to the current selection a comment line
 mark at the begining of each line.
*/
void KateDocument::addStartLineCommentToSelection()
{
  QString commentLineMark = m_highlight->getCommentSingleLineStart() + " ";

  int sl = selectStart.line;
  int el = selectEnd.line;

  // For each line of the selection
  for (int z = el; z >= sl; z--)
    insertText (z, 0, commentLineMark);
}

/*
  Remove from the selection a start comment mark at
  the begining and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSelection()
{
  QString startComment = m_highlight->getCommentStart();
  QString endComment = m_highlight->getCommentEnd();

  int sl = selectStart.line;
  int el = selectEnd.line;
  int sc = selectStart.col;
  int ec = selectEnd.col;

  int startCommentLen = startComment.length();
  int endCommentLen = endComment.length();

  if (ec-endCommentLen < 0)
    return false;

  removeText (el, ec-endCommentLen, el, ec);
  removeText (sl, sc, sl, sc+startCommentLen);

  return true;
}

/*
  Remove from the begining of each line of the
  selection a start comment line mark.
*/
bool KateDocument::removeStartLineCommentFromSelection()
{
  QString shortCommentMark = m_highlight->getCommentSingleLineStart();
  QString longCommentMark = shortCommentMark + " ";

  int sl = selectStart.line;
  int el = selectEnd.line;

  bool removed = false;

  // For each line of the selection
  for (int z = el; z >= sl; z--)
  {
    // Try to remove the long comment mark first
    removed = (removeStringFromBegining(z, longCommentMark)
                 || removeStringFromBegining(z, shortCommentMark)
                 || removed);
  }

  return removed;
}

/*
  Comment or uncomment the selection or the current
  line if there is no selection.
*/
void KateDocument::doComment(VConfig &c, int change)
{
  bool hasStartLineCommentMark = !(m_highlight->getCommentSingleLineStart().isEmpty());
  bool hasStartStopCommentMark = ( !(m_highlight->getCommentStart().isEmpty())
                                   && !(m_highlight->getCommentEnd().isEmpty()) );

  bool removed = false;

  if (change > 0)
  {
    if ( !hasSelection() )
    {
      if ( hasStartLineCommentMark )
        addStartLineCommentToSingleLine(c.cursor.line);
      else if ( hasStartStopCommentMark )
        addStartStopCommentToSingleLine(c.cursor.line);
    }
    else
    {
      if ( hasStartStopCommentMark )
        addStartStopCommentToSelection();
      else if ( hasStartLineCommentMark )
        addStartLineCommentToSelection();
    }
  }
  else
  {
    if ( !hasSelection() )
    {
      removed = ( hasStartLineCommentMark
                  && removeStartLineCommentFromSingleLine(c.cursor.line) )
        || ( hasStartStopCommentMark
             && removeStartStopCommentFromSingleLine(c.cursor.line) );
    }
    else
    {
      removed = ( hasStartStopCommentMark
                  && removeStartStopCommentFromSelection() )
        || ( hasStartLineCommentMark
             && removeStartLineCommentFromSelection() );
    }
  }
}

QString KateDocument::getWord(KateTextCursor &cursor) {
  int start, end, len;

  TextLine::Ptr textLine = getTextLine(cursor.line);
  len = textLine->length();
  start = end = cursor.col;
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;
  len = end - start;
  return QString(&textLine->getText()[start], len);
}

void KateDocument::tagLineRange(int line, int x1, int x2)
{
  for (uint z = 0; z < myViews.count(); z++)
    myViews.at(z)->tagLines(line, line, x1, x2);
}

void KateDocument::tagLines(int start, int end)
{
  for (uint z = 0; z < myViews.count(); z++)
    myViews.at(z)->tagLines(start, end, 0, 0xffffff);
}

void KateDocument::tagAll()
{
  for (uint z = 0; z < myViews.count(); z++)
    myViews.at(z)->tagAll();
}

void KateDocument::updateLines(int startLine, int endLine)
{
  TextLine::Ptr textLine;
  uint line, last_line, ctxNumLen, endCtxLen;
  signed char *ctxNum, *endCtx;

  ctxNum = 0L;
  endCtx = 0L;

  ctxNumLen = 0;
  endCtxLen = 0;

  if (!buffer->line(startLine))
    return;

  last_line = lastLine();

  line = startLine;

  if (line > 0)
  {
    ctxNumLen = getTextLine(line-1)->getContextLength();
    if (ctxNumLen>0)
    {
      if (ctxNum)
        ctxNum=(signed char*)realloc(ctxNum,ctxNumLen);
      else
        ctxNum=(signed char*)malloc(ctxNumLen);
      memcpy(ctxNum,getTextLine(line-1)->getContext(),ctxNumLen);
    }
    else { if (ctxNum) {free(ctxNum); ctxNum=0;}}

  }

  bool stillcontinue=false;

  do
  {
    textLine = getTextLine(line);

    if (!textLine)
      break;

    endCtxLen = textLine->getContextLength();

    if (endCtxLen>0)
    {
      if (endCtx)
        endCtx=(signed char*)realloc(endCtx,endCtxLen);
      else
        endCtx=(signed char*)malloc(endCtxLen);
      memcpy(endCtx,textLine->getContext(),endCtxLen);
    }
    else { if (endCtx) {free(endCtx); endCtx=0;}}

    m_highlight->doHighlight(ctxNum, ctxNumLen, textLine);

    ctxNumLen = textLine->getContextLength();
    if (ctxNumLen>0)
    {
      if (ctxNum)
        ctxNum=(signed char*)realloc(ctxNum,ctxNumLen);
      else
        ctxNum=(signed char*)malloc(ctxNumLen);
      memcpy(ctxNum,textLine->getContext(),ctxNumLen);
    }
    else { if (ctxNum) {free(ctxNum); ctxNum=0;}}

    if (endCtxLen != ctxNumLen)
      stillcontinue = true;
    else
    {
      stillcontinue = false;
      for (uint z=0; z < ctxNumLen; z++)
      {
        if (ctxNum[z] != endCtx[z])
        {
          stillcontinue = true;
          break;
        }
      }
    }

    line++;
    //kdDebug()<<QString("Next line: %1, last_line %2, endLine %3").arg(line).arg(last_line).arg(endLine)<<endl;
  }
  while ((line <= last_line) && (((int)line <= endLine) || stillcontinue));

  if (ctxNum)
    delete [] ctxNum;

  if (endCtx)
    delete [] endCtx;

  tagLines(startLine, line - 1);
}

void KateDocument::updateMaxLength(TextLine::Ptr &textLine)
{
  int len;

  len = textWidth(textLine,textLine->length());

  if (len > maxLength) {
    longestLine = textLine;
    maxLength = len;
    newDocGeometry = true;
  } else {
    if (!longestLine || (textLine == longestLine && len <= maxLength*3/4)) {
      maxLength = -1;
      for (uint i = 0; i < numLines();i++) {
        textLine = getTextLine(i);
        len = textWidth(textLine,textLine->length());
        if (len > maxLength) {
          maxLength = len;
          longestLine = textLine;
        }
      }
      newDocGeometry = true;
    }
  }
}

void KateDocument::slotBufferChanged() {
  newDocGeometry = true;
  //updateLines();//JW
  updateViews();
}

void KateDocument::slotBufferHighlight(uint start,uint stop) {
  //kdDebug(13020)<<"KateDocument::slotBufferHighlight"<<QString("%1-%2").arg(start).arg(stop)<<endl;
  updateLines(start,stop);
//  buffer->startLoadTimer();
}

void KateDocument::updateViews(KateView *exclude)
{
  KateView *view;
  int flags;

  flags = (newDocGeometry) ? KateView::ufDocGeometry : 0;

  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->updateView(flags);
  }

  newDocGeometry = false;
}

QColor &KateDocument::backCol(int x, int y) {
  return (lineColSelected(x,y)) ? colors[1] : colors[0];
 }

QColor &KateDocument::cursorCol(int x, int y)
{
  TextLine::Ptr textLine = getTextLine(y);
  Attribute *a = attribute(textLine->getAttr(x));

  if (lineColSelected (y, x))
    return a->selCol;
  else
    return a->col;
}

bool KateDocument::lineColSelected (int line, int col)
{
  if (!blockSelect)
  {
    if ((line > selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectStart.line) && (col >= selectStart.col) && (line < selectEnd.line))
      return true;

    if ((line == selectEnd.line) && (col < selectEnd.col) && (line > selectStart.line))
      return true;

    if ((line == selectEnd.line) && (col < selectEnd.col) && (line == selectStart.line) && (col >= selectStart.col))
      return true;

    if ((line == selectStart.line) && (selectStart.col == 0) && (col < 0))
      return true;
  }
  else
  {
    if ((line >= selectStart.line) && (line <= selectEnd.line) && (col >= selectStart.col) && (col < selectEnd.col))
      return true;
  }

  return false;
}

bool KateDocument::lineSelected (int line)
{
  if (!blockSelect)
  {
    if ((line > selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectStart.line) && (line < selectEnd.line) && (selectStart.col == 0))
      return true;
  }

  return false;
}

bool KateDocument::lineEndSelected (int line)
{
  if (!blockSelect)
  {
    if ((line >= selectStart.line) && (line < selectEnd.line))
      return true;
  }

  return false;
}

bool KateDocument::lineHasSelected (int line)
{
  if (!blockSelect)
  {
    if ((line > selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectStart.line) && (line < selectEnd.line))
      return true;

    if ((line == selectEnd.line) && (line > selectStart.line))
      return true;

    if ((line == selectEnd.line) && (line == selectStart.line) && (selectEnd.col > selectStart.col))
      return true;
  }
  else
  {
    if ((line <= selectEnd.line) && (line >= selectStart.line) && (selectEnd.col > selectStart.col))
      return true;
  }

  return false;
}

void KateDocument::paintTextLine(QPainter &paint, uint line, int xStart, int xEnd, bool showTabs)
{
  paintTextLine (paint, line, 0, xStart, xEnd, showTabs);
}

void KateDocument::paintTextLine(QPainter &paint, uint line, int y, int xStart, int xEnd, bool showTabs,WhichFont wf)
{
  TextLine::Ptr textLine;
  int len;
  const QChar *s;
  int z, x;
  QChar ch;
  Attribute *a = 0;
  int attr, nextAttr;
  int xs;
  int xc, zc;

  FontStruct *fs=(wf==ViewFont)?&viewFont:&printFont;

  if (line > lastLine())
  {
    paint.fillRect(0, y, xEnd - xStart,fs->fontHeight, colors[0]);
    return;
  }

  textLine = getTextLine(line);

  if (!textLine)
    return;

  len = textLine->length();
  s = textLine->getText();

  // skip to first visible character
  x = 0;
  z = 0;
  do
  {
    xc = x;
    zc = z;

    if (z == len) break;

    ch = s[z];

    if (ch == '\t')
      x += fs->m_tabWidth - (x % fs->m_tabWidth);
    else
    {
      a = attribute(textLine->getAttr(z));

      if (a->bold && a->italic)
       x += fs->myFontMetricsBI.width(ch);
      else if (a->bold)
        x += fs->myFontMetricsBold.width(ch);
      else if (a->italic)
        x += fs->myFontMetricsItalic.width(ch);
      else
        x += fs->myFontMetrics.width(ch);
    }
    z++;
  }
  while (x <= xStart);

  // draw background
  xs = xStart;
  int col = zc;
  while (x < xEnd)
  {
    if (lineColSelected(line, col))
      paint.fillRect(xs - xStart, y, x - xs, fs->fontHeight, colors[1]);
    else
      paint.fillRect(xs - xStart, y, x - xs, fs->fontHeight, colors[0]);

    xs = x;
    col = z;

    attr = textLine->getAttr(z);

    if (z == len) break;

    ch = s[z];

    if (ch == QChar('\t'))
      x += fs->m_tabWidth - (x % fs->m_tabWidth);
    else
    {
      a = attribute(textLine->getAttr(z));

      if (a->bold && a->italic)
        x += fs->myFontMetricsBI.width(ch);
      else if (a->bold)
        x += fs->myFontMetricsBold.width(ch);
      else if (a->italic)
        x += fs->myFontMetricsItalic.width(ch);
      else
        x += fs->myFontMetrics.width(ch);
    }
    z++;
  }

  // is whole line selected ??
  if (lineEndSelected(line))
    paint.fillRect(xs - xStart, y, xEnd - xs, fs->fontHeight, colors[1]);
  else
    paint.fillRect(xs - xStart, y, xEnd - xs, fs->fontHeight, colors[0]);

  //reduce length to visible length
  len = z;

  // draw text
  x = xc;
  z = zc;
  y += fs->fontAscent;// -1;
  attr = -1;

  while (z < len)
  {
    ch = s[z];

    if (ch == QChar('\t'))
    {
      if (z > zc)
      {
        QConstString str((QChar *) &s[zc], z - zc /*+1*/);
        QString s = str.string();
        paint.drawText(x - xStart, y, s);

         if (a->bold && a->italic)
          x += fs->myFontMetricsBI.width(s);
        else if (a->bold)
          x += fs->myFontMetricsBold.width(s);
        else if (a->italic)
          x += fs->myFontMetricsItalic.width(s);
        else
          x += fs->myFontMetrics.width(s);
      }
      zc = z +1;

      if (showTabs)
      {
        nextAttr = textLine->getAttr(z);

          if ((nextAttr != attr) || (lineColSelected(line, col) != lineColSelected(line, z)))
        {
          attr = nextAttr;
          a = attribute (attr);
          col = z;

          if (lineColSelected(line, col))
            paint.setPen(a->selCol);
          else
            paint.setPen(a->col);

            if (a->bold && a->italic)
            paint.setFont(fs->myFontBI);
            else if (a->bold)
            paint.setFont(fs->myFontBold);
            else if (a->italic)
            paint.setFont(fs->myFontItalic);
            else
            paint.setFont(fs->myFont);
        }
        paint.drawPoint(x - xStart, y);
        paint.drawPoint(x - xStart +1, y);
        paint.drawPoint(x - xStart, y -1);
      }
      x += fs->m_tabWidth - (x % fs->m_tabWidth);
    }
    else
    {
      nextAttr = textLine->getAttr(z);

      if ((nextAttr != attr) || (lineColSelected(line, col) != lineColSelected(line, z)))
      {
        if (z > zc)
        {
          QConstString str((QChar *) &s[zc], z - zc /*+1*/);
          QString s = str.string();
          paint.drawText(x - xStart, y, s);

          if (a->bold && a->italic)
            x += fs->myFontMetricsBI.width(s);
          else if (a->bold)
            x += fs->myFontMetricsBold.width(s);
        else if (a->italic)
            x += fs->myFontMetricsItalic.width(s);
         else
            x += fs->myFontMetrics.width(s);
          zc = z;
        }

        attr = nextAttr;
        a = attribute (attr);
        col = z;

        if (lineColSelected(line, col))
          paint.setPen(a->selCol);
        else
          paint.setPen(a->col);

        if (a->bold && a->italic)
         paint.setFont(fs->myFontBI);
        else if (a->bold)
          paint.setFont(fs->myFontBold);
        else if (a->italic)
          paint.setFont(fs->myFontItalic);
        else
          paint.setFont(fs->myFont);
      }
    }

    z++;
  }

  if (z > zc)
  {
    QConstString str((QChar *) &s[zc], z - zc);
    paint.drawText(x - xStart, y, str.string());
  }
}

// Applies the search context, and returns whether a match was found. If one is,
// the length of the string matched is also returned.
bool KateDocument::doSearch(SConfig &sc, const QString &searchFor) {
  int line, col;
  int searchEnd;
  int bufLen, tlen;
  QChar *t;
  TextLine::Ptr textLine;

  if (searchFor.isEmpty()) return false;

  bufLen = 0;
  t = 0L;

  line = sc.cursor.line;
  col = sc.cursor.col;
  if (!(sc.flags & KateDocument::sfBackward)) {
    //forward search
    if (sc.flags & KateDocument::sfSelected) {
      if (line < selectStart.line) {
        line = selectStart.line;
        col = 0;
      }
      searchEnd = selectEnd.line;
    } else searchEnd = lastLine();

    while (line <= searchEnd) {
      textLine = getTextLine(line);
      tlen = textLine->length();
      if (tlen > bufLen) {
        delete t;
        bufLen = (tlen + 255) & (~255);
        t = new QChar[bufLen];
      }
      memcpy(t, textLine->getText(), tlen*sizeof(QChar));
      /*if (sc.flags & KateDocument::sfSelected) {
        pos = 0;
        do {
          pos = textLine->findSelected(pos);
          newPos = textLine->findUnselected(pos);
          memset(&t[pos], 0, (newPos - pos)*sizeof(QChar));
          pos = newPos;
        } while (pos < tlen);
      }*/

      QString text(t, tlen);
      if (sc.flags & KateDocument::sfWholeWords) {
        // Until the end of the line...
        while (col < tlen) {
          // ...find the next match.
          col = sc.search(text, col);
          if (col != -1) {
            // Is the match delimited correctly?
            if (((col == 0) || (!m_highlight->isInWord(t[col]))) &&
              ((col + sc.matchedLength == tlen) || (!m_highlight->isInWord(t[col + sc.matchedLength])))) {
              goto found;
            }
            else {
              // Start again from the next character.
              col++;
            }
          }
          else {
            // No match.
            break;
          }
        }
      }
      else {
        // Non-whole-word search.
        col = sc.search(text, col);
        if (col != -1)
          goto found;
      }
      col = 0;
      line++;
    }
  } else {
    // backward search
    if (sc.flags & KateDocument::sfSelected) {
      if (line > selectEnd.line) {
        line = selectEnd.line;
        col = -1;
      }
      searchEnd = selectStart.line;
    } else searchEnd = 0;

    while (line >= searchEnd) {
      textLine = getTextLine(line);
      tlen = textLine->length();
      if (tlen > bufLen) {
        delete t;
        bufLen = (tlen + 255) & (~255);
        t = new QChar[bufLen];
      }
      memcpy(t, textLine->getText(), tlen*sizeof(QChar));
      /*if (sc.flags & KateDocument::sfSelected) {
        pos = 0;
        do {
          pos = textLine->findSelected(pos);
          newPos = textLine->findUnselected(pos);
          memset(&t[pos], 0, (newPos - pos)*sizeof(QChar));
          pos = newPos;
        } while (pos < tlen);
      }*/

      if (col < 0 || col > tlen) col = tlen;

      QString text(t, tlen);
      if (sc.flags & KateDocument::sfWholeWords) {
        // Until the beginning of the line...
        while (col >= 0) {
          // ...find the next match.
          col = sc.search(text, col);
          if (col != -1) {
            // Is the match delimited correctly?
            if (((col == 0) || (!m_highlight->isInWord(t[col]))) &&
              ((col + sc.matchedLength == tlen) || (!m_highlight->isInWord(t[col + sc.matchedLength])))) {
              goto found;
            }
            else {
              // Start again from the previous character.
              col--;
            }
          }
          else {
            // No match.
            break;
          }
        }
      }
      else {
        // Non-whole-word search.
        col = sc.search(text, col);
        if (col != -1)
          goto found;
      }
      col = -1;
      line--;
    }
  }
  sc.flags |= KateDocument::sfWrapped;
  return false;
found:
  if (sc.flags & KateDocument::sfWrapped) {
    if ((line > sc.startCursor.line || (line == sc.startCursor.line && col >= sc.startCursor.col))
      ^ ((sc.flags & KateDocument::sfBackward) != 0)) return false;
  }
  sc.cursor.col = col;
  sc.cursor.line = line;
  return true;
}

void KateDocument::newBracketMark(KateTextCursor &cursor, BracketMark &bm)
{
  TextLine::Ptr textLine;
  int x, line, count, attr;
  QChar bracket, opposite, ch;
  Attribute *a;

  bm.eXPos = -1; //mark bracked mark as invalid
  x = cursor.col -1; // -1 to look at left side of cursor
  if (x < 0) return;
  line = cursor.line; //current line
  count = 0; //bracket counter for nested brackets

  textLine = getTextLine(line);
  if (!textLine) return;

  bracket = textLine->getChar(x);
  attr = textLine->getAttr(x);

  if (bracket == '(' || bracket == '[' || bracket == '{')
  {
    //get opposite bracket
    opposite = ')';
    if (bracket == '[') opposite = ']';
    if (bracket == '{') opposite = '}';
    //get attribute of bracket (opposite bracket must have the same attribute)
    x++;
    while (line - cursor.line < 40) {
      //go to next line on end of line
      while (x >= (int) textLine->length()) {
        line++;
        if (line > (int) lastLine()) return;
        textLine = getTextLine(line);
        x = 0;
      }
      if (textLine->getAttr(x) == attr) {
        //try to find opposite bracked
        ch = textLine->getChar(x);
        if (ch == bracket) count++; //same bracket : increase counter
        if (ch == opposite) {
          count--;
          if (count < 0) goto found;
        }
      }
      x++;
    }
  }
  else if (bracket == ')' || bracket == ']' || bracket == '}')
  {
    opposite = '(';
    if (bracket == ']') opposite = '[';
    if (bracket == '}') opposite = '{';
    x--;
    while (cursor.line - line < 20) {

      while (x < 0) {
        line--;
        if (line < 0) return;
        textLine = getTextLine(line);
        x = textLine->length() -1;
      }
      if (textLine->getAttr(x) == attr) {
        ch = textLine->getChar(x);
        if (ch == bracket) count++;
        if (ch == opposite) {
          count--;
          if (count < 0) goto found;
        }
      }
      x--;
    }
  }
  return;

found:
  //cursor position of opposite bracket
  bm.cursor.col = x;
  bm.cursor.line = line;
  //x position (start and end) of related bracket
  bm.sXPos = textWidth(textLine, x);
  a = attribute(attr);

   if (a->bold && a->italic)
      bm.eXPos = bm.sXPos + viewFont.myFontMetricsBI.width(bracket);
    else if (a->bold)
      bm.eXPos = bm.sXPos + viewFont.myFontMetricsBold.width(bracket);
    else if (a->italic)
      bm.eXPos = bm.sXPos + viewFont.myFontMetricsItalic.width(bracket);
    else
      bm.eXPos = bm.sXPos + viewFont.myFontMetrics.width(bracket);
}

void KateDocument::clipboardChanged() { //slot
//#if defined(_WS_X11_)
  if (_configFlags & KateDocument::cfSingleSelection) {
    disconnect(QApplication::clipboard(), SIGNAL(dataChanged()),
      this, SLOT(clipboardChanged()));
    clearSelection ();
    updateViews();
  }
//#endif
}

void KateDocument::guiActivateEvent( KParts::GUIActivateEvent *ev )
{
  KParts::ReadWritePart::guiActivateEvent( ev );
  if ( ev->activated() )
    emit selectionChanged();
}

void KateDocument::setDocName (QString docName)
{
  myDocName = docName;
  emit nameChanged (this);
}

void KateDocument::setMTime()
{
    if (fileInfo && !fileInfo->fileName().isEmpty()) {
      fileInfo->refresh();
      mTime = fileInfo->lastModified();
    }
}

void KateDocument::isModOnHD(bool forceReload)
{
  if (fileInfo && !fileInfo->fileName().isEmpty()) {
    fileInfo->refresh();
    if (fileInfo->lastModified() > mTime) {
      if ( forceReload ||
           (KMessageBox::warningContinueCancel(0,
               (i18n("The file %1 has changed on disk.\nDo you want to reload it?\n\nIf you cancel you will lose these changes next time you save this file")).arg(url().filename()),
               i18n("File has changed on Disk"),
               i18n("Yes") ) == KMessageBox::Continue)
          )
        reloadFile();
      else
        setMTime();
    }
  }
}

void KateDocument::reloadFile()
{
  if (fileInfo && !fileInfo->fileName().isEmpty()) {
    KateDocument::openFile();
    setMTime();
  }
}

void KateDocument::slotModChanged()
{
  emit modStateChanged (this);
}

void KateDocument::flush ()
{
  if (!isReadWrite())
    return;

  m_url = KURL();
  fileInfo->setFile (QString());
  setMTime();

  clear();
  updateViews();

  emit fileNameChanged ();
}

void KateDocument::open (const QString &name)
{
  openURL (KURL (name));
}

Attribute *KateDocument::attribute (uint pos)
{
  if (pos < myAttribsLen)
    return &myAttribs[pos];

  return &myAttribs[0];
}

void KateDocument::wrapText (uint col)
{
  uint line = 0;
  int z = 0;

  while(true)
  {
    TextLine::Ptr l = getTextLine(line);

    if (l->length() > col)
    {
      TextLine::Ptr tl = new TextLine();
      buffer->insertLine(line+1,tl);
      const QChar *text = l->getText();

      for (z=col; z>0; z--)
      {
        if (z < 1) break;
        if (text[z].isSpace()) break;
      }

      if (z < 1) z=col;

      l->wrap (tl, z);
    }

    line++;
    if (line >= numLines()) break;
  };

  newDocGeometry=true;
  updateLines();
  updateViews();
}

void KateDocument::setPseudoModal(QWidget *w) {
//  QWidget *old = pseudoModal;

  // (glenebob)
  // this is a temporary hack to make the spell checker work a little
  // better - as kspell progresses, this sort of thing should become
  // obsolete or worked around more cleanly
  // this is relied upon *only* by the spell-check code
  if (pseudoModal && pseudoModal != (QWidget*)1L)
    delete pseudoModal;

//  pseudoModal = 0L;
//  if (old || w) recordReset();

  pseudoModal = w;
}

void KateDocument::setWordWrap (bool on)
{
  if (on != myWordWrap && on)
    wrapText (myWordWrapAt);

  myWordWrap = on;
}

void KateDocument::setWordWrapAt (uint col)
{
  if (myWordWrapAt != col && myWordWrap)
    wrapText (myWordWrapAt);

  myWordWrapAt = col;
}

void KateDocument::applyWordWrap ()
{
  wrapText (myWordWrapAt);
}

uint KateDocument::configFlags ()
{
  return _configFlags;
}

void KateDocument::setConfigFlags (uint flags)
{
  bool updateView;

  if (flags != _configFlags)
  {
    // update the view if visibility of tabs has changed
    updateView = (flags ^ _configFlags) & KateDocument::cfShowTabs;
    _configFlags = flags;
    //emit newStatus();
    if (updateView) updateViews ();
  }
}

KateCursor::KateCursor ( KateDocument *doc)
{
  myDoc = doc;
  myDoc->addCursor (this);
}

KateCursor::~KateCursor ( )
{
  myDoc->removeCursor (this);
}

void KateCursor::position ( uint *, uint * ) const
{

}

bool KateCursor::setPosition ( uint , uint  )
{
  return false;
}

bool KateCursor::insertText ( const QString&  )
{
  return false;
}

bool KateCursor::removeText ( uint  )
{
  return false;
}

QChar KateCursor::currentChar () const
{
  return QChar();
}
