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

#include <qfileinfo.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <qstring.h>

#include <sys/time.h>
#include <unistd.h>

#include <stdio.h>

#include <qtimer.h>
#include <qobject.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qpainter.h>
#include <qfile.h>
#include <qtextstream.h>
#include <qtextcodec.h>
#include <kglobal.h>     
     
#include <kcharsets.h>     
#include <kdebug.h>     
#include <kinstance.h>     
     
#include <kglobalsettings.h>     
#include <kaction.h>     
#include <kstdaction.h>     
#include <qptrstack.h>     
     
#include "katebuffer.h"     
#include "katetextline.h"     
     
#include "katecmd.h"     
     
class KateUndo     
{     
  public:     
    KateUndo (KateDocument *doc, uint type, uint line, uint col, uint len,  QString text);     
    ~KateUndo ();     
     
    void undo ();     
    void redo ();     
     
    enum types     
    {     
      internalInsertText,     
      internalRemoveText,     
      internalWrapLine,     
      internalUnWrapLine,     
      internalInsertLine,     
      internalRemoveLine     
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
     
uint KateDocument::uniqueID = 0;     
     
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
  if (type == KateUndo::internalInsertText)     
  {     
    myDoc->internalRemoveText (line, col, len);     
  }     
  else if (type == KateUndo::internalRemoveText)     
  {     
    myDoc->internalInsertText (line, col, text);     
  }     
  else if (type == KateUndo::internalWrapLine)     
  {     
    myDoc->internalUnWrapLine (line, col);     
  }     
  else if (type == KateUndo::internalUnWrapLine)     
  {     
    myDoc->internalWrapLine (line, col);     
  }     
  else if (type == KateUndo::internalInsertLine)     
  {     
    myDoc->internalRemoveLine (line);     
  }     
  else if (type == KateUndo::internalRemoveLine)     
  {     
    myDoc->internalInsertLine (line, text);     
  }     
}     
     
void KateUndo::redo ()     
{     
  if (type == KateUndo::internalRemoveText)     
  {     
    myDoc->internalRemoveText (line, col, len);     
  }     
  else if (type == KateUndo::internalInsertText)     
  {     
    myDoc->internalInsertText (line, col, text);     
  }     
  else if (type == KateUndo::internalUnWrapLine)     
  {     
    myDoc->internalUnWrapLine (line, col);     
  }     
  else if (type == KateUndo::internalWrapLine)     
  {     
    myDoc->internalWrapLine (line, col);     
  }     
  else if (type == KateUndo::internalRemoveLine)     
  {     
    myDoc->internalRemoveLine (line);     
  }     
  else if (type == KateUndo::internalInsertLine)     
  {     
    myDoc->internalInsertLine (line, text);     
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
     
  for (int pos=items.count()-1; pos >= 0; pos--)     
  {     
    items.at(pos)->undo();     
  }     
}     
     
void KateUndoGroup::redo ()     
{     
  if (items.count() == 0)     
    return;     
     
  for (int pos=0; pos < items.count(); pos++)     
  {     
    items.at(pos)->redo();     
  }     
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
  : Kate::Document (),     
    myFont(KGlobalSettings::fixedFont()), myFontBold(KGlobalSettings::fixedFont()), myFontItalic(KGlobalSettings::fixedFont()), myFontBI(KGlobalSettings::fixedFont()),     
    myFontMetrics (myFont), myFontMetricsBold (myFontBold), myFontMetricsItalic (myFontItalic), myFontMetricsBI (myFontBI),     
    hlManager(HlManager::self ())     
{     
  hlSetByUser = false;     
  PreHighlightedTill=0;     
  RequestPreHighlightTill=0;     
  setInstance( KateFactory::instance() );     
     
  currentUndo = 0L;     
  pseudoModal = 0L;     
       
  myAttribs = 0L;     
  myAttribsLen = 0;     
     
  m_bSingleViewMode=bSingleViewMode;     
  m_bBrowserView = bBrowserView;     
  
  selectStartLine = -1;
  selectStartCol = -1;
  selectEndLine = -1;
  selectEndCol = -1;
  invertedSelection = false;
     
  // some defaults     
  _configFlags = KateDocument::cfAutoIndent | KateDocument::cfBackspaceIndents     
    | KateDocument::cfTabIndents | KateDocument::cfKeepIndentProfile     
    | KateDocument::cfRemoveSpaces     
    | KateDocument::cfDelOnInput | KateDocument::cfMouseAutoCopy | KateDocument::cfWrapCursor     
    | KateDocument::cfShowTabs | KateDocument::cfSmartHome;     
     
  _searchFlags = 0;     
     
  m_url = KURL();     
     
  myEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());     
  maxLength = -1;     

  setFont (KGlobalSettings::fixedFont());     
     
  myDocID = uniqueID;     
  uniqueID++;     
     
  myDocName = QString ("");     
  fileInfo = new QFileInfo ();     
     
  myCmd = new KateCmd (this);     
     
  connect(this,SIGNAL(modifiedChanged ()),this,SLOT(slotModChanged ()));     
     
  buffer = new KateBuffer;     
  connect(buffer, SIGNAL(linesChanged(int)), this, SLOT(slotBufferChanged()));     
//  connect(buffer, SIGNAL(textChanged()), this, SIGNAL(textChanged()));     
  connect(buffer, SIGNAL(needHighlight(long,long)),this,SLOT(slotBufferHighlight(long,long)));     
     
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
  _autoUpdate = true;     
     
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
  m_highlight->release();     
     
  if ( !m_bSingleViewMode )     
  {     
    myViews.setAutoDelete( true );     
    myViews.clear();     
    myViews.setAutoDelete( false );     
  }     
       
  delete [] myAttribs;     
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
     
  for (int i=0; i < buffer->count(); i++)     
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
     
  for (int i=startLine; i < buffer->count(); i++)     
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
  KateViewCursor cursor;     
  KateView *view;     
     
  setPseudoModal(0L);

  cursor.col = cursor.line = 0;
  for (view = myViews.first(); view != 0L; view = myViews.next() ) {
    view->updateCursor(cursor);
    view->tagAll();
  }

  eolMode = KateDocument::eolUnix;

  buffer->clear();
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

  bool newUndo = false;
  if (currentUndo == 0L)
  {
    currentUndo = new KateUndoGroup (this);
    newUndo = true;
  }

  tagEnd = 0;
  tagStart = 0xffffff;
     
  for (uint pos = 0; pos < len; pos++)     
  {     
    ch = s[pos];
     
    if (ch == '\n')
    {
      internalInsertText (line, insertPos, buf);
      internalWrapLine (line, insertPos + buf.length());

      line++;
      endLine++;
      insertPos = 0;
      buf.truncate(0);
    }
    else
      buf += ch; // append char to buffer
  }

  internalInsertText (line, insertPos, buf);

  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);
  }

  if (_autoUpdate)
    updateViews();

  if (newUndo)
  {
    undoItems.append (currentUndo);
    currentUndo = 0L;
    emit undoChanged ();
  }

  emit textChanged ();

  return true;
}

bool KateDocument::removeText ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  TextLine::Ptr l, tl;
  KateView *view;
  int cLine, cCol;
  uint deletePos = 0;
  uint endPos = 0;
  uint line = 0;
  KateViewCursor c;

  tagEnd = 0;
  tagStart = 0xffffff;

  l = getTextLine(startLine);

  if (!l)
    return false;

  bool newUndo = false;
  if (currentUndo == 0L)
  {
    currentUndo = new KateUndoGroup (this);
    newUndo = true;
  }

  if (startLine == endLine)
  {
    internalRemoveText (startLine, startCol, endCol-startCol);
  }
  else if ((startLine+1) == endLine)
  {
    internalRemoveText (startLine, startCol, l->length()-startCol);
    internalRemoveText (startLine+1, 0, endCol);
    internalUnWrapLine (startLine, startCol);
  }
  else
  {
    for (line = startLine; line <= endLine; line++)
    {
      if ((line > startLine) && (line < endLine))
      {
        deletePos = 0;

        internalRemoveText (startLine, deletePos, l->length()-startCol);
        internalUnWrapLine (startLine, deletePos);
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
        internalRemoveText (startLine, deletePos, endPos-deletePos);
      }
    }
  }

  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);
  }

  if (_autoUpdate)
    updateViews();

  if (newUndo)
  {
    undoItems.append (currentUndo);
    currentUndo = 0L;
    emit undoChanged ();
  }

  emit textChanged ();

  return true;
}

bool KateDocument::insertLine( uint l, const QString &str )
{
  if (l > buffer->count())
    return false;

  bool newUndo = false;
  if (currentUndo == 0L)
  {
    currentUndo = new KateUndoGroup (this);
    newUndo = true;
  }

  internalInsertLine (l, str);

  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);
  }

  if (_autoUpdate)
    updateViews();

  if (newUndo)
  {
    undoItems.append (currentUndo);
    currentUndo = 0L;
    emit undoChanged ();
  }

  emit textChanged ();

  return true;
}

bool KateDocument::removeLine( uint line )     
{     
  bool newUndo = false;     
  if (currentUndo == 0L)     
  {     
    currentUndo = new KateUndoGroup (this);     
    newUndo = true;     
  }     
     
  if (!internalRemoveLine (line))     
    return false;     
     
  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);     
  }
     
  if (_autoUpdate)     
    updateViews();     
     
  if (newUndo)     
  {     
    undoItems.append (currentUndo);     
    currentUndo = 0L;
    emit undoChanged ();     
  }     
     
  emit textChanged ();     
     
  return true;
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
bool KateDocument::internalInsertText ( uint line, uint col, const QString &s )
{
  TextLine::Ptr l;

  l = getTextLine(line);

  if (!l)
    return false;

  if (currentUndo)
    currentUndo->addItem (new KateUndo (this, KateUndo::internalInsertText, line, col, s.length(), s));

  l->replace(col, 0, s.unicode(), s.length());

  buffer->changeLine(line);
  updateMaxLength(l);
  tagLine(line);

  emit textChanged ();
  return true;
}

bool KateDocument::internalRemoveText ( uint line, uint col, uint len )
{
  TextLine::Ptr l;
  KateView *view;
  int cLine, cCol;
  KateViewCursor c;

  l = getTextLine(line);

  if (!l)
    return false;

  if (currentUndo)
    currentUndo->addItem (new KateUndo (this, KateUndo::internalRemoveText, line, col, len, l->getString().mid(col, len)));

  l->replace(col, len, 0L, 0);

  buffer->changeLine(line);
  updateMaxLength(l);
  tagLine(line);

  newDocGeometry = true;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->getCursorPosition (&cLine, &cCol);
    cCol = view->myViewInternal->cursor.col;

    if ( (cLine == line) && (cCol > col) )
    {
      if ((cCol - len) >= col)
        cCol = cCol-len;
      else
        cCol = col;
    }

    if (cCol < 0)
      cCol = 0;

    c.line = line;
    c.col = cCol;

    view->updateCursor (c);
  }

  emit textChanged ();
  return true;
}

bool KateDocument::internalWrapLine ( uint line, uint col )
{
  TextLine::Ptr l, tl;
  KateView *view;

  l = getTextLine(line);
  tl = new TextLine();

  if (!l || !tl)
    return false;

  if (currentUndo)
    currentUndo->addItem (new KateUndo (this, KateUndo::internalWrapLine, line, col, 0, 0));

  l->wrap (tl, col);

  buffer->insertLine (line+1, tl);
  buffer->changeLine(line);

  updateMaxLength(l);

  tagLine(line);
  tagLine(line+1);

  if (tagStart > line) tagStart++;
  if (tagEnd > line) tagEnd++;

  newDocGeometry = true;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->insLine(line+1);
  }

  emit textChanged ();
  return true;
}

bool KateDocument::internalUnWrapLine ( uint line, uint col)
{
  TextLine::Ptr l, tl;
  KateView *view;
  int cLine, cCol;
  KateViewCursor c;

  l = getTextLine(line);
  tl = getTextLine(line+1);

  if (!l || !tl)
    return false;

  if (currentUndo)
    currentUndo->addItem (new KateUndo (this, KateUndo::internalUnWrapLine, line, col, 0, 0));

  l->unWrap (col, tl, tl->length());
  l->setContext (tl->getContext(), tl->getContextLength());

  if (longestLine == tl)
    longestLine = 0L;

  buffer->changeLine(line);
  buffer->removeLine(line+1);

  updateMaxLength(l);
  tagLine(line);

  if (tagStart > line && tagStart > 0) tagStart--;
  if (tagEnd > line) tagEnd--;

  newDocGeometry = true;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->getCursorPosition (&cLine, &cCol);
    cCol = view->myViewInternal->cursor.col;

    view->delLine(line+1);

    if ( (cLine == (line+1)) || ((cLine == line) && (cCol >= col)) )
       cCol = col;

    c.line = line;
    c.col = cCol;

    view->updateCursor (c);
  }

  emit textChanged ();
  return true;
}

bool KateDocument::internalInsertLine ( uint line, const QString &s )
{
  KateView *view;

  if (currentUndo)
    currentUndo->addItem (new KateUndo (this, KateUndo::internalInsertLine, line, 0, s.length(), s));

  TextLine::Ptr TL=new TextLine();
  TL->append(s.unicode(),s.length());
  buffer->insertLine(line,TL);
  buffer->changeLine(line);

  updateMaxLength(TL);

  tagLine(line);

  if (tagStart >= line) tagStart++;
  if (tagEnd >= line) tagEnd++;

  newDocGeometry = true;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->insLine(line);
  }

  emit textChanged ();
  return true;
}

bool KateDocument::internalRemoveLine ( uint line )
{
  KateView *view;
  int cLine, cCol;
  KateViewCursor c;

  if (numLines() == 1)
    return false;

  if (currentUndo)
    currentUndo->addItem (new KateUndo (this, KateUndo::internalRemoveLine, line, 0, getTextLine (line)->getString().length(), getTextLine (line)->getString()));

  if (longestLine == getTextLine (line))
    longestLine = 0L;

  buffer->removeLine(line);

  if (tagStart >= line && tagStart > 0) tagStart--;
  if (tagEnd >= line) tagEnd--;

  newDocGeometry = true;
  for (view = myViews.first(); view != 0L; view = myViews.next() )
  {
    view->getCursorPosition (&cLine, &cCol);
    cCol = view->myViewInternal->cursor.col;
    view->delLine(line);

    if ( (cLine == line) )
      cCol = 0;

    c.line = line;
    c.col = cCol;

    view->updateCursor (c);
  }

  emit textChanged ();
  return true;
}

//
// KTextEditor::SelectionInterface stuff
//

bool KateDocument::setSelection ( uint startLine, uint startCol, uint endLine, uint endCol )
{
  int oldStartL, oldEndL;

  oldStartL = selectStartLine;
  oldEndL = selectEndLine;

  selectStartLine = startLine;
  selectStartCol = startCol;
  selectEndLine = endLine;
  selectEndCol = endCol;

  int endL, startL;
  if (oldEndL > selectEndLine)
    endL = oldEndL;
  else
    endL = selectEndLine;

  if (oldStartL < selectStartLine)
    startL = oldStartL;
  else
    startL = selectStartLine;

  tagLines (startL, endL);
  updateViews ();

  emit selectionChanged ();

  return true;
}

bool KateDocument::clearSelection ()
{
  tagLines(selectStartLine,selectEndLine);

  selectStartLine = -1;
  selectStartCol = -1;
  selectEndLine = -1;
  selectEndCol = -1;
  invertedSelection = false;

  updateViews ();

  emit selectionChanged();

  return true;
}

bool KateDocument::hasSelection() const
{
  return ((selectStartCol != selectEndCol) || (selectEndLine != selectStartLine));
}

QString KateDocument::selection() const
{
  TextLine::Ptr textLine;
  QString s;
  
  if (!invertedSelection)
  {
    for (int z=selectStartLine; z <= selectEndLine; z++)
    {
      textLine = getTextLine(z);
      if (!textLine)
        break;
    
      if (!(_configFlags & KateDocument::cfVerticalSelect))
      {
        if ((z > selectStartLine) && (z < selectEndLine))
          s.append (textLine->getString());
  else
  {
    if ((z == selectStartLine) && (z == selectEndLine))
      s.append (textLine->getString().mid(selectStartCol, selectEndCol-selectStartCol));
    else if ((z == selectStartLine))
      s.append (textLine->getString().mid(selectStartCol, textLine->length()-selectStartCol));
    else if ((z == selectEndLine))
      s.append (textLine->getString().mid(0, selectEndCol));    
  }
      }
      else
      {
        s.append (textLine->getString().mid(selectStartCol, selectEndCol-selectStartCol));
      }
      
      if (z < selectEndLine)
  s.append (QChar('\n'));
    }
  }
  
  return s;  
}

struct DeleteSelection {
  int line;
  int lineLen;
  int deleteLine;
  int deleteStart;
  int len;
};

bool KateDocument::removeSelectedText ()
{
  TextLine::Ptr textLine = 0L;
  QPtrStack<DeleteSelection> selectedLines;

  if (!hasSelection())
    return false;

  currentUndo = new KateUndoGroup (this);

  _autoUpdate = false;

  if (!invertedSelection)
  {
    for (int z=selectStartLine; z <= selectEndLine; z++)
    {
      textLine = getTextLine(z);
      if (!textLine)
        break;

      DeleteSelection *curLine = new DeleteSelection;

      curLine->deleteStart = textLine->length();
      curLine->line = z;
      curLine->lineLen = textLine->length();

       if (!(_configFlags & KateDocument::cfVerticalSelect))
       {
         if ((z > selectStartLine) && (z < selectEndLine))
           curLine->deleteLine = 1;
           else
         {
            if ((z == selectStartLine) && (z == selectEndLine))
          {
            curLine->deleteStart = selectStartCol;
            curLine->len = selectEndCol-selectStartCol;
          }
          else if ((z == selectStartLine))
          {
            curLine->deleteStart = selectStartCol;
            curLine->len = textLine->length()-selectStartCol;
          }
           else if ((z == selectEndLine))
          {
            curLine->deleteStart = 0;
            curLine->len = selectEndCol;
          }
        }
      }
      else
      {
        curLine->deleteStart = selectStartCol;
        curLine->len = selectEndCol-selectStartCol;
      }

      selectedLines.push (curLine);
    }
  }

  while (selectedLines.count() > 0)
  {
    DeleteSelection *curLine = selectedLines.pop ();

    if (curLine->deleteLine == 1)
      removeLine (curLine->line);
    else if (curLine->deleteStart+curLine->len > curLine->lineLen)
      removeText (curLine->line, curLine->deleteStart, curLine->line+1, 0);
    else
      removeText (curLine->line, curLine->deleteStart, curLine->line, curLine->deleteStart+curLine->len);
  }

  _autoUpdate = true;
  clearSelection();

  undoItems.append (currentUndo);
  currentUndo = 0L;
  emit undoChanged ();

  return true;
}

bool KateDocument::selectAll()
{
  return setSelection (0, 0, lastLine(), getTextLine(lastLine())->length());
}

bool KateDocument::invertSelection()
{/*
  TextLine::Ptr textLine;

  select.col = -1;

//  if (selectStart != 0 || selectEnd != lastLine()) recordReset();

  selectStart = 0;
  selectEnd = lastLine();

  tagLines(selectStart,selectEnd);

  for (int z = selectStart; z < selectEnd; z++) {
    textLine = getTextLine(z);
    textLine->toggleSelectEol(0);
  }
  textLine = getTextLine(selectEnd);
  textLine->toggleSelect(0,textLine->length());
  optimizeSelection();
  emit selectionChanged();

  updateViews ();

  return true;*/
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
     
  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);
  }

  updateViews();

  emit undoChanged ();
}

void KateDocument::redo()
{
  redoItems.last()->redo();
  undoItems.append (redoItems.last());
  redoItems.removeLast ();

  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);
  }

  updateViews();

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
  uint line, col, pos;
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

      if (line-1 >= 0)
        col = getTextLine(line-1)->length();

      line--;
    }
  }

  return false;
}

bool KateDocument::searchText (unsigned int startLine, unsigned int startCol, const QRegExp &regexp, unsigned int *foundAtLine, unsigned int *foundAtCol, unsigned int *matchLen, bool backwards)
{
  uint line, col, pos;
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

      if (line-1 >= 0)
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

//  hlNumber = n;

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
    for (int i=0; i < buffer->count(); i++)     
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
    for (int i=0; i < buffer->count(); i++)
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

void KateDocument::setFont (QFont font)     
{     
  //kdDebug()<<"Kate:: setFont"<<endl;     
  int oldwidth=myFontMetrics.width('W');  //Quick & Dirty Hack (by JoWenn) //Remove in KDE 3.0     
  myFont = font;     
  myFontBold = QFont (font);     
  myFontBold.setBold (true);     
     
  myFontItalic = QFont (font);     
  myFontItalic.setItalic (true);     
     
  myFontBI = QFont (font);     
  myFontBI.setBold (true);     
  myFontBI.setItalic (true);     
     
  myFontMetrics = QFontMetrics (myFont);
  myFontMetricsBold = QFontMetrics (myFontBold);     
  myFontMetricsItalic = QFontMetrics (myFontItalic);     
  myFontMetricsBI = QFontMetrics (myFontBI);     
  int newwidth=myFontMetrics.width('W'); //Quick & Dirty Hack (by JoWenn)  //Remove in KDE 3.0     
  maxLength=maxLength*(float)newwidth/(float)oldwidth; //Quick & Dirty Hack (by JoWenn)  //Remove in KDE 3.0     
     
  updateFontData();     
  updateViews(); //Quick & Dirty Hack (by JoWenn) //Remove in KDE 3.0     
}     
     
long  KateDocument::needPreHighlight(long till)     
{     
  int max=numLines()-1;     
  if (till>max)     
    {     
      till=max;     
    }     
  if (PreHighlightedTill>=till) return -1;     
     
  long tmp=RequestPreHighlightTill;     
  if (RequestPreHighlightTill<till)     
    {     
      RequestPreHighlightTill=till;     
      if (tmp<=PreHighlightedTill) QTimer::singleShot(10,this,SLOT(doPreHighlight()));     
    }     
  return RequestPreHighlightTill;     
}     
     
void KateDocument::doPreHighlight()     
{     
  int from = PreHighlightedTill;     
  int till = PreHighlightedTill+200;     
  int max = numLines()-1;     
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
  updateFontData();     
     
  maxLength = -1;     
  for (int i=0; i < buffer->count(); i++)     
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
     
void KateDocument::readConfig()     
{     
  KConfig *config = KateFactory::instance()->config();     
  config->setGroup("Kate Document");     
     
  _searchFlags = config->readNumEntry("SearchFlags", KateDocument::sfPrompt);     
  _configFlags = config->readNumEntry("ConfigFlags", _configFlags) & ~KateDocument::cfMark;     
     
  myWordWrap = config->readBoolEntry("Word Wrap On", false);     
  myWordWrapAt = config->readNumEntry("Word Wrap At", 80);     
  if (myWordWrap)     
    wrapText (myWordWrapAt);     
     
  setTabWidth(config->readNumEntry("TabWidth", 8));     
  setUndoSteps(config->readNumEntry("UndoSteps", 50));     
  setFont (config->readFontEntry("Font", &myFont));     
     
  colors[0] = config->readColorEntry("Color Background", &colors[0]);     
  colors[1] = config->readColorEntry("Color Selected", &colors[1]);     
     
  config->sync();
}     
     
void KateDocument::writeConfig()     
{     
  KConfig *config = KateFactory::instance()->config();     
  config->setGroup("Kate Document");     
     
  config->writeEntry("SearchFlags",_searchFlags);     
  config->writeEntry("ConfigFlags",_configFlags);     
     
  config->writeEntry("Word Wrap On", myWordWrap);     
  config->writeEntry("Word Wrap At", myWordWrapAt);     
  config->writeEntry("TabWidth", tabChars);     
  config->writeEntry("Font", myFont);     
  config->writeEntry("Color Background", colors[0]);     
  config->writeEntry("Color Selected", colors[1]);     
     
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
      if ( numLines() < l[i] ) break;
      getTextLine( l[i] )->addMark( Bookmark );
    }
  }
}

void KateDocument::writeSessionConfig(KConfig *config)
{
  config->writeEntry("URL", m_url.url() ); // ### encoding?? (Simon)
  config->writeEntry("Highlight", m_highlight->name());
  // anders: save bookmarks
  QPtrList<Kate::Mark> l = marks();
  QValueList<int> ml;
  for (uint i=0; i < l.count(); i++) {
    if ( l.at(i)->type == 1) // only save bookmarks
     ml << l.at(i)->line;
  }
  if ( ml.count() )
    config->writeEntry("Bookmarks", ml);
}

void KateDocument::makeAttribs()
{
  hlManager->makeAttribs(this, m_highlight);
  updateFontData();
  updateLines();
}

void KateDocument::updateFontData() {
  int maxAscent, maxDescent;
  int tabWidth;
  KateView *view;

  maxAscent = myFontMetrics.ascent();
  maxDescent = myFontMetrics.descent();
  tabWidth = myFontMetrics.width(' ');
     
  fontHeight = maxAscent + maxDescent + 1;     
  fontAscent = maxAscent;     
  m_tabWidth = tabChars*tabWidth;     
     
  for (view = myViews.first(); view != 0L; view = myViews.next() ) {     
    view->myViewInternal->drawBuffer->resize(view->width(),fontHeight);     
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
     
uint KateDocument::textWidth(const TextLine::Ptr &textLine, int cursorX) {     
  int x;     
  int z;     
  QChar ch;     
  Attribute *a;     
     
  x = 0;     
  for (z = 0; z < cursorX; z++) {     
    ch = textLine->getChar(z);     
    a = attribute(textLine->getAttr(z));     
     
    if (ch == '\t')     
      x += m_tabWidth - (x % m_tabWidth);     
    else if (a->bold && a->italic)     
      x += myFontMetricsBI.width(ch);     
    else if (a->bold)     
      x += myFontMetricsBold.width(ch);     
    else if (a->italic)     
      x += myFontMetricsItalic.width(ch);     
    else     
      x += myFontMetrics.width(ch);     
  }     
  return x;     
}     
     
uint KateDocument::textWidth(KateViewCursor &cursor) {     
  if (cursor.col < 0)     
     cursor.col = 0;
  if (cursor.line < 0)     
     cursor.line = 0;     
  if (cursor.line >= numLines())     
     cursor.line = lastLine();     
  return textWidth(getTextLine(cursor.line),cursor.col);
}     
     
uint KateDocument::textWidth(bool wrapCursor, KateViewCursor &cursor, int xPos) {     
  int len;     
  int x, oldX;     
  int z;     
  QChar ch;     
  Attribute *a;     
     
  if (cursor.line < 0) cursor.line = 0;     
  if (cursor.line > lastLine()) cursor.line = lastLine();     
  TextLine::Ptr textLine = getTextLine(cursor.line);     
  len = textLine->length();     
     
  x = oldX = z = 0;     
  while (x < xPos && (!wrapCursor || z < len)) {     
    oldX = x;     
    ch = textLine->getChar(z);     
    a = attribute(textLine->getAttr(z));     
     
    if (ch == '\t')     
      x += m_tabWidth - (x % m_tabWidth);     
    else if (a->bold && a->italic)     
      x += myFontMetricsBI.width(ch);     
    else if (a->bold)     
      x += myFontMetricsBold.width(ch);     
    else if (a->italic)     
      x += myFontMetricsItalic.width(ch);     
    else     
      x += myFontMetrics.width(ch);     
     
    z++;     
  }     
  if (xPos - oldX < x - xPos && z > 0) {     
    z--;     
    x = oldX;     
  }     
  cursor.col = z;     
  return x;     
}     
     
uint KateDocument::textPos(const TextLine::Ptr &textLine, int xPos) {     
  int x, oldX;     
  int z;     
  QChar ch;     
  Attribute *a;     

  x = oldX = z = 0;     
  while (x < xPos) { // && z < len) {     
    oldX = x;     
    ch = textLine->getChar(z);     
    a = attribute(textLine->getAttr(z));     
     
    if (ch == '\t')     
      x += m_tabWidth - (x % m_tabWidth);     
    else if (a->bold && a->italic)     
      x += myFontMetricsBI.width(ch);     
    else if (a->bold)     
      x += myFontMetricsBold.width(ch);     
    else if (a->italic)     
      x += myFontMetricsItalic.width(ch);     
    else     
      x += myFontMetrics.width(ch);     
     
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
     
uint KateDocument::textHeight() {     
  return numLines()*fontHeight;     
}     
     
uint KateDocument::currentColumn(KateViewCursor &cursor)     
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

  if (_configFlags & KateDocument::cfDelOnInput)
  {
    if (hasSelection())
    {
      removeSelectedText();
      view->getCursorPosition (&line, &col);
      col = view->myViewInternal->cursor.col;
    }
  }

  _autoUpdate = false;

  if (_configFlags & KateDocument::cfOvr)
  {
    if ((col+buf.length()) <= textLine->length())
      removeText (line, col, line, col+buf.length());
    else
      removeText (line, col, line, textLine->length());
  }

  insertText (line, col, buf);
  col += pos;

  KateViewCursor c;
  c.line = line;
  c.col = col;

  view->updateCursor(c);

  _autoUpdate = true;
  updateViews ();

/*
  if (myWordWrap && myWordWrapAt > 0) {
    int line;
    const QChar *s;
//    int pos;
    KateViewCursor actionCursor;

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
     
QString tabString(int pos, int tabChars) {     
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
     
  if (tagStart <= tagEnd) {
    updateLines(tagStart, tagEnd);
    setModified(true);
  }

  c.view->updateCursor(c.cursor);
}

void KateDocument::killLine(VConfig &c)
{
  removeLine (c.cursor.line);
}

void KateDocument::backspace(VConfig &c) {

  if (c.cursor.col <= 0 && c.cursor.line <= 0) return;

  if (c.cursor.col > 0)
  {     
     
    if (!(_configFlags & KateDocument::cfBackspaceIndents)) {     
      // ordinary backspace
      //c.cursor.col--;
      removeText(c.cursor.line, c.cursor.col-1, c.cursor.line, c.cursor.col);     
    } else {     
      // backspace indents: erase to next indent position     
      int l = 1; // del one char     
     
      TextLine::Ptr textLine = getTextLine(c.cursor.line);     
      int pos = textLine->firstChar();     
      if (pos < 0 || pos >= c.cursor.col) {     
        // only spaces on left side of cursor     
        // search a line with less spaces     
        int y = c.cursor.line;     
        while (y > 0) {     
          textLine = getTextLine(--y);
          pos = textLine->firstChar();
          if (pos >= 0 && pos < c.cursor.col) {     
            l = c.cursor.col - pos; // del more chars     
            break;     
          }     
        }     
      }     
      // break effectively jumps here     
      //c.cursor.col -= l;     
      removeText(c.cursor.line, c.cursor.col-1, c.cursor.line, c.cursor.col);     
    }     
  } else {     
    // c.cursor.col == 0: wrap to previous line     
     
    c.cursor.line--;     
    c.cursor.col = getTextLine(c.cursor.line)->length();     
    removeText (c.cursor.line, c.cursor.col, c.cursor.line+1, 0);     
  }     
}     
     
void KateDocument::del(VConfig &c)
{     
  if (c.cursor.col < getTextLine(c.cursor.line)->length())     
  {     
    removeText(c.cursor.line, c.cursor.col, c.cursor.line, c.cursor.col+1);
  }     
  else     
  {     
    removeText(c.cursor.line, c.cursor.col, c.cursor.line+1, 0);     
  }
}
     
void KateDocument::cut(VConfig &c) {     
     
  if (!hasSelection()) return;

  copy(_configFlags);
  removeSelectedText();
}

void KateDocument::copy(int flags) {

  if (!hasSelection()) return;

  QString s = selection ();
  if (!s.isEmpty()) {
//#if defined(_WS_X11_)
    if (_configFlags & KateDocument::cfSingleSelection)
      disconnect(QApplication::clipboard(), SIGNAL(dataChanged()), this, 0);
//#endif
    QApplication::clipboard()->setText(s);
//#if defined(_WS_X11_)
    if (_configFlags & KateDocument::cfSingleSelection) {
      connect(QApplication::clipboard(), SIGNAL(dataChanged()),
        this, SLOT(clipboardChanged()));
    }
//#endif
  }
}

void KateDocument::paste(VConfig &c) {
  QString s = QApplication::clipboard()->text();
  if (!s.isEmpty()) {
    insertText(c.cursor.line, c.cursor.col, s);
  }
}

void KateDocument::selectTo(VConfig &c, KateViewCursor &cursor, int cXPos)
{
  int sl, sc, el, ec;

  sl = selectStartLine;
  sc = selectStartCol;
  el = selectEndLine;
  ec = selectEndCol;

  if (sl == -1)
  {
    sl = c.cursor.line;
    sc = c.cursor.col;
  }
  
  el = cursor.line;
  ec = cursor.col;
  
  if ((el < sl) || ((el == sl) && (sc > ec)))
  {
    int tmpStart, tmpEnd;
    tmpStart = sl;
    tmpEnd = sc;
    sl = el;
    sc = ec;
    el = tmpStart;
    ec = tmpEnd;
  }
  
  setSelection (sl, sc, el, ec);
}

void KateDocument::selectWord(KateViewCursor &cursor, int flags) {
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

void KateDocument::selectLength(KateViewCursor &cursor, int length, int flags) {
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
      for (line = selectStartLine; line <= selectEndLine; line++) {
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

    for (line = selectStartLine; line <= selectEndLine; line++) {
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
  KateViewCursor cursor;

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
/*     
  QString marked (selection());     
  int preDeleteLine = -1, preDeleteCol = -1;     
  c.view->getCursorPosition (&preDeleteLine, &preDeleteCol);     
     
  if (marked.length() > 0)     
    c.view->keyDelete ();     
     
  int line = -1, col = -1;     
  c.view->getCursorPosition (&line, &col);     
     
  c.view->insertText (startComment + marked + endComment);*/     
}     
     
/*     
  Add to the current selection a comment line     
 mark at the begining of each line.     
*/     
void KateDocument::addStartLineCommentToSelection()     
{     
     
  QString commentLineMark = m_highlight->getCommentSingleLineStart() + " ";     
     
/*     
  // For each line of the selection     
  for (c.cursor.line = selectStart; c.cursor.line <= selectEnd; c.cursor.line++) {     
    TextLine* textLine = getTextLine(c.cursor.line);     
    if (textLine->isSelected() || textLine->numSelected()) {     
      // Add the comment line mark     
      insertText (c.cursor.line, c.cursor.col, commentLineMark);     
    }     
  }     
     
  // Go back to the last commented line     
  c.cursor.line--;*/     
}     
     
/*     
  Remove from the selection a start comment mark at     
  the begining and a stop comment mark at the end.     
*/     
bool KateDocument::removeStartStopCommentFromSelection()     
{     
  // kdDebug(13020)<<"KateDocument::removeStartStopCommentFromSelection"<<endl;     
/*     
  QString startComment = m_highlight->getCommentStart();     
  QString endComment = m_highlight->getCommentEnd();     
     
  int startCommentLen = startComment.length();     
  int endCommentLen = endComment.length();     
     
  QString marked (c.view->markedText ());     
  int preDeleteLine = -1, preDeleteCol = -1;     
  c.view->getCursorPosition (&preDeleteLine, &preDeleteCol);     
     
  int start = marked.find (startComment);     
  int end = marked.findRev (endComment);     
     
  bool okRemove = (start > -1) && (end > -1);     
  if (okRemove)     
    {     
      marked.remove (start, startCommentLen);     
      marked.remove (end-startCommentLen, endCommentLen);     
     
      c.view->keyDelete ();     
     
      int line = -1, col = -1;     
      c.view->getCursorPosition (&line, &col);     
      c.view->insertText (marked);     
    }     
     
  return okRemove;*/     
}     
     
/*     
  Remove from the begining of each line of the     
  selection a start comment line mark.     
*/     
bool KateDocument::removeStartLineCommentFromSelection()     
{     
  // kdDebug(13020)<<"KateDocument::removeStartLineCommentFromSelection"<<endl;     
/*     
  QString shortCommentMark = m_highlight->getCommentSingleLineStart();     
  QString longCommentMark = shortCommentMark + " ";     
     
  // Save cursor position     
  int xpos = c.cursor.col;     
  int ypos = c.cursor.line;     
     
  bool removed = false;     
     
  // For each line of the selection     
  for (c.cursor.line = selectStart; c.cursor.line <= selectEnd; c.cursor.line++) {     
    TextLine* textLine = getTextLine(c.cursor.line);     
    if (textLine->isSelected() || textLine->numSelected()) {     
      // Try to remove the long comment mark first     
      removed = (removeStringFromBegining(c, longCommentMark)     
                 || removeStringFromBegining(c, shortCommentMark)     
                 || removed);     
    }     
  }     
     
  // Go back to saved cursor position     
  TextLine* textLine = getTextLine(ypos);     
  c.cursor.col = QMIN(textLine->length(), xpos);     
  c.cursor.line = ypos;     
     
  return removed;*/     
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
     
QString KateDocument::getWord(KateViewCursor &cursor) {     
  int start, end, len;
     
  TextLine::Ptr textLine = getTextLine(cursor.line);     
  len = textLine->length();     
  start = end = cursor.col;     
  while (start > 0 && m_highlight->isInWord(textLine->getChar(start - 1))) start--;     
  while (end < len && m_highlight->isInWord(textLine->getChar(end))) end++;     
  len = end - start;     
  return QString(&textLine->getText()[start], len);     
}     
     
void KateDocument::tagLineRange(int line, int x1, int x2) {     
  int z;     
     
  for (z = 0; z < (int) myViews.count(); z++) {     
    myViews.at(z)->tagLines(line, line, x1, x2);     
  }     
}     
     
void KateDocument::tagLines(int start, int end) {     
  int z;     

  for (z = 0; z < (int) myViews.count(); z++) {     
    myViews.at(z)->tagLines(start, end, 0, 0xffffff);     
  }     
}     
     
void KateDocument::tagAll() {     
  int z;     

  for (z = 0; z < (int) myViews.count(); z++) {
    myViews.at(z)->tagAll();
  }
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

    endCtx = textLine->getContext();
    endCtxLen = textLine->getContextLength();

    kdDebug()<<QString("Calling doHighlight for line %1").arg(line)<<endl;

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
  }
  while ((line <= last_line) && ((line <= endLine) || stillcontinue));
  
  if (ctxNum)
    delete [] ctxNum;

  tagLines(startLine, line - 1);
}


void KateDocument::updateMaxLength(TextLine::Ptr &textLine) {
  int len;

  len = textWidth(textLine,textLine->length());

  if (len > maxLength) {
    longestLine = textLine;
    maxLength = len;
    newDocGeometry = true;
  } else {
    if (!longestLine || (textLine == longestLine && len <= maxLength*3/4)) {
      maxLength = -1;
      for (int i = 0; i < numLines();i++) {     
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

void KateDocument::slotBufferHighlight(long start,long stop) {     
  //kdDebug(13020)<<"KateDocument::slotBufferHighlight"<<QString("%1-%2").arg(start).arg(stop)<<endl;     
  updateLines(start,stop);     
//  buffer->startLoadTimer();     
}     
     
void KateDocument::updateViews(KateView *exclude) {     
  KateView *view;     
  int flags;     
  bool markState = hasSelection();     

  flags = (newDocGeometry) ? KateView::ufDocGeometry : 0;     
  for (view = myViews.first(); view != 0L; view = myViews.next() ) {     
    if (view != exclude) view->updateView(flags);     

  }
  newDocGeometry = false;
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

void KateDocument::paintTextLine(QPainter &paint, int line, int xStart, int xEnd, bool showTabs)
{
  paintTextLine (paint, line, 0, xStart, xEnd, showTabs);
}

bool KateDocument::lineColSelected (int line, int col)
{
  bool b = false;

  if (!(_configFlags & KateDocument::cfVerticalSelect))
  {
    if ((line > selectStartLine) && (line < selectEndLine))
      b = true;

    if (!b && (line == selectStartLine) && (col >= selectStartCol) && (line < selectEndLine))
      b = true;

    if (!b && (line == selectEndLine) && (col < selectEndCol) && (line > selectStartLine))
      b = true;

    if (!b && (line == selectEndLine) && (col < selectEndCol) && (line == selectStartLine) && (col >= selectStartCol))
      b = true;  

    if (!b && (line == selectStartLine) && (selectStartCol == 0) && (col < 0))
      b = true;
  }
  else
  {
    if ((line >= selectStartLine) && (line <= selectEndLine) && (col >= selectStartCol) && (col < selectEndCol))
      b = true;
  }
  
  if (!invertedSelection)
    return b;
  else
    return !b;
}

bool KateDocument::lineSelected (int line)
{
  bool b = false;

  if (!(_configFlags & KateDocument::cfVerticalSelect))
    if ((line > selectStartLine) && (line < selectEndLine))
      b = true;

  if (!invertedSelection)
    return b;
  else
    return !b;
}

bool KateDocument::lineHasSelected (int line)
{
  bool b = false;

  if (!(_configFlags & KateDocument::cfVerticalSelect))
  {
    if ((line > selectStartLine) && (line < selectEndLine))
      b = true;

    if ((line == selectStartLine) && (line < selectEndLine))
      b = true;

    if ((line == selectEndLine) && (line > selectStartLine))
      b = true;

    if ((line == selectEndLine) && (line == selectStartLine) && (selectEndCol > selectStartCol))
      b = true;
  }
  else
  {
    if ((line <= selectEndLine) && (line >= selectStartLine) && (selectEndCol > selectStartCol))
      b = true;
  }  
  
  if (!invertedSelection)
    return b;
  else
    return !b;
}

void KateDocument::paintTextLine(QPainter &paint, int line, int y, int xStart, int xEnd, bool showTabs)
{
  TextLine::Ptr textLine;
  int len;
  const QChar *s;
  int z, x;
  QChar ch;
  Attribute *a;
  int attr, nextAttr;
  int xs;
  int xc, zc;

  if (line > lastLine())
  {
    paint.fillRect(0, y, xEnd - xStart,fontHeight, colors[0]);
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
      x += m_tabWidth - (x % m_tabWidth);
    else
    {
      a = attribute(textLine->getAttr(z));

      if (a->bold && a->italic)
       x += myFontMetricsBI.width(ch);
      else if (a->bold)
        x += myFontMetricsBold.width(ch);
      else if (a->italic)
        x += myFontMetricsItalic.width(ch);
      else
        x += myFontMetrics.width(ch);
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
      paint.fillRect(xs - xStart, y, x - xs, fontHeight, colors[1]);
    else
      paint.fillRect(xs - xStart, y, x - xs, fontHeight, colors[0]);

    xs = x;
    col = z;

    attr = textLine->getAttr(z);

    if (z == len) break;

    ch = s[z];

    if (ch == QChar('\t'))
      x += m_tabWidth - (x % m_tabWidth);
    else
    {
      a = attribute(textLine->getAttr(z));

      if (a->bold && a->italic)
        x += myFontMetricsBI.width(ch);
      else if (a->bold)
        x += myFontMetricsBold.width(ch);
      else if (a->italic)
        x += myFontMetricsItalic.width(ch);
      else
        x += myFontMetrics.width(ch);
    }
    z++;
  }

  // is whole line selected ??
  if (lineSelected(line))
    paint.fillRect(xs - xStart, y, xEnd - xs, fontHeight, colors[1]);
  else
    paint.fillRect(xs - xStart, y, xEnd - xs, fontHeight, colors[0]);

  //reduce length to visible length
  len = z;

  // draw text
  x = xc;
  z = zc;
  y += fontAscent;// -1;
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
          x += myFontMetricsBI.width(s);
        else if (a->bold)
          x += myFontMetricsBold.width(s);
        else if (a->italic)
          x += myFontMetricsItalic.width(s);
        else
          x += myFontMetrics.width(s);
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
            paint.setFont(myFontBI);
            else if (a->bold)
            paint.setFont(myFontBold);
            else if (a->italic)
            paint.setFont(myFontItalic);
            else
            paint.setFont(myFont);
        }
        paint.drawPoint(x - xStart, y);
        paint.drawPoint(x - xStart +1, y);
        paint.drawPoint(x - xStart, y -1);
      }
      x += m_tabWidth - (x % m_tabWidth);
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
            x += myFontMetricsBI.width(s);
          else if (a->bold)
            x += myFontMetricsBold.width(s);
        else if (a->italic)
            x += myFontMetricsItalic.width(s);
         else
            x += myFontMetrics.width(s);
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
         paint.setFont(myFontBI);
        else if (a->bold)
          paint.setFont(myFontBold);
        else if (a->italic)
          paint.setFont(myFontItalic);
        else
          paint.setFont(myFont);
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
  int pos, newPos;

  if (searchFor.isEmpty()) return false;
     
  bufLen = 0;     
  t = 0L;     
     
  line = sc.cursor.line;     
  col = sc.cursor.col;
  if (!(sc.flags & KateView::sfBackward)) {
    //forward search
    if (sc.flags & KateView::sfSelected) {
      if (line < selectStartLine) {
        line = selectStartLine;
        col = 0;
      }
      searchEnd = selectEndLine;
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
      /*if (sc.flags & KateView::sfSelected) {
        pos = 0;
        do {
          pos = textLine->findSelected(pos);
          newPos = textLine->findUnselected(pos);
          memset(&t[pos], 0, (newPos - pos)*sizeof(QChar));
          pos = newPos;
        } while (pos < tlen);
      }*/

      QString text(t, tlen);
      if (sc.flags & KateView::sfWholeWords) {
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
    if (sc.flags & KateView::sfSelected) {
      if (line > selectEndLine) {
        line = selectEndLine;
        col = -1;
      }
      searchEnd = selectStartLine;
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
      /*if (sc.flags & KateView::sfSelected) {
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
      if (sc.flags & KateView::sfWholeWords) {
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
  sc.flags |= KateView::sfWrapped;
  return false;
found:
  if (sc.flags & KateView::sfWrapped) {
    if ((line > sc.startCursor.line || (line == sc.startCursor.line && col >= sc.startCursor.col))
      ^ ((sc.flags & KateView::sfBackward) != 0)) return false;
  }
  sc.cursor.col = col;
  sc.cursor.line = line;
  return true;
}

void KateDocument::tagLine(int line) {

  if (tagStart > line) tagStart = line;
  if (tagEnd < line) tagEnd = line;
}

void KateDocument::newBracketMark(KateViewCursor &cursor, BracketMark &bm)
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
        if (line > lastLine()) return;     
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
      bm.eXPos = bm.sXPos + myFontMetricsBI.width(bracket);     
    else if (a->bold)     
      bm.eXPos = bm.sXPos + myFontMetricsBold.width(bracket);     
    else if (a->italic)     
      bm.eXPos = bm.sXPos + myFontMetricsItalic.width(bracket);     
    else     
      bm.eXPos = bm.sXPos + myFontMetrics.width(bracket);     
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
     
QPtrList<Kate::Mark> KateDocument::marks ()     
{     
  QPtrList<Kate::Mark> list;     
  TextLine::Ptr line;     
     
  for (int i=0; i < numLines(); i++)     
  {     
    line = getTextLine(i);     
    if (line->mark() != 0)     
    {     
      Kate::Mark *mark=new Kate::Mark;     
      mark->line = i;     
      mark->type = line->mark();     
      list.append (mark);     
    }     
  }     
     
  return list;     
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
  int line = 0;     
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
     
void KateCursor::position ( uint *line, uint *col ) const
{

}

bool KateCursor::setPosition ( uint line, uint col )
{

}

bool KateCursor::insertText ( const QString& text )
{

}

bool KateCursor::removeText ( uint numberOfCharacters )
{
     
}     
     
QChar KateCursor::currentChar () const     
{     
     
}     
     
    
