/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002,2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002,2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2003 Anakim Border <aborder@sources.sourceforge.net>

   Based on:
     KWriteView : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateviewinternal.h"
#include "kateviewinternal.moc"

#include "kateview.h"
#include "katedocument.h"
#include "katecodefoldinghelpers.h"
#include "kateiconborder.h"
#include "katehighlight.h"
#include "katesupercursor.h"
#include "katerenderer.h"
#include "katecodecompletion.h"
#include "kateconfig.h"

#include <kcursor.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <kurldrag.h>

#include <qstyle.h>
#include <qdragobject.h>
#include <qpopupmenu.h>
#include <qdropsite.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qclipboard.h>

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
  : QWidget (view, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )
  , editSessionNumber (0)
  , editIsRunning (false)
  , m_view (view)
  , m_doc (doc)
  , cursor (doc, true, 0, 0, this)
  , m_startPos(0,0)
  , m_oldStartPos(0,0)
  , m_madeVisible(false)
  , m_shiftKeyPressed (false)
  , m_columnScrollDisplayed(false)
  , m_selChangedByUser (false)
  , m_preserveMaxX(false)
  , m_currentMaxX(0)
  , m_updatingView(true)
  , m_cachedMaxStartPos(-1, -1)
  , m_dragScrollTimer(this)
  , m_suppressColumnScrollBar(false)
  , m_textHintEnabled(false)
  , m_textHintMouseX(-1)
  , m_textHintMouseY(-1)
{
  setMinimumSize (0,0);

  // cursor
  cursor.setMoveOnInsert (true);

  //
  // scrollbar for lines
  //
  m_lineScroll = new KateScrollBar(QScrollBar::Vertical, m_view);
  m_lineScroll->show();
  m_lineScroll->setTracking (true);

  m_lineLayout = new QVBoxLayout();
  m_colLayout = new QHBoxLayout();

  m_colLayout->addWidget(m_lineScroll);
  m_lineLayout->addLayout(m_colLayout);

  if (m_view->dynWordWrap()) {
    m_dummy = 0L;
  } else {
    // bottom corner box
    m_dummy = new QWidget(m_view);
    m_dummy->setFixedHeight(style().scrollBarExtent().width());
    m_dummy->show();
    m_lineLayout->addWidget(m_dummy);
  }

  m_view->m_grid->addMultiCellLayout(m_lineLayout, 0, 1, 2, 2);

  // Hijack the line scroller's controls, so we can scroll nicely for word-wrap
  connect(m_lineScroll, SIGNAL(prevPage()), SLOT(scrollPrevPage()));
  connect(m_lineScroll, SIGNAL(nextPage()), SLOT(scrollNextPage()));

  connect(m_lineScroll, SIGNAL(prevLine()), SLOT(scrollPrevLine()));
  connect(m_lineScroll, SIGNAL(nextLine()), SLOT(scrollNextLine()));

  connect(m_lineScroll, SIGNAL(sliderMoved(int)), SLOT(scrollLines(int)));
  connect(m_lineScroll, SIGNAL(sliderMMBMoved(int)), SLOT(scrollLines(int)));

  // catch wheel events, completing the hijack
  m_lineScroll->installEventFilter(this);

  //
  // scrollbar for columns
  //
  m_columnScroll = new QScrollBar(QScrollBar::Horizontal,m_view);
  m_columnScroll->hide();
  m_columnScroll->setTracking(true);
  m_view->m_grid->addMultiCellWidget(m_columnScroll, 1, 1, 0, 1);
  m_startX = 0;
  m_oldStartX = 0;

  connect( m_columnScroll, SIGNAL( valueChanged (int) ),
           this, SLOT( scrollColumns (int) ) );

  //
  // iconborder ;)
  //
  leftBorder = new KateIconBorder( this, m_view );
  m_view->m_grid->addWidget(leftBorder, 0, 0);
  leftBorder->show ();

  connect( leftBorder, SIGNAL(toggleRegionVisibility(unsigned int)),
           m_doc->foldingTree(), SLOT(toggleRegionVisibility(unsigned int)));

  connect( doc->foldingTree(), SIGNAL(regionVisibilityChangedAt(unsigned int)),
           this, SLOT(slotRegionVisibilityChangedAt(unsigned int)));
  connect( doc, SIGNAL(codeFoldingUpdated()),
           this, SLOT(slotCodeFoldingChanged()) );

  displayCursor.setPos(0, 0);

  scrollTimer = 0;

  cursor.setPos(0, 0);
  cursorTimer = 0;
  cXPos = 0;

  m_textHintTimer=0;

  possibleTripleClick = false;

  setAcceptDrops( true );
  setBackgroundMode( NoBackground );

  // event filter
  installEventFilter(this);

  // set cursor
  setCursor( KCursor::ibeamCursor() );

  dragInfo.state = diNone;

  // Drag & scroll
  connect( &m_dragScrollTimer, SIGNAL( timeout() ),
             this, SLOT( doDragScroll() ) );

  updateView ();
}

KateViewInternal::~KateViewInternal ()
{
}

void KateViewInternal::prepareForDynWrapChange()
{
  // Which is the current view line?
  m_wrapChangeViewLine = displayViewLine(displayCursor, true);
}

void KateViewInternal::dynWrapChanged()
{
  if (m_view->dynWordWrap()) {
    delete m_dummy;
    m_columnScroll->hide();
    m_columnScrollDisplayed = false;

  } else {
    // bottom corner box
    m_dummy = new QWidget(m_view);
    m_dummy->setFixedSize( style().scrollBarExtent().width(),
                                  style().scrollBarExtent().width() );
    m_dummy->show();
    m_lineLayout->addWidget(m_dummy);
  }

  tagAll();
  updateView();

  if (m_view->dynWordWrap())
    scrollColumns(0);

  // Determine where the cursor should be to get the cursor on the same view line
  if (m_wrapChangeViewLine != -1) {
    KateTextCursor newStart = viewLineOffset(displayCursor, -m_wrapChangeViewLine);

    // Account for the scrollbar in non-dyn-word-wrap mode
    if (!m_view->dynWordWrap() && scrollbarVisible(newStart.line())) {
      int lines = linesDisplayed() - 1;

      if (m_view->height() != height())
        lines++;

      if (newStart.line() + lines == displayCursor.line())
        newStart = viewLineOffset(displayCursor, 1 - m_wrapChangeViewLine);
    }

    makeVisible(newStart, newStart.col(), true);

  } else {
    update();
  }
}

KateTextCursor KateViewInternal::endPos() const
{
  int viewLines = linesDisplayed() - 1;

  if (viewLines < 0) {
    kdDebug(13030) << "WARNING: viewLines wrong!" << endl;
    viewLines = 0;
  }

  // Check to make sure that lineRanges isn't invalid
  if (!lineRanges.count() || lineRanges[0].line == -1 || viewLines >= (int)lineRanges.count()) {
    // Switch off use of the cache
    return KateTextCursor(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));
  }

  for (int i = viewLines; i >= 0; i--) {
    LineRange& thisRange = lineRanges[i];

    if (thisRange.line == -1) continue;

    if (thisRange.virtualLine >= (int)m_doc->numVisLines()) {
      // Cache is too out of date
      return KateTextCursor(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));
    }

    return KateTextCursor(thisRange.virtualLine, thisRange.wrap ? thisRange.endCol - 1 : thisRange.endCol);
  }

  Q_ASSERT(false);
  kdDebug(13030) << "WARNING: could not find a lineRange at all" << endl;
  return KateTextCursor(-1, -1);
}

uint KateViewInternal::endLine() const
{
  return endPos().line();
}

LineRange KateViewInternal::yToLineRange(uint y) const
{
  return lineRanges[y / m_view->renderer()->fontHeight()];
}

int KateViewInternal::lineToY(uint viewLine) const
{
  return (viewLine-startLine()) * m_view->renderer()->fontHeight();
}

void KateViewInternal::slotIncFontSizes()
{
  m_view->renderer()->increaseFontSizes();
}

void KateViewInternal::slotDecFontSizes()
{
  m_view->renderer()->decreaseFontSizes();
}

/**
 * Line is the real line number to scroll to.
 */
void KateViewInternal::scrollLines ( int line )
{
  KateTextCursor newPos(line, 0);
  scrollPos(newPos);
}

// This can scroll less than one true line
void KateViewInternal::scrollViewLines(int offset)
{
  KateTextCursor c = viewLineOffset(startPos(), offset);
  scrollPos(c);

  m_lineScroll->blockSignals(true);
  m_lineScroll->setValue(startLine());
  m_lineScroll->blockSignals(false);
}

void KateViewInternal::scrollNextPage()
{
  scrollViewLines(QMAX( linesDisplayed() - 1, 0 ));
}

void KateViewInternal::scrollPrevPage()
{
  scrollViewLines(-QMAX( linesDisplayed() - 1, 0 ));
}

void KateViewInternal::scrollPrevLine()
{
  scrollViewLines(-1);
}

void KateViewInternal::scrollNextLine()
{
  scrollViewLines(1);
}

KateTextCursor KateViewInternal::maxStartPos(bool changed)
{
  if (m_cachedMaxStartPos.line() == -1 || changed)
  {
    KateTextCursor end(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));

    m_cachedMaxStartPos = viewLineOffset(end, -(linesDisplayed() - 1));
  }

  // If we're not dynamic word-wrapping, the horizontal scrollbar is hidden and will appear, increment the maxStart by 1
  if (!m_view->dynWordWrap() && m_columnScroll->isHidden() && scrollbarVisible(m_cachedMaxStartPos.line()))
  {
    KateTextCursor end(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));

    return viewLineOffset(end, -linesDisplayed());
  }

  return m_cachedMaxStartPos;
}

// c is a virtual cursor
void KateViewInternal::scrollPos(KateTextCursor& c, bool force)
{
  if (!force && ((!m_view->dynWordWrap() && c.line() == (int)startLine()) || c == startPos()))
    return;

  if (c.line() < 0)
    c.setLine(0);

  KateTextCursor limit = maxStartPos();
  if (c > limit) {
    c = limit;

    // overloading this variable, it's not used in non-word wrap
    // used to set the lineScroll to the max value
    if (m_view->dynWordWrap())
      m_suppressColumnScrollBar = true;

    // Re-check we're not just scrolling to the same place
    if (!force && ((!m_view->dynWordWrap() && c.line() == (int)startLine()) || c == startPos()))
      return;
  }

  int viewLinesScrolled = displayViewLine(c);

  m_oldStartPos = m_startPos;
  m_startPos = c;

  // set false here but reversed if we return to makeVisible
  m_madeVisible = false;

  if (!force) {
    int lines = linesDisplayed();
    if ((int)m_doc->numVisLines() < lines) {
      KateTextCursor end(m_doc->numVisLines() - 1, m_doc->lineLength(m_doc->getRealLine(m_doc->numVisLines() - 1)));
      lines = QMIN((int)linesDisplayed(), displayViewLine(end) + 1);
    }

    Q_ASSERT(lines >= 0);

    if (QABS(viewLinesScrolled) < lines) {
      //KApplication::kApplication()->processEvents();
      updateView(false, viewLinesScrolled);
      int scrollHeight = -(viewLinesScrolled * m_view->renderer()->fontHeight());
      scroll(0, scrollHeight);
      leftBorder->scroll(0, scrollHeight);
      return;
    }
  }

  updateView();
  update();
  leftBorder->update();
}

void KateViewInternal::scrollColumns ( int x )
{
  if (x == m_startX)
    return;

  if (x < 0)
    x = 0;

  int dx = m_startX - x;
  m_oldStartX = m_startX;
  m_startX = x;

  if (QABS(dx) < width())
    scroll(dx, 0);
  else
    update();

  m_columnScroll->blockSignals(true);
  m_columnScroll->setValue(m_startX);
  m_columnScroll->blockSignals(false);
}

// If changed is true, the lines that have been set dirty have been updated.
void KateViewInternal::updateView(bool changed, int viewLinesScrolled)
{
  m_updatingView = true;

  uint contentLines = m_doc->visibleLines();

  m_lineScroll->blockSignals(true);

  KateTextCursor maxStart = maxStartPos(changed);
  int maxLineScrollRange = maxStart.line();
  if (m_view->dynWordWrap() && maxStart.col() != 0)
    maxLineScrollRange++;
  m_lineScroll->setRange(0, maxLineScrollRange);

  if (m_view->dynWordWrap() && m_suppressColumnScrollBar) {
    m_suppressColumnScrollBar = false;
    m_lineScroll->setValue(maxStart.line());
  } else {
    m_lineScroll->setValue(startPos().line());
  }
  m_lineScroll->setSteps(1, height() / m_view->renderer()->fontHeight());
  m_lineScroll->blockSignals(false);

  uint oldSize = lineRanges.size ();
  uint newSize = (height() / m_view->renderer()->fontHeight()) + 1;
  if (oldSize != newSize) {
    lineRanges.resize((height() / m_view->renderer()->fontHeight()) + 1);
    if (newSize > oldSize) {
      static LineRange blank;
      for (uint i = oldSize; i < newSize; i++) {
        lineRanges[i] = blank;
      }
    }
  }

  if (oldSize < lineRanges.size ())
  {
    for (uint i=oldSize; i < lineRanges.size(); i++)
      lineRanges[i].dirty = true;
  }

  // Move the lineRanges data if we've just scrolled...
  if (viewLinesScrolled != 0) {
    // loop backwards if we've just scrolled up...
    bool forwards = viewLinesScrolled >= 0 ? true : false;
    for (uint z = forwards ? 0 : lineRanges.count() - 1; z < lineRanges.count(); forwards ? z++ : z--) {
      uint oldZ = z + viewLinesScrolled;
      if (oldZ < lineRanges.count()) {
        lineRanges[z] = lineRanges[oldZ];
      } else {
        lineRanges[z].dirty = true;
      }
    }
  }

  if (m_view->dynWordWrap())
  {
    KateTextCursor realStart = startPos();
    realStart.setLine(m_doc->getRealLine(realStart.line()));

    LineRange startRange = range(realStart);
    uint line = startRange.virtualLine;
    int realLine = startRange.line;
    uint oldLine = line;
    int startCol = startRange.startCol;
    int startX = startRange.startX, endX = startRange.startX;
    int shiftX = startRange.startX ? startRange.shiftX : 0;
    bool wrap = false;
    int newViewLine = startRange.viewLine;
    // z is the current display view line
    TextLine::Ptr text = m_doc->kateTextLine(realLine);

    bool alreadyDirty = false;

    for (uint z = 0; z < lineRanges.size(); z++)
    {
      if (oldLine != line) {
        realLine = (int)m_doc->getRealLine(line);

        if (z)
          lineRanges[z-1].startsInvisibleBlock = (realLine != lineRanges[z-1].line + 1);

        text = m_doc->kateTextLine(realLine);
        startCol = 0;
        startX = 0;
        endX = 0;
        shiftX = 0;
        newViewLine = 0;
        oldLine = line;
      }

      if (line >= contentLines || !text)
      {
        if (lineRanges[z].line != -1)
          lineRanges[z].dirty = true;

        lineRanges[z].clear();

        line++;
      }
      else
      {
        if (lineRanges[z].line != realLine || lineRanges[z].startCol != startCol)
          alreadyDirty = lineRanges[z].dirty = true;

        if (lineRanges[z].dirty || (changed && alreadyDirty)) {
          alreadyDirty = true;

          lineRanges[z].virtualLine = line;
          lineRanges[z].line = realLine;
          lineRanges[z].startsInvisibleBlock = false;

          int tempEndX = 0;

          int endCol = m_view->renderer()->textWidth(text, startCol, width() - shiftX, &wrap, &tempEndX);

          endX += tempEndX;

          if (wrap)
          {
            if (startX == 0)
            {
              int pos = text->nextNonSpaceChar(0);

              if (pos > 0)
                shiftX = m_view->renderer()->textWidth(text, pos);

              if (shiftX > ((double)width() / 100 * 80 /* FIXME customisable percentage */))
                shiftX = 0;
            }

            if ((lineRanges[z].startX != startX) || (lineRanges[z].endX != endX) ||
                (lineRanges[z].startCol != startCol) || (lineRanges[z].endCol != endCol))
              lineRanges[z].dirty = true;

            lineRanges[z].startCol = startCol;
            lineRanges[z].endCol = endCol;
            lineRanges[z].startX = startX;
            lineRanges[z].endX = endX;
            lineRanges[z].viewLine = newViewLine;
            lineRanges[z].wrap = true;

            startCol = endCol;
            startX = endX;
          }
          else
          {
            if ((lineRanges[z].startX != startX) || (lineRanges[z].endX != endX) ||
                (lineRanges[z].startCol != startCol) || (lineRanges[z].endCol != endCol))
              lineRanges[z].dirty = true;

            lineRanges[z].startCol = startCol;
            lineRanges[z].endCol = endCol;
            lineRanges[z].startX = startX;
            lineRanges[z].endX = endX;
            lineRanges[z].viewLine = newViewLine;
            lineRanges[z].wrap = false;

            line++;
          }

          lineRanges[z].shiftX = shiftX;

        } else {
          // The cached data is still intact
          if (lineRanges[z].wrap) {
            startCol = lineRanges[z].endCol;
            startX = lineRanges[z].endX;
            endX = lineRanges[z].endX;
          } else {
            line++;
          }
        }
      }
      newViewLine++;
    }
  }
  else
  {
    uint z = 0;

    for(; (z + startLine() < contentLines) && (z < lineRanges.size()); z++)
    {
      if (lineRanges[z].dirty || lineRanges[z].line != (int)m_doc->getRealLine(z + startLine())) {
        lineRanges[z].dirty = true;

        lineRanges[z].line = m_doc->getRealLine( z + startLine() );
        if (z)
          lineRanges[z-1].startsInvisibleBlock = (lineRanges[z].line != lineRanges[z-1].line + 1);

        lineRanges[z].virtualLine = z + startLine();
        lineRanges[z].startCol = 0;
        lineRanges[z].endCol = m_doc->lineLength(lineRanges[z].line);
        lineRanges[z].startX = 0;
        lineRanges[z].endX = m_view->renderer()->textWidth( m_doc->kateTextLine( lineRanges[z].line ), -1 );
        lineRanges[z].shiftX = 0;
        lineRanges[z].viewLine = 0;
        lineRanges[z].wrap = false;
      }
    }

    for (; z < lineRanges.size(); z++)
    {
      if (lineRanges[z].line != -1)
        lineRanges[z].dirty = true;

      lineRanges[z].clear();
    }

    if (scrollbarVisible(startLine()))
    {
      m_columnScroll->blockSignals(true);

      int max = maxLen(startLine()) - width();
      if (max < 0)
        max = 0;

      m_columnScroll->setRange(0, max);

      m_columnScroll->setValue(m_startX);

      // Approximate linescroll
      m_columnScroll->setSteps(m_view->renderer()->config()->fontMetrics()->width('a'), width());

      m_columnScroll->blockSignals(false);

      if (!m_columnScroll->isVisible ()  && !m_suppressColumnScrollBar)
      {
        m_columnScroll->show();
        m_columnScrollDisplayed = true;
      }
    }
    else if (m_columnScroll->isVisible () && !m_suppressColumnScrollBar && (startX() == 0))
    {
      m_columnScroll->hide();
      m_columnScrollDisplayed = false;
    }
  }

  m_updatingView = false;

  if (changed)
    paintText (0,0,width(), height(), true);
}

void KateViewInternal::paintText (int x, int y, int width, int height, bool paintOnlyDirty)
{
  int xStart = startX() + x;
  int xEnd = xStart + width;
  uint h = m_view->renderer()->fontHeight();
  uint startz = (y / h);
  uint endz = startz + 1 + (height / h);
  uint lineRangesSize = lineRanges.size();

  // TODO config option?
  static bool doublebuffer = true;

  if (doublebuffer && drawBuffer.width() != KateViewInternal::width() || drawBuffer.height() != (int)h)
    drawBuffer.resize(KateViewInternal::width(), (int)h);

  if (doublebuffer && drawBuffer.isNull())
    return;

  QPainter paint;

  // TODO put in the proper places
  m_view->renderer()->setCaretStyle(m_view->isOverwriteMode() ? KateRenderer::Replace : KateRenderer::Insert);
  m_view->renderer()->setShowTabs(m_doc->configFlags() & KateDocument::cfShowTabs);

  for (uint z=startz; z <= endz; z++)
  {
    if ( (z >= lineRangesSize) || ((lineRanges[z].line == -1) && (!paintOnlyDirty || lineRanges[z].dirty)) )
    {
      if (!(z >= lineRangesSize))
        lineRanges[z].dirty = false;

      paint.begin(this);
      paint.fillRect( x, z * h, width, h, *m_view->renderer()->config()->backgroundColor() );
      paint.end();
    }
    else if (!paintOnlyDirty || lineRanges[z].dirty)
    {
      lineRanges[z].dirty = false;

      if (doublebuffer) {
        paint.begin(&drawBuffer);

      } else {
        paint.begin(this);
        paint.translate(x, z*h);
      }

      m_view->renderer()->paintTextLine(paint, &lineRanges[z], xStart, xEnd, &cursor, &bm);

      if (doublebuffer)
        bitBlt (this, x, z * h, &drawBuffer, 0, 0, width);

      paint.end();
    }
  }
}

/**
 * this function ensures a certain location is visible on the screen.
 * if endCol is -1, ignore making the columns visible.
 */
void KateViewInternal::makeVisible (const KateTextCursor& c, uint endCol, bool force)
{
  //kdDebug() << "MakeVisible start [" << startPos().line << "," << startPos().col << "] end [" << endPos().line << "," << endPos().col << "] -> request: [" << c.line << "," << c.col << "]" <<endl;// , new start [" << scroll.line << "," << scroll.col << "] lines " << (linesDisplayed() - 1) << " height " << height() << endl;
    // if the line is in a folded region, unfold all the way up
    //if ( m_doc->foldingTree()->findNodeForLine( c.line )->visible )
    //  kdDebug()<<"line ("<<c.line<<") should be visible"<<endl;

  if ( force )
  {

    KateTextCursor scroll = c;
    scrollPos(scroll, force);
  }
  else if ( c > viewLineOffset(endPos(), -m_minLinesVisible) )
  {
    KateTextCursor scroll = viewLineOffset(c, -(linesDisplayed() - m_minLinesVisible - 1));

    if (!m_view->dynWordWrap() && m_columnScroll->isHidden())
      if (scrollbarVisible(scroll.line()))
        scroll.setLine(scroll.line() + 1);

    scrollPos(scroll);
  }
  else if ( c < viewLineOffset(startPos(), m_minLinesVisible) )
  {
    KateTextCursor scroll = viewLineOffset(c, -m_minLinesVisible);
    scrollPos(scroll);
  }
  else
  {
    // Check to see that we're not showing blank lines
    KateTextCursor max = maxStartPos();
    if (startPos() > max) {
      scrollPos(max, max.col());
    }
  }

  if (!m_view->dynWordWrap() && endCol != (uint)-1)
  {
    int sX = (int)m_view->renderer()->textWidth (m_doc->kateTextLine( m_doc->getRealLine( c.line() ) ), c.col() );

    int sXborder = sX-8;
    if (sXborder < 0)
      sXborder = 0;

    if (sX < m_startX)
      scrollColumns (sXborder);
    else if  (sX > m_startX + width())
      scrollColumns (sX - width() + 8);
  }

  m_madeVisible = !force;
}

void KateViewInternal::slotRegionVisibilityChangedAt(unsigned int)
{
  kdDebug(13030) << "slotRegionVisibilityChangedAt()" << endl;
  m_cachedMaxStartPos.setLine(-1);
  KateTextCursor max = maxStartPos();
  if (startPos() > max)
    scrollPos(max);

  updateView();
  update();
  leftBorder->update();
}

void KateViewInternal::slotCodeFoldingChanged()
{
  leftBorder->update();
}

void KateViewInternal::slotRegionBeginEndAddedRemoved(unsigned int)
{
  kdDebug(13030) << "slotRegionBeginEndAddedRemoved()" << endl;
  // FIXME: performance problem
  leftBorder->update();
}

void KateViewInternal::showEvent ( QShowEvent *e )
{
  updateView ();

  QWidget::showEvent (e);
}

uint KateViewInternal::linesDisplayed() const
{
  int h = height();
  int fh = m_view->renderer()->fontHeight();

  return (h - (h % fh)) / fh;
}

QPoint KateViewInternal::cursorCoordinates()
{
  int viewLine = displayViewLine(displayCursor, true);

  if (viewLine == -1)
    return QPoint(-1, -1);

  uint y = viewLine * m_view->renderer()->fontHeight();
  uint x = cXPos - m_startX - lineRanges[viewLine].startX + leftBorder->width() + lineRanges[viewLine].xOffset();

  return QPoint(x, y);
}

void KateViewInternal::doReturn()
{
  KateTextCursor c = cursor;
  m_doc->newLine( c, this );
  updateCursor( c );
  updateView();
}

void KateViewInternal::doDelete()
{
  m_doc->del( cursor );
}

void KateViewInternal::doBackspace()
{
  m_doc->backspace( cursor );
}

void KateViewInternal::doPaste()
{
  m_doc->paste( m_view );
}

void KateViewInternal::doTranspose()
{
  m_doc->transpose( cursor );
}

void KateViewInternal::doDeleteWordLeft()
{
  wordLeft( true );
  m_doc->removeSelectedText();
  update();
}

void KateViewInternal::doDeleteWordRight()
{
  wordRight( true );
  m_doc->removeSelectedText();
  update();
}

class CalculatingCursor : public KateTextCursor {
public:
  CalculatingCursor( const KateDocument& doc )
    : KateTextCursor(), m_doc( doc )
    { Q_ASSERT( valid() ); }
  CalculatingCursor( const KateDocument& doc, const KateTextCursor& c )
    : KateTextCursor( c ), m_doc( doc )
    { Q_ASSERT( valid() ); }
  // This one constrains its arguments to valid positions
  CalculatingCursor( const KateDocument& doc, uint line, uint col )
    : KateTextCursor( line, col ), m_doc( doc )
    { makeValid(); }
  virtual CalculatingCursor& operator+=( int n ) = 0;
  virtual CalculatingCursor& operator-=( int n ) = 0;
  CalculatingCursor& operator++() { return operator+=( 1 ); }
  CalculatingCursor& operator--() { return operator-=( 1 ); }
  void makeValid() {
    m_line = QMAX( 0, QMIN( int( m_doc.numLines() - 1 ), line() ) );
    if (const_cast<KateDocument&>(m_doc).wrapCursor())
      m_col = QMAX( 0, QMIN( m_doc.lineLength( line() ), col() ) );
    else
      m_col = QMAX( 0, col() );
    Q_ASSERT( valid() );
  }
  void toEdge( Bias bias ) {
    if( bias == left ) m_col = 0;
    else if( bias == right ) m_col = m_doc.lineLength( line() );
  }
  bool atEdge() const { return atEdge( left ) || atEdge( right ); }
  bool atEdge( Bias bias ) const {
    switch( bias ) {
    case left:  return col() == 0;
    case none:  return atEdge();
    case right: return col() == m_doc.lineLength( line() );
    default: Q_ASSERT(false); return false;
    }
  }
protected:
  // Argh! KateDocument::configFlags() is not const :(
  bool valid() const {
    return line() >= 0 &&
            uint( line() ) < m_doc.numLines() &&
            col()  >= 0 &&
            (!(const_cast<KateDocument&>(m_doc).wrapCursor()) || col() <= m_doc.lineLength( line() ));
  }
  const KateDocument& m_doc;
};

class BoundedCursor : public CalculatingCursor {
public:
  BoundedCursor( const KateDocument& doc )
    : CalculatingCursor( doc ) {};
  BoundedCursor( const KateDocument& doc, const KateTextCursor& c )
    : CalculatingCursor( doc, c ) {};
  BoundedCursor( const KateDocument& doc, uint line, uint col )
    : CalculatingCursor( doc, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    m_col += n;
    m_col = QMAX( 0, col() );
    //col = QMIN( col, m_doc.lineLength( line ) );
    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    return operator+=( -n );
  }
};

class WrappingCursor : public CalculatingCursor {
public:
  WrappingCursor( const KateDocument& doc )
    : CalculatingCursor( doc ) {};
  WrappingCursor( const KateDocument& doc, const KateTextCursor& c )
    : CalculatingCursor( doc, c ) {};
  WrappingCursor( const KateDocument& doc, uint line, uint col )
    : CalculatingCursor( doc, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    if( n < 0 ) return operator-=( -n );
    int len = m_doc.lineLength( line() );
    if( col() + n <= len ) {
      m_col += n;
    } else if( uint( line() ) < m_doc.numLines() - 1 ) {
      n -= len - col() + 1;
      m_col = 0;
      m_line++;
      operator+=( n );
    } else {
      m_col = len;
    }
    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    if( n < 0 ) return operator+=( -n );
    if( col() - n >= 0 ) {
      m_col -= n;
    } else if( line() > 0 ) {
      n -= col() + 1;
      m_line--;
      m_col = m_doc.lineLength( line() );
      operator-=( n );
    } else {
      m_col = 0;
    }
    Q_ASSERT( valid() );
    return *this;
  }
};

void KateViewInternal::moveChar( Bias bias, bool sel )
{
  KateTextCursor c;
  // Wrap cursor off and dynamic word wrap are mutually exclusive for now, supporting it would be too hackish
  if ( m_view->dynWordWrap() || m_doc->wrapCursor() ) {
    c = WrappingCursor( *m_doc, cursor ) += bias;
  } else {
    c = BoundedCursor( *m_doc, cursor ) += bias;
  }
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorLeft(  bool sel ) { moveChar( left,  sel ); }
void KateViewInternal::cursorRight( bool sel ) { moveChar( right, sel ); }

void KateViewInternal::moveWord( Bias bias, bool sel )
{
  // This matches the word-moving in QTextEdit, QLineEdit etc.

  WrappingCursor c( *m_doc, cursor );
  if( !c.atEdge( bias ) ) {
    Highlight* h = m_doc->highlight();

    bool moved = false;
    while( !c.atEdge( bias ) && !h->isInWord( m_doc->textLine( c.line() )[ c.col() - (bias == left ? 1 : 0) ] ) )
    {
      c += bias;
      moved = true;
    }

    if ( bias != right || !moved )
    {
      while( !c.atEdge( bias ) &&  h->isInWord( m_doc->textLine( c.line() )[ c.col() - (bias == left ? 1 : 0) ] ) )
        c += bias;
      if ( bias == right )
      {
        while ( !c.atEdge( bias ) && m_doc->textLine( c.line() )[ c.col() ].isSpace() )
          c+= bias;
      }
    }

  } else {
    c += bias;
  }
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::wordLeft ( bool sel ) { moveWord( left,  sel ); }
void KateViewInternal::wordRight( bool sel ) { moveWord( right, sel ); }

void KateViewInternal::moveEdge( Bias bias, bool sel )
{
  BoundedCursor c( *m_doc, cursor );
  c.toEdge( bias );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::home( bool sel )
{
  if (m_view->dynWordWrap() && currentRange().startCol) {
    // Allow us to go to the real start if we're already at the start of the view line
    if (cursor.col() != currentRange().startCol) {
      KateTextCursor c(cursor.line(), currentRange().startCol);
      updateSelection( c, sel );
      updateCursor( c );
      return;
    }
  }

  if( !(m_doc->configFlags() & KateDocument::cfSmartHome) ) {
    moveEdge( left, sel );
    return;
  }

  KateTextCursor c = cursor;
  int lc = m_doc->kateTextLine( c.line() )->firstChar();

  if( lc < 0 || c.col() == lc ) {
    c.setCol(0);
  } else {
    c.setCol(lc);
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::end( bool sel )
{
  if (m_view->dynWordWrap() && currentRange().wrap) {
    // Allow us to go to the real end if we're already at the end of the view line
    if (cursor.col() < currentRange().endCol - 1) {
      KateTextCursor c(cursor.line(), currentRange().endCol - 1);
      updateSelection( c, sel );
      updateCursor( c );
      return;
    }
  }

  moveEdge( right, sel );
}

LineRange KateViewInternal::range(int realLine, const LineRange* previous)
{
  // look at the cache first
  if (!m_updatingView && realLine >= lineRanges[0].line && realLine <= lineRanges[lineRanges.count() - 1].line)
    for (uint i = 0; i < lineRanges.count(); i++)
      if (realLine == lineRanges[i].line)
        if (!m_view->dynWordWrap() || (!previous && lineRanges[i].startCol == 0) || (previous && lineRanges[i].startCol == previous->endCol))
          return lineRanges[i];

  // Not in the cache, we have to create it
  LineRange ret;

  TextLine::Ptr text = m_doc->kateTextLine(realLine);
  if (!text) {
    return LineRange();
  }

  if (!m_view->dynWordWrap()) {
    Q_ASSERT(!previous);
    ret.line = realLine;
    ret.virtualLine = m_doc->getVirtualLine(realLine);
    ret.startCol = 0;
    ret.endCol = m_doc->lineLength(realLine);
    ret.startX = 0;
    ret.endX = m_view->renderer()->textWidth(text, -1);
    ret.viewLine = 0;
    ret.wrap = false;
    return ret;
  }

  ret.endCol = (int)m_view->renderer()->textWidth(text, previous ? previous->endCol : 0, width() - (previous ? previous->shiftX : 0), &ret.wrap, &ret.endX);

  Q_ASSERT(ret.endCol > ret.startCol);

  ret.line = realLine;

  if (previous) {
    ret.virtualLine = previous->virtualLine;
    ret.startCol = previous->endCol;
    ret.startX = previous->endX;
    ret.endX += previous->endX;
    ret.shiftX = previous->shiftX;
    ret.viewLine = previous->viewLine + 1;

  } else {
    // TODO worthwhile optimising this to get the data out of the initial textWidth call?
    if (true /** make optional */) {
  int pos = text->nextNonSpaceChar(0);

    if (pos > 0)
      ret.shiftX = m_view->renderer()->textWidth(text, pos);

    if (ret.shiftX > ((double)width() / 100 * 80 /* FIXME customisable percentage */))
      ret.shiftX = 0;
  }

    ret.virtualLine = m_doc->getVirtualLine(realLine);
    ret.startCol = 0;
    ret.startX = 0;
    ret.viewLine = 0;
  }

  return ret;
}

LineRange KateViewInternal::currentRange()
{
  Q_ASSERT(m_view->dynWordWrap());

  return range(cursor);
}

LineRange KateViewInternal::previousRange()
{
  uint currentViewLine = viewLine(cursor);

  if (currentViewLine)
    return range(cursor.line(), currentViewLine - 1);
  else
    return range(m_doc->getRealLine(displayCursor.line() - 1), -1);
}

LineRange KateViewInternal::nextRange()
{
  uint currentViewLine = viewLine(cursor) + 1;

  if (currentViewLine >= viewLineCount(cursor.line())) {
    currentViewLine = 0;
    return range(cursor.line() + 1, currentViewLine);
  } else {
    return range(cursor.line(), currentViewLine);
  }
}

LineRange KateViewInternal::range(const KateTextCursor& realCursor)
{
  Q_ASSERT(m_view->dynWordWrap());

  LineRange thisRange;
  bool first = true;

  do {
    thisRange = range(realCursor.line(), first ? 0L : &thisRange);
    first = false;
  } while (thisRange.wrap && !(realCursor.col() >= thisRange.startCol && realCursor.col() < thisRange.endCol) && thisRange.startCol != thisRange.endCol);

  return thisRange;
}

LineRange KateViewInternal::range(uint realLine, int viewLine)
{
  Q_ASSERT(m_view->dynWordWrap());

  LineRange thisRange;
  bool first = true;

  do {
    thisRange = range(realLine, first ? 0L : &thisRange);
    first = false;
  } while (thisRange.wrap && viewLine != thisRange.viewLine && thisRange.startCol != thisRange.endCol);

  if (viewLine != -1 && viewLine != thisRange.viewLine)
    kdDebug(13030) << "WARNING: viewLine " << viewLine << " of line " << realLine << " does not exist." << endl;

  return thisRange;
}

/**
 * This returns the view line upon which realCursor is situated.
 * The view line is the number of lines in the view from the first line
 * The supplied cursor should be in real lines.
 */
uint KateViewInternal::viewLine(const KateTextCursor& realCursor)
{
  if (!m_view->dynWordWrap()) return 0;

  if (realCursor.col() == 0) return 0;

  LineRange thisRange;
  bool first = true;

  do {
    thisRange = range(realCursor.line(), first ? 0L : &thisRange);
    first = false;
  } while (thisRange.wrap && !(realCursor.col() >= thisRange.startCol && realCursor.col() < thisRange.endCol) && thisRange.startCol != thisRange.endCol);

  return thisRange.viewLine;
}

int KateViewInternal::displayViewLine(const KateTextCursor& virtualCursor, bool limitToVisible)
{
  KateTextCursor work = startPos();

  int limit = linesDisplayed();

  // Efficient non-word-wrapped path
  if (!m_view->dynWordWrap()) {
    int ret = virtualCursor.line() - startLine();
    if (limitToVisible && (ret < 0 || ret > limit))
      return -1;
    else
      return ret;
  }

  if (work == virtualCursor) {
    return 0;
  }

  int ret = -viewLine(work);
  bool forwards = (work < virtualCursor) ? true : false;

  // FIXME switch to using ranges? faster?
  if (forwards) {
    while (work.line() != virtualCursor.line()) {
      ret += viewLineCount(m_doc->getRealLine(work.line()));
      work.setLine(work.line() + 1);
      if (limitToVisible && ret > limit)
        return -1;
    }
  } else {
    while (work.line() != virtualCursor.line()) {
      work.setLine(work.line() - 1);
      ret -= viewLineCount(m_doc->getRealLine(work.line()));
      if (limitToVisible && ret < 0)
        return -1;
    }
  }

  // final difference
  KateTextCursor realCursor = virtualCursor;
  realCursor.setLine(m_doc->getRealLine(realCursor.line()));
  if (realCursor.col() == -1) realCursor.setCol(m_doc->lineLength(realCursor.line()));
  ret += viewLine(realCursor);

  if (limitToVisible && (ret < 0 || ret > limit))
    return -1;

  return ret;
}

uint KateViewInternal::lastViewLine(uint realLine)
{
  if (!m_view->dynWordWrap()) return 0;

  LineRange thisRange;
  bool first = true;

  do {
    thisRange = range(realLine, first ? 0L : &thisRange);
    first = false;
  } while (thisRange.wrap && thisRange.startCol != thisRange.endCol);

  return thisRange.viewLine;
}

uint KateViewInternal::viewLineCount(uint realLine)
{
  return lastViewLine(realLine) + 1;
}

/*
 * This returns the cursor which is offset by (offset) view lines.
 * This is the main function which is called by code not specifically dealing with word-wrap.
 * The opposite conversion (cursor to offset) can be done with displayViewLine.
 *
 * The cursors involved are virtual cursors (ie. equivalent to displayCursor)
 */
KateTextCursor KateViewInternal::viewLineOffset(const KateTextCursor& virtualCursor, int offset, bool keepX)
{
  if (!m_view->dynWordWrap()) {
    KateTextCursor ret(QMIN((int)m_doc->visibleLines() - 1, virtualCursor.line() + offset), 0);

    if (ret.line() < 0)
      ret.setLine(0);

    if (keepX) {
      int realLine = m_doc->getRealLine(ret.line());
      ret.setCol(m_doc->lineLength(realLine) - 1);

      if (m_currentMaxX > cXPos)
        cXPos = m_currentMaxX;

      if (m_doc->wrapCursor())
        cXPos = QMIN(cXPos, (int)m_view->renderer()->textWidth(m_doc->kateTextLine(realLine), m_doc->lineLength(realLine)));

      m_view->renderer()->textWidth(ret, cXPos);
    }

    return ret;
  }

  KateTextCursor realCursor = virtualCursor;
  realCursor.setLine(m_doc->getRealLine(virtualCursor.line()));

  uint cursorViewLine = viewLine(realCursor);

  int currentOffset = 0;
  int virtualLine = 0;

  bool forwards = (offset > 0) ? true : false;

  if (forwards) {
    currentOffset = lastViewLine(realCursor.line()) - cursorViewLine;
    if (offset <= currentOffset) {
      // the answer is on the same line
      LineRange thisRange = range(realCursor.line(), cursorViewLine + offset);
      Q_ASSERT(thisRange.virtualLine == virtualCursor.line());
      return KateTextCursor(virtualCursor.line(), thisRange.startCol);
    }

    virtualLine = virtualCursor.line() + 1;

  } else {
    offset = -offset;
    currentOffset = cursorViewLine;
    if (offset <= currentOffset) {
      // the answer is on the same line
      LineRange thisRange = range(realCursor.line(), cursorViewLine - offset);
      Q_ASSERT(thisRange.virtualLine == virtualCursor.line());
      return KateTextCursor(virtualCursor.line(), thisRange.startCol);
    }

    virtualLine = virtualCursor.line() - 1;
  }

  currentOffset++;

  while (virtualLine >= 0 && virtualLine < (int)m_doc->visibleLines())
  {
    LineRange thisRange;
    bool first = true;
    int realLine = m_doc->getRealLine(virtualLine);

    do {
      thisRange = range(realLine, first ? 0L : &thisRange);
      first = false;

      if (offset == currentOffset) {
        if (!forwards) {
          // We actually want it the other way around
          int requiredViewLine = lastViewLine(realLine) - thisRange.viewLine;
          if (requiredViewLine != thisRange.viewLine) {
            thisRange = range(realLine, requiredViewLine);
          }
        }

        KateTextCursor ret(virtualLine, thisRange.startCol);

        // keep column position
        if (keepX) {
          ret.setCol(thisRange.endCol - 1);
          KateTextCursor realCursorTemp(m_doc->getRealLine(virtualCursor.line()), virtualCursor.col());
          int visibleX = m_view->renderer()->textWidth(realCursorTemp) - range(realCursorTemp).startX;
          int xOffset = thisRange.startX;

          if (m_currentMaxX > visibleX)
            visibleX = m_currentMaxX;

          cXPos = xOffset + visibleX;

          cXPos = QMIN(cXPos, lineMaxCursorX(thisRange));

          m_view->renderer()->textWidth(ret, cXPos);
        }

        return ret;
      }

      currentOffset++;

    } while (thisRange.wrap);

    if (forwards)
      virtualLine++;
    else
      virtualLine--;
  }

  // Looks like we were asked for something a bit exotic.
  // Return the max/min valid position.
  if (forwards)
    return KateTextCursor(m_doc->visibleLines() - 1, m_doc->lineLength(m_doc->visibleLines() - 1));
  else
    return KateTextCursor(0, 0);
}

int KateViewInternal::lineMaxCursorX(const LineRange& range)
{
  int maxX = range.endX;

  if (maxX && range.wrap) {
    QChar lastCharInLine = m_doc->kateTextLine(range.line)->getChar(range.endCol - 1);
    maxX -= m_view->renderer()->config()->fontMetrics()->width(lastCharInLine);
  }

  return maxX;
}

int KateViewInternal::lineMaxCol(const LineRange& range)
{
  int maxCol = range.endCol;

  if (maxCol && range.wrap)
    maxCol--;

  return maxCol;
}

void KateViewInternal::cursorUp(bool sel)
{
  if (displayCursor.line() == 0 && (!m_view->dynWordWrap() || viewLine(cursor) == 0))
    return;

  int newLine = cursor.line(), newCol = 0, xOffset = 0, startCol = 0;
  m_preserveMaxX = true;

  if (m_view->dynWordWrap()) {
    // Dynamic word wrapping - navigate on visual lines rather than real lines
    LineRange thisRange = currentRange();
    // This is not the first line because that is already simplified out above
    LineRange pRange = previousRange();

    // Ensure we're in the right spot
    Q_ASSERT((cursor.line() == thisRange.line) &&
             (cursor.col() >= thisRange.startCol) &&
             (!thisRange.wrap || cursor.col() < thisRange.endCol));

    int visibleX = m_view->renderer()->textWidth(cursor) - thisRange.startX;

    startCol = pRange.startCol;
    xOffset = pRange.startX;
    newLine = pRange.line;

    //kdDebug() << k_funcinfo << m_currentMaxX << " " << visibleX << endl;

    // FIXME FIXME move this calculation into a seperate function
    if (m_currentMaxX - thisRange.xOffset() > visibleX)
      visibleX = m_currentMaxX - thisRange.xOffset();

    //kdDebug() << m_currentMaxX << " " << visibleX << endl;

    cXPos = xOffset + visibleX;

    //kdDebug() << cXPos << endl;

    if (pRange.startX == 0 && pRange.wrap)
      cXPos += pRange.shiftX;

    //kdDebug() << cXPos << endl;

    cXPos = QMIN(cXPos, lineMaxCursorX(pRange));

    newCol = QMIN((int)m_view->renderer()->textPos(newLine, visibleX, startCol), lineMaxCol(pRange));

  } else {
    newLine = m_doc->getRealLine(displayCursor.line() - 1);

    if ((m_doc->wrapCursor()) && m_currentMaxX > cXPos)
      cXPos = m_currentMaxX;
  }

  KateTextCursor c(newLine, newCol);
  m_view->renderer()->textWidth(c, cXPos);

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  if ((displayCursor.line() >= (int)m_doc->numVisLines() - 1) && (!m_view->dynWordWrap() || viewLine(cursor) == lastViewLine(cursor.line())))
    return;

  int newLine = cursor.line(), newCol = 0, xOffset = 0, startCol = 0;
  m_preserveMaxX = true;

  if (m_view->dynWordWrap()) {
    // Dynamic word wrapping - navigate on visual lines rather than real lines
    LineRange thisRange = currentRange();
    // This is not the last line because that is already simplified out above
    LineRange nRange = nextRange();

    // Ensure we're in the right spot
    Q_ASSERT((cursor.line() == thisRange.line) &&
             (cursor.col() >= thisRange.startCol) &&
             (!thisRange.wrap || cursor.col() < thisRange.endCol));

    int visibleX = m_view->renderer()->textWidth(cursor) - thisRange.startX;

    if (!thisRange.wrap) {
      newLine = m_doc->getRealLine(displayCursor.line() + 1);
    } else {
      startCol = thisRange.endCol;
      xOffset = thisRange.endX;
    }

/*    kdDebug() << k_funcinfo << m_currentMaxX << " " << visibleX << " " << cXPos << endl;*/

    if (m_currentMaxX - nRange.xOffset() > visibleX)
      visibleX = m_currentMaxX - nRange.xOffset();

    cXPos = xOffset + visibleX;

    if (thisRange.startX == 0 && thisRange.wrap)
      cXPos = QMAX(xOffset, cXPos - thisRange.shiftX);

/*    kdDebug() << k_funcinfo << m_view->renderer()->textWidth(cursor) << " " << m_currentMaxX << " " << visibleX << " " << cXPos << endl;*/

    cXPos = QMIN(cXPos, lineMaxCursorX(nRange));

//     kdDebug() << k_funcinfo << cXPos << endl;

    newCol = QMIN((int)m_view->renderer()->textPos(newLine, visibleX, startCol), lineMaxCol(nRange));

//     kdDebug() << thisRange.startX << " -> " << thisRange.endX << ", " << nRange.startX << " -> " << nRange.endX << endl;

  } else {
    newLine = m_doc->getRealLine(displayCursor.line() + 1);

    if ((m_doc->wrapCursor()) && m_currentMaxX > cXPos)
      cXPos = m_currentMaxX;
  }

  KateTextCursor c(newLine, newCol);
  m_view->renderer()->textWidth(c, cXPos);

  updateSelection(c, sel);
  updateCursor(c);
}

void KateViewInternal::cursorToMatchingBracket( bool sel )
{
  KateTextCursor start( cursor ), end;

  if( !m_doc->findMatchingBracket( start, end ) )
    return;

  // The cursor is now placed just to the left of the matching bracket.
  // If it's an ending bracket, put it to the right (so we can easily
  // get back to the original bracket).
  if( end > start )
    end.setCol(end.col() + 1);

  updateSelection( end, sel );
  updateCursor( end );
}

void KateViewInternal::topOfView( bool sel )
{
  KateTextCursor c = viewLineOffset(startPos(), m_minLinesVisible);
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottomOfView( bool sel )
{
  // FIXME account for wordwrap
  KateTextCursor c = viewLineOffset(endPos(), -m_minLinesVisible);
  updateSelection( c, sel );
  updateCursor( c );
}

// lines is the offset to scroll by
void KateViewInternal::scrollLines( int lines, bool sel )
{
  KateTextCursor c = viewLineOffset(displayCursor, lines, true);

  // Fix the virtual cursor -> real cursor
  c.setLine(m_doc->getRealLine(c.line()));

  updateSelection( c, sel );
  updateCursor( c );
}

// This is a bit misleading... it's asking for the view to be scrolled, not the cursor
void KateViewInternal::scrollUp()
{
  KateTextCursor newPos = viewLineOffset(m_startPos, -1);
  scrollPos(newPos);
}

void KateViewInternal::scrollDown()
{
  KateTextCursor newPos = viewLineOffset(m_startPos, 1);
  scrollPos(newPos);
}

void KateViewInternal::setAutoCenterLines(int viewLines, bool updateView)
{
  m_autoCenterLines = viewLines;
  m_minLinesVisible = QMIN(int((linesDisplayed() - 1)/2), m_autoCenterLines);
  if (updateView)
    KateViewInternal::updateView();
}

void KateViewInternal::pageUp( bool sel )
{
  // remember the view line and x pos
  int viewLine = displayViewLine(displayCursor);
  bool atTop = (startPos().line() == 0 && startPos().col() == 0);

  // Adjust for an auto-centering cursor
  int lineadj = 2 * m_minLinesVisible;
  int cursorStart = (linesDisplayed() - 1) - viewLine;
  if (cursorStart < m_minLinesVisible)
    lineadj -= m_minLinesVisible - cursorStart;

  int linesToScroll = -QMAX( (linesDisplayed() - 1) - lineadj, 0 );
  m_preserveMaxX = true;

  // don't scroll the full view in case the scrollbar appears
  if (!m_view->dynWordWrap()) {
    if (scrollbarVisible(startLine() + linesToScroll + viewLine)) {
      if (!m_columnScrollDisplayed) {
        linesToScroll++;
      }
    } else {
      if (m_columnScrollDisplayed) {
        linesToScroll--;
      }
    }
  }

  if (!m_doc->pageUpDownMovesCursor () && !atTop) {
    int xPos = m_view->renderer()->textWidth(cursor) - currentRange().startX;

    KateTextCursor newStartPos = viewLineOffset(startPos(), linesToScroll - 1);
    scrollPos(newStartPos);

    // put the cursor back approximately where it was
    KateTextCursor newPos = viewLineOffset(newStartPos, viewLine, true);
    newPos.setLine(m_doc->getRealLine(newPos.line()));

    LineRange newLine = range(newPos);

    if (m_currentMaxX - newLine.xOffset() > xPos)
      xPos = m_currentMaxX - newLine.xOffset();

    cXPos = QMIN(newLine.startX + xPos, lineMaxCursorX(newLine));

    m_view->renderer()->textWidth( newPos, cXPos );

    m_preserveMaxX = true;
    updateSelection( newPos, sel );
    updateCursor(newPos);

  } else {
    scrollLines( linesToScroll, sel );
  }
}

void KateViewInternal::pageDown( bool sel )
{
  // remember the view line
  int viewLine = displayViewLine(displayCursor);
  bool atEnd = startPos() >= m_cachedMaxStartPos;

  // Adjust for an auto-centering cursor
  int lineadj = 2 * m_minLinesVisible;
  int cursorStart = m_minLinesVisible - viewLine;
  if (cursorStart > 0)
    lineadj -= cursorStart;

  int linesToScroll = QMAX( (linesDisplayed() - 1) - lineadj, 0 );
  m_preserveMaxX = true;

  // don't scroll the full view in case the scrollbar appears
  if (!m_view->dynWordWrap()) {
    if (scrollbarVisible(startLine() + linesToScroll + viewLine - (linesDisplayed() - 1))) {
      if (!m_columnScrollDisplayed) {
        linesToScroll--;
      }
    } else {
      if (m_columnScrollDisplayed) {
        linesToScroll--;
      }
    }
  }

  if (!m_doc->pageUpDownMovesCursor () && !atEnd) {
    int xPos = m_view->renderer()->textWidth(cursor) - currentRange().startX;

    KateTextCursor newStartPos = viewLineOffset(startPos(), linesToScroll + 1);
    scrollPos(newStartPos);

    // put the cursor back approximately where it was
    KateTextCursor newPos = viewLineOffset(newStartPos, viewLine, true);
    newPos.setLine(m_doc->getRealLine(newPos.line()));

    LineRange newLine = range(newPos);

    if (m_currentMaxX - newLine.xOffset() > xPos)
      xPos = m_currentMaxX - newLine.xOffset();

    cXPos = QMIN(newLine.startX + xPos, lineMaxCursorX(newLine));

    m_view->renderer()->textWidth( newPos, cXPos );

    m_preserveMaxX = true;
    updateSelection( newPos, sel );
    updateCursor(newPos);

  } else {
    scrollLines( linesToScroll, sel );
  }
}

bool KateViewInternal::scrollbarVisible(uint startLine)
{
  return maxLen(startLine) > width() - 8;
}

int KateViewInternal::maxLen(uint startLine)
{
  Q_ASSERT(!m_view->dynWordWrap());

  int displayLines = (m_view->height() / m_view->renderer()->fontHeight()) + 1;

  int maxLen = 0;

  for (int z = 0; z < displayLines; z++) {
    int virtualLine = startLine + z;

    if (virtualLine < 0 || virtualLine >= (int)m_doc->visibleLines())
      break;

    LineRange thisRange = range((int)m_doc->getRealLine(virtualLine));

    maxLen = QMAX(maxLen, thisRange.endX);
  }

  return maxLen;
}

void KateViewInternal::top( bool sel )
{
  KateTextCursor c( 0, cursor.col() );
  m_view->renderer()->textWidth( c, cXPos );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom( bool sel )
{
  KateTextCursor c( m_doc->lastLine(), cursor.col() );
  m_view->renderer()->textWidth( c, cXPos );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::top_home( bool sel )
{
  KateTextCursor c( 0, 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom_end( bool sel )
{
  KateTextCursor c( m_doc->lastLine(), m_doc->lineLength( m_doc->lastLine() ) );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::updateSelection( const KateTextCursor& newCursor, bool keepSel )
{
  if( keepSel )
  {
    m_doc->selectTo( cursor, newCursor );
    m_selChangedByUser = true;
  }
  else if ( !(m_doc->configFlags() & KateDocument::cfPersistent) )
    m_doc->clearSelection();
}

void KateViewInternal::updateCursor( const KateTextCursor& newCursor, bool force )
{
  if ( !force && (cursor == newCursor) ) {
    if ( !m_madeVisible ) {
      makeVisible ( displayCursor, displayCursor.col() );
    }
    return;
  }

  // unfold if required
  TextLine::Ptr l = m_doc->kateTextLine( newCursor.line() );
  if ( l && ! l->isVisible() )
    m_doc->foldingTree()->ensureVisible( newCursor.line() );

  KateTextCursor oldDisplayCursor = displayCursor;

  cursor.setPos (newCursor);
  displayCursor.setPos (m_doc->getVirtualLine(cursor.line()), cursor.col());

  cXPos = m_view->renderer()->textWidth( cursor );
  makeVisible ( displayCursor, displayCursor.col() );

  updateBracketMarks();

  // It's efficient enough to just tag them both without checking to see if they're on the same view line
  tagLine(oldDisplayCursor);
  tagLine(displayCursor);

  QPoint cursorP = cursorCoordinates();
  setMicroFocusHint( cursorP.x(), cursorP.y(), 0, m_view->renderer()->fontHeight() );

  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
    m_view->renderer()->setDrawCaret(true);
  }

  // Remember the maximum X position if requested
  if (m_preserveMaxX)
    m_preserveMaxX = false;
  else
    if (m_view->dynWordWrap())
      m_currentMaxX = m_view->renderer()->textWidth(displayCursor) - currentRange().startX + currentRange().xOffset();
    else
      m_currentMaxX = cXPos;

  //kdDebug() << "m_currentMaxX: " << m_currentMaxX << ", cXPos: " << cXPos << endl;
  //kdDebug(13030) << "Cursor now located at real " << cursor.line << "," << cursor.col << ", virtual " << displayCursor.line << ", " << displayCursor.col << "; Top is " << startLine() << ", " << startPos().col << "; Old top is " << m_oldStartPos.line << ", " << m_oldStartPos.col << endl;

  paintText(0, 0, width(), height(), true);

  emit m_view->cursorPositionChanged();
}

void KateViewInternal::updateBracketMarks()
{
  if ( bm.isValid() ) {
    KateTextCursor bmStart(m_doc->getVirtualLine(bm.start().line()), bm.start().col());
    KateTextCursor bmEnd(m_doc->getVirtualLine(bm.end().line()), bm.end().col());
    tagLine(bmStart);
    tagLine(bmEnd);
  }

  m_doc->newBracketMark( cursor, bm );

  if ( bm.isValid() ) {
    KateTextCursor bmStart(m_doc->getVirtualLine(bm.start().line()), bm.start().col());
    KateTextCursor bmEnd(m_doc->getVirtualLine(bm.end().line()), bm.end().col());
    tagLine(bmStart);
    tagLine(bmEnd);
  }
}

bool KateViewInternal::tagLine(const KateTextCursor& virtualCursor)
{
  int viewLine = displayViewLine(virtualCursor, true);
  if (viewLine >= 0 && viewLine < (int)lineRanges.count()) {
    lineRanges[viewLine].dirty = true;
    leftBorder->update (0, lineToY(viewLine), leftBorder->width(), m_view->renderer()->fontHeight());
    return true;
  }
  return false;
}

bool KateViewInternal::tagLines( int start, int end, bool realLines )
{
  return tagLines(KateTextCursor(start, 0), KateTextCursor(end, -1), realLines);
}

bool KateViewInternal::tagLines(KateTextCursor start, KateTextCursor end, bool realCursors)
{
  if (realCursors)
  {
    //kdDebug()<<"realLines is true"<<endl;
    start.setLine(m_doc->getVirtualLine( start.line() ));
    end.setLine(m_doc->getVirtualLine( end.line() ));
  }

  if (end.line() < (int)startLine())
  {
    //kdDebug()<<"end<startLine"<<endl;
    return false;
  }
  if (start.line() > (int)endLine())
  {
    //kdDebug()<<"start> endLine"<<start<<" "<<((int)endLine())<<endl;
    return false;
  }

  //kdDebug(13030) << "tagLines( [" << start.line << "," << start.col << "], [" << end.line << "," << end.col << "] )\n";

  bool ret = false;

  for (uint z = 0; z < lineRanges.size(); z++)
  {
    if ((lineRanges[z].virtualLine > start.line() || (lineRanges[z].virtualLine == start.line() && lineRanges[z].endCol >= start.col() && start.col() != -1)) && (lineRanges[z].virtualLine < end.line() || (lineRanges[z].virtualLine == end.line() && (lineRanges[z].startCol <= end.col() || end.col() == -1)))) {
      ret = lineRanges[z].dirty = true;
      //kdDebug() << "Tagged line " << lineRanges[z].line << endl;
    }
  }

  if (!m_view->dynWordWrap())
  {
    int y = lineToY( start.line() );
    // FIXME is this enough for when multiple lines are deleted
    int h = (end.line() - start.line() + 2) * m_view->renderer()->fontHeight();
    if (end.line() == (int)m_doc->numVisLines() - 1)
      h = height();

    leftBorder->update (0, y, leftBorder->width(), h);
  }
  else
  {
    // FIXME Do we get enough good info in editRemoveText to optimise this more?
    //bool justTagged = false;
    for (uint z = 0; z < lineRanges.size(); z++)
    {
      if ((lineRanges[z].virtualLine > start.line() || (lineRanges[z].virtualLine == start.line() && lineRanges[z].endCol >= start.col() && start.col() != -1)) && (lineRanges[z].virtualLine < end.line() || (lineRanges[z].virtualLine == end.line() && (lineRanges[z].startCol <= end.col() || end.col() == -1))))
      {
        //justTagged = true;
        leftBorder->update (0, z * m_view->renderer()->fontHeight(), leftBorder->width(), leftBorder->height());
        break;
      }
      /*else if (justTagged)
      {
        justTagged = false;
        leftBorder->update (0, z * m_doc->viewFont.fontHeight, leftBorder->width(), m_doc->viewFont.fontHeight);
        break;
      }*/
    }
  }

  return ret;
}

void KateViewInternal::tagAll()
{
  //kdDebug(13030) << "tagAll()" << endl;
  for (uint z = 0; z < lineRanges.size(); z++)
  {
      lineRanges[z].dirty = true;
  }

  leftBorder->updateFont();
  leftBorder->update ();
}

void KateViewInternal::paintCursor()
{
  if (tagLine(displayCursor))
    paintText (0,0,width(), height(), true);
}

// Point in content coordinates
void KateViewInternal::placeCursor( const QPoint& p, bool keepSelection, bool updateSelection )
{
  LineRange thisRange = yToLineRange(p.y());

  if (thisRange.line == -1) {
    for (int i = (p.y() / m_view->renderer()->fontHeight()); i >= 0; i--) {
      thisRange = lineRanges[i];
      if (thisRange.line != -1)
        break;
    }
    Q_ASSERT(thisRange.line != -1);
  }

  int realLine = thisRange.line;
  int visibleLine = thisRange.virtualLine;
  uint startCol = thisRange.startCol;

  visibleLine = QMAX( 0, QMIN( visibleLine, int(m_doc->numVisLines()) - 1 ) );

  KateTextCursor c(realLine, 0);

  int x = QMIN(QMAX(0, p.x() - thisRange.xOffset()), lineMaxCursorX(thisRange) - thisRange.startX);

  m_view->renderer()->textWidth( c, startX() + x, startCol);

  if (updateSelection)
    KateViewInternal::updateSelection( c, keepSelection );
  updateCursor( c );
}

// Point in content coordinates
bool KateViewInternal::isTargetSelected( const QPoint& p )
{
  LineRange thisRange = yToLineRange(p.y());

  TextLine::Ptr textLine = m_doc->kateTextLine( thisRange.line );
  if( !textLine )
    return false;

  int col = m_view->renderer()->textPos( textLine, p.x() - thisRange.xOffset(), thisRange.startCol );

  return m_doc->lineColSelected( thisRange.virtualLine, col );
}

//
// START EVENT HANDLING STUFF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

bool KateViewInternal::eventFilter( QObject *obj, QEvent *e )
{
  if (obj == m_lineScroll)
  {
    // the second condition is to make sure a scroll on the vertical bar doesn't cause a horizontal scroll ;)
    if (e->type() == QEvent::Wheel && m_lineScroll->minValue() != m_lineScroll->maxValue())
    {
      wheelEvent((QWheelEvent*)e);
      return true;
    }

    // continue processing
    return QWidget::eventFilter( obj, e );
  }

  switch( e->type() )
  {
    case QEvent::KeyPress:
    {
      QKeyEvent *k = (QKeyEvent *)e;

      if ((k->key() == Qt::Key_Escape) && !(m_doc->configFlags() & KateDocument::cfPersistent) )
      {
        m_doc->clearSelection();
        return true;
      }
      else if ( !((k->state() & ControlButton) || (k->state() & AltButton)) )
      {
        keyPressEvent( k );
        return k->isAccepted();
      }

    } break;

    case QEvent::DragMove:
    {
      QPoint currentPoint = ((QDragMoveEvent*) e)->pos();

      QRect doNotScrollRegion( scrollMargin, scrollMargin,
                          width() - scrollMargin * 2,
                          height() - scrollMargin * 2 );

      if ( !doNotScrollRegion.contains( currentPoint ) )
      {
          startDragScroll();
          // Keep sending move events
          ( (QDragMoveEvent*)e )->accept( QRect(0,0,0,0) );
      }

      dragMoveEvent((QDragMoveEvent*)e);
    } break;

    case QEvent::DragLeave:
      stopDragScroll();
      break;

    default:
      break;
  }

  return QWidget::eventFilter( obj, e );
}

void KateViewInternal::keyPressEvent( QKeyEvent* e )
{
  KKey key(e);

   if (key == Qt::Key_Left)
  {
    m_view->cursorLeft();
    e->accept();
    return;
  }

  if (key == Qt::Key_Right)
  {
    m_view->cursorRight();
    e->accept();
    return;
  }

  if (key == Qt::Key_Down)
  {
    m_view->down();
    e->accept();
    return;
  }

  if (key == Qt::Key_Up)
  {
    m_view->up();
    e->accept();
    return;
  }

  if( !m_doc->isReadWrite() )
  {
    e->ignore();
    return;
  }

  if ((key == Qt::Key_Return) || (key == Qt::Key_Enter) ||
      (key == SHIFT + Qt::Key_Return) || (key == SHIFT + Qt::Key_Enter))
  {
    m_view->keyReturn();
    e->accept();
    return;
  }

  if (key == Qt::Key_Backspace || key == SHIFT + Qt::Key_Backspace)
  {
    m_view->backspace();
    e->accept();
    return;
  }

  if (key == Qt::Key_Delete)
  {
    m_view->keyDelete();
    e->accept();
    return;
  }

  if( (key == Qt::Key_Tab || key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab)
      && (m_doc->configFlags() & KateDocumentConfig::cfTabIndents) )
  {
    if( key == Qt::Key_Tab )
    {
      if (m_doc->hasSelection() || (m_doc->configFlags() & KateDocumentConfig::cfTabIndentsMode))
        m_doc->indent( m_view, cursor.line(), 1 );
      else
        m_doc->insertIndentChars ( m_view );

      e->accept();
      return;
    }

    if (key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab)
    {
      m_doc->indent( m_view, cursor.line(), -1 );
      e->accept();
      return;
    }
  }

  if ( !(e->state() & ControlButton) && !(e->state() & AltButton)
       && m_doc->typeChars ( m_view, e->text() ) )
  {
    e->accept();
    return;
  }

  e->ignore();
}

void KateViewInternal::keyReleaseEvent( QKeyEvent* e )
{
  KKey key(e);

  if (key == SHIFT)
    m_shiftKeyPressed = true;
  else
  {
    if (m_shiftKeyPressed)
    {
      m_shiftKeyPressed = false;

      if (m_selChangedByUser)
      {
        QApplication::clipboard()->setSelectionMode( true );
        m_doc->copy();
        QApplication::clipboard()->setSelectionMode( false );

        m_selChangedByUser = false;
      }
    }
  }

  e->ignore();
  return;
}

void KateViewInternal::mousePressEvent( QMouseEvent* e )
{
  switch (e->button())
  {
    case LeftButton:
        m_selChangedByUser = false;

        if (possibleTripleClick)
        {
          possibleTripleClick = false;

          m_doc->selectLine( cursor );
          QApplication::clipboard()->setSelectionMode( true );
          m_doc->copy();
          QApplication::clipboard()->setSelectionMode( false );

          cursor.setCol(0);
          updateCursor( cursor );
          return;
        }

        if( isTargetSelected( e->pos() ) )
        {
          dragInfo.state = diPending;
          dragInfo.start = e->pos();
        }
        else
        {
          dragInfo.state = diNone;

          placeCursor( e->pos(), e->state() & ShiftButton );
          scrollX = 0;
          scrollY = 0;
          if( !scrollTimer )
            scrollTimer = startTimer(50);
        }

        e->accept ();
        break;

    case RightButton:
      if ( !isTargetSelected( e->pos() ) )
        placeCursor( e->pos() );

      if (leftBorder->positionToArea( e->pos() ) != KateIconBorder::IconBorder)
        if (m_view->popup())
            m_view->popup()->popup( mapToGlobal( e->pos() ) );

      e->accept ();
      break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e)
{
  switch (e->button())
  {
    case LeftButton:
      m_doc->selectWord( cursor );

      // Move cursor to end of selected word
      if (m_doc->hasSelection())
      {
        QApplication::clipboard()->setSelectionMode( true );
        m_doc->copy();
        QApplication::clipboard()->setSelectionMode( false );

        cursor.setPos(m_doc->selectEnd);
        updateCursor( cursor );
      }

      possibleTripleClick = true;
      QTimer::singleShot ( QApplication::doubleClickInterval(), this, SLOT(tripleClickTimeout()) );

      e->accept ();
      break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::tripleClickTimeout()
{
  possibleTripleClick = false;
}

void KateViewInternal::mouseReleaseEvent( QMouseEvent* e )
{
  switch (e->button())
  {
    case LeftButton:
      if (m_selChangedByUser)
      {
        QApplication::clipboard()->setSelectionMode( true );
        m_doc->copy();
        QApplication::clipboard()->setSelectionMode( false );

        m_selChangedByUser = false;
      }

      if (dragInfo.state == diPending)
        placeCursor( e->pos() );
      else if (dragInfo.state == diNone)
      {
        killTimer(scrollTimer);
        scrollTimer = 0;
      }

      dragInfo.state = diNone;

      e->accept ();
      break;

    case MidButton:
      placeCursor( e->pos() );

      if( m_doc->isReadWrite() )
      {
        QApplication::clipboard()->setSelectionMode( true );
        doPaste();
        QApplication::clipboard()->setSelectionMode( false );
      }

      e->accept ();
      break;

    default:
      e->ignore ();
      break;
  }
}

void KateViewInternal::mouseMoveEvent( QMouseEvent* e )
{
  if( e->state() & LeftButton )
  {
    if (dragInfo.state == diPending)
    {
      // we had a mouse down, but haven't confirmed a drag yet
      // if the mouse has moved sufficiently, we will confirm
      QPoint p( e->pos() - dragInfo.start );

      // we've left the drag square, we can start a real drag operation now
      if( p.manhattanLength() > KGlobalSettings::dndEventDelay() )
        doDrag();

      return;
    }

    mouseX = e->x();
    mouseY = e->y();

    scrollX = 0;
    scrollY = 0;
    int d = m_view->renderer()->fontHeight();

    if (mouseX < 0)
      scrollX = -d;

    if (mouseX > width())
      scrollX = d;

    if (mouseY < 0)
    {
      mouseY = 0;
      scrollY = -d;
    }

    if (mouseY > height())
    {
      mouseY = height();
      scrollY = d;
    }

    placeCursor( QPoint( mouseX, mouseY ), true );
  }
  else
  {
    if (m_textHintEnabled)
    {
       if (m_textHintTimer) killTimer(m_textHintTimer);
       m_textHintTimer=startTimer(m_textHintTimeout);
       m_textHintMouseX=e->x();
       m_textHintMouseY=e->y();
    }
  }
}

void KateViewInternal::paintEvent(QPaintEvent *e)
{
  QRect updateR = e->rect();
  paintText (updateR.x(), updateR.y(), updateR.width(), updateR.height());
}

void KateViewInternal::resizeEvent(QResizeEvent* e)
{
  bool expandedHorizontally = width() > e->oldSize().width();
  bool expandedVertically = height() > e->oldSize().height();

  m_madeVisible = false;

  if (height() != e->oldSize().height()) {
    setAutoCenterLines(m_autoCenterLines, false);
  }

  if (height() != e->oldSize().height())
    m_cachedMaxStartPos.setPos(-1, -1);

  if (m_view->dynWordWrap()) {
    bool dirtied = false;

    int currentViewLine = displayViewLine(displayCursor, true);

    for (uint i = 0; i < lineRanges.count(); i++) {
      // find the first dirty line
      // the word wrap updateView algorithm is forced to check all lines after a dirty one
      if (lineRanges[i].wrap ||
         (!expandedHorizontally && (lineRanges[i].endX - lineRanges[i].startX) > width())) {
        dirtied = lineRanges[i].dirty = true;
        break;
      }
    }

    if (dirtied || expandedVertically) {
      updateView(true);
      leftBorder->update();

      // keep the cursor on-screen if it was previously
      if (currentViewLine >= 0)
        makeVisible(displayCursor, displayCursor.col());
    }

  } else {
    updateView();

    if (expandedHorizontally && startX() > 0)
      scrollColumns(startX() - (width() - e->oldSize().width()));
  }

  if (expandedVertically) {
    KateTextCursor max = maxStartPos();
    if (startPos() > max)
      scrollPos(max);
  }
}

void KateViewInternal::timerEvent( QTimerEvent* e )
{
  if (e->timerId() == cursorTimer)
  {
    m_view->renderer()->setDrawCaret(!m_view->renderer()->drawCaret());
    paintCursor();
  }
  else if (e->timerId() == scrollTimer && (scrollX | scrollY))
  {
    scrollLines (startPos().line() + (scrollY / (int)m_view->renderer()->fontHeight()));

    placeCursor( QPoint( mouseX, mouseY ), true );
    //kdDebug()<<"scroll timer: X: "<<mouseX<<" Y: "<<mouseY<<endl;
  }
  else if ((e->timerId() == m_textHintTimer) && m_textHintEnabled) //the m_textHintEnabled shouldn't be needed
  {
      killTimer(m_textHintTimer);
      m_textHintTimer=0;

      LineRange thisRange = yToLineRange(m_textHintMouseY);
      if (thisRange.line == -1) return;
      if (m_textHintMouseX> (lineMaxCursorX(thisRange) - thisRange.startX)) return;
      int realLine = thisRange.line;
      int startCol = thisRange.startCol;
      KateTextCursor c(realLine, 0);
      m_view->renderer()->textWidth( c, startX() + m_textHintMouseX, startCol);
      QString tmp;
      emit m_view->needTextHint(c.line(), c.col(), tmp);
      if (!tmp.isEmpty()) kdDebug()<<"Hint text: "<<tmp<<endl;
  }
}

void KateViewInternal::focusInEvent (QFocusEvent *)
{
  cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
  if ((m_textHintTimer==0) && m_textHintEnabled) m_textHintTimer = startTimer( m_textHintTimeout );
  paintCursor();
  m_doc->m_activeView = m_view;
  emit m_view->gotFocus( m_view );
}

void KateViewInternal::focusOutEvent (QFocusEvent *)
{
  if( ! m_view->m_codeCompletion->codeCompletionVisible() )
  {
    if( cursorTimer )
    {
      killTimer( cursorTimer );
      cursorTimer = 0;
    }
    paintCursor();
    emit m_view->lostFocus( m_view );
  }
  if (m_textHintTimer)
  {
  killTimer(m_textHintTimer);
  m_textHintTimer=0;
  }
}

void KateViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(m_doc->selection(), this);
  dragInfo.dragObject->dragCopy();
}

void KateViewInternal::dragEnterEvent( QDragEnterEvent* event )
{
  event->accept( (QTextDrag::canDecode(event) && m_doc->isReadWrite()) ||
                  KURLDrag::canDecode(event) );
}

void KateViewInternal::dragMoveEvent( QDragMoveEvent* event )
{
  // track the cursor to the current drop location
  placeCursor( event->pos(), true, false );
}

void KateViewInternal::dropEvent( QDropEvent* event )
{
  if ( KURLDrag::canDecode(event) ) {

      emit dropEventPass(event);

  } else if ( QTextDrag::canDecode(event) && m_doc->isReadWrite() ) {

    QString text;

    if (!QTextDrag::decode(event, text))
      return;

    // is the source our own document?
    bool priv = false;
    if (event->source() && event->source()->inherits("KateViewInternal"))
      priv = m_doc->ownedView( ((KateViewInternal*)(event->source()))->m_view );

    // dropped on a text selection area?
    bool selected = isTargetSelected( event->pos() );

    if( priv && selected ) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    }

    // atm only copy the text, no move
    m_doc->insertText( cursor.line(), cursor.col(), text );
    placeCursor( event->pos() );

    updateView();
  }
}

//
// END EVENT HANDLING STUFF !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//

void KateViewInternal::clear()
{
  cursor.setPos(0, 0);
  displayCursor.setPos(0, 0);
}

void KateViewInternal::wheelEvent(QWheelEvent* e)
{
  if (m_lineScroll->minValue() != m_lineScroll->maxValue() && e->orientation() != Qt::Horizontal) {
    // React to this as a vertical event
    if ( ( e->state() & ControlButton ) || ( e->state() & ShiftButton ) ) {
      if (e->delta() > 0)
        scrollPrevPage();
      else
        scrollNextPage();
    } else {
      scrollViewLines(-((e->delta() / 120) * QApplication::wheelScrollLines()));
    }

  } else if (!m_columnScroll->isHidden()) {
    QWheelEvent copy = *e;
    QApplication::sendEvent(m_columnScroll, &copy);

  } else {
    e->ignore();
  }
}

void KateViewInternal::startDragScroll()
{
  if ( !m_dragScrollTimer.isActive() ) {
    m_suppressColumnScrollBar = true;
    m_dragScrollTimer.start( scrollTime );
  }
}

void KateViewInternal::stopDragScroll()
{
  m_suppressColumnScrollBar = false;
  m_dragScrollTimer.stop();
  updateView();
}

void KateViewInternal::doDragScroll()
{
  QPoint p = this->mapFromGlobal( QCursor::pos() );

  int dx = 0, dy = 0;
  if ( p.y() < scrollMargin ) {
    dy = p.y() - scrollMargin;
  } else if ( p.y() > height() - scrollMargin ) {
    dy = scrollMargin - (height() - p.y());
  }
  if ( p.x() < scrollMargin ) {
    dx = p.x() - scrollMargin;
  } else if ( p.x() > width() - scrollMargin ) {
    dx = scrollMargin - (width() - p.x());
  }
  dy /= 4;

  if (dy)
    scrollLines(startPos().line() + dy);
  if (dx)
    scrollColumns(m_startX + dx);
  if (!dy && !dx)
    stopDragScroll();
}

void KateViewInternal::enableTextHints(int timeout)
{
  m_textHintTimeout=timeout;
  m_textHintEnabled=true;
  if (!m_textHintTimer) m_textHintTimer=startTimer(timeout);
}

void KateViewInternal::disableTextHints()
{
  m_textHintEnabled=false;
  if (m_textHintTimer)
  {
    killTimer(m_textHintTimer);
    m_textHintTimer=0;
  }
}

// BEGIN EDIT STUFF
void KateViewInternal::editStart()
{
  editSessionNumber++;

  if (editSessionNumber > 1)
    return;

  editIsRunning = true;
  editOldCursor = cursor;
}

void KateViewInternal::editEnd(int editTagLineStart, int editTagLineEnd, bool tagFrom)
{
   if (editSessionNumber == 0)
    return;

  editSessionNumber--;

  if (editSessionNumber > 0)
    return;

  if (tagFrom && (editTagLineStart <= int(m_doc->getRealLine(startLine()))))
    tagAll();
  else
    tagLines (editTagLineStart, tagFrom ? m_doc->lastLine() : editTagLineEnd, true);

  if (editOldCursor == cursor)
    updateBracketMarks();

  updateView(true);

  if (editOldCursor != cursor)
  {
    m_madeVisible = false;
    updateCursor ( cursor, true );
  }
  else
  {
    makeVisible(displayCursor, displayCursor.col());
  }

  editIsRunning = false;
}

void KateViewInternal::editSetCursor (const KateTextCursor &cursor)
{
  if (this->cursor != cursor)
  {
    this->cursor.setPos (cursor);
  }
}
// END

// BEGIN KateScrollBar
KateScrollBar::KateScrollBar (Orientation orientation, QWidget* parent, const char* name)
  : QScrollBar (orientation, parent, name)
  , m_middleMouseDown (false)
{
  connect(this, SIGNAL(valueChanged(int)), SLOT(sliderMaybeMoved(int)));
}

void KateScrollBar::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == MidButton)
    m_middleMouseDown = true;

  QScrollBar::mousePressEvent(e);
}

void KateScrollBar::mouseReleaseEvent(QMouseEvent* e)
{
  QScrollBar::mouseReleaseEvent(e);

  m_middleMouseDown = false;
}

void KateScrollBar::sliderMaybeMoved(int value)
{
  if (m_middleMouseDown)
    emit sliderMMBMoved(value);
}
// END

// kate: space-indent on; indent-width 2; replace-tabs on;
