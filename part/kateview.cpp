/***************************************************************************
                          kateview.cpp  -  description
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

#include "kateview.h"
#include "kateview.moc"

#include "katedocument.h"
#include "katecmd.h"
#include "katefactory.h"
#include "katehighlight.h"
#include "kateviewdialog.h"
#include "katedialogs.h"
#include "katefiledialog.h"

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
#include <qprintdialog.h>
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

#include "katetextline.h"
#include "kateiconborder.h"

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc) : QWidget(view)
{
  waitForPreHighlight=0;
  myView = view;
  myDoc = doc;

  iconBorderWidth  = 16;
  iconBorderHeight = 800;

  QWidget::setCursor(ibeamCursor);
  setBackgroundMode(NoBackground);
  KCursor::setAutoHideCursor( this, true, true );

  setFocusPolicy(StrongFocus);

  xScroll = new QScrollBar(QScrollBar::Horizontal,myView);
  yScroll = new QScrollBar(QScrollBar::Vertical,myView);
  connect(xScroll,SIGNAL(valueChanged(int)),SLOT(changeXPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),SLOT(changeYPos(int)));

  connect( doc, SIGNAL (preHighlightChanged(uint)),this,SLOT(slotPreHighlightUpdate(uint)));

  xPos = 0;
  yPos = 0;
  
  xCoord = 0;
  yCoord = 0;

  scrollTimer = 0;

  cursor.col = 0;
  cursor.line = 0;
  cursorOn = false;
  cursorTimer = 0;
  cXPos = 0;
  cOldXPos = 0;

  startLine = 0;
  endLine = -1;

  exposeCursor = false;
  updateState = 0;
  numLines = 0;
  lineRanges = 0L;
  newXPos = -1;
  newYPos = -1;

  drawBuffer = new QPixmap ();
  drawBuffer->setOptimization (QPixmap::BestOptim);

  bm.sXPos = 0;
  bm.eXPos = -1;

  setAcceptDrops(true);
  dragInfo.state = diNone;
}


KateViewInternal::~KateViewInternal()
{
  delete [] lineRanges;
  delete drawBuffer;
}


void KateViewInternal::slotPreHighlightUpdate(uint line)
{
  //kdDebug()<<QString("slotPreHighlightUpdate - Wait for: %1, line:  %2").arg(waitForPreHighlight).arg(line)<<endl;
  if (waitForPreHighlight !=0)
    {
       if (line>=waitForPreHighlight)
         {
           waitForPreHighlight=0;
           repaint();
         }
    }
}

void KateViewInternal::doCursorCommand(VConfig &c, int cmdNum) {

  switch (cmdNum) {
    case KateView::cmLeft:
      cursorLeft(c);
      break;
    case KateView::cmRight:
      cursorRight(c);
      break;
    case KateView::cmWordLeft:
      wordLeft(c);
      break;
    case KateView::cmWordRight:
      wordRight(c);
      break;
    case KateView::cmHome:
      home(c);
      break;
    case KateView::cmEnd:
      end(c);
      break;
    case KateView::cmUp:
      cursorUp(c);
      break;
    case KateView::cmDown:
      cursorDown(c);
      break;
    case KateView::cmScrollUp:
      scrollUp(c);
      break;
    case KateView::cmScrollDown:
      scrollDown(c);
      break;
    case KateView::cmTopOfView:
      topOfView(c);
      break;
    case KateView::cmBottomOfView:
      bottomOfView(c);
      break;
    case KateView::cmPageUp:
      pageUp(c);
      break;
    case KateView::cmPageDown:
      pageDown(c);
      break;
    case KateView::cmTop:
      top_home(c);
      break;
    case KateView::cmBottom:
      bottom_end(c);
      break;
  }
}

void KateViewInternal::doEditCommand(VConfig &c, int cmdNum)
{
  switch (cmdNum) {
    case KateView::cmCopy:
      myDoc->copy(c.flags);
      return;
  }

  if (!myView->doc()->isReadWrite()) return;

  switch (cmdNum) {
    case KateView::cmReturn:
      if (c.flags & KateDocument::cfDelOnInput) myDoc->removeSelectedText();
      myDoc->newLine(c);
      //emit returnPressed();
      //e->ignore();
      return;
    case KateView::cmDelete:
      if ((c.flags & KateDocument::cfDelOnInput) && myDoc->hasSelection())
        myDoc->removeSelectedText();
      else myDoc->del(c);
      return;
    case KateView::cmBackspace:
      if ((c.flags & KateDocument::cfDelOnInput) && myDoc->hasSelection())
        myDoc->removeSelectedText();
      else myDoc->backspace(c);
      return;
    case KateView::cmKillLine:
      myDoc->killLine(c);
      return;
    case KateView::cmCut:
      myDoc->cut(c);
      return;
    case KateView::cmPaste:
      if (c.flags & KateDocument::cfDelOnInput) myDoc->removeSelectedText();
      myDoc->paste(c);
      return;
    case KateView::cmIndent:
      myDoc->indent(c);
      return;
    case KateView::cmUnindent:
      myDoc->unIndent(c);
      return;
    case KateView::cmCleanIndent:
      myDoc->cleanIndent(c);
      return;
    case KateView::cmComment:
      myDoc->comment(c);
      return;
    case KateView::cmUncomment:
      myDoc->unComment(c);
      return;
  }
}

void KateViewInternal::cursorLeft(VConfig &c) {

  cursor.col--;
  if (c.flags & KateDocument::cfWrapCursor && cursor.col < 0 && cursor.line > 0) {
    cursor.line--;
    cursor.col = myDoc->textLength(cursor.line);
  }
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::cursorRight(VConfig &c) {

  if (c.flags & KateDocument::cfWrapCursor) {
    if (cursor.col >= myDoc->textLength(cursor.line)) {
      if (cursor.line == myDoc->lastLine()) return;
      cursor.line++;
      cursor.col = -1;
    }
  }
  cursor.col++;
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::wordLeft(VConfig &c) {
  Highlight *highlight;

  highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);

  if (cursor.col > 0) {
    do {
      cursor.col--;
    } while (cursor.col > 0 && !highlight->isInWord(textLine->getChar(cursor.col)));
    while (cursor.col > 0 && highlight->isInWord(textLine->getChar(cursor.col -1)))
      cursor.col--;
  } else {
    if (cursor.line > 0) {
      cursor.line--;
      textLine = myDoc->getTextLine(cursor.line);
      cursor.col = textLine->length();
    }
  }

  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::wordRight(VConfig &c) {
  Highlight *highlight;
  int len;

  highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);
  len = textLine->length();

  if (cursor.col < len) {
    do {
      cursor.col++;
    } while (cursor.col < len && highlight->isInWord(textLine->getChar(cursor.col)));
    while (cursor.col < len && !highlight->isInWord(textLine->getChar(cursor.col)))
      cursor.col++;
  } else {
    if (cursor.line < myDoc->lastLine()) {
      cursor.line++;
      textLine = myDoc->getTextLine(cursor.line);
      cursor.col = 0;
    }
  }

  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::home(VConfig &c) {
  int lc;

  lc = (c.flags & KateDocument::cfSmartHome) ? myDoc->getTextLine(cursor.line)->firstChar() : 0;
  if (lc <= 0 || cursor.col == lc) {
    cursor.col = 0;
    cOldXPos = cXPos = 0;
  } else {
    cursor.col = lc;
    cOldXPos = cXPos = myDoc->textWidth(cursor);
  }

  changeState(c);
}

void KateViewInternal::end(VConfig &c) {
  cursor.col = myDoc->textLength(cursor.line);
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}


void KateViewInternal::cursorUp(VConfig &c) {

  cursor.line--;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
}


void KateViewInternal::cursorDown(VConfig &c) {
  int x;

  if (cursor.line == myDoc->lastLine()) {
    x = myDoc->textLength(cursor.line);
    if (cursor.col >= x) return;
    cursor.col = x;
    cXPos = myDoc->textWidth(cursor);
  } else {
    cursor.line++;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, cOldXPos);
  }
  changeState(c);
}

void KateViewInternal::scrollUp(VConfig &c) {

  if (! yPos) return;

  newYPos = yPos - myDoc->fontHeight;
  if (cursor.line == (yPos + height())/myDoc->fontHeight -1) {
    cursor.line--;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);

    changeState(c);
  }
}

void KateViewInternal::scrollDown(VConfig &c) {

  if (endLine >= myDoc->lastLine()) return;

  newYPos = yPos + myDoc->fontHeight;
  if (cursor.line == (yPos + myDoc->fontHeight -1)/myDoc->fontHeight) {
    cursor.line++;
    cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
    changeState(c);
  }
}

void KateViewInternal::topOfView(VConfig &c) {

  cursor.line = (yPos + myDoc->fontHeight -1)/myDoc->fontHeight;
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::bottomOfView(VConfig &c) {

  cursor.line = (yPos + height())/myDoc->fontHeight -1;
  if (cursor.line < 0) cursor.line = 0;
  if (cursor.line > myDoc->lastLine()) cursor.line = myDoc->lastLine();
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::pageUp(VConfig &c) {
  int lines = (endLine - startLine - 1);

  if (lines <= 0) lines = 1;

  if (!(c.flags & KateDocument::cfPageUDMovesCursor) && yPos > 0) {
    newYPos = yPos - lines * myDoc->fontHeight;
    if (newYPos < 0) newYPos = 0;
  }
  cursor.line -= lines;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor, cOldXPos);
  changeState(c);
//  cursorPageUp(c);
}

void KateViewInternal::pageDown(VConfig &c) {

  int lines = (endLine - startLine - 1);

  if (!(c.flags & KateDocument::cfPageUDMovesCursor) && endLine < myDoc->lastLine()) {
    if (lines < myDoc->lastLine() - endLine)
      newYPos = yPos + lines * myDoc->fontHeight;
    else
      newYPos = yPos + (myDoc->lastLine() - endLine) * myDoc->fontHeight;
  }
  cursor.line += lines;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
//  cursorPageDown(c);
}

// go to the top, same X position
void KateViewInternal::top(VConfig &c) {

//  cursor.col = 0;
  cursor.line = 0;
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom, same X position
void KateViewInternal::bottom(VConfig &c) {

//  cursor.col = 0;
  cursor.line = myDoc->lastLine();
  cXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the top left corner
void KateViewInternal::top_home(VConfig &c)
{
  cursor.line = 0;
  cursor.col = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom right corner
void KateViewInternal::bottom_end(VConfig &c) {

  cursor.line = myDoc->lastLine();
  cursor.col = myDoc->textLength(cursor.line);
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}


void KateViewInternal::changeXPos(int p) {
  int dx;

  dx = xPos - p;
  xPos = p;
  if (QABS(dx) < width()) scroll(dx, 0); else update();
}

void KateViewInternal::changeYPos(int p) {
  int dy;

  dy = yPos - p;
  yPos = p;
  clearDirtyCache(height());

  if (QABS(dy) < height())
  {
    scroll(0, dy);
    leftBorder->scroll(0, dy);
  }
  else
    update();
}


void KateViewInternal::getVConfig(VConfig &c) {

  c.view = myView;
  c.cursor = cursor;
  c.cXPos = cXPos;
  c.flags = myView->myDoc->_configFlags;
}

void KateViewInternal::changeState(VConfig &c) {
  /*
   * we need to be sure to kill the selection on an attempted cursor
   * movement even if the cursor doesn't physically move,
   * but we need to be careful not to do some other things in this case,
   * like we don't want to expose the cursor
   */

//  if (cursor.col == c.cursor.col && cursor.line == c.cursor.line) return;
  bool nullMove = (cursor.col == c.cursor.col && cursor.line == c.cursor.line);

//  if (cursor.line != c.cursor.line || c.flags & KateDocument::cfMark) myDoc->recordReset();

  if (! nullMove) {

    exposeCursor = true;

    // mark old position of cursor as dirty
    if (cursorOn) {
      tagLines(c.cursor.line, c.cursor.line, c.cXPos, c.cXPos + myDoc->charWidth(c.cursor));
      cursorOn = false;
    }

    // mark old bracket mark position as dirty
    if (bm.sXPos < bm.eXPos) {
      tagLines(bm.cursor.line, bm.cursor.line, bm.sXPos, bm.eXPos);
    }
    // make new bracket mark
    myDoc->newBracketMark(cursor, bm);

    // remove trailing spaces when leaving a line
    if (c.flags & KateDocument::cfRemoveSpaces && cursor.line != c.cursor.line) {
      TextLine::Ptr textLine = myDoc->getTextLine(c.cursor.line);
      unsigned int newLen = textLine->lastChar();
      if (newLen != textLine->length()) {
        textLine->truncate(newLen);
        // if some spaces are removed, tag the line as dirty
        myDoc->tagLines(c.cursor.line, c.cursor.line);
      }
    }
  }

  if (c.flags & KateDocument::cfMark) {
    if (! nullMove)
      myDoc->selectTo(c, cursor, cXPos);
  } else {
    if (!(c.flags & KateDocument::cfPersistent))
      myDoc->clearSelection();
  }
}

void KateViewInternal::insLine(int line) {

  if (line <= cursor.line) {
    cursor.line++;
  }
  if (line < startLine) {
    startLine++;
    endLine++;
    yPos += myDoc->fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }
}

void KateViewInternal::delLine(int line) {

  if (line <= cursor.line && cursor.line > 0) {
    cursor.line--;
  }
  if (line < startLine) {
    startLine--;
    endLine--;
    yPos -= myDoc->fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }
}

void KateViewInternal::updateCursor()
{
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}


void KateViewInternal::updateCursor(KateViewCursor &newCursor)
{
  if (!(myDoc->_configFlags & KateDocument::cfPersistent)) myDoc->clearSelection();

  exposeCursor = true;
  if (cursorOn) {
    tagLines(cursor.line, cursor.line, cXPos, cXPos +myDoc->charWidth(cursor));
    cursorOn = false;
  }

  if (bm.sXPos < bm.eXPos) {
    tagLines(bm.cursor.line, bm.cursor.line, bm.sXPos, bm.eXPos);
  }
  myDoc->newBracketMark(newCursor, bm);

  cursor = newCursor;
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}

// init the line dirty cache
void KateViewInternal::clearDirtyCache(int height) {
  int lines, z;

  // calc start and end line of visible part
  startLine = yPos/myDoc->fontHeight;
  endLine = (yPos + height -1)/myDoc->fontHeight;

  updateState = 0;

  lines = endLine - startLine +1;
  if (lines > numLines) { // resize the dirty cache
    numLines = lines*2;
    delete [] lineRanges;
    lineRanges = new LineRange[numLines];
  }

  for (z = 0; z < lines; z++) { // clear all lines
    lineRanges[z].start = 0xffffff;
    lineRanges[z].end = -2;
  }
  newXPos = newYPos = -1;
}

void KateViewInternal::tagLines(int start, int end, int x1, int x2) {
  LineRange *r;
  int z;

  start -= startLine;
  if (start < 0) start = 0;
  end -= startLine;
  if (end > endLine - startLine) end = endLine - startLine;

  if (x1 <= 0) x1 = -2;
  if (x1 < xPos-2) x1 = xPos-2;
  if (x2 > width() + xPos-2) x2 = width() + xPos-2;
  if (x1 >= x2) return;

  r = &lineRanges[start];
  for (z = start; z <= end; z++) {
    if (x1 < r->start) r->start = x1;
    if (x2 > r->end) r->end = x2;
    r++;
    updateState |= 1;
  }
}

void KateViewInternal::tagAll() {
  updateState = 3;
}

void KateViewInternal::setPos(int x, int y) {
  newXPos = x;
  newYPos = y;
}

void KateViewInternal::center() {
  newXPos = 0;
  newYPos = cursor.line*myDoc->fontHeight - height()/2;
  if (newYPos < 0) newYPos = 0;
}

void KateViewInternal::updateView(int flags) {
  int fontHeight;
  int oldXPos, oldYPos;
  int w, h;
  int z;
  bool b;
  int xMax, yMax;
  int cYPos;
  int cXPosMin, cXPosMax, cYPosMin, cYPosMax;
  int dx, dy;
  int pageScroll;
  int scrollbarWidth = style().scrollBarExtent().width();

//debug("upView %d %d %d %d %d", exposeCursor, updateState, flags, newXPos, newYPos);
  if (exposeCursor || flags & KateView::ufDocGeometry) {
    emit myView->cursorPositionChanged();
  } else {
    if (updateState == 0 && newXPos < 0 && newYPos < 0) return;
  }

  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = startTimer(KApplication::cursorFlashTime() / 2);
    cursorOn = true;
  }

  oldXPos = xPos;
  oldYPos = yPos;
/*  if (flags & ufPos) {
    xPos = newXPos;
    yPos = newYPos;
    exposeCursor = true;
  }*/
  if (newXPos >= 0) xPos = newXPos;
  if (newYPos >= 0) yPos = newYPos;

  fontHeight = myDoc->fontHeight;
  cYPos = cursor.line*fontHeight;

  z = 0;
  do {
    w = myView->width() - 4;
    h = myView->height() - 4;

    xMax = myDoc->textWidth() - w;
    b = (xPos > 0 || xMax > 0);
    if (b) h -= scrollbarWidth;
    yMax = myDoc->textHeight() - h;
    if (yPos > 0 || yMax > 0) {
      w -= scrollbarWidth;
      xMax += scrollbarWidth;
      if (!b && xMax > 0) {
        h -= scrollbarWidth;
        yMax += scrollbarWidth;
      }
    }

    if (!exposeCursor) break;
//    if (flags & KateView::ufNoScroll) break;
/*
    if (flags & KateView::ufCenter) {
      cXPosMin = xPos + w/3;
      cXPosMax = xPos + (w*2)/3;
      cYPosMin = yPos + h/3;
      cYPosMax = yPos + ((h - fontHeight)*2)/3;
    } else {*/
      cXPosMin = xPos + 4;
      cXPosMax = xPos + w - 8;
      cYPosMin = yPos;
      cYPosMax = yPos + (h - fontHeight);
//    }

    if (cXPos < cXPosMin) {
      xPos -= cXPosMin - cXPos;
    }
    if (xPos < 0) xPos = 0;
    if (cXPos > cXPosMax) {
      xPos += cXPos - cXPosMax;
    }
    if (cYPos < cYPosMin) {
      yPos -= cYPosMin - cYPos;
    }
    if (yPos < 0) yPos = 0;
    if (cYPos > cYPosMax) {
      yPos += cYPos - cYPosMax;
    }

    z++;
  } while (z < 2);

  if (xMax < xPos) xMax = xPos;
  if (yMax < yPos) yMax = yPos;

  if (xMax > 0) {
    pageScroll = w - (w % fontHeight) - fontHeight;
    if (pageScroll <= 0)
      pageScroll = fontHeight;

    xScroll->blockSignals(true);
    xScroll->setGeometry(2,h + 2,w,scrollbarWidth);
    xScroll->setRange(0,xMax);
    xScroll->setValue(xPos);
    xScroll->setSteps(fontHeight,pageScroll);
    xScroll->blockSignals(false);
    xScroll->show();
  } else xScroll->hide();

  if (yMax > 0) {
    pageScroll = h - (h % fontHeight) - fontHeight;
    if (pageScroll <= 0)
      pageScroll = fontHeight;

    yScroll->blockSignals(true);
    yScroll->setGeometry(w + 2,2,scrollbarWidth,h);
    yScroll->setRange(0,yMax);
    yScroll->setValue(yPos);
    yScroll->setSteps(fontHeight,pageScroll);
    yScroll->blockSignals(false);
    yScroll->show();
  } else yScroll->hide();

  if (w != width() || h != height()) {
    clearDirtyCache(h);
    resize(w,h);
  } else {
    dx = oldXPos - xPos;
    dy = oldYPos - yPos;

    b = updateState == 3;
    if (flags & KateView::ufUpdateOnScroll) {
      b |= dx || dy;
    } else {
      b |= QABS(dx)*3 > w*2 || QABS(dy)*3 > h*2;
    }

    if (b) {
      clearDirtyCache(h);
      update();
    } else {
      if (dy)
        leftBorder->scroll(0, dy);
      if (updateState > 0) paintTextLines(oldXPos, oldYPos);
      clearDirtyCache(h);

      if (dx || dy) {
        scroll(dx,dy);
//        kapp->syncX();
//        scroll2(dx - dx/2,dy - dy/2);
//      } else {
      }
      if (cursorOn) paintCursor();
      if (bm.eXPos > bm.sXPos) paintBracketMark();
    }
  }
  exposeCursor = false;
//  updateState = 0;
}


void KateViewInternal::paintTextLines(int xPos, int yPos) {
//  int xStart, xEnd;
  int line;//, z;
  int h;
  LineRange *r;

  if (!drawBuffer) return;
  if (drawBuffer->isNull()) return;

  QPainter paint;
  paint.begin(drawBuffer);

  h = myDoc->fontHeight;
  r = lineRanges;
  for (line = startLine; line <= endLine; line++) {
    if (r->start < r->end) {
//debug("painttextline %d %d %d", line, r->start, r->end);
      myDoc->paintTextLine(paint, line, r->start, r->end, myView->myDoc->_configFlags & KateDocument::cfShowTabs);
      bitBlt(this, r->start - (xPos-2), line*h - yPos, drawBuffer, 0, 0,
        r->end - r->start, h);
        leftBorder->paintLine(line);
    }
    r++;
  }

  paint.end();
}

void KateViewInternal::paintCursor() {
  int h, w,w2,y, x;
  static int cx = 0, cy = 0, ch = 0;

  h = myDoc->fontHeight;
  y = h*cursor.line - yPos;
  x = cXPos - (xPos-2);

  if(myDoc->myFont != font()) setFont(myDoc->myFont);
  if(cx != x || cy != y || ch != h){
    cx = x;
    cy = y;
    ch = h;
    setMicroFocusHint(cx, cy, 0, ch - 2);
  }

  w2 = myDoc->charWidth(cursor);
  w = myView->isOverwriteMode() ? w2 : 2;

  xCoord = x;
  yCoord = y+h;

  QPainter paint;
  if (cursorOn) {
    QColor &fg = myDoc->cursorCol(cursor.col,cursor.line);
    QColor &bg = myDoc->backCol(cursor.col, cursor.line);
    QColor xor_fg (qRgb(fg.red()^bg.red(), fg.green()^bg.green(), fg.blue()^bg.blue()),
                   fg.pixel()^bg.pixel());
 
    paint.begin(this);
    paint.setClipping(false);
    paint.setPen(myDoc->cursorCol(cursor.col,cursor.line));
    paint.setRasterOp(XorROP);

    //h += y - 1;
    paint.fillRect(x, y, w, h, xor_fg);
    paint.end();
   } else {
    if (drawBuffer && !drawBuffer->isNull()) {
      paint.begin(drawBuffer);
      myDoc->paintTextLine(paint, cursor.line, cXPos, cXPos + w2,myView->myDoc->_configFlags & KateDocument::cfShowTabs);
      bitBlt(this,x,y, drawBuffer,0,0, w2, h);
      paint.end();
    }
  }

}

void KateViewInternal::paintBracketMark() {
  int y;

  y = myDoc->fontHeight*(bm.cursor.line +1) - yPos -1;

  QPainter paint;
  paint.begin(this);
  paint.setPen(myDoc->cursorCol(bm.cursor.col, bm.cursor.line));

  paint.drawLine(bm.sXPos - (xPos-2), y, bm.eXPos - (xPos-2) -1, y);
  paint.end();
}

void KateViewInternal::placeCursor(int x, int y, int flags) {
  VConfig c;

  getVConfig(c);
  c.flags |= flags;
  cursor.line = (yPos + y)/myDoc->fontHeight;
  cXPos = cOldXPos = myDoc->textWidth(c.flags & KateDocument::cfWrapCursor, cursor,xPos-2 + x);
  changeState(c);
}

// given physical coordinates, report whether the text there is selected
bool KateViewInternal::isTargetSelected(int x, int y) {

  y = (yPos + y) / myDoc->fontHeight;

  TextLine::Ptr line = myDoc->getTextLine(y);
  if (!line)
    return false;

  x = myDoc->textPos(line, x);

  return myDoc->lineColSelected(y, x);
}

void KateViewInternal::focusInEvent(QFocusEvent *) {
//  debug("got focus %d",cursorTimer);

  if (!cursorTimer) {
    cursorTimer = startTimer(KApplication::cursorFlashTime() / 2);
    cursorOn = true;
    paintCursor();
  }
}

void KateViewInternal::focusOutEvent(QFocusEvent *) {
//  debug("lost focus %d", cursorTimer);

  if (cursorTimer) {
    killTimer(cursorTimer);
    cursorTimer = 0;
  }

  if (cursorOn) {
    cursorOn = false;
    paintCursor();
  }
}

void KateViewInternal::keyPressEvent(QKeyEvent *e) {
  VConfig c;
  getVConfig(c);

  if (myView->doc()->isReadWrite()) {
    if (c.flags & KateDocument::cfTabIndents && myDoc->hasSelection()) {
      if (e->key() == Qt::Key_Tab) {
        myDoc->indent(c);
        myDoc->updateViews();
        return;
      }
      if (e->key() == Qt::Key_Backtab) {
        myDoc->unIndent(c);
        myDoc->updateViews();
        return;
      }
    }
    if ( !(e->state() & ControlButton ) && myDoc->insertChars (c.cursor.line, c.cursor.col, e->text(), this->myView) )
    {
      myDoc->updateViews();
      e->accept();
      return;
    }
  }
  e->ignore();
}

void KateViewInternal::mousePressEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {

    if (isTargetSelected(e->x(), e->y())) {
      // we have a mousedown on selected text
      // we initialize the drag info thingy as pending from this position

      dragInfo.state = diPending;
      dragInfo.start.col = e->x();
      dragInfo.start.line = e->y();
    } else {
      // we have no reason to ever start a drag from here
      dragInfo.state = diNone;

      int flags;

      flags = 0;
      if (e->state() & ShiftButton) {
        flags |= KateDocument::cfMark;
        if (e->state() & ControlButton) flags |= KateDocument::cfMark | KateDocument::cfKeepSelection;
      }
      placeCursor(e->x(), e->y(), flags);
      scrollX = 0;
      scrollY = 0;
      if (!scrollTimer) scrollTimer = startTimer(50);
      myDoc->updateViews();
    }
  }
  if (e->button() == MidButton) {
    placeCursor(e->x(), e->y());
    if (myView->doc()->isReadWrite())
      myView->paste();
  }
  if (myView->rmbMenu && e->button() == RightButton) {
    myView->rmbMenu->popup(mapToGlobal(e->pos()));
  }
  myView->mousePressEvent(e); // this doesn't do anything, does it?
  // it does :-), we need this for KDevelop, so please don't uncomment it again -Sandy
}

void KateViewInternal::mouseDoubleClickEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    VConfig c;
    getVConfig(c);
    myDoc->selectWord(c.cursor, c.flags);
    myDoc->updateViews();
  }
}

void KateViewInternal::mouseReleaseEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor(e->x(), e->y(), 0);
      myDoc->updateViews();
    } else if (dragInfo.state == diNone) {
      if (myView->doc()->configFlags() & KateDocument::cfMouseAutoCopy) {
        QApplication::clipboard()->setSelectionMode( true );
        myView->copy();
        QApplication::clipboard()->setSelectionMode( false );
      }
      killTimer(scrollTimer);
      scrollTimer = 0;
    }
    dragInfo.state = diNone;
  }
}

void KateViewInternal::mouseMoveEvent(QMouseEvent *e) {

  if (e->state() & LeftButton) {
    int flags;
    int d;
    int x = e->x(),
        y = e->y();

    if (dragInfo.state == diPending) {
      // we had a mouse down, but haven't confirmed a drag yet
      // if the mouse has moved sufficiently, we will confirm

      if (x > dragInfo.start.col + 4 || x < dragInfo.start.col - 4 ||
          y > dragInfo.start.line + 4 || y < dragInfo.start.line - 4) {
        // we've left the drag square, we can start a real drag operation now
        doDrag();
      }
      return;
    } else if (dragInfo.state == diDragging) {
      // this isn't technically needed because mouseMoveEvent is suppressed during
      // Qt drag operations, replaced by dragMoveEvent
      return;
    }

    mouseX = e->x();
    mouseY = e->y();
    scrollX = 0;
    scrollY = 0;
    d = myDoc->fontHeight;
    if (mouseX < 0) {
      mouseX = 0;
      scrollX = -d;
    }
    if (mouseX > width()) {
      mouseX = width();
      scrollX = d;
    }
    if (mouseY < 0) {
      mouseY = 0;
      scrollY = -d;
    }
    if (mouseY > height()) {
      mouseY = height();
      scrollY = d;
    }
//debug("modifiers %d", ((KGuiCmdApp *) kapp)->getModifiers());
    flags = KateDocument::cfMark;
    if (e->state() & ControlButton) flags |= KateDocument::cfKeepSelection;
    placeCursor(mouseX, mouseY, flags);
    myDoc->updateViews(/*ufNoScroll*/);
  }
}



void KateViewInternal::wheelEvent( QWheelEvent *e )
{
  if( yScroll->isVisible() == true )
  {
    QApplication::sendEvent( yScroll, e );
  }
}



void KateViewInternal::paintEvent(QPaintEvent *e) {
  int xStart, xEnd;
  int h;
  int line, y, yEnd;

  QRect updateR = e->rect();

  if (!drawBuffer) return;
  if (drawBuffer->isNull()) return;

  QPainter paint;
  paint.begin(drawBuffer);

  xStart = xPos-2 + updateR.x();
  xEnd = xStart + updateR.width();

  h = myDoc->fontHeight;
  line = (yPos + updateR.y()) / h;
  y = line*h - yPos;
  yEnd = updateR.y() + updateR.height();
  waitForPreHighlight=myDoc->needPreHighlight(waitForPreHighlight=line+((uint)(yEnd-y)/h)+5);

  while (y < yEnd)
  {
    //TextLine *textLine;
    //int ctxNum = 0;
    myDoc->paintTextLine(paint, line, xStart, xEnd, myView->myDoc->_configFlags & KateDocument::cfShowTabs);
    bitBlt(this, updateR.x(), y, drawBuffer, 0, 0, updateR.width(), h);
    leftBorder->paintLine(line);
    line++;
    y += h;
  }
  paint.end();

  if (cursorOn) paintCursor();
  if (bm.eXPos > bm.sXPos) paintBracketMark();
}

void KateViewInternal::resizeEvent(QResizeEvent *)
{
  drawBuffer->resize (width(), myDoc->fontHeight);
  leftBorder->resize(iconBorderWidth, height());
}

void KateViewInternal::timerEvent(QTimerEvent *e) {
  if (e->timerId() == cursorTimer) {
    cursorOn = !cursorOn;
    paintCursor();
  }
  if (e->timerId() == scrollTimer && (scrollX | scrollY)) {
    xScroll->setValue(xPos + scrollX);
    yScroll->setValue(yPos + scrollY);

    placeCursor(mouseX, mouseY, KateDocument::cfMark);
    myDoc->updateViews(/*ufNoScroll*/);
  }
}

/////////////////////////////////////
// Drag and drop handlers
//

// call this to start a drag from this view
void KateViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(myDoc->selection(), this);
  dragInfo.dragObject->dragCopy();
}

void KateViewInternal::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept( (QTextDrag::canDecode(event) && myView->doc()->isReadWrite()) || QUriDrag::canDecode(event) );
}

void KateViewInternal::dropEvent( QDropEvent *event )
{
  if ( QUriDrag::canDecode(event) ) {

      emit dropEventPass(event);

  } else if ( QTextDrag::canDecode(event) && myView->doc()->isReadWrite() ) {

    QString   text;

    if (QTextDrag::decode(event, text)) {
      bool      priv, selected;

      // is the source our own document?
      priv = myDoc->ownedView((KateView*)(event->source()));
      // dropped on a text selection area?
      selected = isTargetSelected(event->pos().x(), event->pos().y());

      if (priv && selected) {
        // this is a drag that we started and dropped on our selection
        // ignore this case
        return;
      }

      VConfig c;
      KateViewCursor cursor;

      getVConfig(c);
      cursor = c.cursor;

      if (priv) {
        // this is one of mine (this document), not dropped on the selection
        if (event->action() == QDropEvent::Move) {
          myDoc->removeSelectedText();
          getVConfig(c);
          cursor = c.cursor;
        } else {
        }
        placeCursor(event->pos().x(), event->pos().y());
        getVConfig(c);
        cursor = c.cursor;
      } else {
        // this did not come from this document
        if (! selected) {
          placeCursor(event->pos().x(), event->pos().y());
          getVConfig(c);
          cursor = c.cursor;
        }
      }
      myDoc->insertText(c.cursor.line, c.cursor.col, text);
      cursor = c.cursor;

      updateCursor(cursor);
      myDoc->updateViews();
    }
  }
}

uint KateView::uniqueID = 0;

KateView::KateView(KateDocument *doc, QWidget *parent, const char * name) : Kate::View (doc, parent, name)
{
  setInstance( KateFactory::instance() );
  
  initCodeCompletionImplementation();

  myViewID = uniqueID;
  uniqueID++;

  active = false;
  myIconBorder = false;

  myDoc = doc;
  myViewInternal = new KateViewInternal (this,doc);
  myViewInternal->move(2, 2);
  myViewInternal->leftBorder = new KateIconBorder(this, myViewInternal);
  myViewInternal->leftBorder->setGeometry(2, 2, myViewInternal->iconBorderWidth, myViewInternal->iconBorderHeight);
  myViewInternal->leftBorder->hide();

  doc->addView( this );

  connect(myViewInternal,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(dropEventPassEmited(QDropEvent *)));

  replacePrompt = 0L;
  rmbMenu = 0L;

  setFocusProxy( myViewInternal );
  myViewInternal->setFocus();
  resize(parent->width() -4, parent->height() -4);

  myViewInternal->installEventFilter( this );

  if (!doc->m_bBrowserView)
  {
    setXMLFile( "katepartui.rc" );
  }
  else
  {
    (void)new KateBrowserExtension( myDoc, this );
    myDoc->setXMLFile( "katepartbrowserui.rc" );
  }

  setupActions();

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( doc, SIGNAL( undoChanged() ), this, SLOT( slotNewUndo() ) );
  connect( doc, SIGNAL( fileNameChanged() ), this, SLOT( slotFileStatusChanged() ) );
  connect( doc, SIGNAL( hlChanged() ), this, SLOT( slotHighlightChanged() ) );

  if ( doc->m_bBrowserView )
  {
    connect( this, SIGNAL( dropEventPass(QDropEvent*) ), this, SLOT( slotDropEventPass(QDropEvent*) ) );
  }

  slotUpdate();
}

KateView::~KateView()
{
  if (myDoc && !myDoc->m_bSingleViewMode)
    myDoc->removeView( this );

  delete myViewInternal;
}

void KateView::initCodeCompletionImplementation()
{
  myCC_impl=new CodeCompletion_Impl(this);
  connect(myCC_impl,SIGNAL(completionAborted()),this,SIGNAL(completionAborted()));
  connect(myCC_impl,SIGNAL(completionDone()),this,SIGNAL(completionDone()));
  connect(myCC_impl,SIGNAL(completionHided()),this,SIGNAL(completionHided()));
}

QPoint KateView::cursorCoordinates()
{
  return QPoint(myViewInternal->xCoord, myViewInternal->yCoord);
}

void KateView::copy () const
{
  myDoc->copy(myDoc->_configFlags);
}

void KateView::setupActions()
{
    kdDebug() << "KateView::setupActions()" << endl; // delete me -- ellis
    KStdAction::close( this, SLOT(flush()), actionCollection(), "file_close" );

    KStdAction::save(this, SLOT(save()), actionCollection());

    // setup edit menu
    editUndo = KStdAction::undo(myDoc, SLOT(undo()), actionCollection());
    editRedo = KStdAction::redo(myDoc, SLOT(redo()), actionCollection());

    KStdAction::cut(this, SLOT(cut()), actionCollection());
    KStdAction::copy(this, SLOT(copy()), actionCollection());
    KStdAction::paste(this, SLOT(paste()), actionCollection());

    if ( myDoc->m_bBrowserView )
    {
      KStdAction::saveAs(this, SLOT(saveAs()), myDoc->actionCollection());
      KStdAction::find(this, SLOT(find()), myDoc->actionCollection(), "find");
      KStdAction::findNext(this, SLOT(findAgain()), myDoc->actionCollection(), "find_again");
      KStdAction::findPrev(this, SLOT(findPrev()), myDoc->actionCollection(), "find_prev");
      KStdAction::gotoLine(this, SLOT(gotoLine()), myDoc->actionCollection(), "goto_line" );
      new KAction(i18n("&Configure Editor..."), 0, myDoc, SLOT(configDialog()),myDoc->actionCollection(), "set_confdlg");
			setHighlight = new KateViewHighlightAction(this,i18n("&Highlight Mode"),myDoc->actionCollection(),"set_highlight");
      KStdAction::selectAll(myDoc, SLOT(selectAll()), myDoc->actionCollection(), "select_all");
      new KAction(i18n("&Deselect All"), 0, myDoc, SLOT(clearSelection ()),
                myDoc->actionCollection(), "unselect_all");
      new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, this, SLOT(slotIncFontSizes()),
                myDoc->actionCollection(), "incFontSizes");
      new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, this, SLOT(slotDecFontSizes()),
                myDoc->actionCollection(), "decFontSizes");
      new KAction(i18n("&Toggle Block Selection"), Key_F4, myDoc, SLOT(toggleBlockSelectionMode()),
                                             myDoc->actionCollection(), "set_verticalSelect");
    }
    else
    {
      KStdAction::saveAs(this, SLOT(saveAs()), actionCollection());
      KStdAction::find(this, SLOT(find()), actionCollection());
      KStdAction::findNext(this, SLOT(findAgain()), actionCollection());
      KStdAction::findPrev(this, SLOT(findPrev()), actionCollection(), "edit_find_prev");
      KStdAction::gotoLine(this, SLOT(gotoLine()), actionCollection());
      new KAction(i18n("&Configure Editor..."), 0, myDoc, SLOT(configDialog()),actionCollection(), "set_confdlg");
      setHighlight = new KateViewHighlightAction(this,i18n("&Highlight Mode"), actionCollection(), "set_highlight");
      KStdAction::selectAll(myDoc, SLOT(selectAll()), actionCollection());
      new KAction(i18n("&Deselect All"), 0, myDoc, SLOT(clearSelection()),
                actionCollection(), "edit_deselectAll");
      new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, this, SLOT(slotIncFontSizes()),
                actionCollection(), "incFontSizes");
      new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, this, SLOT(slotDecFontSizes()),
                actionCollection(), "decFontSizes");
      new KAction(i18n("&Toggle Block Selection"), Key_F4, myDoc, SLOT(toggleBlockSelectionMode()),
                                             actionCollection(), "set_verticalSelect");
    }

  new KAction(i18n("Apply Word Wrap"), "", 0, myDoc, SLOT(applyWordWrap()), actionCollection(), "edit_apply_wordwrap");

  KStdAction::replace(this, SLOT(replace()), actionCollection());

  new KAction(i18n("Editing Co&mmand"), Qt::CTRL+Qt::Key_M, this, SLOT(slotEditCommand()),
                                  actionCollection(), "edit_cmd");

    // setup bookmark menu
    bookmarkToggle = new KAction(i18n("Toggle &Bookmark"), Qt::CTRL+Qt::Key_B, this, SLOT(toggleBookmark()), actionCollection(), "edit_bookmarkToggle");
    bookmarkClear = new KAction(i18n("Clear Bookmarks"), 0, this, SLOT(clearBookmarks()), actionCollection(), "edit_bookmarksClear");

    // connect settings menu aboutToshow
    bookmarkMenu = new KActionMenu(i18n("&Bookmarks"), actionCollection(), "bookmarks");
    connect(bookmarkMenu->popupMenu(), SIGNAL(aboutToShow()), this, SLOT(bookmarkMenuAboutToShow()));

    new KToggleAction(i18n("Show &Icon Border"), Key_F6, this, SLOT(toggleIconBorder()), actionCollection(), "view_border");

    // setup Tools menu
    KStdAction::spelling(myDoc, SLOT(spellcheck()), actionCollection());
    new KAction(i18n("&Indent"), "indent", Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              actionCollection(), "tools_indent");
    new KAction(i18n("&Unindent"), "unindent", Qt::CTRL+Qt::Key_U, this, SLOT(unIndent()),
                                actionCollection(), "tools_unindent");
    new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   actionCollection(), "tools_cleanIndent");
    new KAction(i18n("C&omment"), CTRL+Qt::Key_NumberSign, this, SLOT(comment()),
                               actionCollection(), "tools_comment");
    new KAction(i18n("Unco&mment"), CTRL+SHIFT+Qt::Key_NumberSign, this, SLOT(uncomment()),
                                 actionCollection(), "tools_uncomment");

    QStringList list;
    setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, actionCollection(), "set_eol");
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

void KateView::slotHighlightChanged()
{
//	setHighlight->slotAboutToShow();
//    setHighlight->setCurrentItem(getHl());
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
  switch ( ev->key() )
    {
        case Key_Left:
            if ( ev->state() & ShiftButton )
            {
                if ( ev->state() & ControlButton )
                    shiftWordLeft();
                else
                    shiftCursorLeft();
            }
            else if ( ev->state() & ControlButton )
                wordLeft();
            else
                cursorLeft();
            break;
        case Key_Right:
            if ( ev->state() & ShiftButton )
            {
                if ( ev->state() & ControlButton )
                    shiftWordRight();
                else
                    shiftCursorRight();
            }
            else if ( ev->state() & ControlButton )
                wordRight();
            else
                cursorRight();
            break;
        case Key_Home:
            if ( ev->state() & ShiftButton )
            {
                if ( ev->state() & ControlButton )
                    shiftTop();
                else
                    shiftHome();
            }
            else if ( ev->state() & ControlButton )
                top();
            else
                home();
            break;
        case Key_End:
            if ( ev->state() & ShiftButton )
            {
                if ( ev->state() & ControlButton )
                    shiftBottom();
                else
                    shiftEnd();
            }
            else if ( ev->state() & ControlButton )
                bottom();
            else
                end();
            break;
        case Key_Up:
            if ( ev->state() & ShiftButton )
                shiftUp();
            else if ( ev->state() & ControlButton )
                scrollUp();
            else
                up();
            break;
        case Key_Down:
            if ( ev->state() & ShiftButton )
                shiftDown();
            else if ( ev->state() & ControlButton )
                scrollDown();
            else
                down();
            break;
        case Key_PageUp:
            if ( ev->state() & ShiftButton )
                shiftPageUp();
            else if ( ev->state() & ControlButton )
                topOfView();
            else
                pageUp();
            break;
        case Key_PageDown:
            if ( ev->state() & ShiftButton )
                shiftPageDown();
            else if ( ev->state() & ControlButton )
                bottomOfView();
            else
                pageDown();
            break;
        case Key_Return:
        case Key_Enter:
            doEditCommand(KateView::cmReturn);
            break;
        case Key_Delete:
            if ( ev->state() & ControlButton )
            {
               VConfig c;
               shiftWordRight();
               myViewInternal->getVConfig(c);
               myDoc->removeSelectedText();
               myViewInternal->update();
            }
            else keyDelete();
            break;
        case Key_Backspace:
            if ( ev->state() & ControlButton )
            {
               VConfig c;
               shiftWordLeft();
               myViewInternal->getVConfig(c);
               myDoc->removeSelectedText();
               myViewInternal->update();
            }
            else backspace();
            break;
        case Key_Insert:
            toggleInsert();
            break;
        case Key_K:
            if ( ev->state() & ControlButton )
            {
                killLine();
                break;
            }
        default:
            KTextEditor::View::keyPressEvent( ev );
            return;
            break;
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

void KateView::setCursorPosition( int line, int col, bool /*mark*/ )
{
  setCursorPositionInternal( line, col, tabWidth() );
}

void KateView::setCursorPositionReal( int line, int col, bool /*mark*/ )
{
  setCursorPositionInternal( line, col, 1 );
}

void KateView::getCursorPosition( int *line, int *col )
{
  if ( line )
    *line = currentLine();

  if ( col )
    *col = currentColumn();
}

void KateView::getCursorPositionReal( int *line, int *col )
{
  if ( line )
    *line = currentLine();

  if ( col )
    *col = currentCharNum();
}


int KateView::currentLine() {
  return myViewInternal->cursor.line;
}

int KateView::currentColumn() {
  return myDoc->currentColumn(myViewInternal->cursor);
}

int KateView::currentCharNum() {
  return myViewInternal->cursor.col;
}

void KateView::setCursorPositionInternal(int line, int col, int tabwidth) {
  KateViewCursor cursor;

  TextLine::Ptr textLine = myDoc->getTextLine(line);
  QString line_str = QString(textLine->getText(), textLine->length());

  int z;
  int x = 0;
  for (z = 0; z < (int)line_str.length() && z <= col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  cursor.col = x;
  cursor.line = line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
//  myViewInternal->updateView(ufPos, 0, line*myDoc->fontHeight - height()/2);
//  myDoc->updateViews(myViewInternal); //uptade all other views except this one
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

void KateView::toggleInsert() {
  myDoc->setConfigFlags(myDoc->_configFlags ^ KateDocument::cfOvr);
}

QString KateView::currentTextLine() {
  TextLine::Ptr textLine = myDoc->getTextLine(myViewInternal->cursor.line);
  return QString(textLine->getText(), textLine->length());
}

QString KateView::currentWord() {
  return myDoc->getWord(myViewInternal->cursor);
}

QString KateView::word(int x, int y) {
  KateViewCursor cursor;
  cursor.line = (myViewInternal->yPos + y)/myDoc->fontHeight;
  if (cursor.line < 0 || cursor.line > myDoc->lastLine()) return QString();
  cursor.col = myDoc->textPos(myDoc->getTextLine(cursor.line), myViewInternal->xPos-2 + x);
  return myDoc->getWord(cursor);
}

void KateView::insertText(const QString &s)
{
  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->insertText(c.cursor.line, c.cursor.col, s);
  myDoc->updateViews();
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
      myDoc->saveAs(myDoc->url());
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

	  if (data.url.isEmpty())
	    return SAVE_CANCEL;

    query = checkOverwrite( data.url );
  }
  while (query != KMessageBox::Yes);

  if( query == KMessageBox::Cancel )
    return SAVE_CANCEL;

	myDoc->setEncoding (data.encoding);
  myDoc->saveAs(data.url);
  return SAVE_OK;
}

void KateView::doCursorCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  if (cmdNum & selectFlag) c.flags |= KateDocument::cfMark;
  if (cmdNum & multiSelectFlag) c.flags |= KateDocument::cfMark | KateDocument::cfKeepSelection;
  cmdNum &= ~(selectFlag | multiSelectFlag);
  myViewInternal->doCursorCommand(c, cmdNum);
  myDoc->updateViews();
}

void KateView::doEditCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  myViewInternal->doEditCommand(c, cmdNum);
  myDoc->updateViews();
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

  dlg = new GotoLineDialog(this, myViewInternal->cursor.line + 1, myDoc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int linenumber ) 
{
  KateViewCursor cursor;

  cursor.col = 0;
  cursor.line = linenumber;
  myDoc->needPreHighlight(cursor.line);
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView(KateView::ufUpdateOnScroll);
  myDoc->updateViews(this); //uptade all other views except this one
 }

void KateView::initSearch(SConfig &s, int flags) {

  myDoc->s.flags = flags;
  myDoc->s.setPattern(myDoc->searchForList.first());

  if (!(myDoc->s.flags & KateDocument::sfFromBeginning)) {
    // If we are continuing a backward search, make sure we do not get stuck
    // at an existing match.
    myDoc->s.cursor = myViewInternal->cursor;
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
  myDoc->s.startCursor = myDoc->s.cursor;
}

void KateView::continueSearch(SConfig &s) {

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
  KateViewCursor cursor;
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
  KateViewCursor cursor;
  bool started;

  QString searchFor = myDoc->searchForList.first();
  QString replaceWith = myDoc->replaceWithList.first();
  rlen = replaceWith.length();

  switch (result) {
    case KateView::srYes: //yes
      myDoc->removeText (myDoc->s.cursor.line, myDoc->s.cursor.col, myDoc->s.cursor.line, myDoc->s.cursor.col + myDoc->s.matchedLength);
      myDoc->insertText (myDoc->s.cursor.line, myDoc->s.cursor.col, replaceWith);
      replaces++;
      if (myDoc->s.cursor.line == myDoc->s.startCursor.line && myDoc->s.cursor.col < myDoc->s.startCursor.col)
        myDoc->s.startCursor.col += rlen - myDoc->s.matchedLength;
      if (!(myDoc->s.flags & KateDocument::sfBackward)) myDoc->s.cursor.col += rlen;
      break;
    case KateView::srNo: //no
      if (!(myDoc->s.flags & KateDocument::sfBackward)) myDoc->s.cursor.col += myDoc->s.matchedLength;
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
          if (myDoc->s.cursor.line == myDoc->s.startCursor.line && myDoc->s.cursor.col < myDoc->s.startCursor.col)
            myDoc->s.startCursor.col += rlen - myDoc->s.matchedLength;
          if (!(myDoc->s.flags & KateDocument::sfBackward)) myDoc->s.cursor.col += rlen;
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

void KateView::exposeFound(KateViewCursor &cursor, int slen, int flags, bool replace) {
  int x1, x2, y1, y2, xPos, yPos;

  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->selectLength(cursor,slen,c.flags);

  TextLine::Ptr textLine = myDoc->getTextLine(cursor.line);
  x1 = myDoc->textWidth(textLine,cursor.col)        -10;
  x2 = myDoc->textWidth(textLine,cursor.col + slen) +20;
  y1 = myDoc->fontHeight*cursor.line                 -10;
  y2 = y1 + myDoc->fontHeight                     +30;

  xPos = myViewInternal->xPos;
  yPos = myViewInternal->yPos;

  if (x1 < 0) x1 = 0;
  if (replace) y2 += 90;

  if (x1 < xPos || x2 > xPos + myViewInternal->width()) {
    xPos = x2 - myViewInternal->width();
  }
  if (y1 < yPos || y2 > yPos + myViewInternal->height()) {
    xPos = x2 - myViewInternal->width();
    yPos = myDoc->fontHeight*cursor.line - height()/3;
  }
  myViewInternal->setPos(xPos, yPos);
  myViewInternal->updateView(flags);// | ufPos,xPos,yPos);
  myDoc->updateViews(this);
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
  KateViewCursor cursor;

  myViewInternal->xPos = config->readNumEntry("XPos");
  myViewInternal->yPos = config->readNumEntry("YPos");
  cursor.col = config->readNumEntry("CursorX");
  cursor.line = config->readNumEntry("CursorY");
  myViewInternal->updateCursor(cursor);
  myIconBorder = config->readBoolEntry("IconBorder on");
  setIconBorder(myIconBorder);
}

void KateView::writeSessionConfig(KConfig *config)
{
  config->writeEntry("XPos",myViewInternal->xPos);
  config->writeEntry("YPos",myViewInternal->yPos);
  config->writeEntry("CursorX",myViewInternal->cursor.col);
  config->writeEntry("CursorY",myViewInternal->cursor.line);
  config->writeEntry("IconBorder on", myIconBorder);
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

void KateView::paintEvent(QPaintEvent *e)
{
  int x, y;

  QRect updateR = e->rect();                    // update rectangle
//  debug("Update rect = ( %i, %i, %i, %i )",
//    updateR.col(),updateR.line(), updateR.width(), updateR.height() );

  int ux1 = updateR.x();
  int uy1 = updateR.y();
  int ux2 = ux1 + updateR.width();
  int uy2 = uy1 + updateR.height();

  QPainter paint;
  paint.begin(this);

  QColorGroup g = colorGroup();
  x = width();
  y = height();

  paint.setPen(g.dark());
  if (uy1 <= 0) paint.drawLine(0,0,x-2,0);
  if (ux1 <= 0) paint.drawLine(0,1,0,y-2);

  paint.setPen(black);
  if (uy1 <= 1) paint.drawLine(1,1,x-3,1);
  if (ux1 <= 1) paint.drawLine(1,2,1,y-3);

  paint.setPen(g.midlight());
  if (uy2 >= y-1) paint.drawLine(1,y-2,x-3,y-2);
  if (ux2 >= x-1) paint.drawLine(x-2,1,x-2,y-2);

  paint.setPen(g.light());
  if (uy2 >= y) paint.drawLine(0,y-1,x-2,y-1);
  if (ux2 >= x) paint.drawLine(x-1,0,x-1,y-1);

  x -= 2 + 16;
  y -= 2 + 16;
  if (ux2 > x && uy2 > y) {
    paint.fillRect(x,y,16,16,g.background());
  }
  paint.end();
}

void KateView::resizeEvent(QResizeEvent *) {

//  debug("Resize %d, %d",e->size().width(),e->size().height());

//myViewInternal->resize(width() -20, height() -20);
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

// Applies the search context to the given string, and returns whether a match was found. If one is,
// the length of the string matched is also returned.
int SConfig::search(QString &text, int index) {
  bool regExp = (flags & KateDocument::sfRegularExpression);
  bool caseSensitive = (flags & KateDocument::sfCaseSensitive);

  if (flags & KateDocument::sfBackward) {
    if (regExp) {
      index = text.findRev(m_regExp, index);
    }
    else {
      index = text.findRev(m_pattern, index, caseSensitive);
    }
  }
  else {
    if (regExp) {
      index = text.find(m_regExp, index);
    }
    else {
      index = text.find(m_pattern, index, caseSensitive);
    }
  }

  // Work out the matched length.
  if (index != -1)
  {
    if (regExp) {

//			  from what I can we don't care about the position just
//			  whether it matched
					int pos = m_regExp.search(text, index); // &matchedLength, false);
					if(pos >-1)	matchedLength=m_regExp.matchedLength();
		}
    else {
      matchedLength = m_pattern.length();
    }
  }
  return index;
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

  emit gotFocus (this);
}

bool KateView::eventFilter (QObject *object, QEvent *event)
{
  if ( object == myViewInternal )
    KCursor::autoHideEventFilter( object, event );

  if ( (event->type() == QEvent::FocusIn) )
    emit gotFocus (this);

  if ( (event->type() == QEvent::KeyPress) )
    {
        QKeyEvent * ke=(QKeyEvent *)event;

        if ((ke->key()==Qt::Key_Tab) || (ke->key()==Qt::Key_BackTab))
          {
            myViewInternal->keyPressEvent(ke);
            return true;
          }
    }
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
  myIconBorder = enable;

  if (myIconBorder)
  {
    myViewInternal->move(myViewInternal->iconBorderWidth+2, 2);
    myViewInternal->leftBorder->show();
  }
  else
  {
    myViewInternal->leftBorder->hide();
    myViewInternal->move(2, 2);
  }
}

void KateView::toggleIconBorder ()
{
  setIconBorder (!myIconBorder);
}

void KateView::gotoMark (KTextEditor::Mark *mark)
{
  KateViewCursor cursor;

  cursor.col = 0;
  cursor.line = mark->line;
  myDoc->needPreHighlight(cursor.line);
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
  myViewInternal->updateView(KateView::ufUpdateOnScroll);
  myDoc->updateViews(this);
}

void KateView::toggleBookmark ()
{
  /*TextLine::Ptr line = myDoc->getTextLine (currentLine());

  if (line->mark()&KateDocument::markType01)
    line->delMark(KateDocument::markType01));
  else
    line->addMark(KateDocument::Bookmark);

  myDoc->tagLines (currentLine(), currentLine());
  myDoc->updateViews();*/
}

void KateView::clearBookmarks()
{/*
  QPtrList<Kate::Mark> list = myDoc->marks();

  for (int i=0; (uint) i < list.count(); i++)
  {
    if (list.at(i)->type&KateDocument::Bookmark)
    {
      myDoc->getTextLine(list.at(i)->line)->delMark(KateDocument::Bookmark);
      myDoc->tagLines(list.at(i)->line, list.at(i)->line);
    }
  }

  myDoc->updateViews();*/
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
  QFont font = myDoc->getFont();
  font.setPointSize (font.pointSize()+1);
  myDoc->setFont (font);
}

void KateView::slotDecFontSizes ()
{
  QFont font = myDoc->getFont();
  font.setPointSize (font.pointSize()-1);
  myDoc->setFont (font);
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
