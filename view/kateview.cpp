/***************************************************************************
                          katedocument.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
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

/*
   Copyright (C) 1998, 1999 Jochen Wilhelmy
                            digisnap@cs.tu-berlin.de

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "kateview.h"
#include "kateview.moc"

#include "../document/katedocument.h"
#include "../document/katecmd.h"
#include "../factory/katefactory.h"

#include "../document/katedialogs.h"

#include <kurldrag.h>
#include <qfocusdata.h>
#include <kdebug.h>
#include <kprinter.h>
#include <kapp.h>

#include <klineeditdlg.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <list>

#include <qstring.h>
#include <qwidget.h>
#include <qfont.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qevent.h>
#include <qdir.h>
#include <qprintdialog.h>
#include <qpaintdevicemetrics.h>
#include <qdropsite.h>
#include <qdragobject.h>
#include <qiodevice.h>
#include <qbuffer.h>
#include <qtextcodec.h>
#include <qfocusdata.h>

#include <kapp.h>
#include <kcursor.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kstringhandler.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kparts/event.h>
#include <kxmlgui.h>
#include <dcopclient.h>
#include <qregexp.h>

#include <X11/Xlib.h> //used to have XSetTransientForHint()

#include "../document/katetextline.h"
#include "kateviewdialog.h"
#include "kateundohistory.h"

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifndef _PATH_TMP
#define _PATH_TMP "/tmp/"
#endif

struct BufferInfo {
  void *user;
  int w;
  int h;
};

QList<BufferInfo> bufferInfoList;
QPixmap *buffer = 0;

QPixmap *getBuffer(void *user) {
  BufferInfo *info;

  if (!buffer) buffer = new QPixmap;
  info = new BufferInfo;
  info->user = user;
  info->w = 0;
  info->h = 0;
  bufferInfoList.append(info);
  return buffer;
}

void resizeBuffer(void *user, int w, int h) {
  int z;
  BufferInfo *info;
  int maxW, maxH;

  maxW = w;
  maxH = h;
  for (z = 0; z < (int) bufferInfoList.count(); z++) {
    info = bufferInfoList.at(z);
    if (info->user == user) {
      info->w = w;
      info->h = h;
    } else {
      if (info->w > maxW) maxW = info->w;
      if (info->h > maxH) maxH = info->h;
    }
  }
  if (maxW != buffer->width() || maxH != buffer->height()) {
    buffer->resize(maxW,maxH);
  }
}

void releaseBuffer(void *user) {
  int z;
  BufferInfo *info;

  for (z = (int) bufferInfoList.count() -1; z >= 0 ; z--) {
    info = bufferInfoList.at(z);
    if (info->user == user) bufferInfoList.remove(z);
  }
  resizeBuffer(0,0,0);
}


KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc, bool HandleOwnDND) : QWidget(view)
{
  myView = view;
  myDoc = doc;

  QWidget::setCursor(ibeamCursor);
  setBackgroundMode(NoBackground);
  KCursor::setAutoHideCursor( this, true );

  setFocusPolicy(StrongFocus);
  move(2,2);

  xScroll = new QScrollBar(QScrollBar::Horizontal,myView);
  yScroll = new QScrollBar(QScrollBar::Vertical,myView);
  connect(xScroll,SIGNAL(valueChanged(int)),SLOT(changeXPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),SLOT(changeYPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),myView,SIGNAL(scrollValueChanged(int)));

  xPos = 0;
  yPos = 0;

  scrollTimer = 0;

  cursor.x = 0;
  cursor.y = 0;
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

  drawBuffer = getBuffer(this);

  bm.sXPos = 0;
  bm.eXPos = -1;

  setAcceptDrops(true);
  HandleURIDrops = HandleOwnDND;
  dragInfo.state = diNone;
}

KateViewInternal::~KateViewInternal()
{
  delete [] lineRanges;
  releaseBuffer(this);
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

void KateViewInternal::doEditCommand(VConfig &c, int cmdNum) {

  switch (cmdNum) {
    case KateView::cmCopy:
      myDoc->copy(c.flags);
      return;
    case KateView::cmSelectAll:
      myDoc->selectAll();
      return;
    case KateView::cmDeselectAll:
      myDoc->deselectAll();
      return;
    case KateView::cmInvertSelection:
      myDoc->invertSelection();
      return;
  }
  if (myView->isReadOnly()) return;
  switch (cmdNum) {
    case KateView::cmReturn:
      if (c.flags & KateView::cfDelOnInput) myDoc->delMarkedText(c);
      myDoc->newLine(c);
      //emit returnPressed();
      //e->ignore();
      return;
    case KateView::cmDelete:
      if ((c.flags & KateView::cfDelOnInput) && myDoc->hasMarkedText())
        myDoc->delMarkedText(c);
      else myDoc->del(c);
      return;
    case KateView::cmBackspace:
      if ((c.flags & KateView::cfDelOnInput) && myDoc->hasMarkedText())
        myDoc->delMarkedText(c);
      else myDoc->backspace(c);
      return;
    case KateView::cmKillLine:
      myDoc->killLine(c);
      return;
    case KateView::cmCut:
      myDoc->cut(c);
      return;
    case KateView::cmPaste:
      if (c.flags & KateView::cfDelOnInput) myDoc->delMarkedText(c);
      myDoc->paste(c);
      return;
    case KateView::cmUndo:
      myDoc->undo(c);
      return;
    case KateView::cmRedo:
      myDoc->redo(c);
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

  cursor.x--;
  if (c.flags & KateView::cfWrapCursor && cursor.x < 0 && cursor.y > 0) {
    cursor.y--;
    cursor.x = myDoc->textLength(cursor.y);
  }
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::cursorRight(VConfig &c) {

  if (c.flags & KateView::cfWrapCursor) {
    if (cursor.x >= myDoc->textLength(cursor.y)) {
      if (cursor.y == myDoc->lastLine()) return;
      cursor.y++;
      cursor.x = -1;
    }
  }
  cursor.x++;
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::wordLeft(VConfig &c) {
  Highlight *highlight;

  highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->getTextLine(cursor.y);

  if (cursor.x > 0) {
    do {
      cursor.x--;
    } while (cursor.x > 0 && !highlight->isInWord(textLine->getChar(cursor.x)));
    while (cursor.x > 0 && highlight->isInWord(textLine->getChar(cursor.x -1)))
      cursor.x--;
  } else {
    if (cursor.y > 0) {
      cursor.y--;
      textLine = myDoc->getTextLine(cursor.y);
      cursor.x = textLine->length();
    }
  }

  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::wordRight(VConfig &c) {
  Highlight *highlight;
  int len;

  highlight = myDoc->highlight();
  TextLine::Ptr textLine = myDoc->getTextLine(cursor.y);
  len = textLine->length();

  if (cursor.x < len) {
    do {
      cursor.x++;
    } while (cursor.x < len && highlight->isInWord(textLine->getChar(cursor.x)));
    while (cursor.x < len && !highlight->isInWord(textLine->getChar(cursor.x)))
      cursor.x++;
  } else {
    if (cursor.y < myDoc->lastLine()) {
      cursor.y++;
      textLine = myDoc->getTextLine(cursor.y);
      cursor.x = 0;
    }
  }

  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}

void KateViewInternal::home(VConfig &c) {
  int lc;

  lc = (c.flags & KateView::cfSmartHome) ? myDoc->getTextLine(cursor.y)->firstChar() : 0;
  if (lc <= 0 || cursor.x == lc) {
    cursor.x = 0;
    cOldXPos = cXPos = 0;
  } else {
    cursor.x = lc;
    cOldXPos = cXPos = myDoc->textWidth(cursor);
  }

  changeState(c);
}

void KateViewInternal::end(VConfig &c) {
  cursor.x = myDoc->textLength(cursor.y);
  cOldXPos = cXPos = myDoc->textWidth(cursor);
  changeState(c);
}


void KateViewInternal::cursorUp(VConfig &c) {

  cursor.y--;
  cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
}


void KateViewInternal::cursorDown(VConfig &c) {
  int x;

  if (cursor.y == myDoc->lastLine()) {
    x = myDoc->textLength(cursor.y);
    if (cursor.x >= x) return;
    cursor.x = x;
    cXPos = myDoc->textWidth(cursor);
  } else {
    cursor.y++;
    cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor, cursor, cOldXPos);
  }
  changeState(c);
}

void KateViewInternal::scrollUp(VConfig &c) {

  if (! yPos) return;

  newYPos = yPos - myDoc->fontHeight;
  if (cursor.y == (yPos + height())/myDoc->fontHeight -1) {
    cursor.y--;
    cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor,cursor,cOldXPos);

    changeState(c);
  }
}

void KateViewInternal::scrollDown(VConfig &c) {

  if (endLine >= myDoc->lastLine()) return;

  newYPos = yPos + myDoc->fontHeight;
  if (cursor.y == (yPos + myDoc->fontHeight -1)/myDoc->fontHeight) {
    cursor.y++;
    cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor,cursor,cOldXPos);
    changeState(c);
  }
}

void KateViewInternal::topOfView(VConfig &c) {

  cursor.y = (yPos + myDoc->fontHeight -1)/myDoc->fontHeight;
  cursor.x = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::bottomOfView(VConfig &c) {

  cursor.y = (yPos + height())/myDoc->fontHeight -1;
  if (cursor.y < 0) cursor.y = 0;
  if (cursor.y > myDoc->lastLine()) cursor.y = myDoc->lastLine();
  cursor.x = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KateViewInternal::pageUp(VConfig &c) {
  int lines = (endLine - startLine - 1);

  if (lines <= 0) lines = 1;

  if (!(c.flags & KateView::cfPageUDMovesCursor) && yPos > 0) {
    newYPos = yPos - lines * myDoc->fontHeight;
    if (newYPos < 0) newYPos = 0;
  }
  cursor.y -= lines;
  cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor, cursor, cOldXPos);
  changeState(c);
//  cursorPageUp(c);
}

void KateViewInternal::pageDown(VConfig &c) {

  int lines = (endLine - startLine - 1);

  if (!(c.flags & KateView::cfPageUDMovesCursor) && endLine < myDoc->lastLine()) {
    if (lines < myDoc->lastLine() - endLine)
      newYPos = yPos + lines * myDoc->fontHeight;
    else
      newYPos = yPos + (myDoc->lastLine() - endLine) * myDoc->fontHeight;
  }
  cursor.y += lines;
  cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
//  cursorPageDown(c);
}

// go to the top, same X position
void KateViewInternal::top(VConfig &c) {

//  cursor.x = 0;
  cursor.y = 0;
  cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom, same X position
void KateViewInternal::bottom(VConfig &c) {

//  cursor.x = 0;
  cursor.y = myDoc->lastLine();
  cXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the top left corner
void KateViewInternal::top_home(VConfig &c)
{
  cursor.y = 0;
  cursor.x = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom right corner
void KateViewInternal::bottom_end(VConfig &c) {

  cursor.y = myDoc->lastLine();
  cursor.x = myDoc->textLength(cursor.y);
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
  if (QABS(dy) < height()) scroll(0, dy); else update();
}


void KateViewInternal::getVConfig(VConfig &c) {

  c.view = myView;
  c.cursor = cursor;
  c.cXPos = cXPos;
  c.flags = myView->configFlags;
  c.wrapAt = myView->wrapAt;
}

void KateViewInternal::changeState(VConfig &c) {
  /*
   * we need to be sure to kill the selection on an attempted cursor
   * movement even if the cursor doesn't physically move,
   * but we need to be careful not to do some other things in this case,
   * like we don't want to expose the cursor
   */

//  if (cursor.x == c.cursor.x && cursor.y == c.cursor.y) return;
  bool nullMove = (cursor.x == c.cursor.x && cursor.y == c.cursor.y);

//  if (cursor.y != c.cursor.y || c.flags & KateView::cfMark) myDoc->recordReset();

  if (! nullMove) {
    myDoc->unmarkFound();

    exposeCursor = true;

    // mark old position of cursor as dirty
    if (cursorOn) {
      tagLines(c.cursor.y, c.cursor.y, c.cXPos -2, c.cXPos +3);
      cursorOn = false;
    }

    // mark old bracket mark position as dirty
    if (bm.sXPos < bm.eXPos) {
      tagLines(bm.cursor.y, bm.cursor.y, bm.sXPos, bm.eXPos);
    }
    // make new bracket mark
    myDoc->newBracketMark(cursor, bm);

    // remove trailing spaces when leaving a line
    if (c.flags & KateView::cfRemoveSpaces && cursor.y != c.cursor.y) {
      TextLine::Ptr textLine = myDoc->getTextLine(c.cursor.y);
      int newLen = textLine->lastChar();
      if (newLen != textLine->length()) {
        textLine->truncate(newLen);
        // if some spaces are removed, tag the line as dirty
        myDoc->tagLines(c.cursor.y, c.cursor.y);
      }
    }
  }

  if (c.flags & KateView::cfMark) {
    if (! nullMove)
      myDoc->selectTo(c, cursor, cXPos);
  } else {
    if (!(c.flags & KateView::cfPersistent))
      myDoc->deselectAll();
  }
}

void KateViewInternal::insLine(int line) {

  if (line <= cursor.y) {
    cursor.y++;
  }
  if (line < startLine) {
    startLine++;
    endLine++;
    yPos += myDoc->fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }

  //bookmarks
  KWBookmark *b;
  for (b = myView->bookmarks.first(); b != 0L; b = myView->bookmarks.next()) {
    if (b->cursor.y >= line) {
      b->cursor.y++;
      b->yPos += myDoc->fontHeight;
    }
  }
}

void KateViewInternal::delLine(int line) {

  if (line <= cursor.y && cursor.y > 0) {
    cursor.y--;
  }
  if (line < startLine) {
    startLine--;
    endLine--;
    yPos -= myDoc->fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }

  //bookmarks
  KWBookmark *b;
  for (b = myView->bookmarks.first(); b != 0L; b = myView->bookmarks.next()) {
    if (b->cursor.y > line) {
      b->cursor.y--;
      b->yPos -= myDoc->fontHeight;
//      if (b->yPos < 0) b->yPos = 0;
    }
  }
}

void KateViewInternal::updateCursor() {
  cOldXPos = cXPos = myDoc->textWidth(cursor);
}


void KateViewInternal::updateCursor(PointStruc &newCursor) {
  updateCursor(newCursor, myView->config());
}

void KateViewInternal::updateCursor(PointStruc &newCursor, int flags) {

  if (!(flags & KateView::cfPersistent)) myDoc->deselectAll();
  myDoc->unmarkFound();

  exposeCursor = true;
  if (cursorOn) {
    tagLines(cursor.y, cursor.y, cXPos -2, cXPos +3);
    cursorOn = false;
  }

  if (bm.sXPos < bm.eXPos) {
    tagLines(bm.cursor.y, bm.cursor.y, bm.sXPos, bm.eXPos);
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
  newYPos = cursor.y*myDoc->fontHeight - height()/2;
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

//debug("upView %d %d %d %d %d", exposeCursor, updateState, flags, newXPos, newYPos);
  if (exposeCursor || flags & KateView::ufDocGeometry) {
    emit myView->newCurPos();
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
  cYPos = cursor.y*fontHeight;

  z = 0;
  do {
    w = myView->width() - 4;
    h = myView->height() - 4;

    xMax = myDoc->textWidth() - w;
    b = (xPos > 0 || xMax > 0);
    if (b) h -= 16;
    yMax = myDoc->textHeight() - h;
    if (yPos > 0 || yMax > 0) {
      w -= 16;
      xMax += 16;
      if (!b && xMax > 0) {
        h -= 16;
        yMax += 16;
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
    xScroll->setGeometry(2,h + 2,w,16);
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
    yScroll->setGeometry(w + 2,2,16,h);
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

  QPainter paint;
  paint.begin(drawBuffer);

  h = myDoc->fontHeight;
  r = lineRanges;
  for (line = startLine; line <= endLine; line++) {
    if (r->start < r->end) {
//debug("painttextline %d %d %d", line, r->start, r->end);
      myDoc->paintTextLine(paint, line, r->start, r->end, myView->configFlags & KateView::cfShowTabs);
      bitBlt(this, r->start - (xPos-2), line*h - yPos, drawBuffer, 0, 0,
        r->end - r->start, h);
    }
    r++;
  }

  paint.end();
}

void KateViewInternal::paintCursor() {
  int h, y, x;
  static int cx = 0, cy = 0, ch = 0;

  h = myDoc->fontHeight;
  y = h*cursor.y - yPos;
  x = cXPos - (xPos-2);

  QFont f = myDoc->getTextFont(cursor.x, cursor.y);
  if(f != font()) setFont(f);
  if(cx != x || cy != y || ch != h){
    cx = x;
    cy = y;
    ch = h;
    setMicroFocusHint(cx, cy, 0, ch - 2);
  }

  QPainter paint;
  if (cursorOn) {
    paint.begin(this);
    paint.setClipping(false);
    paint.setPen(myDoc->cursorCol(cursor.x,cursor.y));

    h += y - 1;
    paint.drawLine(x, y, x, h);
    paint.drawLine(x-2, y, x+2, y);
    paint.drawLine(x-2, h, x+2, h);
  } else {
    paint.begin(drawBuffer);
    myDoc->paintTextLine(paint, cursor.y, cXPos - 2, cXPos + 3, myView->configFlags & KateView::cfShowTabs);
    bitBlt(this,x - 2,y, drawBuffer, 0, 0, 5, h);
  }
  paint.end();
}

void KateViewInternal::paintBracketMark() {
  int y;

  y = myDoc->fontHeight*(bm.cursor.y +1) - yPos -1;

  QPainter paint;
  paint.begin(this);
  paint.setPen(myDoc->cursorCol(bm.cursor.x, bm.cursor.y));

  paint.drawLine(bm.sXPos - (xPos-2), y, bm.eXPos - (xPos-2) -1, y);
  paint.end();
}

void KateViewInternal::placeCursor(int x, int y, int flags) {
  VConfig c;

  getVConfig(c);
  c.flags |= flags;
  cursor.y = (yPos + y)/myDoc->fontHeight;
  cXPos = cOldXPos = myDoc->textWidth(c.flags & KateView::cfWrapCursor, cursor,xPos-2 + x);
  changeState(c);
}

// convert the given physical coordinates to logical (line/column within the document)
/*
void KateViewInternal::calcLogicalPosition(int &x, int &y) {

  TextLine   line;

  y = (yPos + y)/myDoc->fontHeight;

  line = myDoc->textLine(y);

  x = myDoc->textPos(myDoc->textLine(y), x);
}
*/
// given physical coordinates, report whether the text there is selected
bool KateViewInternal::isTargetSelected(int x, int y) {

  y = (yPos + y) / myDoc->fontHeight;

  TextLine::Ptr line = myDoc->getTextLine(y);
  if (!line)
    return false;

  x = myDoc->textPos(line, x);

  return line->isSelected(x);
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
//  int ascii;

/*  if (e->state() & AltButton) {
    e->ignore();
    return;
  }*/
//  debug("ascii %i, key %i, state %i",e->ascii(), e->key(), e->state());

  getVConfig(c);
//  ascii = e->ascii();

  if (!myView->isReadOnly()) {
    if (c.flags & KateView::cfTabIndents && myDoc->hasMarkedText()) {
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
    if ( !(e->state() & ControlButton ) && myDoc->insertChars(c, e->text())) {
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
      dragInfo.start.x = e->x();
      dragInfo.start.y = e->y();
    } else {
      // we have no reason to ever start a drag from here
      dragInfo.state = diNone;

      int flags;

      flags = 0;
      if (e->state() & ShiftButton) {
        flags |= KateView::cfMark;
        if (e->state() & ControlButton) flags |= KateView::cfMark | KateView::cfKeepSelection;
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
    if (! myView->isReadOnly())
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
      if (myView->config() & KateView::cfMouseAutoCopy) myView->copy();
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

      if (x > dragInfo.start.x + 4 || x < dragInfo.start.x - 4 ||
          y > dragInfo.start.y + 4 || y < dragInfo.start.y - 4) {
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
    flags = KateView::cfMark;
    if (e->state() & ControlButton) flags |= KateView::cfKeepSelection;
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
//  debug("update rect  = ( %i, %i, %i, %i )",
//    updateR.x(),updateR.y(), updateR.width(), updateR.height() );

  QPainter paint;
  paint.begin(drawBuffer);

  xStart = xPos-2 + updateR.x();
  xEnd = xStart + updateR.width();

  h = myDoc->fontHeight;
  line = (yPos + updateR.y()) / h;
  y = line*h - yPos;
  yEnd = updateR.y() + updateR.height();

  while (y < yEnd)
  {
    TextLine *textLine;
    int ctxNum = 0;

    if ((myDoc->getTextLineCount()-1)>line)
    {
      textLine = myDoc->getTextLine(line);
      if (line > 0)
        ctxNum = myDoc->getTextLine(line - 1)->getContext();

      ctxNum = myDoc->highlight()->doHighlight(ctxNum,textLine);
      textLine->setContext(ctxNum);
    }

    myDoc->paintTextLine(paint, line, xStart, xEnd, myView->configFlags & KateView::cfShowTabs);
    bitBlt(this, updateR.x(), y, drawBuffer, 0, 0, updateR.width(), h);

    line++;
    y += h;
  }
  paint.end();

  if (cursorOn) paintCursor();
  if (bm.eXPos > bm.sXPos) paintBracketMark();
}

void KateViewInternal::resizeEvent(QResizeEvent *)
{
  resizeBuffer(this, width(), myDoc->fontHeight);
}

void KateViewInternal::timerEvent(QTimerEvent *e) {
  if (e->timerId() == cursorTimer) {
    cursorOn = !cursorOn;
    paintCursor();
  }
  if (e->timerId() == scrollTimer && (scrollX | scrollY)) {
    xScroll->setValue(xPos + scrollX);
    yScroll->setValue(yPos + scrollY);

    placeCursor(mouseX, mouseY, KateView::cfMark);
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
  dragInfo.dragObject = new QTextDrag(myDoc->markedText(0), this);
  if (myView->isReadOnly()) {
    dragInfo.dragObject->dragCopy();
  } else {

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   drag() is broken for move operations in Qt - dragCopy() is the only safe way
   to go right now

    if (dragInfo.dragObject->drag()) {
      // the drag has completed and it turned out to be a move operation
      if (! myDoc->ownedView((KateViewInternal*)(QDragObject::target()))) {
        // the target is not me - we need to delete our selection
        VConfig c;
        getVConfig(c);
        myDoc->delMarkedText(c);
        myDoc->updateViews();
      }
    }
*/
    dragInfo.dragObject->dragCopy();

  }
}

void KateViewInternal::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept( (QTextDrag::canDecode(event) && ! myView->isReadOnly()) || QUriDrag::canDecode(event) );
}

void KateViewInternal::dropEvent( QDropEvent *event )
{
  if ( QUriDrag::canDecode(event) ) {
    QStrList  urls;

    if (! HandleURIDrops) {
      // the container should handle this one for us...
      emit dropEventPass(event);
    }
    else
    {
      KURL::List textlist;
      if (!KURLDrag::decode(event, textlist)) return;
      KURL::List::Iterator i=textlist.begin();
      if (myView->canDiscard()) myView->loadURL(*i);
    }
  } else if ( QTextDrag::canDecode(event) && ! myView->isReadOnly() ) {

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
      PointStruc cursor;

      getVConfig(c);
      cursor = c.cursor;

      if (priv) {
        // this is one of mine (this document), not dropped on the selection
        if (event->action() == QDropEvent::Move) {
          myDoc->delMarkedText(c);
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
      myDoc->insert(c, text);
      cursor = c.cursor;

      updateCursor(cursor);
      myDoc->updateViews();
    }
  }
}


KWBookmark::KWBookmark() {
  cursor.y = -1; //mark bookmark as invalid
}

KateView::KateView(KateDocument *doc, QWidget *parent, const char * name, bool HandleOwnDND, bool deleteDoc) : KateViewIface (doc, parent, name), DCOPObject(name)
{
  setInstance( KateFactory::instance() );

  myDeleteDoc = deleteDoc;
  active = false;

  myDoc = doc;
  myViewInternal = new KateViewInternal(this,doc,HandleOwnDND);

  doc->addView( this );

  connect(myViewInternal,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(dropEventPassEmited(QDropEvent *)));

  // some defaults
  configFlags = KateView::cfAutoIndent | KateView::cfSpaceIndent | KateView::cfBackspaceIndents
    | KateView::cfTabIndents | KateView::cfKeepIndentProfile
    | KateView::cfReplaceTabs | KateView::cfSpaceIndent | KateView::cfRemoveSpaces
    | KateView::cfDelOnInput | KateView::cfMouseAutoCopy
    | KateView::cfGroupUndo | KateView::cfShowTabs | KateView::cfSmartHome;
  wrapAt = 80;
  searchFlags = 0;
  replacePrompt = 0L;
  rmbMenu = 0L;
  bookmarks.setAutoDelete(true);

  //KSpell initial values
  kspell.kspell = 0;
  kspell.ksc = new KSpellConfig; //default KSpellConfig to start
  kspell.kspellon = FALSE;

  setFocusProxy( myViewInternal );
  myViewInternal->setFocus();
  resize(parent->width() -4, parent->height() -4);

  m_tempSaveFile = 0;

  printer = new KPrinter();

  myViewInternal->installEventFilter( this );

  if (!doc->hasBrowserExtension())
  {
    setXMLFile( "katepartui.rc" );
  }
  else
  {
    (void)new KateBrowserExtension( myDoc );
    myDoc->setXMLFile( "katepartbrowserui.rc" );
  }

  setupActions();

  connect( this, SIGNAL( newStatus() ), this, SLOT( slotUpdate() ) );
  connect( this, SIGNAL( newUndo() ), this, SLOT( slotNewUndo() ) );
  connect( this, SIGNAL( fileChanged() ), this, SLOT( slotFileStatusChanged() ) );
  connect( doc, SIGNAL( highlightChanged() ), this, SLOT( slotHighlightChanged() ) );
  if ( doc->hasBrowserExtension() )
    connect( this, SIGNAL( dropEventPass(QDropEvent*) ), this, SLOT( slotDropEventPass(QDropEvent*) ) );

  setHighlight->setCurrentItem(getHl());
  slotUpdate();
}

KateView::~KateView()
{
  QMap<KIO::Job *, NetData>::Iterator it = m_mapNetData.begin();
  while ( it != m_mapNetData.end() )
  {
      KIO::Job *job = it.key();
      m_mapNetData.remove( it );
      job->kill();
      it = m_mapNetData.begin();
  }
  if (kspell.kspell)
  {
    kspell.kspell->setAutoDelete(true);
    kspell.kspell->cleanUp(); // need a way to wait for this to complete
  }
  delete kspell.ksc;

  if (myDoc && !myDoc->m_bSingleViewMode)
    myDoc->removeView( this );

  if ( !myDoc->m_bSingleViewMode )
  {
    if ( myDoc->isLastView(0) && myDeleteDoc )
      delete myDoc;
  }

  delete myViewInternal;

  delete m_tempSaveFile;
  delete printer;
}

void KateView::setupActions()
{
    KStdAction::close( this, SLOT(newDoc()), actionCollection(), "file_close" );
    fileRecent = KStdAction::openRecent(this, SLOT(slotOpenRecent(const KURL&)),
                                        actionCollection());

    fileSave = KStdAction::save(this, SLOT(save()), actionCollection());
    KStdAction::saveAs(this, SLOT(saveAs()), actionCollection());

    // setup edit menu
    editUndo = KStdAction::undo(this, SLOT(undo()), actionCollection());
    editRedo = KStdAction::redo(this, SLOT(redo()), actionCollection());
    editUndoHist = new KAction(i18n("Undo/Redo &History..."), 0, this, SLOT(undoHistory()),
                               actionCollection(), "edit_undoHistory");
    editCut = KStdAction::cut(this, SLOT(cut()), actionCollection());
    editPaste = KStdAction::copy(this, SLOT(copy()), actionCollection());
    editReplace = KStdAction::paste(this, SLOT(paste()), actionCollection());
    KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());
    new KAction(i18n("&Deselect All"), 0, this, SLOT(deselectAll()),
                actionCollection(), "edit_deselectAll");
    new KAction(i18n("Invert &Selection"), 0, this, SLOT(invertSelection()),
                actionCollection(), "edit_invertSelection");

    if ( myDoc->hasBrowserExtension() )
    {
      KStdAction::find(this, SLOT(find()), myDoc->actionCollection(), "find");
      KStdAction::findNext(this, SLOT(findAgain()), myDoc->actionCollection(), "find_again");
      KStdAction::gotoLine(this, SLOT(gotoLine()), myDoc->actionCollection(), "goto_line" );
      new KAction(i18n("Configure Highlighti&ng..."), 0, this, SLOT(hlDlg()),myDoc->actionCollection(), "set_confHighlight");
      setHighlight = new KSelectAction(i18n("&Highlight Mode"), 0, myDoc->actionCollection(), "set_highlight");
    }
    else
    {
      KStdAction::find(this, SLOT(find()), actionCollection());
      KStdAction::findNext(this, SLOT(findAgain()), actionCollection());
      KStdAction::gotoLine(this, SLOT(gotoLine()), actionCollection());
      new KAction(i18n("Configure Highlighti&ng..."), 0, this, SLOT(hlDlg()),actionCollection(), "set_confHighlight");
      setHighlight = new KSelectAction(i18n("&Highlight Mode"), 0, actionCollection(), "set_highlight");
    }

    KStdAction::replace(this, SLOT(replace()), actionCollection());
    editInsert = new KAction(i18n("&Insert File..."), 0, this, SLOT(insertFile()),
                             actionCollection(), "edit_insertFile");

   KAction *editCmd = new KAction(i18n("&Editing Command"), Qt::CTRL+Qt::Key_D, this, SLOT(slotEditCommand()),
                                  actionCollection(), "edit_cmd");

    // setup Go menu
    KAction *addAct = new KAction(i18n("&Add Marker"), Qt::CTRL+Qt::Key_M, this, SLOT(addBookmark()),
                                  actionCollection(), "go_addMarker");
    connect(this, SIGNAL(bookAddChanged(bool)),addAct,SLOT(setEnabled(bool)));
    new KAction(i18n("&Set Marker..."), 0, this, SLOT(setBookmark()),
                actionCollection(), "go_setMarker");
    KAction *clearAct = new KAction(i18n("&Clear Markers"), 0, this, SLOT(clearBookmarks()),
                                    actionCollection(), "go_clearMarkers");
    connect(this, SIGNAL(bookClearChanged(bool)),clearAct,SLOT(setEnabled(bool)));
    clearAct->setEnabled(false);

    // setup Tools menu
    toolsSpell = KStdAction::spelling(this, SLOT(spellcheck()), actionCollection());
    toolsIndent = new KAction(i18n("&Indent"), Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              actionCollection(), "tools_indent");
    toolsUnindent = new KAction(i18n("&Unindent"), Qt::CTRL+Qt::Key_U, this, SLOT(unIndent()),
                                actionCollection(), "tools_unindent");
    toolsCleanIndent = new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   actionCollection(), "tools_cleanIndent");
    toolsComment = new KAction(i18n("C&omment"), 0, this, SLOT(comment()),
                               actionCollection(), "tools_comment");
    toolsUncomment = new KAction(i18n("Unco&mment"), 0, this, SLOT(uncomment()),
                                 actionCollection(), "tools_uncomment");

    setVerticalSelection = new KToggleAction(i18n("&Vertical Selection"), 0, this, SLOT(toggleVertical()),
                                             actionCollection(), "set_verticalSelect");

    connect(setHighlight, SIGNAL(activated(int)), this, SLOT(setHl(int)));
    QStringList list;
    for (int z = 0; z < HlManager::self()->highlights(); z++)
        list.append(i18n(HlManager::self()->hlName(z)));
    setHighlight->setItems(list);

    setEndOfLine = new KSelectAction(i18n("&End Of Line"), 0, actionCollection(), "set_eol");
    connect(setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
    list.clear();
    list.append("&Unix");
    list.append("&Macintosh");
    list.append("&Windows/Dos");
    setEndOfLine->setItems(list);
}

void KateView::slotUpdate()
{
    int cfg = config();
    bool readOnly = isReadOnly();

    setVerticalSelection->setChecked(cfg & KateView::cfVerticalSelect);

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

    slotNewUndo();
}
void KateView::slotFileStatusChanged()
{
  int eol = getEol()-1;
  eol = eol>=0? eol: 0;

    setEndOfLine->setCurrentItem(eol);

    if ( !doc()->url().isEmpty() )
        //set recent files popup menu
        fileRecent->addURL(doc()->url());

}
void KateView::slotNewUndo()
{
    int state = undoState();

    editUndoHist->setEnabled(state & 1 || state & 2);

    QString t = i18n("Und&o");   // it would be nicer to fetch the original string
    if (state & 1) {
        editUndo->setEnabled(true);
        t += ' ';
        t += i18n(undoTypeName(nextUndoType()));
    } else {
        editUndo->setEnabled(false);
    }
    editUndo->setText(t);

    t = i18n("Re&do");   // it would be nicer to fetch the original string
    if (state & 2) {
        editRedo->setEnabled(true);
        t += ' ';
        t += i18n(undoTypeName(nextRedoType()));
    } else {
        editRedo->setEnabled(false);
    }
    editRedo->setText(t);
}

void KateView::slotHighlightChanged()
{
    setHighlight->setCurrentItem(getHl());
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
            keyReturn();
            break;
        case Key_Delete:
            if ( ev->state() & ControlButton )
            {
               VConfig c;
               shiftWordRight();
               myViewInternal->getVConfig(c);
               myDoc->delMarkedText(c);
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
               myDoc->delMarkedText(c);
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

void KateView::slotOpenRecent( const KURL &url )
{
    if ( canDiscard() )
        loadURL( url );
}

void KateView::setCursorPosition( int line, int col, bool /*mark*/ )
{
  setCursorPositionInternal( line, col );
}

void KateView::getCursorPosition( int *line, int *col )
{
  if ( line )
    *line = currentLine();

  if ( col )
    *col = currentColumn();
}


int KateView::currentLine() {
  return myViewInternal->cursor.y;
}

int KateView::currentColumn() {
  return myDoc->currentColumn(myViewInternal->cursor);
}

int KateView::currentCharNum() {
  return myViewInternal->cursor.x;
}

void KateView::setCursorPositionInternal(int line, int col) {
  PointStruc cursor;

  cursor.x = col;
  cursor.y = line;
  myViewInternal->updateCursor(cursor);
  myViewInternal->center();
//  myDoc->unmarkFound();
//  myViewInternal->updateView(ufPos, 0, line*myDoc->fontHeight - height()/2);
//  myDoc->updateViews(myViewInternal); //uptade all other views except this one
  myDoc->updateViews();
}

int KateView::config() {
  int flags;

  flags = configFlags;
  if (myDoc->singleSelection()) flags |= KateView::cfSingleSelection;
  return flags;
}

void KateView::setConfig(int flags) {
  bool updateView;

  // cfSingleSelection is a doc-property
  myDoc->setSingleSelection(flags & KateView::cfSingleSelection);
  flags &= ~KateView::cfSingleSelection;

  if (flags != configFlags) {
    // update the view if visibility of tabs has changed
    updateView = (flags ^ configFlags) & KateView::cfShowTabs;
    configFlags = flags;
    emit newStatus();
    if (updateView) myViewInternal->update();
  }
}

int KateView::tabWidth() {
  return myDoc->tabChars;
}

void KateView::setTabWidth(int w) {
  myDoc->setTabWidth(w);
  myDoc->updateViews();
}

int KateView::undoSteps() {
  return myDoc->undoSteps;
}

void KateView::setUndoSteps(int s) {
  myDoc->setUndoSteps(s);
}

bool KateView::isReadOnly() {
  return myDoc->readOnly;
}

bool KateView::isModified() {
  return myDoc->modified;
}

void KateView::setReadOnly(bool m) {
  myDoc->setReadOnly(m);
}

void KateView::setModified(bool m) {
  myDoc->setModified(m);
}

bool KateView::isLastView() {
  return myDoc->isLastView(1);
}

KateDocument *KateView::doc() {
  return myDoc;
}

int KateView::undoState() {
  if (isReadOnly())
    return 0;
  else
    return myDoc->undoState;
}

int KateView::nextUndoType() {
  return myDoc->nextUndoType();
}

int KateView::nextRedoType() {
  return myDoc->nextRedoType();
}

void KateView::undoTypeList(QValueList<int> &lst)
{
  myDoc->undoTypeList(lst);
}

void KateView::redoTypeList(QValueList<int> &lst)
{
  myDoc->redoTypeList(lst);
}

const char * KateView::undoTypeName(int type) {
  return KateActionGroup::typeName(type);
}

void KateView::copySettings(KateView *w) {
  configFlags = w->configFlags;
  wrapAt = w->wrapAt;
  searchFlags = w->searchFlags;
}


QColor* KateView::getColors()
{
  return myDoc->colors;
}


void KateView::applyColors()
{
   myDoc->tagAll();
   myDoc->updateViews();
}


bool KateView::isOverwriteMode() const
{
  return ( configFlags & KateView::cfOvr );
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    setConfig( configFlags ^ KateView::cfOvr );
  else
    setConfig( configFlags | KateView::cfOvr );
}

void KateView::toggleInsert() {
  setConfig(configFlags ^ KateView::cfOvr);
}

void KateView::toggleVertical() {
  setConfig(configFlags ^ KateView::cfVerticalSelect);
  emit statusMsg(configFlags & KateView::cfVerticalSelect ? i18n("Vertical Selections On") : i18n("Vertical Selections Off"));
}


int KateView::numLines() {
  return myDoc->numLines();
}

QString KateView::text() {
  return myDoc->text();
}

QString KateView::currentTextLine() {
  TextLine::Ptr textLine = myDoc->getTextLine(myViewInternal->cursor.y);
  return QString(textLine->getText(), textLine->length());
}

QString KateView::textLine(int num) {
  TextLine::Ptr textLine = myDoc->getTextLine(num);
  return QString(textLine->getText(), textLine->length());
}

QString KateView::currentWord() {
  return myDoc->getWord(myViewInternal->cursor);
}

QString KateView::word(int x, int y) {
  PointStruc cursor;
  cursor.y = (myViewInternal->yPos + y)/myDoc->fontHeight;
  if (cursor.y < 0 || cursor.y > myDoc->lastLine()) return QString();
  cursor.x = myDoc->textPos(myDoc->getTextLine(cursor.y), myViewInternal->xPos-2 + x);
  return myDoc->getWord(cursor);
}

void KateView::setText(const QString &s) {
  myDoc->setText(s);
  myDoc->updateViews();
}

void KateView::insertText(const QString &s, bool /*mark*/) {
  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->insert(c, s);
  myDoc->updateViews();
}

bool KateView::hasMarkedText() {
  return myDoc->hasMarkedText();
}

QString KateView::markedText() {
  return myDoc->markedText(configFlags);
}

#ifdef NEW_CODE
void KateView::loadFile(const QString &file, QTextCodec *codec, bool insert)
{
  VConfig c;

  if (!insert) {
    myDoc->clear();
    myDoc->loadFile(file, codec);
  } else {
    // TODO: Not yet supported.
#if 0
    myViewInternal->getVConfig(c);
    if (c.flags & cfDelOnInput) myDoc->delMarkedText(c);
    myDoc->insertFile(c, dev);
    myDoc->updateViews();
#endif
  }
}
#else
void KateView::loadFile(QIODevice &dev, bool insert) {
  VConfig c;

  if (!insert) {
    myDoc->loadFile(dev);
  } else {
    myViewInternal->getVConfig(c);
    if (c.flags & KateView::cfDelOnInput) myDoc->delMarkedText(c);
    myDoc->insertFile(c, dev);
    myDoc->updateViews();
  }
}
#endif

void KateView::writeFile(QIODevice &dev) {
#ifdef NEW_CODE
  // TODO: Not yet implemented.
#else
  myDoc->writeFile(dev);
  myDoc->updateViews();
#endif
}


bool KateView::loadFile(const QString &name, int flags) {
  QFileInfo info(name);
  if (!info.exists()) {
    if (flags & KateView::lfNewFile) return true;
    KMessageBox::sorry(this, i18n("The specified File does not exist"));
    return false;
  }
  if (info.isDir()) {
    KMessageBox::sorry(this, i18n("You have specified a directory"));
    return false;
  }
  if (!info.isReadable()) {
    KMessageBox::sorry(this, i18n("You do not have read permission to this file"));
    return false;
  }

#ifdef NEW_CODE
  // TODO: Select a proper codec.
  loadFile(name, QTextCodec::codecForLocale(), flags & KateView::lfInsert);
  return true;
#else
  QFile f(name);
  if (f.open(IO_ReadOnly)) {
    loadFile(f,flags & KateView::lfInsert);
    f.close();
    return true;
  }
#endif
  KMessageBox::sorry(this, i18n("An error occured while trying to open this document"));
  return false;
}

bool KateView::writeFile(const QString &name) {

  QFileInfo info(name);
  if(info.exists() && !info.isWritable()) {
    KMessageBox::sorry(this, i18n("You do not have write permission to this file"));
    return false;
  }
#ifdef NEW_CODE
  if (myDoc->writeFile(name, QTextCodec::codecForLocale()))
     return true; // Success
#else
  QFile f(name);
  if (f.open(IO_WriteOnly | IO_Truncate)) {
    writeFile(f);
    f.close();
    return true;//myDoc->setFileName(name);
  }
#endif
  KMessageBox::sorry(this, i18n("An error occured while trying to write this document"));
  return false;
}


void KateView::loadURL(const KURL &url, int flags) {
/*
    TODO: Add newDoc code for non-local files. Currently this is not supported there
          - Martijn Klingens
*/
  KURL u(url);

  if (u.isMalformed()) {
      QString s = i18n("Malformed URL\n%1").arg(url.prettyURL());
      KMessageBox::sorry(this, s);
      return;
  }

  if ( !url.isLocalFile() )
  {
    emit statusMsg(i18n("Loading..."));

    NetData d;
    d.m_url = url;
    d.m_flags = flags;

    KIO::Job *job = KIO::get( url );
    m_mapNetData.insert( job, d );

#ifdef NEW_CODE
    myDoc->clear();
#endif
    connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotJobReadResult( KIO::Job * ) ) );
    connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( slotJobData( KIO::Job *, const QByteArray & ) ) );
  }
  else {
    if ( flags & KateView::lfInsert ) {
      if ( loadFile( url.path(), flags ) )
        emit statusMsg( i18n( "Inserted : %1" ).arg( url.fileName() ) );
      else
        emit statusMsg( QString::null );
    } else {
      if (QFileInfo(url.path()).exists()) {
        if ( loadFile( url.path(), flags ) ) {
          myDoc->setURL( url, !(flags & KateView::lfNoAutoHl ) );
          emit statusMsg( i18n( "Read : %1" ).arg( url.fileName() ) );
        } else
          emit statusMsg( QString::null );
      } else {           // don't start whining, just make a new document
        myDoc->clear();
        myDoc->setURL( url, !(flags & KateView::lfNoAutoHl ) );
        myDoc->updateViews();
        emit statusMsg( i18n( "New File : %1" ).arg( url.fileName() ) );
        myDoc->setNewDoc( true ); // File is new, check for overwrite!
      }
    }
  }
}


void KateView::writeURL(const KURL &url, int ) {
/*
    TODO: Add newDoc code for non-local files. Currently this is not supported there
          - Martijn Klingens
*/

    // url
    emit statusMsg(i18n("Saving..."));
    /*
    KIOJob * iojob = new KIOJob;
    iojob->setGUImode ( KIOJob::NONE );
    QString tmpFile;
    tmpFile = QString(_PATH_TMP"/kwrite%1").arg(time(0L));

    m_sNet.insert( iojob->id(), new QString(u.url()) );
    m_sLocal.insert( iojob->id(), new QString(tmpFile));
    m_flags.insert( iojob->id(), new int(flags));

    connect(iojob,SIGNAL(sigFinished( int )),this,SLOT(slotPUTFinished( int )));
    connect(iojob,SIGNAL(sigError(int, const char *)),this,SLOT(slotIOJobError(int, const char *)));
    iojob->copy(tmpFile, url);

    if (!writeFile(tmpFile)) return;
    }*/

  QString path;

  delete m_tempSaveFile;
  if ( !url.isLocalFile() )
  {
    m_tempSaveFile = new KTempFile;
    path = m_tempSaveFile->name();
  }
  else
  {
    m_tempSaveFile = 0;
    path = url.path();
  }

  if ( !writeFile( path ) )
     return;

  if ( !url.isLocalFile() )
  {
    emit enableUI( false );

    if ( KIO::NetAccess::upload( m_tempSaveFile->name(), url ) )
    {
      myDoc->setModified( false );
      emit statusMsg( i18n( "Wrote %1" ).arg( url.fileName() ) );
    }
    else
    {
      emit statusMsg( QString::null );
      KMessageBox::error( this, KIO::NetAccess::lastErrorString() );
    }

    delete m_tempSaveFile;
    m_tempSaveFile = 0;

    emit enableUI( true );
  }
  else
  {
      myDoc->setModified( false );
      emit statusMsg( i18n( "Wrote %1" ).arg( url.fileName() ) );
      myDoc->setNewDoc( false ); // File is not new anymore
  }
}

void KateView::slotJobReadResult( KIO::Job *job )
{
    QMap<KIO::Job *, NetData>::Iterator it = m_mapNetData.find( job );
    assert( it != m_mapNetData.end() );
    QByteArray data = (*it).m_data;
    int flags = (*it).m_flags;
    KURL url = (*it).m_url;
    m_mapNetData.remove( it );

    if ( job->error() )
        job->showErrorDialog();
#ifdef NEW_CODE
    QString msg;

    if ( flags & KateView::lfInsert )
        msg = i18n( "Inserted : %1" ).arg( url.fileName() );
    else
        msg = i18n( "Read : %1" ).arg( url.fileName() );

    emit statusMsg( msg );
    // Something else todo?
#else
    else
        loadInternal( data, url, flags );
#endif
}

void KateView::loadInternal( const QByteArray &data, const KURL &url, int flags )
{
#ifdef NEW_CODE
    // TODO: Not yet supported.
#else
    QBuffer buff( data );
    buff.open( IO_ReadOnly );
    loadFile( buff, flags );

    QString msg;

    if ( flags & KateView::lfInsert )
        msg = i18n( "Inserted : %1" ).arg( url.fileName() );
    else
    {
        myDoc->setURL( url, !(flags & KateView::lfNoAutoHl ) );
        myDoc->updateLines();
        myDoc->updateViews();

        msg = i18n( "Read : %1" ).arg( url.fileName() );
    }

    emit statusMsg( msg );

    if ( flags & KateView::lfNewFile )
        myDoc->setModified( false );
#endif
}

void KateView::slotJobData( KIO::Job *job, const QByteArray &data )
{
#ifdef NEW_CODE
    myDoc->appendData(data, QTextCodec::codecForLocale());
#else
    QMap<KIO::Job *, NetData>::Iterator it = m_mapNetData.find( job );
    assert( it != m_mapNetData.end() );
    QBuffer buff( (*it).m_data );
    buff.open(IO_WriteOnly | IO_Append );
    buff.writeBlock( data.data(), data.size() );
    buff.close();
#endif
}

bool KateView::canDiscard() {
  int query;

  if (isModified()) {
    query = KMessageBox::warningYesNoCancel(this,
      i18n("The current Document has been modified.\nWould you like to save it?"));
    switch (query) {
      case KMessageBox::Yes: //yes
        if (save() == CANCEL) return false;
        if (isModified()) {
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

void KateView::newDoc() {

  if (canDiscard()) clear();
}

void KateView::open() {
  KURL url;

  if (!canDiscard()) return;
//  if (myDoc->hasFileName()) s = QFileInfo(myDoc->fileName()).dirPath();
//    else s = QDir::currentDirPath();

  url = KFileDialog::getOpenURL(myDoc->url().url(), QString::null, this);
  if (url.isEmpty()) return;
  loadURL(url);
}

void KateView::insertFile() {
  if (isReadOnly())
    return;

  KURL  url = KFileDialog::getOpenURL(myDoc->url().url(), QString::null, this);
  if (url.isEmpty()) return;
  loadURL(url,KateView::lfInsert);
}

KateView::fileResult KateView::save() {
  int query = KMessageBox::Yes;
  if (isModified()) {
    if (!myDoc->url().fileName().isEmpty() && ! isReadOnly()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( myDoc->isNewDoc() )
      {
        query = checkOverwrite( myDoc->url() );
        if( query == KMessageBox::Cancel )
          return CANCEL;
      }
      if( query == KMessageBox::Yes )
      writeURL(myDoc->url(),!(KateView::lfNoAutoHl));
      else  // Do not overwrite already existing document:
        return saveAs();
    } // New, unnamed document:
    else
      return saveAs();
  } else emit statusMsg(i18n("No changes need to be saved"));
  return OK;
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

KateView::fileResult KateView::saveAs() {
  KURL url;
  int query;

  do {
    query = KMessageBox::Yes;

    url = KFileDialog::getSaveURL(myDoc->url().url(), QString::null,this);
    if (url.isEmpty()) return CANCEL;

    query = checkOverwrite( url );
  }
  while (query != KMessageBox::Yes);

  if( query == KMessageBox::Cancel )
    return CANCEL;

  writeURL(url);
  myDoc->setURL( url, false );
  return OK;
}

void KateView::doCursorCommand(int cmdNum) {
  VConfig c;
  myViewInternal->getVConfig(c);
  if (cmdNum & selectFlag) c.flags |= KateView::cfMark;
  if (cmdNum & multiSelectFlag) c.flags |= KateView::cfMark | KateView::cfKeepSelection;
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


void KateView::clear() {
  if (isReadOnly())
    return;

  myDoc->clear();
  myDoc->clearFileName();
  myDoc->updateViews();
}

void KateView::undoMultiple(int count) {
  if (isReadOnly())
    return;

  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->undo(c, count);
  myDoc->updateViews();
}

void KateView::redoMultiple(int count) {
  if (isReadOnly())
    return;

  VConfig c;
  myViewInternal->getVConfig(c);
  myDoc->redo(c, count);
  myDoc->updateViews();
}

void KateView::undoHistory()
{
  UndoHistory   *undoH;

  undoH = new UndoHistory(this, this, "UndoHistory", true);

  undoH->setCaption(i18n("Undo/Redo History"));

  connect(this,SIGNAL(newUndo()),undoH,SLOT(newUndo()));
  connect(undoH,SIGNAL(undo(int)),this,SLOT(undoMultiple(int)));
  connect(undoH,SIGNAL(redo(int)),this,SLOT(redoMultiple(int)));

  undoH->exec();

  delete undoH;
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

  if (!myDoc->hasMarkedText()) searchFlags &= ~KateView::sfSelected;

  searchDialog = new SearchDialog(this, myDoc->searchForList, myDoc->replaceWithList,
  searchFlags & ~KateView::sfReplace);

  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
   QString str;
   if (myDoc->hasMarkedText())
     str = markedText();

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
    searchFlags = searchDialog->getFlags() | (searchFlags & KateView::sfPrompt);
    initSearch(s, searchFlags);
    searchAgain(s);
  }
  delete searchDialog;
}

void KateView::replace() {
  SearchDialog *searchDialog;

  if (isReadOnly()) return;

  if (!myDoc->hasMarkedText()) searchFlags &= ~KateView::sfSelected;
  searchDialog = new SearchDialog(this, myDoc->searchForList, myDoc->replaceWithList,
    searchFlags | KateView::sfReplace);

  // If the user has marked some text we use that otherwise
  // use the word under the cursor.
   QString str;
   if (myDoc->hasMarkedText())
     str = markedText();

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
    searchFlags = searchDialog->getFlags();
    initSearch(s, searchFlags);
    replaceAgain();
  }
  delete searchDialog;
}

//usleep(50000);
//XSync(qt_xdisplay(),true);
//kapp->syncX();
//debug("xpending %d",XPending(qt_xdisplay()));
//    myViewInternal->tagAll();
//    searchAgain();

void KateView::findAgain() {
  initSearch(s, searchFlags | KateView::sfFromCursor |KateView::sfPrompt | KateView::sfAgain);
  if (s.flags & KateView::sfReplace) replaceAgain(); else searchAgain(s);
}

void KateView::gotoLine() {
  GotoLineDialog *dlg;
  PointStruc cursor;

  dlg = new GotoLineDialog(this, myViewInternal->cursor.y + 1, myDoc->numLines());
//  dlg = new GotoLineDialog(myViewInternal->cursor.y + 1, this);

  if (dlg->exec() == QDialog::Accepted) {
//    myDoc->recordReset();
    cursor.x = 0;
    cursor.y = dlg->getLine() - 1;
    myViewInternal->updateCursor(cursor);
    myViewInternal->center();
    myViewInternal->updateView(KateView::ufUpdateOnScroll);
    myDoc->updateViews(this); //uptade all other views except this one
  }
  delete dlg;
}


void KateView::initSearch(SConfig &s, int flags) {

  s.flags = flags;
  s.setPattern(myDoc->searchForList.first());

  if (s.flags & KateView::sfFromCursor) {
    // If we are continuing a backward search, make sure we do not get stuck
    // at an existing match.
    if ((s.flags & KateView::sfAgain) &&
      (s.flags & KateView::sfBackward) &&
      (s.cursor.x == myViewInternal->cursor.x) &&
      (s.cursor.y == myViewInternal->cursor.y)) {
      s.cursor.x--;
    }
    else {
      s.cursor = myViewInternal->cursor;
    }
  } else {
    if (!(s.flags & KateView::sfBackward)) {
      s.cursor.x = 0;
      s.cursor.y = 0;
    } else {
      s.cursor.x = -1;
      s.cursor.y = myDoc->lastLine();
    }
    s.flags |= KateView::sfFinished;
  }
  if (!(s.flags & KateView::sfBackward)) {
    if (!(s.cursor.x || s.cursor.y))
      s.flags |= KateView::sfFinished;
  }
  s.startCursor = s.cursor;
}

void KateView::continueSearch(SConfig &s) {

  if (!(s.flags & KateView::sfBackward)) {
    s.cursor.x = 0;
    s.cursor.y = 0;
  } else {
    s.cursor.x = -1;
    s.cursor.y = myDoc->lastLine();
  }
  s.flags |= KateView::sfFinished;
  s.flags &= ~KateView::sfAgain;
}

void KateView::searchAgain(SConfig &s) {
  int query;
  PointStruc cursor;
  QString str;

  QString searchFor = myDoc->searchForList.first();

  if( searchFor.isEmpty() ) {
    find();
    return;
  }

  do {
    query = KMessageBox::Cancel;
    if (myDoc->doSearch(s,searchFor)) {
      cursor = s.cursor;
      if (!(s.flags & KateView::sfBackward))
        s.cursor.x += s.matchedLength;
      myViewInternal->updateCursor(s.cursor); //does deselectAll()
      exposeFound(cursor,s.matchedLength,(s.flags & KateView::sfAgain) ? 0 : KateView::ufUpdateOnScroll,false);
    } else {
      if (!(s.flags & KateView::sfFinished)) {
        // ask for continue
        if (!(s.flags & KateView::sfBackward)) {
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

//void qt_enter_modal(QWidget *);


void KateView::replaceAgain() {
  if (isReadOnly())
    return;

  replaces = 0;
  if (s.flags & KateView::sfPrompt) {
    doReplaceAction(-1);
  } else {
    doReplaceAction(KateView::srAll);
  }
}

void KateView::doReplaceAction(int result, bool found) {
  int rlen;
  PointStruc cursor;
  bool started;

  QString searchFor = myDoc->searchForList.first();
  QString replaceWith = myDoc->replaceWithList.first();
  rlen = replaceWith.length();

  switch (result) {
    case KateView::srYes: //yes
      myDoc->recordStart(this, s.cursor, configFlags,
        KateActionGroup::ugReplace, true);
      myDoc->recordReplace(s.cursor, s.matchedLength, replaceWith);
      replaces++;
      if (s.cursor.y == s.startCursor.y && s.cursor.x < s.startCursor.x)
        s.startCursor.x += rlen - s.matchedLength;
      if (!(s.flags & KateView::sfBackward)) s.cursor.x += rlen;
      myDoc->recordEnd(this, s.cursor, configFlags | KateView::cfPersistent);
      break;
    case KateView::srNo: //no
      if (!(s.flags & KateView::sfBackward)) s.cursor.x += s.matchedLength;
      break;
    case KateView::srAll: //replace all
      deleteReplacePrompt();
      do {
        started = false;
        while (found || myDoc->doSearch(s,searchFor)) {
          if (!started) {
            found = false;
            myDoc->recordStart(this, s.cursor, configFlags,
              KateActionGroup::ugReplace);
            started = true;
          }
          myDoc->recordReplace(s.cursor, s.matchedLength, replaceWith);
          replaces++;
          if (s.cursor.y == s.startCursor.y && s.cursor.x < s.startCursor.x)
            s.startCursor.x += rlen - s.matchedLength;
          if (!(s.flags & KateView::sfBackward)) s.cursor.x += rlen;
        }
        if (started) myDoc->recordEnd(this, s.cursor,
          configFlags | KateView::cfPersistent);
      } while (!askReplaceEnd());
      return;
    case KateView::srCancel: //cancel
      deleteReplacePrompt();
      return;
    default:
      replacePrompt = 0L;
  }

  do {
    if (myDoc->doSearch(s,searchFor)) {
      //text found: highlight it, show replace prompt if needed and exit
      cursor = s.cursor;
      if (!(s.flags & KateView::sfBackward)) cursor.x += s.matchedLength;
      myViewInternal->updateCursor(cursor); //does deselectAll()
      exposeFound(s.cursor,s.matchedLength,(s.flags & KateView::sfAgain) ? 0 : KateView::ufUpdateOnScroll,true);
      if (replacePrompt == 0L) {
        replacePrompt = new ReplacePrompt(this);
        XSetTransientForHint(qt_xdisplay(),replacePrompt->winId(),topLevelWidget()->winId());
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

void KateView::exposeFound(PointStruc &cursor, int slen, int flags, bool replace) {
  int x1, x2, y1, y2, xPos, yPos;

  myDoc->markFound(cursor,slen);

  TextLine::Ptr textLine = myDoc->getTextLine(cursor.y);
  x1 = myDoc->textWidth(textLine,cursor.x)        -10;
  x2 = myDoc->textWidth(textLine,cursor.x + slen) +20;
  y1 = myDoc->fontHeight*cursor.y                 -10;
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
    yPos = myDoc->fontHeight*cursor.y - height()/3;
  }
  myViewInternal->setPos(xPos, yPos);
  myViewInternal->updateView(flags);// | ufPos,xPos,yPos);
  myDoc->updateViews(this);
//  myDoc->updateViews();
}

void KateView::deleteReplacePrompt() {
  myDoc->setPseudoModal(0L);
}

bool KateView::askReplaceEnd() {
  QString str;
  int query;

  myDoc->updateViews();
  if (s.flags & KateView::sfFinished) {
    // replace finished
    str = i18n("%1 replace(s) made").arg(replaces);
    KMessageBox::information(this, str, i18n("Replace"));
    return true;
  }

  // ask for continue
  if (!(s.flags & KateView::sfBackward)) {
    // forward search
    str = i18n("%1 replace(s) made.\n"
               "End of document reached.\n"
               "Continue from the beginning?").arg(replaces);
    query = KMessageBox::questionYesNo(this, str, i18n("Replace"),
		i18n("Continue"), i18n("Stop"));
  } else {
    // backward search
    str = i18n("%1 replace(s) made.\n"
                "Beginning of document reached.\n"
                "Continue from the end?").arg(replaces);
    query = KMessageBox::questionYesNo(this, str, i18n("Replace"),
                i18n("Continue"), i18n("Stop"));
  }
  replaces = 0;
  continueSearch(s);
  return (query == KMessageBox::No);
}

void KateView::replaceSlot() {
  doReplaceAction(replacePrompt->result(),true);
}

void KateView::installPopup(QPopupMenu *rmb_Menu)
{
  rmbMenu = rmb_Menu;

  updateBookmarks();
}


void KateView::setBookmark()
{
  QPopupMenu *popup;
  int z;

  popup = new QPopupMenu(0L);

  for (z = 0; z < 9; z++) {
    if ((z < (int) bookmarks.count())&&(bookmarks.at(z)->cursor.y != -1))
      popup->insertItem(i18n("&%1 - Line %2").arg(z+1)
                        .arg(KGlobal::locale()->formatNumber(bookmarks.at(z)->cursor.y + 1, 0)),z);
    else
      popup->insertItem(i18n("&%1").arg(z+1),z);
  }
  if ((z < (int) bookmarks.count())&&(bookmarks.at(z)->cursor.y != -1))
     popup->insertItem(i18n("1&0 - Line %1")
                        .arg(KGlobal::locale()->formatNumber(bookmarks.at(z)->cursor.y + 1, 0)),z);
  else
     popup->insertItem(i18n("&10"),z);

  popup->move(mapToGlobal(QPoint((width() - 41/*popup->width()*/)/2,
    (height() - 211/*popup->height()*/)/2)));

  z = popup->exec();
//  debug("map %d %d",popup->width(),popup->height());
  delete popup;
  if (z >= 0) {
    setBookmark(z);
  }
}

void KateView::addBookmark()
{
  int z;

  for (z = 0; z < (int) bookmarks.count(); z++) {
    if (bookmarks.at(z)->cursor.y == -1) break;
  }
  setBookmark(z);
}

void KateView::clearBookmarks()
{
  bookmarks.clear();
  updateBookmarks();
}

void KateView::setBookmark(int n)
{
  KWBookmark *b;

  if (n >= 10) return;
  while ((int) bookmarks.count() <= n) bookmarks.append(new KWBookmark());
  b = bookmarks.at(n);
  b->xPos = myViewInternal->xPos;
  b->yPos = myViewInternal->yPos;
  b->cursor = myViewInternal->cursor;

  updateBookmarks();
}

void KateView::slotGotoBookmark()
{
    QCString nam = sender()->name();
    gotoBookmark( nam.toInt() );
}

void KateView::gotoBookmark(int n)
{
  KWBookmark *b;

  if (n-666 < 0 || n-666 >= (int) bookmarks.count()) return;
  b = bookmarks.at(n-666);
  if (b->cursor.y == -1) return;
//  myDoc->recordReset();
  myViewInternal->updateCursor(b->cursor);
  myViewInternal->setPos(b->xPos, b->yPos);
//  myViewInternal->updateView(ufPos, b->xPos, b->yPos);
//  myDoc->updateViews(myViewInternal); //uptade all other views except this one
  myDoc->updateViews();
}


void KateView::updateBookmarks()
{
  KWBookmark *b;
  int bookCount=0;
  int keys[] = { Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9, Key_0 };

  unplugActionList( "bookmarks" );

  bookmarkActionList.setAutoDelete( true );
  bookmarkActionList.clear();

  for (int z = 0; z < (int) bookmarks.count(); z++) {
    b = bookmarks.at(z);
    if (b->cursor.y >= 0) {
      ++bookCount;

      QCString nam;
      nam.setNum( z + 666 );

      KAction *act = new KAction( i18n("Line: %1").arg(KGlobal::locale()->formatNumber(b->cursor.y + 1, 0) ),
                                  ALT+keys[z], this, SLOT( slotGotoBookmark() ), 0, nam );

      bookmarkActionList.append( act );
    }
  }

  plugActionList( "bookmarks", bookmarkActionList );

  emit(bookClearChanged(bookCount>0));
  emit(bookAddChanged(bookCount<10));
}


void KateView::readConfig(KConfig *config)
{
  searchFlags = config->readNumEntry("SearchFlags", KateView::sfPrompt);
  configFlags = config->readNumEntry("ConfigFlags", configFlags) & ~KateView::cfMark;
  wrapAt = config->readNumEntry("WrapAt", wrapAt);
}

void KateView::writeConfig(KConfig *config) {

  config->writeEntry("SearchFlags",searchFlags);
  config->writeEntry("ConfigFlags",configFlags);
  config->writeEntry("WrapAt",wrapAt);
}

void KateView::readSessionConfig(KConfig *config) {
  PointStruc cursor;
  int count, z;
  char s1[16];
  QString s2;
  KWBookmark *b;

  readConfig(config);

  myViewInternal->xPos = config->readNumEntry("XPos");
  myViewInternal->yPos = config->readNumEntry("YPos");
  cursor.x = config->readNumEntry("CursorX");
  cursor.y = config->readNumEntry("CursorY");
  myViewInternal->updateCursor(cursor);

  count = config->readNumEntry("Bookmarks");
  for (z = 0; z < count; z++) {
    b = new KWBookmark();
    bookmarks.append(b);
    sprintf(s1,"Bookmark%d",z+1);
    s2 = config->readEntry(s1);
    if (!s2.isEmpty()) {
      sscanf(s2.ascii(),"%d,%d,%d,%d",&b->xPos,&b->yPos,&b->cursor.x,&b->cursor.y);
    }
  }
}

void KateView::writeSessionConfig(KConfig *config) {
  int z;
  char s1[16];
  char s2[64];
  KWBookmark *b;

  writeConfig(config);

  config->writeEntry("XPos",myViewInternal->xPos);
  config->writeEntry("YPos",myViewInternal->yPos);
  config->writeEntry("CursorX",myViewInternal->cursor.x);
  config->writeEntry("CursorY",myViewInternal->cursor.y);

  config->writeEntry("Bookmarks",bookmarks.count());
  for (z = 0; z < (int) bookmarks.count(); z++) {
    b = bookmarks.at(z);
    if (b->cursor.y != -1) {
      sprintf(s1,"Bookmark%d",z+1);
      sprintf(s2,"%d,%d,%d,%d",b->xPos,b->yPos,b->cursor.x,b->cursor.y);
      config->writeEntry(s1,s2);
    }
  }
}

void KateView::hlDlg() {
  HighlightDialog *dlg;
  HlManager *hlManager;
  HlDataList hlDataList;
  ItemStyleList defaultStyleList;
  ItemFont defaultFont;

  hlManager = myDoc->hlManager;

  defaultStyleList.setAutoDelete(true);
  hlManager->getDefaults(defaultStyleList,defaultFont);

  hlDataList.setAutoDelete(true);
  //this gets the data from the KConfig object
  hlManager->getHlDataList(hlDataList);

  dlg = new HighlightDialog(hlManager, &defaultStyleList, &defaultFont, &hlDataList,
    myDoc->highlightNum(), this);
//  dlg->hlChanged(myDoc->highlightNum());
  if (dlg->exec() == QDialog::Accepted) {
    //this stores the data into the KConfig object
    hlManager->setHlDataList(hlDataList);
    hlManager->setDefaults(defaultStyleList,defaultFont);
  }
  delete dlg;
}

int KateView::getHl() {
  return myDoc->highlightNum();
}

void KateView::setHl(int n) {
  myDoc->setHighlight(n);
  myDoc->updateViews();
}

int KateView::getEol() {
  return myDoc->eolMode;
}

void KateView::setEol(int eol) {
  if (isReadOnly())
    return;

  myDoc->eolMode = eol;
  myDoc->setModified(true);
}



void KateView::paintEvent(QPaintEvent *e) {
  int x, y;

  QRect updateR = e->rect();                    // update rectangle
//  debug("Update rect = ( %i, %i, %i, %i )",
//    updateR.x(),updateR.y(), updateR.width(), updateR.height() );

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



//  Spellchecking methods

void KateView::spellcheck()
{
  if (isReadOnly())
    return;

  kspell.kspell= new KSpell (this, "KateView: Spellcheck", this,
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

void KateView::spellcheck2(KSpell *)
{
  myDoc->setReadOnly (TRUE);

  // this is a hack, setPseudoModal() has been hacked to recognize 0x01
  // as special (never tries to delete it)
  // this should either get improved (with a #define or something),
  // or kspell should provide access to the spell widget.
  myDoc->setPseudoModal((QWidget*)0x01);

  kspell.spell_tmptext = text();


  kspell.kspellon = TRUE;
  kspell.kspellMispellCount = 0;
  kspell.kspellReplaceCount = 0;
  kspell.kspellPristine = ! myDoc->isModified();

  kspell.kspell->setProgressResolution (1);

  kspell.kspell->check(kspell.spell_tmptext);
}

void KateView::misspelling (QString origword, QStringList *, unsigned pos)
{
  int line;
  unsigned int cnt;

  // Find pos  -- CHANGEME: store the last found pos's cursor
  //   and do these searched relative to that to
  //   (significantly) increase the speed of the spellcheck

  for (cnt = 0, line = 0 ; line <= myDoc->lastLine() && cnt <= pos ; line++)
    cnt += myDoc->getTextLine(line)->length()+1;

  // Highlight the mispelled word
  PointStruc cursor;
  line--;
  cursor.x = pos - (cnt - myDoc->getTextLine(line)->length()) + 1;
  cursor.y = line;
//  deselectAll(); // shouldn't the spell check be allowed within selected text?
  kspell.kspellMispellCount++;
  myViewInternal->updateCursor(cursor); //this does deselectAll() if no persistent selections
  myDoc->markFound(cursor,origword.length());
  myDoc->updateViews();
}

void KateView::corrected (QString originalword, QString newword, unsigned pos)
{
  //we'll reselect the original word in case the user has played with
  //the selection

  int line;
  unsigned int cnt=0;

  if(newword != originalword)
    {

      // Find pos
      for (line = 0 ; line <= myDoc->lastLine() && cnt <= pos ; line++)
        cnt += myDoc->getTextLine(line)->length() + 1;

      // Highlight the mispelled word
      PointStruc cursor;
      line--;
      cursor.x = pos - (cnt-myDoc->getTextLine(line)->length()) + 1;
      cursor.y = line;
      myViewInternal->updateCursor(cursor);
      myDoc->markFound(cursor, newword.length());

      myDoc->recordStart(this, cursor, configFlags,
        KateActionGroup::ugSpell, true, kspell.kspellReplaceCount > 0);
      myDoc->recordReplace(cursor, originalword.length(), newword);
      myDoc->recordEnd(this, cursor, configFlags | KateView::cfGroupUndo);

      kspell.kspellReplaceCount++;
    }

}

void KateView::spellResult (const QString &)
{
  deselectAll(); //!!! this should not be done with persistent selections

//  if (kspellReplaceCount) myDoc->recordReset();

  // we know if the check was cancelled
  // we can safely use the undo mechanism to backout changes
  // in case of a cancel, because we force the entire spell check
  // into one group (record)
  if (kspell.kspell->dlgResult() == 0) {
    if (kspell.kspellReplaceCount) {
      // backout the spell check
      VConfig c;
      myViewInternal->getVConfig(c);
      myDoc->undo(c);
      // clear the redo list, so the cancelled spell check can't be redo'ed <- say that word ;-)
      myDoc->clearRedo();
      // make sure the modified flag is turned back off
      // if we started with a clean buffer
      if (kspell.kspellPristine)
        myDoc->setModified(false);
    }
  }

  myDoc->setPseudoModal(0L);
  myDoc->setReadOnly (FALSE);

  // if we marked up the text, clear it now
  if (kspell.kspellMispellCount)
    myDoc->unmarkFound();

  myDoc->updateViews();

  kspell.kspell->cleanUp();
}

void KateView::spellCleanDone ()
{
  KSpell::spellStatus status = kspell.kspell->status();
  kspell.spell_tmptext = "";
  delete kspell.kspell;

  kspell.kspell = 0;
  kspell.kspellon = FALSE;

  if (status == KSpell::Error)
  {
     KMessageBox::sorry(this, i18n("ISpell could not be started.\n"
     "Please make sure you have ISpell properly configured and in your PATH."));
  }
  else if (status == KSpell::Crashed)
  {
     myDoc->setPseudoModal(0L);
     myDoc->setReadOnly (FALSE);

     // if we marked up the text, clear it now
     if (kspell.kspellMispellCount)
        myDoc->unmarkFound();

     myDoc->updateViews();
     KMessageBox::sorry(this, i18n("ISpell seems to have crashed."));
  }
  else
  {
     emit spellcheck_done();
  }
}

void KateView::dropEventPassEmited (QDropEvent* e)
{
  emit dropEventPass(e);
}

void KateView::printDlg ()
{
  if ( printer->setup( this ) )
  {
    QPainter paint( printer );

    int y = 0;


    QPaintDeviceMetrics pdm( printer );
    int max = pdm.height();


     QFont font( "Helvetica", 8 );
     paint.setFont( font );
     QFontMetrics fm = paint.fontMetrics();

    int lineCount = 0;
    while (  lineCount <= myDoc->lastLine()  )
    {



       if (y+fm.ascent()+fm.descent() >= max )
      {
        printer->newPage();
        y=0;
      }

     y += fm.ascent();
     paint.drawText( 10, y, myDoc-> textLine(lineCount) );
     y += fm.descent();





      lineCount++;
    }
    }
}

// Applies a new pattern to the search context.
void SConfig::setPattern(QString &newPattern) {
  bool regExp = (flags & KateView::sfRegularExpression);

  m_pattern = newPattern;
  if (regExp) {
    m_regExp.setCaseSensitive(flags & KateView::sfCaseSensitive);
    m_regExp.setPattern(m_pattern);
  }
}

// Applies the search context to the given string, and returns whether a match was found. If one is,
// the length of the string matched is also returned.
int SConfig::search(QString &text, int index) {
  bool regExp = (flags & KateView::sfRegularExpression);
  bool caseSensitive = (flags & KateView::sfCaseSensitive);

  if (flags & KateView::sfBackward) {
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
      m_regExp.match(text, index, &matchedLength, false);
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
  if ( (event->type() == 8) )
    emit gotFocus (this);

  if ( (event->type() == 6) )
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

void KateView::searchAgain (bool back)
{
  bool b= (searchFlags & sfBackward) > 0;
  initSearch(s, (searchFlags & ((b==back)?~sfBackward:~0))  // clear flag for forward searching
                | sfFromCursor | sfPrompt | sfAgain | ((b!=back)?sfBackward:0) );
  if (s.flags & sfReplace)
    replaceAgain();
  else
    KateView::searchAgain(s);
}

void KateView::slotEditCommand ()
{
  bool ok;
  QString cmd = KLineEditDlg::getText("Editing Command", "", &ok, this);

  if (ok)
    myDoc->cmd()->execCmd (cmd, currentLine(), currentColumn(), this);
}

KateBrowserExtension::KateBrowserExtension( KateDocument *doc )
: KParts::BrowserExtension( doc, "katepartbrowserextension" )
{
  m_doc = doc;
  connect( m_doc, SIGNAL( selectionChanged() ), this, SLOT( slotSelectionChanged() ) );
}

void KateBrowserExtension::copy()
{
  m_doc->copy( 0 );
}

void KateBrowserExtension::slotSelectionChanged()
{
  emit enableAction( "copy", m_doc->hasMarkedText() );
}
