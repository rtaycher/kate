/***************************************************************************
                          kantdocument.cpp  -  description
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


#include "kantview.h"
#include "kantview.moc"

#include "../document/kantdocument.h"

#include <kaction.h>

#include <qfocusdata.h>
#include <kdebug.h>
#include <kapp.h>

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
#include <qprinter.h>
#include <qprintdialog.h>
#include <qpaintdevicemetrics.h>
#include <qdropsite.h>
#include <qdragobject.h>
#include <qiodevice.h>
#include <qbuffer.h>
#include <qtextcodec.h>

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

#include <X11/Xlib.h> //used to have XSetTransientForHint()

#include "../document/kanttextline.h"
#include "kantviewdialog.h"
#include "kantundohistory.h"

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


KantViewInternal::KantViewInternal(KantView *write, KantDocument *doc, bool HandleOwnDND)
  : QWidget(write) {

  kWrite = write;
  kWriteDoc = doc;

  QWidget::setCursor(ibeamCursor);
  setBackgroundMode(NoBackground);
  KCursor::setAutoHideCursor( this, true );

  setFocusPolicy(StrongFocus);
  move(2,2);

  xScroll = new QScrollBar(QScrollBar::Horizontal,write);
  yScroll = new QScrollBar(QScrollBar::Vertical,write);
  connect(xScroll,SIGNAL(valueChanged(int)),SLOT(changeXPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),SLOT(changeYPos(int)));
  connect(yScroll,SIGNAL(valueChanged(int)),kWrite,SIGNAL(scrollValueChanged(int)));

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

KantViewInternal::~KantViewInternal() {
  delete [] lineRanges;
  releaseBuffer(this);
}


void KantViewInternal::doCursorCommand(VConfig &c, int cmdNum) {

  switch (cmdNum) {
    case KantView::cmLeft:
      cursorLeft(c);
      break;
    case KantView::cmRight:
      cursorRight(c);
      break;
    case KantView::cmWordLeft:
      wordLeft(c);
      break;
    case KantView::cmWordRight:
      wordRight(c);
      break;
    case KantView::cmHome:
      home(c);
      break;
    case KantView::cmEnd:
      end(c);
      break;
    case KantView::cmUp:
      cursorUp(c);
      break;
    case KantView::cmDown:
      cursorDown(c);
      break;
    case KantView::cmScrollUp:
      scrollUp(c);
      break;
    case KantView::cmScrollDown:
      scrollDown(c);
      break;
    case KantView::cmTopOfView:
      topOfView(c);
      break;
    case KantView::cmBottomOfView:
      bottomOfView(c);
      break;
    case KantView::cmPageUp:
      pageUp(c);
      break;
    case KantView::cmPageDown:
      pageDown(c);
      break;
    case KantView::cmTop:
      top_home(c);
      break;
    case KantView::cmBottom:
      bottom_end(c);
      break;
  }
}

void KantViewInternal::doEditCommand(VConfig &c, int cmdNum) {

  switch (cmdNum) {
    case KantView::cmCopy:
      kWriteDoc->copy(c.flags);
      return;
    case KantView::cmSelectAll:
      kWriteDoc->selectAll();
      return;
    case KantView::cmDeselectAll:
      kWriteDoc->deselectAll();
      return;
    case KantView::cmInvertSelection:
      kWriteDoc->invertSelection();
      return;
  }
  if (kWrite->isReadOnly()) return;
  switch (cmdNum) {
    case KantView::cmReturn:
      if (c.flags & KantView::cfDelOnInput) kWriteDoc->delMarkedText(c);
      kWriteDoc->newLine(c);
      //emit returnPressed();
      //e->ignore();
      return;
    case KantView::cmDelete:
      if ((c.flags & KantView::cfDelOnInput) && kWriteDoc->hasMarkedText())
        kWriteDoc->delMarkedText(c);
      else kWriteDoc->del(c);
      return;
    case KantView::cmBackspace:
      if ((c.flags & KantView::cfDelOnInput) && kWriteDoc->hasMarkedText())
        kWriteDoc->delMarkedText(c);
      else kWriteDoc->backspace(c);
      return;
    case KantView::cmKillLine:
      kWriteDoc->killLine(c);
      return;
    case KantView::cmCut:
      kWriteDoc->cut(c);
      return;
    case KantView::cmPaste:
      if (c.flags & KantView::cfDelOnInput) kWriteDoc->delMarkedText(c);
      kWriteDoc->paste(c);
      return;
    case KantView::cmUndo:
      kWriteDoc->undo(c);
      return;
    case KantView::cmRedo:
      kWriteDoc->redo(c);
      return;
    case KantView::cmIndent:
      kWriteDoc->indent(c);
      return;
    case KantView::cmUnindent:
      kWriteDoc->unIndent(c);
      return;
    case KantView::cmCleanIndent:
      kWriteDoc->cleanIndent(c);
      return;
    case KantView::cmComment:
      kWriteDoc->comment(c);
      return;
    case KantView::cmUncomment:
      kWriteDoc->unComment(c);
      return;
  }
}



void KantViewInternal::cursorLeft(VConfig &c) {

  cursor.x--;
  if (c.flags & KantView::cfWrapCursor && cursor.x < 0 && cursor.y > 0) {
    cursor.y--;
    cursor.x = kWriteDoc->textLength(cursor.y);
  }
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  changeState(c);
}

void KantViewInternal::cursorRight(VConfig &c) {

  if (c.flags & KantView::cfWrapCursor) {
    if (cursor.x >= kWriteDoc->textLength(cursor.y)) {
      if (cursor.y == kWriteDoc->lastLine()) return;
      cursor.y++;
      cursor.x = -1;
    }
  }
  cursor.x++;
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  changeState(c);
}

void KantViewInternal::wordLeft(VConfig &c) {
  Highlight *highlight;

  highlight = kWriteDoc->highlight();
  TextLine::Ptr textLine = kWriteDoc->getTextLine(cursor.y);

  if (cursor.x > 0) {
    do {
      cursor.x--;
    } while (cursor.x > 0 && !highlight->isInWord(textLine->getChar(cursor.x)));
    while (cursor.x > 0 && highlight->isInWord(textLine->getChar(cursor.x -1)))
      cursor.x--;
  } else {
    if (cursor.y > 0) {
      cursor.y--;
      textLine = kWriteDoc->getTextLine(cursor.y);
      cursor.x = textLine->length();
    }
  }

/*
  do {
    if (cursor.x <= 0) {
      if (cursor.y > 0) {
        cursor.y--;
        textLine = kWriteDoc->textLine(cursor.y);
        cursor.x = textLine->length() -1;
      } else break;
    } else cursor.x--;
  } while (cursor.x < 0 || !highlight->isInWord(textLine->getChar(cursor.x)));
  while (cursor.x > 0 && highlight->isInWord(textLine->getChar(cursor.x -1)))
    cursor.x--;
*/
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  changeState(c);
}

void KantViewInternal::wordRight(VConfig &c) {
  Highlight *highlight;
  int len;

  highlight = kWriteDoc->highlight();
  TextLine::Ptr textLine = kWriteDoc->getTextLine(cursor.y);
  len = textLine->length();

  if (cursor.x < len) {
    do {
      cursor.x++;
    } while (cursor.x < len && highlight->isInWord(textLine->getChar(cursor.x)));
    while (cursor.x < len && !highlight->isInWord(textLine->getChar(cursor.x)))
      cursor.x++;
  } else {
    if (cursor.y < kWriteDoc->lastLine()) {
      cursor.y++;
      textLine = kWriteDoc->getTextLine(cursor.y);
      cursor.x = 0;
    }
  }
/*
  while (cursor.x < len && highlight->isInWord(textLine->getChar(cursor.x)))
    cursor.x++;
  do {
    if (cursor.x >= len) {
      if (cursor.y < kWriteDoc->lastLine()) {
        cursor.y++;
        textLine = kWriteDoc->textLine(cursor.y);
        len = textLine->length();
        cursor.x = 0;
      } else break;
    } else cursor.x++;
  } while (cursor.x >= len || !highlight->isInWord(textLine->getChar(cursor.x)));
*/
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  changeState(c);
}

void KantViewInternal::home(VConfig &c) {
  int lc;

  lc = (c.flags & KantView::cfSmartHome) ? kWriteDoc->getTextLine(cursor.y)->firstChar() : 0;
  if (lc <= 0 || cursor.x == lc) {
    cursor.x = 0;
    cOldXPos = cXPos = 0;
  } else {
    cursor.x = lc;
    cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  }

  changeState(c);
}

void KantViewInternal::end(VConfig &c) {
/*
  TextLine::Ptr textLine = kWriteDoc->textLine(cursor.y);
  if (c.flags & KantView::cfRemoveSpaces) { // ignore trailing spaces
    cursor.x = textLine->lastChar();
  } else {
    cursor.x = textLine->length();
  }*/
  cursor.x = kWriteDoc->textLength(cursor.y);
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  changeState(c);
}


void KantViewInternal::cursorUp(VConfig &c) {

  cursor.y--;
  cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
}


void KantViewInternal::cursorDown(VConfig &c) {
  int x;

  if (cursor.y == kWriteDoc->lastLine()) {
    x = kWriteDoc->textLength(cursor.y);
    if (cursor.x >= x) return;
    cursor.x = x;
    cXPos = kWriteDoc->textWidth(cursor);
  } else {
    cursor.y++;
    cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor, cursor, cOldXPos);
  }
  changeState(c);
}

void KantViewInternal::scrollUp(VConfig &c) {

  if (! yPos) return;

  newYPos = yPos - kWriteDoc->fontHeight;
  if (cursor.y == (yPos + height())/kWriteDoc->fontHeight -1) {
    cursor.y--;
    cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor,cursor,cOldXPos);

    changeState(c);
  }
}

void KantViewInternal::scrollDown(VConfig &c) {

  if (endLine >= kWriteDoc->lastLine()) return;

  newYPos = yPos + kWriteDoc->fontHeight;
  if (cursor.y == (yPos + kWriteDoc->fontHeight -1)/kWriteDoc->fontHeight) {
    cursor.y++;
    cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor,cursor,cOldXPos);
    changeState(c);
  }
}

void KantViewInternal::topOfView(VConfig &c) {

  cursor.y = (yPos + kWriteDoc->fontHeight -1)/kWriteDoc->fontHeight;
  cursor.x = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KantViewInternal::bottomOfView(VConfig &c) {

  cursor.y = (yPos + height())/kWriteDoc->fontHeight -1;
  if (cursor.y < 0) cursor.y = 0;
  if (cursor.y > kWriteDoc->lastLine()) cursor.y = kWriteDoc->lastLine();
  cursor.x = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

void KantViewInternal::pageUp(VConfig &c) {
  int lines = (endLine - startLine - 1);

  if (lines <= 0) lines = 1;

  if (!(c.flags & KantView::cfPageUDMovesCursor) && yPos > 0) {
    newYPos = yPos - lines * kWriteDoc->fontHeight;
    if (newYPos < 0) newYPos = 0;
  }
  cursor.y -= lines;
  cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor, cursor, cOldXPos);
  changeState(c);
//  cursorPageUp(c);
}

void KantViewInternal::pageDown(VConfig &c) {

  int lines = (endLine - startLine - 1);

  if (!(c.flags & KantView::cfPageUDMovesCursor) && endLine < kWriteDoc->lastLine()) {
    if (lines < kWriteDoc->lastLine() - endLine)
      newYPos = yPos + lines * kWriteDoc->fontHeight;
    else
      newYPos = yPos + (kWriteDoc->lastLine() - endLine) * kWriteDoc->fontHeight;
  }
  cursor.y += lines;
  cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor,cursor,cOldXPos);
  changeState(c);
//  cursorPageDown(c);
}

// go to the top, same X position
void KantViewInternal::top(VConfig &c) {

//  cursor.x = 0;
  cursor.y = 0;
  cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom, same X position
void KantViewInternal::bottom(VConfig &c) {

//  cursor.x = 0;
  cursor.y = kWriteDoc->lastLine();
  cXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor,cursor,cOldXPos);
//  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the top left corner
void KantViewInternal::top_home(VConfig &c)
{
  cursor.y = 0;
  cursor.x = 0;
  cOldXPos = cXPos = 0;
  changeState(c);
}

// go to the bottom right corner
void KantViewInternal::bottom_end(VConfig &c) {

  cursor.y = kWriteDoc->lastLine();
  cursor.x = kWriteDoc->textLength(cursor.y);
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
  changeState(c);
}


void KantViewInternal::changeXPos(int p) {
  int dx;

  dx = xPos - p;
  xPos = p;
  if (QABS(dx) < width()) scroll(dx, 0); else update();
}

void KantViewInternal::changeYPos(int p) {
  int dy;

  dy = yPos - p;
  yPos = p;
  clearDirtyCache(height());
  if (QABS(dy) < height()) scroll(0, dy); else update();
}


void KantViewInternal::getVConfig(VConfig &c) {

  c.view = kWrite;
  c.cursor = cursor;
  c.cXPos = cXPos;
  c.flags = kWrite->configFlags;
  c.wrapAt = kWrite->wrapAt;
}

void KantViewInternal::changeState(VConfig &c) {
  /*
   * we need to be sure to kill the selection on an attempted cursor
   * movement even if the cursor doesn't physically move,
   * but we need to be careful not to do some other things in this case,
   * like we don't want to expose the cursor
   */

//  if (cursor.x == c.cursor.x && cursor.y == c.cursor.y) return;
  bool nullMove = (cursor.x == c.cursor.x && cursor.y == c.cursor.y);

//  if (cursor.y != c.cursor.y || c.flags & KantView::cfMark) kWriteDoc->recordReset();

  if (! nullMove) {
    kWriteDoc->unmarkFound();

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
    kWriteDoc->newBracketMark(cursor, bm);

    // remove trailing spaces when leaving a line
    if (c.flags & KantView::cfRemoveSpaces && cursor.y != c.cursor.y) {
      TextLine::Ptr textLine = kWriteDoc->getTextLine(c.cursor.y);
      int newLen = textLine->lastChar();
      if (newLen != textLine->length()) {
        textLine->truncate(newLen);
        // if some spaces are removed, tag the line as dirty
        kWriteDoc->tagLines(c.cursor.y, c.cursor.y);
      }
    }
  }

  if (c.flags & KantView::cfMark) {
    if (! nullMove)
      kWriteDoc->selectTo(c, cursor, cXPos);
  } else {
    if (!(c.flags & KantView::cfPersistent))
      kWriteDoc->deselectAll();
  }
}

/*
void KantViewInternal::insLine(int line) {
  if (insert) {
    if (start.y < startLine && end.y < startLine) {
      startLine += dy;
      endLine += dy;
      yPos += dy*kWriteDoc->fontHeight;
    } else if (start.y <= endLine || end.y <= endLine) {
      if (dy == 0) {
        if (start.y == cursor.y) cursorOn = false;
        tagLines(start.y,start.y);
      } else tagAll();
    }
  } else {
    if (dy >= 0) tagLines(start.y,end.y); else tagLines(end.y,start.y);
  }
}
*/

void KantViewInternal::insLine(int line) {

  if (line <= cursor.y) {
    cursor.y++;
  }
  if (line < startLine) {
    startLine++;
    endLine++;
    yPos += kWriteDoc->fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }

  //bookmarks
  KWBookmark *b;
  for (b = kWrite->bookmarks.first(); b != 0L; b = kWrite->bookmarks.next()) {
    if (b->cursor.y >= line) {
      b->cursor.y++;
      b->yPos += kWriteDoc->fontHeight;
    }
  }
}

void KantViewInternal::delLine(int line) {

  if (line <= cursor.y && cursor.y > 0) {
    cursor.y--;
  }
  if (line < startLine) {
    startLine--;
    endLine--;
    yPos -= kWriteDoc->fontHeight;
  } else if (line <= endLine) {
    tagAll();
  }

  //bookmarks
  KWBookmark *b;
  for (b = kWrite->bookmarks.first(); b != 0L; b = kWrite->bookmarks.next()) {
    if (b->cursor.y > line) {
      b->cursor.y--;
      b->yPos -= kWriteDoc->fontHeight;
//      if (b->yPos < 0) b->yPos = 0;
    }
  }
}

void KantViewInternal::updateCursor() {
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
}


void KantViewInternal::updateCursor(PointStruc &newCursor) {
  updateCursor(newCursor, kWrite->config());
}

void KantViewInternal::updateCursor(PointStruc &newCursor, int flags) {

  if (!(flags & KantView::cfPersistent)) kWriteDoc->deselectAll();
  kWriteDoc->unmarkFound();

  exposeCursor = true;
  if (cursorOn) {
    tagLines(cursor.y, cursor.y, cXPos -2, cXPos +3);
    cursorOn = false;
  }

  if (bm.sXPos < bm.eXPos) {
    tagLines(bm.cursor.y, bm.cursor.y, bm.sXPos, bm.eXPos);
  }
  kWriteDoc->newBracketMark(newCursor, bm);

  cursor = newCursor;
  cOldXPos = cXPos = kWriteDoc->textWidth(cursor);
}

// init the line dirty cache
void KantViewInternal::clearDirtyCache(int height) {
  int lines, z;

  // calc start and end line of visible part
  startLine = yPos/kWriteDoc->fontHeight;
  endLine = (yPos + height -1)/kWriteDoc->fontHeight;

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

void KantViewInternal::tagLines(int start, int end, int x1, int x2) {
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

void KantViewInternal::tagAll() {
  updateState = 3;
}

void KantViewInternal::setPos(int x, int y) {
  newXPos = x;
  newYPos = y;
}

void KantViewInternal::center() {
  newXPos = 0;
  newYPos = cursor.y*kWriteDoc->fontHeight - height()/2;
  if (newYPos < 0) newYPos = 0;
}

void KantViewInternal::updateView(int flags) {
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
  if (exposeCursor || flags & KantView::ufDocGeometry) {
    emit kWrite->newCurPos();
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

  fontHeight = kWriteDoc->fontHeight;
  cYPos = cursor.y*fontHeight;

  z = 0;
  do {
    w = kWrite->width() - 4;
    h = kWrite->height() - 4;

    xMax = kWriteDoc->textWidth() - w;
    b = (xPos > 0 || xMax > 0);
    if (b) h -= 16;
    yMax = kWriteDoc->textHeight() - h;
    if (yPos > 0 || yMax > 0) {
      w -= 16;
      xMax += 16;
      if (!b && xMax > 0) {
        h -= 16;
        yMax += 16;
      }
    }

    if (!exposeCursor) break;
//    if (flags & KantView::ufNoScroll) break;
/*
    if (flags & KantView::ufCenter) {
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

/*
    if (cXPos < xPos + 4) {
      xPos = cXPos - 8;
      if (xPos < 0) xPos = 0;
    }
    if (cXPos > xPos + w - 4) {
      xPos = cXPos - w +8;
    }

    if (cYPos < yPos) {
      yPos = cYPos;
      if (yPos < 0) yPos = 0;
    }
    if (cYPos > yPos + (h-fontHeight)) {
      yPos = cYPos - (h-fontHeight);
    }
*/
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

  //startLine = yPos / fontHeight;
  //endLine = (yPos + h -1) / fontHeight;
/*
  if (endLine < startLine) {
    endLine = startLine;
    visibleLines = 0;
  } else {
    visibleLines = (h - (h % fontHeight)) / fontHeight;
  }
*/
  if (w != width() || h != height()) {
    clearDirtyCache(h);
    resize(w,h);
  } else {
    dx = oldXPos - xPos;
    dy = oldYPos - yPos;

    b = updateState == 3;
    if (flags & KantView::ufUpdateOnScroll) {
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


void KantViewInternal::paintTextLines(int xPos, int yPos) {
//  int xStart, xEnd;
  int line;//, z;
  int h;
  LineRange *r;

  QPainter paint;
  paint.begin(drawBuffer);

  h = kWriteDoc->fontHeight;
  r = lineRanges;
  for (line = startLine; line <= endLine; line++) {
    if (r->start < r->end) {
//debug("painttextline %d %d %d", line, r->start, r->end);
      kWriteDoc->paintTextLine(paint, line, r->start, r->end, kWrite->configFlags & KantView::cfShowTabs);
      bitBlt(this, r->start - (xPos-2), line*h - yPos, drawBuffer, 0, 0,
        r->end - r->start, h);
    }
    r++;
  }
/*
  xStart = xPos-2;
  xEnd = xStart + width();
  h = kWriteDoc->fontHeight;
  for (z = 0; z < updateState; z++) {
    line = updateLines[z];
    kWriteDoc->paintTextLine(paint,line,xStart,xEnd);
    bitBlt(this,0,line*h - yPos,drawBuffer,0,0,width(),h);
  }*/
  paint.end();
}

void KantViewInternal::paintCursor() {
  int h, y, x;
  static int cx = 0, cy = 0, ch = 0;

  h = kWriteDoc->fontHeight;
  y = h*cursor.y - yPos;
  x = cXPos - (xPos-2);

  QFont f = kWriteDoc->getTextFont(cursor.x, cursor.y);
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
    paint.setPen(kWriteDoc->cursorCol(cursor.x,cursor.y));

    h += y - 1;
    paint.drawLine(x, y, x, h);
    paint.drawLine(x-2, y, x+2, y);
    paint.drawLine(x-2, h, x+2, h);
  } else {
    paint.begin(drawBuffer);
    kWriteDoc->paintTextLine(paint, cursor.y, cXPos - 2, cXPos + 3, kWrite->configFlags & KantView::cfShowTabs);
    bitBlt(this,x - 2,y, drawBuffer, 0, 0, 5, h);
  }
  paint.end();
}

void KantViewInternal::paintBracketMark() {
  int y;

  y = kWriteDoc->fontHeight*(bm.cursor.y +1) - yPos -1;

  QPainter paint;
  paint.begin(this);
  paint.setPen(kWriteDoc->cursorCol(bm.cursor.x, bm.cursor.y));

  paint.drawLine(bm.sXPos - (xPos-2), y, bm.eXPos - (xPos-2) -1, y);
  paint.end();
}

void KantViewInternal::placeCursor(int x, int y, int flags) {
  VConfig c;

  getVConfig(c);
  c.flags |= flags;
  cursor.y = (yPos + y)/kWriteDoc->fontHeight;
  cXPos = cOldXPos = kWriteDoc->textWidth(c.flags & KantView::cfWrapCursor, cursor,xPos-2 + x);
  changeState(c);
}

// convert the given physical coordinates to logical (line/column within the document)
/*
void KantViewInternal::calcLogicalPosition(int &x, int &y) {

  TextLine   line;

  y = (yPos + y)/kWriteDoc->fontHeight;

  line = kWriteDoc->textLine(y);

  x = kWriteDoc->textPos(kWriteDoc->textLine(y), x);
}
*/
// given physical coordinates, report whether the text there is selected
bool KantViewInternal::isTargetSelected(int x, int y) {

  y = (yPos + y) / kWriteDoc->fontHeight;

  TextLine::Ptr line = kWriteDoc->getTextLine(y);
  if (!line)
    return false;

  x = kWriteDoc->textPos(line, x);

  return line->isSelected(x);
}

void KantViewInternal::focusInEvent(QFocusEvent *) {
//  debug("got focus %d",cursorTimer);

  if (!cursorTimer) {
    cursorTimer = startTimer(KApplication::cursorFlashTime() / 2);
    cursorOn = true;
    paintCursor();
  }
}

void KantViewInternal::focusOutEvent(QFocusEvent *) {
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

void KantViewInternal::keyPressEvent(QKeyEvent *e) {
  VConfig c;
//  int ascii;

/*  if (e->state() & AltButton) {
    e->ignore();
    return;
  }*/
//  debug("ascii %i, key %i, state %i",e->ascii(), e->key(), e->state());

  getVConfig(c);
//  ascii = e->ascii();

  if (!kWrite->isReadOnly()) {
    if (c.flags & KantView::cfTabIndents && kWriteDoc->hasMarkedText()) {
      if (e->key() == Qt::Key_Tab) {
        kWriteDoc->indent(c);
        kWriteDoc->updateViews();
        return;
      }
      if (e->key() == Qt::Key_Backtab) {
        kWriteDoc->unIndent(c);
        kWriteDoc->updateViews();
        return;
      }
    }
    if ( !(e->state() & ControlButton ) && kWriteDoc->insertChars(c, e->text())) {
      kWriteDoc->updateViews();
      e->accept();
      return;
    }
  }
  e->ignore();
}

void KantViewInternal::mousePressEvent(QMouseEvent *e) {

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
        flags |= KantView::cfMark;
        if (e->state() & ControlButton) flags |= KantView::cfMark | KantView::cfKeepSelection;
      }
      placeCursor(e->x(), e->y(), flags);
      scrollX = 0;
      scrollY = 0;
      if (!scrollTimer) scrollTimer = startTimer(50);
      kWriteDoc->updateViews();
    }
  }
  if (e->button() == MidButton) {
    placeCursor(e->x(), e->y());
    if (! kWrite->isReadOnly())
      kWrite->paste();
  }
  if (kWrite->rmbMenu && e->button() == RightButton) {
    kWrite->rmbMenu->popup(mapToGlobal(e->pos()));
  }
  kWrite->mousePressEvent(e); // this doesn't do anything, does it?
  // it does :-), we need this for KDevelop, so please don't uncomment it again -Sandy
}

void KantViewInternal::mouseDoubleClickEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    VConfig c;
    getVConfig(c);
    kWriteDoc->selectWord(c.cursor, c.flags);
    kWriteDoc->updateViews();
  }
}

void KantViewInternal::mouseReleaseEvent(QMouseEvent *e) {

  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor(e->x(), e->y(), 0);
      kWriteDoc->updateViews();
    } else if (dragInfo.state == diNone) {
      if (kWrite->config() & KantView::cfMouseAutoCopy) kWrite->copy();
      killTimer(scrollTimer);
      scrollTimer = 0;
    }
    dragInfo.state = diNone;
  }
}

void KantViewInternal::mouseMoveEvent(QMouseEvent *e) {

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
    d = kWriteDoc->fontHeight;
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
    flags = KantView::cfMark;
    if (e->state() & ControlButton) flags |= KantView::cfKeepSelection;
    placeCursor(mouseX, mouseY, flags);
    kWriteDoc->updateViews(/*ufNoScroll*/);
  }
}



void KantViewInternal::wheelEvent( QWheelEvent *e )
{
  if( yScroll->isVisible() == true )
  {
    QApplication::sendEvent( yScroll, e );
  }
}



void KantViewInternal::paintEvent(QPaintEvent *e) {
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

  h = kWriteDoc->fontHeight;
  line = (yPos + updateR.y()) / h;
  y = line*h - yPos;
  yEnd = updateR.y() + updateR.height();

  while (y < yEnd) {
    kWriteDoc->paintTextLine(paint, line, xStart, xEnd, kWrite->configFlags & KantView::cfShowTabs);
//    if (cursorOn && line == cursor.y) paintCursor(paint,cXPos - xStart,h);
    bitBlt(this, updateR.x(), y, drawBuffer, 0, 0, updateR.width(), h);

    line++;
    y += h;
  }
  paint.end();

  if (cursorOn) paintCursor();
  if (bm.eXPos > bm.sXPos) paintBracketMark();
}

void KantViewInternal::resizeEvent(QResizeEvent *) {
//  debug("KantViewInternal::resize");
  resizeBuffer(this, width(), kWriteDoc->fontHeight);
//  update();

}

void KantViewInternal::timerEvent(QTimerEvent *e) {
  if (e->timerId() == cursorTimer) {
    cursorOn = !cursorOn;
    paintCursor();
  }
  if (e->timerId() == scrollTimer && (scrollX | scrollY)) {
    xScroll->setValue(xPos + scrollX);
    yScroll->setValue(yPos + scrollY);

    placeCursor(mouseX, mouseY, KantView::cfMark);
    kWriteDoc->updateViews(/*ufNoScroll*/);
  }
}

/////////////////////////////////////
// Drag and drop handlers
//

// call this to start a drag from this view
void KantViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(kWriteDoc->markedText(0), this);
  if (kWrite->isReadOnly()) {
    dragInfo.dragObject->dragCopy();
  } else {

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   drag() is broken for move operations in Qt - dragCopy() is the only safe way
   to go right now

    if (dragInfo.dragObject->drag()) {
      // the drag has completed and it turned out to be a move operation
      if (! kWriteDoc->ownedView((KantViewInternal*)(QDragObject::target()))) {
        // the target is not me - we need to delete our selection
        VConfig c;
        getVConfig(c);
        kWriteDoc->delMarkedText(c);
        kWriteDoc->updateViews();
      }
    }
*/
    dragInfo.dragObject->dragCopy();

  }
}

void KantViewInternal::dragEnterEvent( QDragEnterEvent *event )
{
  event->accept( (QTextDrag::canDecode(event) && ! kWrite->isReadOnly()) || QUriDrag::canDecode(event) );
}

/*
void KantViewInternal::dragMoveEvent( QDragMoveEvent * )
{
}
void KantViewInternal::dragLeaveEvent( QDragLeaveEvent * )
{
  // we should implement a shadow cursor here
}
*/

void KantViewInternal::dropEvent( QDropEvent *event )
{
  if ( QUriDrag::canDecode(event) ) {
    QStrList  urls;

    if (! HandleURIDrops) {
      // the container should handle this one for us...
      emit dropEventPass(event);
    } else {
      // we can only load one url
      // this is why a smarter container should do this if possible
      if (QUriDrag::decode(event, urls)) {
        char *s;
        s = urls.first();
        if (s) {
          // Load the first file in this window
          if (s == urls.getFirst()) {
            if (kWrite->canDiscard()) kWrite->loadURL(s);
          }
        }
      }
    }
  } else if ( QTextDrag::canDecode(event) && ! kWrite->isReadOnly() ) {

    QString   text;

    if (QTextDrag::decode(event, text)) {
      bool      priv, selected;

      // is the source our own document?
      priv = kWriteDoc->ownedView((KantView*)(event->source()));
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
          kWriteDoc->delMarkedText(c);
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
      kWriteDoc->insert(c, text);
      cursor = c.cursor;

      updateCursor(cursor);
      kWriteDoc->updateViews();
    }
  }
}


KWBookmark::KWBookmark() {
  cursor.y = -1; //mark bookmark as invalid
}

KantView::KantView(QWidget *parent, KantDocument *doc, const char * name, bool HandleOwnDND, bool deleteDoc)
  : KTextEditor::View(doc, parent, name), DCOPObject(name) {
  //setInstance( KantViewFactory::instance() );

  active = false;
  deleteDoc = false;

  kWriteDoc = doc;
  m_singleViewMode = doc->isSingleViewMode();
  myDeleteDoc = deleteDoc;
  kWriteView = new KantViewInternal(this,doc,HandleOwnDND);

  doc->addView( this );

  connect(kWriteView,SIGNAL(dropEventPass(QDropEvent *)),this,SLOT(dropEventPassEmited(QDropEvent *)));

  setXMLFile( "kwriteui.rc" );

  // some defaults
  configFlags = KantView::cfAutoIndent | KantView::cfSpaceIndent | KantView::cfBackspaceIndents
    | KantView::cfTabIndents | KantView::cfKeepIndentProfile
    | KantView::cfReplaceTabs | KantView::cfSpaceIndent | KantView::cfRemoveSpaces
    | KantView::cfDelOnInput | KantView::cfMouseAutoCopy
    | KantView::cfGroupUndo | KantView::cfShowTabs | KantView::cfSmartHome;
  wrapAt = 80;
  searchFlags = 0;
  replacePrompt = 0L;
  rmbMenu = 0L;
  bookmarks.setAutoDelete(true);

  //KSpell initial values
  kspell.kspell = 0;
  kspell.ksc = new KSpellConfig; //default KSpellConfig to start
  kspell.kspellon = FALSE;

  setFocusProxy( kWriteView );
  kWriteView->setFocus();
  resize(parent->width() -4, parent->height() -4);

  m_tempSaveFile = 0;

  printer = new QPrinter();

  kWriteView->installEventFilter( this );
}

KantView::~KantView() {
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

  // KParts has already deleted the doc, if in single-view mode
  if ( !m_singleViewMode )
  {
    kWriteDoc->removeView( this );

    if ( kWriteDoc->isLastView(0) && myDeleteDoc )
      delete kWriteDoc;
  }

  delete kWriteView;

  delete m_tempSaveFile;
  delete printer;
}

void KantView::keyPressEvent( QKeyEvent *ev )
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
               kWriteView->getVConfig(c);
               kWriteDoc->delMarkedText(c);
               kWriteView->update();
            }
            else keyDelete();
            break;
        case Key_Backspace:
            if ( ev->state() & ControlButton )
            {
               VConfig c;
               shiftWordLeft();
               kWriteView->getVConfig(c);
               kWriteDoc->delMarkedText(c);
               kWriteView->update();
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

void KantView::customEvent( QCustomEvent *ev )
{
    if ( KParts::GUIActivateEvent::test( ev ) && static_cast<KParts::GUIActivateEvent *>( ev )->activated() )
    {
        installPopup(static_cast<QPopupMenu *>(factory()->container("rb_popup", this) ) );
        return;
    }

    KTextEditor::View::customEvent( ev );
    return;
}

void KantView::slotOpenRecent( const KURL &url )
{
    if ( canDiscard() )
        loadURL( url );
}

void KantView::setCursorPosition( int line, int col, bool /*mark*/ )
{
  setCursorPositionInternal( line, col );
}

void KantView::getCursorPosition( int *line, int *col )
{
  if ( line )
    *line = currentLine();

  if ( col )
    *col = currentColumn();
}


int KantView::currentLine() {
  return kWriteView->cursor.y;
}

int KantView::currentColumn() {
  return kWriteDoc->currentColumn(kWriteView->cursor);
}

int KantView::currentCharNum() {
  return kWriteView->cursor.x;
}

void KantView::setCursorPositionInternal(int line, int col) {
  PointStruc cursor;

  cursor.x = col;
  cursor.y = line;
  kWriteView->updateCursor(cursor);
  kWriteView->center();
//  kWriteDoc->unmarkFound();
//  kWriteView->updateView(ufPos, 0, line*kWriteDoc->fontHeight - height()/2);
//  kWriteDoc->updateViews(kWriteView); //uptade all other views except this one
  kWriteDoc->updateViews();
}

int KantView::config() {
  int flags;

  flags = configFlags;
  if (kWriteDoc->singleSelection()) flags |= KantView::cfSingleSelection;
  return flags;
}

void KantView::setConfig(int flags) {
  bool updateView;

  // cfSingleSelection is a doc-property
  kWriteDoc->setSingleSelection(flags & KantView::cfSingleSelection);
  flags &= ~KantView::cfSingleSelection;

  if (flags != configFlags) {
    // update the view if visibility of tabs has changed
    updateView = (flags ^ configFlags) & KantView::cfShowTabs;
    configFlags = flags;
    emit newStatus();
    if (updateView) kWriteView->update();
  }
}

int KantView::tabWidth() {
  return kWriteDoc->tabChars;
}

void KantView::setTabWidth(int w) {
  kWriteDoc->setTabWidth(w);
  kWriteDoc->updateViews();
}

int KantView::undoSteps() {
  return kWriteDoc->undoSteps;
}

void KantView::setUndoSteps(int s) {
  kWriteDoc->setUndoSteps(s);
}

/*
bool KantView::isOverwriteMode() {
  return (configFlags & cfOvr);
} */

bool KantView::isReadOnly() {
  return kWriteDoc->readOnly;
}

bool KantView::isModified() {
  return kWriteDoc->modified;
}

void KantView::setReadOnly(bool m) {
  kWriteDoc->setReadOnly(m);
}

void KantView::setModified(bool m) {
  kWriteDoc->setModified(m);
}

bool KantView::isLastView() {
  return kWriteDoc->isLastView(1);
}

KantDocument *KantView::doc() {
  return kWriteDoc;
}

int KantView::undoState() {
  if (isReadOnly())
    return 0;
  else
    return kWriteDoc->undoState;
}

int KantView::nextUndoType() {
  return kWriteDoc->nextUndoType();
}

int KantView::nextRedoType() {
  return kWriteDoc->nextRedoType();
}

void KantView::undoTypeList(QValueList<int> &lst)
{
  kWriteDoc->undoTypeList(lst);
}

void KantView::redoTypeList(QValueList<int> &lst)
{
  kWriteDoc->redoTypeList(lst);
}

const char * KantView::undoTypeName(int type) {
  return KWActionGroup::typeName(type);
}

void KantView::copySettings(KantView *w) {
  configFlags = w->configFlags;
  wrapAt = w->wrapAt;
  searchFlags = w->searchFlags;
}


QColor* KantView::getColors()
{
  return kWriteDoc->colors;
}


void KantView::applyColors()
{
   kWriteDoc->tagAll();
   kWriteDoc->updateViews();
}


bool KantView::isOverwriteMode() const
{
  return ( configFlags & KantView::cfOvr );
}

void KantView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    setConfig( configFlags ^ KantView::cfOvr );
  else
    setConfig( configFlags | KantView::cfOvr );
}

void KantView::toggleInsert() {
  setConfig(configFlags ^ KantView::cfOvr);
}

void KantView::toggleVertical() {
  setConfig(configFlags ^ KantView::cfVerticalSelect);
  emit statusMsg(configFlags & KantView::cfVerticalSelect ? i18n("Vertical Selections On") : i18n("Vertical Selections Off"));
}


int KantView::numLines() {
  return kWriteDoc->numLines();
}

QString KantView::text() {
  return kWriteDoc->text();
}

QString KantView::currentTextLine() {
  TextLine::Ptr textLine = kWriteDoc->getTextLine(kWriteView->cursor.y);
  return QString(textLine->getText(), textLine->length());
}

QString KantView::textLine(int num) {
  TextLine::Ptr textLine = kWriteDoc->getTextLine(num);
  return QString(textLine->getText(), textLine->length());
}

QString KantView::currentWord() {
  return kWriteDoc->getWord(kWriteView->cursor);
}

QString KantView::word(int x, int y) {
  PointStruc cursor;
  cursor.y = (kWriteView->yPos + y)/kWriteDoc->fontHeight;
  if (cursor.y < 0 || cursor.y > kWriteDoc->lastLine()) return QString();
  cursor.x = kWriteDoc->textPos(kWriteDoc->getTextLine(cursor.y), kWriteView->xPos-2 + x);
  return kWriteDoc->getWord(cursor);
}

void KantView::setText(const QString &s) {
  kWriteDoc->setText(s);
  kWriteDoc->updateViews();
}

void KantView::insertText(const QString &s, bool /*mark*/) {
  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->insert(c, s);
  kWriteDoc->updateViews();
}

bool KantView::hasMarkedText() {
  return kWriteDoc->hasMarkedText();
}

QString KantView::markedText() {
  return kWriteDoc->markedText(configFlags);
}

#ifdef NEW_CODE
void KantView::loadFile(const QString &file, QTextCodec *codec, bool insert)
{
  VConfig c;

  if (!insert) {
    kWriteDoc->clear();
    kWriteDoc->loadFile(file, codec);
  } else {
    // TODO: Not yet supported.
#if 0
    kWriteView->getVConfig(c);
    if (c.flags & cfDelOnInput) kWriteDoc->delMarkedText(c);
    kWriteDoc->insertFile(c, dev);
    kWriteDoc->updateViews();
#endif
  }
}
#else
void KantView::loadFile(QIODevice &dev, bool insert) {
  VConfig c;

  if (!insert) {
    kWriteDoc->loadFile(dev);
  } else {
    kWriteView->getVConfig(c);
    if (c.flags & KantView::cfDelOnInput) kWriteDoc->delMarkedText(c);
    kWriteDoc->insertFile(c, dev);
    kWriteDoc->updateViews();
  }
}
#endif

void KantView::writeFile(QIODevice &dev) {
#ifdef NEW_CODE
  // TODO: Not yet implemented.
#else
  kWriteDoc->writeFile(dev);
  kWriteDoc->updateViews();
#endif
}


bool KantView::loadFile(const QString &name, int flags) {
  QFileInfo info(name);
  if (!info.exists()) {
    if (flags & KantView::lfNewFile) return true;
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
  loadFile(name, QTextCodec::codecForLocale(), flags & KantView::lfInsert);
  return true;
#else
  QFile f(name);
  if (f.open(IO_ReadOnly)) {
    loadFile(f,flags & KantView::lfInsert);
    f.close();
    return true;
  }
#endif
  KMessageBox::sorry(this, i18n("An error occured while trying to open this document"));
  return false;
}

bool KantView::writeFile(const QString &name) {

  QFileInfo info(name);
  if(info.exists() && !info.isWritable()) {
    KMessageBox::sorry(this, i18n("You do not have write permission to this file"));
    return false;
  }
#ifdef NEW_CODE
  if (kWriteDoc->writeFile(name, QTextCodec::codecForLocale()))
     return true; // Success
#else
  QFile f(name);
  if (f.open(IO_WriteOnly | IO_Truncate)) {
    writeFile(f);
    f.close();
    return true;//kWriteDoc->setFileName(name);
  }
#endif
  KMessageBox::sorry(this, i18n("An error occured while trying to write this document"));
  return false;
}


void KantView::loadURL(const KURL &url, int flags) {
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
    kWriteDoc->clear();
#endif
    connect( job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotJobReadResult( KIO::Job * ) ) );
    connect( job, SIGNAL( data( KIO::Job *, const QByteArray & ) ), this, SLOT( slotJobData( KIO::Job *, const QByteArray & ) ) );
  }
  else {
    if ( flags & KantView::lfInsert ) {
      if ( loadFile( url.path(), flags ) )
        emit statusMsg( i18n( "Inserted : %1" ).arg( url.fileName() ) );
      else
        emit statusMsg( QString::null );
    } else {
      if (QFileInfo(url.path()).exists()) {
        if ( loadFile( url.path(), flags ) ) {
          kWriteDoc->setURL( url, !(flags & KantView::lfNoAutoHl ) );
          emit statusMsg( i18n( "Read : %1" ).arg( url.fileName() ) );
        } else
          emit statusMsg( QString::null );
      } else {           // don't start whining, just make a new document
        kWriteDoc->clear();
        kWriteDoc->setURL( url, !(flags & KantView::lfNoAutoHl ) );
        kWriteDoc->updateViews();
        emit statusMsg( i18n( "New File : %1" ).arg( url.fileName() ) );
        kWriteDoc->setNewDoc( true ); // File is new, check for overwrite!
      }
    }
  }
}


void KantView::writeURL(const KURL &url, int ) {
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
      kWriteDoc->setModified( false );
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
      kWriteDoc->setModified( false );
      emit statusMsg( i18n( "Wrote %1" ).arg( url.fileName() ) );
      kWriteDoc->setNewDoc( false ); // File is not new anymore
  }
}

void KantView::slotJobReadResult( KIO::Job *job )
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

    if ( flags & KantView::lfInsert )
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

void KantView::loadInternal( const QByteArray &data, const KURL &url, int flags )
{
#ifdef NEW_CODE
    // TODO: Not yet supported.
#else
    QBuffer buff( data );
    buff.open( IO_ReadOnly );
    loadFile( buff, flags );

    QString msg;

    if ( flags & KantView::lfInsert )
        msg = i18n( "Inserted : %1" ).arg( url.fileName() );
    else
    {
        kWriteDoc->setURL( url, !(flags & KantView::lfNoAutoHl ) );
        kWriteDoc->updateLines();
        kWriteDoc->updateViews();

        msg = i18n( "Read : %1" ).arg( url.fileName() );
    }

    emit statusMsg( msg );

    if ( flags & KantView::lfNewFile )
        kWriteDoc->setModified( false );
#endif
}

void KantView::slotJobData( KIO::Job *job, const QByteArray &data )
{
#ifdef NEW_CODE
    kWriteDoc->appendData(data, QTextCodec::codecForLocale());
#else
    QMap<KIO::Job *, NetData>::Iterator it = m_mapNetData.find( job );
    assert( it != m_mapNetData.end() );
    QBuffer buff( (*it).m_data );
    buff.open(IO_WriteOnly | IO_Append );
    buff.writeBlock( data.data(), data.size() );
    buff.close();
#endif
}

bool KantView::canDiscard() {
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

void KantView::newDoc() {

  if (canDiscard()) clear();
}

void KantView::open() {
  KURL url;

  if (!canDiscard()) return;
//  if (kWriteDoc->hasFileName()) s = QFileInfo(kWriteDoc->fileName()).dirPath();
//    else s = QDir::currentDirPath();

  url = KFileDialog::getOpenURL(kWriteDoc->url().url(), QString::null, this);
  if (url.isEmpty()) return;
  loadURL(url);
}

void KantView::insertFile() {
  if (isReadOnly())
    return;

  KURL  url = KFileDialog::getOpenURL(kWriteDoc->url().url(), QString::null, this);
  if (url.isEmpty()) return;
  loadURL(url,KantView::lfInsert);
}

KantView::fileResult KantView::save() {
  int query = KMessageBox::Yes;
  if (isModified()) {
    if (!kWriteDoc->url().fileName().isEmpty() && ! isReadOnly()) {
      // If document is new but has a name, check if saving it would
      // overwrite a file that has been created since the new doc
      // was created:
      if( kWriteDoc->isNewDoc() )
      {
        query = checkOverwrite( kWriteDoc->url() );
        if( query == KMessageBox::Cancel )
          return CANCEL;
      }
      if( query == KMessageBox::Yes )
      writeURL(kWriteDoc->url(),!(KantView::lfNoAutoHl));
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
int KantView::checkOverwrite( KURL u )
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

KantView::fileResult KantView::saveAs() {
  KURL url;
  int query;

  do {
    query = KMessageBox::Yes;

    url = KFileDialog::getSaveURL(kWriteDoc->url().url(), QString::null,this);
    if (url.isEmpty()) return CANCEL;

    query = checkOverwrite( url );
  }
  while (query != KMessageBox::Yes);

  if( query == KMessageBox::Cancel )
    return CANCEL;

  writeURL(url);
  kWriteDoc->setURL( url, false );
  return OK;
}

void KantView::doCursorCommand(int cmdNum) {
  VConfig c;
  kWriteView->getVConfig(c);
  if (cmdNum & selectFlag) c.flags |= KantView::cfMark;
  if (cmdNum & multiSelectFlag) c.flags |= KantView::cfMark | KantView::cfKeepSelection;
  cmdNum &= ~(selectFlag | multiSelectFlag);
  kWriteView->doCursorCommand(c, cmdNum);
  kWriteDoc->updateViews();
}

void KantView::doEditCommand(int cmdNum) {
  VConfig c;
  kWriteView->getVConfig(c);
  kWriteView->doEditCommand(c, cmdNum);
  kWriteDoc->updateViews();
}


void KantView::clear() {
  if (isReadOnly())
    return;

  kWriteDoc->clear();
  kWriteDoc->clearFileName();
  kWriteDoc->updateViews();
}
/*
void KantView::cut() {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->cut(kWriteView,c);
  kWriteDoc->updateViews();
}

void KantView::copy() {
  kWriteDoc->copy(configFlags);
}

void KantView::paste() {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->paste(kWriteView, c);
  kWriteDoc->updateViews();
}

void KantView::undo() {
  undoMultiple(1);
}

void KantView::redo() {
  redoMultiple(1);
}
*/
void KantView::undoMultiple(int count) {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->undo(c, count);
  kWriteDoc->updateViews();
}

void KantView::redoMultiple(int count) {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->redo(c, count);
  kWriteDoc->updateViews();
}

void KantView::undoHistory()
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
/*
void KantView::indent() {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->indent(kWriteView,c);
  kWriteDoc->updateViews();
}

void KantView::unIndent() {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->unIndent(kWriteView,c);
  kWriteDoc->updateViews();
}

void KantView::cleanIndent() {
  if (isReadOnly())
    return;

  VConfig c;
  kWriteView->getVConfig(c);
  kWriteDoc->cleanIndent(kWriteView,c);
  kWriteDoc->updateViews();
}

void KantView::selectAll() {
  kWriteDoc->selectAll();
  kWriteDoc->updateViews();
}

void KantView::deselectAll() {
  kWriteDoc->deselectAll();
  kWriteDoc->updateViews();
}

void KantView::invertSelection() {
  kWriteDoc->invertSelection();
  kWriteDoc->updateViews();
}
*/

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

void KantView::find() {
  SearchDialog *searchDialog;

  if (!kWriteDoc->hasMarkedText()) searchFlags &= ~KantView::sfSelected;
  searchDialog = new SearchDialog(this, kWriteDoc->searchForList, kWriteDoc->replaceWithList,
    searchFlags & ~KantView::sfReplace);

  kWriteView->focusOutEvent(0L);// QT bug ?
  if (searchDialog->exec() == QDialog::Accepted) {
    kwview_addToStrList(kWriteDoc->searchForList, searchDialog->getSearchFor());
    searchFlags = searchDialog->getFlags() | (searchFlags & KantView::sfPrompt);
    initSearch(s, searchFlags);
    searchAgain(s);
  }
  delete searchDialog;
}

void KantView::replace() {
  SearchDialog *searchDialog;

  if (isReadOnly()) return;

  if (!kWriteDoc->hasMarkedText()) searchFlags &= ~KantView::sfSelected;
  searchDialog = new SearchDialog(this, kWriteDoc->searchForList, kWriteDoc->replaceWithList,
    searchFlags | KantView::sfReplace);

  kWriteView->focusOutEvent(0L);// QT bug ?
  if (searchDialog->exec() == QDialog::Accepted) {
//    kWriteDoc->recordReset();
    kwview_addToStrList(kWriteDoc->searchForList, searchDialog->getSearchFor());
    kwview_addToStrList(kWriteDoc->replaceWithList, searchDialog->getReplaceWith());
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
//    kWriteView->tagAll();
//    searchAgain();

void KantView::findAgain() {
  initSearch(s, searchFlags | KantView::sfFromCursor |KantView::sfPrompt | KantView::sfAgain);
  if (s.flags & KantView::sfReplace) replaceAgain(); else searchAgain(s);
}

void KantView::gotoLine() {
  GotoLineDialog *dlg;
  PointStruc cursor;

  dlg = new GotoLineDialog(this, kWriteView->cursor.y + 1, kWriteDoc->numLines());
//  dlg = new GotoLineDialog(kWriteView->cursor.y + 1, this);

  if (dlg->exec() == QDialog::Accepted) {
//    kWriteDoc->recordReset();
    cursor.x = 0;
    cursor.y = dlg->getLine() - 1;
    kWriteView->updateCursor(cursor);
    kWriteView->center();
    kWriteView->updateView(KantView::ufUpdateOnScroll);
    kWriteDoc->updateViews(this); //uptade all other views except this one
  }
  delete dlg;
}


void KantView::initSearch(SConfig &s, int flags) {

  s.flags = flags;
  s.setPattern(kWriteDoc->searchForList.first());

  if (s.flags & KantView::sfFromCursor) {
    // If we are continuing a backward search, make sure we do not get stuck
    // at an existing match.
    if ((s.flags & KantView::sfAgain) &&
      (s.flags & KantView::sfBackward) &&
      (s.cursor.x == kWriteView->cursor.x) &&
      (s.cursor.y == kWriteView->cursor.y)) {
      s.cursor.x--;
    }
    else {
      s.cursor = kWriteView->cursor;
    }
  } else {
    if (!(s.flags & KantView::sfBackward)) {
      s.cursor.x = 0;
      s.cursor.y = 0;
    } else {
      s.cursor.x = -1;
      s.cursor.y = kWriteDoc->lastLine();
    }
    s.flags |= KantView::sfFinished;
  }
  if (!(s.flags & KantView::sfBackward)) {
    if (!(s.cursor.x || s.cursor.y))
      s.flags |= KantView::sfFinished;
  }
  s.startCursor = s.cursor;
}

void KantView::continueSearch(SConfig &s) {

  if (!(s.flags & KantView::sfBackward)) {
    s.cursor.x = 0;
    s.cursor.y = 0;
  } else {
    s.cursor.x = -1;
    s.cursor.y = kWriteDoc->lastLine();
  }
  s.flags |= KantView::sfFinished;
  s.flags &= ~KantView::sfAgain;
}

void KantView::searchAgain(SConfig &s) {
  int query;
  PointStruc cursor;
  QString str;

  QString searchFor = kWriteDoc->searchForList.first();

  if( searchFor.isEmpty() ) {
    find();
    return;
  }

  do {
    query = KMessageBox::Cancel;
    if (kWriteDoc->doSearch(s,searchFor)) {
      cursor = s.cursor;
      if (!(s.flags & KantView::sfBackward))
        s.cursor.x += s.matchedLength;
      kWriteView->updateCursor(s.cursor); //does deselectAll()
      exposeFound(cursor,s.matchedLength,(s.flags & KantView::sfAgain) ? 0 : KantView::ufUpdateOnScroll,false);
    } else {
      if (!(s.flags & KantView::sfFinished)) {
        // ask for continue
        if (!(s.flags & KantView::sfBackward)) {
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


void KantView::replaceAgain() {
  if (isReadOnly())
    return;

  replaces = 0;
  if (s.flags & KantView::sfPrompt) {
    doReplaceAction(-1);
  } else {
    doReplaceAction(KantView::srAll);
  }
}

void KantView::doReplaceAction(int result, bool found) {
  int rlen;
  PointStruc cursor;
  bool started;

  QString searchFor = kWriteDoc->searchForList.first();
  QString replaceWith = kWriteDoc->replaceWithList.first();
  rlen = replaceWith.length();

  switch (result) {
    case KantView::srYes: //yes
      kWriteDoc->recordStart(this, s.cursor, configFlags,
        KWActionGroup::ugReplace, true);
      kWriteDoc->recordReplace(s.cursor, s.matchedLength, replaceWith);
      replaces++;
      if (s.cursor.y == s.startCursor.y && s.cursor.x < s.startCursor.x)
        s.startCursor.x += rlen - s.matchedLength;
      if (!(s.flags & KantView::sfBackward)) s.cursor.x += rlen;
      kWriteDoc->recordEnd(this, s.cursor, configFlags | KantView::cfPersistent);
      break;
    case KantView::srNo: //no
      if (!(s.flags & KantView::sfBackward)) s.cursor.x += s.matchedLength;
      break;
    case KantView::srAll: //replace all
      deleteReplacePrompt();
      do {
        started = false;
        while (found || kWriteDoc->doSearch(s,searchFor)) {
          if (!started) {
            found = false;
            kWriteDoc->recordStart(this, s.cursor, configFlags,
              KWActionGroup::ugReplace);
            started = true;
          }
          kWriteDoc->recordReplace(s.cursor, s.matchedLength, replaceWith);
          replaces++;
          if (s.cursor.y == s.startCursor.y && s.cursor.x < s.startCursor.x)
            s.startCursor.x += rlen - s.matchedLength;
          if (!(s.flags & KantView::sfBackward)) s.cursor.x += rlen;
        }
        if (started) kWriteDoc->recordEnd(this, s.cursor,
          configFlags | KantView::cfPersistent);
      } while (!askReplaceEnd());
      return;
    case KantView::srCancel: //cancel
      deleteReplacePrompt();
      return;
    default:
      replacePrompt = 0L;
  }

  do {
    if (kWriteDoc->doSearch(s,searchFor)) {
      //text found: highlight it, show replace prompt if needed and exit
      cursor = s.cursor;
      if (!(s.flags & KantView::sfBackward)) cursor.x += s.matchedLength;
      kWriteView->updateCursor(cursor); //does deselectAll()
      exposeFound(s.cursor,s.matchedLength,(s.flags & KantView::sfAgain) ? 0 : KantView::ufUpdateOnScroll,true);
      if (replacePrompt == 0L) {
        replacePrompt = new ReplacePrompt(this);
        XSetTransientForHint(qt_xdisplay(),replacePrompt->winId(),topLevelWidget()->winId());
        kWriteDoc->setPseudoModal(replacePrompt);//disable();
        connect(replacePrompt,SIGNAL(clicked()),this,SLOT(replaceSlot()));
        replacePrompt->show(); //this is not modal
      }
      return; //exit if text found
    }
    //nothing found: repeat until user cancels "repeat from beginning" dialog
  } while (!askReplaceEnd());
  deleteReplacePrompt();
}

void KantView::exposeFound(PointStruc &cursor, int slen, int flags, bool replace) {
  int x1, x2, y1, y2, xPos, yPos;

  kWriteDoc->markFound(cursor,slen);

  TextLine::Ptr textLine = kWriteDoc->getTextLine(cursor.y);
  x1 = kWriteDoc->textWidth(textLine,cursor.x)        -10;
  x2 = kWriteDoc->textWidth(textLine,cursor.x + slen) +20;
  y1 = kWriteDoc->fontHeight*cursor.y                 -10;
  y2 = y1 + kWriteDoc->fontHeight                     +30;

  xPos = kWriteView->xPos;
  yPos = kWriteView->yPos;

  if (x1 < 0) x1 = 0;
  if (replace) y2 += 90;

  if (x1 < xPos || x2 > xPos + kWriteView->width()) {
    xPos = x2 - kWriteView->width();
  }
  if (y1 < yPos || y2 > yPos + kWriteView->height()) {
    xPos = x2 - kWriteView->width();
    yPos = kWriteDoc->fontHeight*cursor.y - height()/3;
  }
  kWriteView->setPos(xPos, yPos);
  kWriteView->updateView(flags);// | ufPos,xPos,yPos);
  kWriteDoc->updateViews(this);
//  kWriteDoc->updateViews();
}

void KantView::deleteReplacePrompt() {
  kWriteDoc->setPseudoModal(0L);
}

bool KantView::askReplaceEnd() {
  QString str;
  int query;

  kWriteDoc->updateViews();
  if (s.flags & KantView::sfFinished) {
    // replace finished
    str = i18n("%1 replace(s) made").arg(replaces);
    KMessageBox::information(this, str, i18n("Replace"));
    return true;
  }

  // ask for continue
  if (!(s.flags & KantView::sfBackward)) {
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

void KantView::replaceSlot() {
  doReplaceAction(replacePrompt->result(),true);
}


/*void KantView::format() {
  dlg = new FormatDialog()
  if (dlg->exec() == QDialog::Accepted) {

  }
  delete dlg;
}            */

void KantView::installPopup(QPopupMenu *rmb_Menu)
{
  rmbMenu = rmb_Menu;

  updateBookmarks();
}


void KantView::setBookmark()
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

void KantView::addBookmark()
{
  int z;

  for (z = 0; z < (int) bookmarks.count(); z++) {
    if (bookmarks.at(z)->cursor.y == -1) break;
  }
  setBookmark(z);
}

void KantView::clearBookmarks()
{
  bookmarks.clear();
  updateBookmarks();
}

void KantView::setBookmark(int n)
{
  KWBookmark *b;

  if (n >= 10) return;
  while ((int) bookmarks.count() <= n) bookmarks.append(new KWBookmark());
  b = bookmarks.at(n);
  b->xPos = kWriteView->xPos;
  b->yPos = kWriteView->yPos;
  b->cursor = kWriteView->cursor;

  updateBookmarks();
}

void KantView::slotGotoBookmark()
{
    QCString nam = sender()->name();
    gotoBookmark( nam.toInt() );
}

void KantView::gotoBookmark(int n)
{
  KWBookmark *b;

  if (n-666 < 0 || n-666 >= (int) bookmarks.count()) return;
  b = bookmarks.at(n-666);
  if (b->cursor.y == -1) return;
//  kWriteDoc->recordReset();
  kWriteView->updateCursor(b->cursor);
  kWriteView->setPos(b->xPos, b->yPos);
//  kWriteView->updateView(ufPos, b->xPos, b->yPos);
//  kWriteDoc->updateViews(kWriteView); //uptade all other views except this one
  kWriteDoc->updateViews();
}


void KantView::updateBookmarks()
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


void KantView::readConfig(KConfig *config)
{
  searchFlags = config->readNumEntry("SearchFlags", KantView::sfPrompt);
  configFlags = config->readNumEntry("ConfigFlags", configFlags) & ~KantView::cfMark;
  wrapAt = config->readNumEntry("WrapAt", wrapAt);
}

void KantView::writeConfig(KConfig *config) {

  config->writeEntry("SearchFlags",searchFlags);
  config->writeEntry("ConfigFlags",configFlags);
  config->writeEntry("WrapAt",wrapAt);
}

void KantView::readSessionConfig(KConfig *config) {
  PointStruc cursor;
  int count, z;
  char s1[16];
  QString s2;
  KWBookmark *b;

/*
  searchFlags = config->readNumEntry("SearchFlags",sfPrompt);
  configFlags = config->readNumEntry("ConfigFlags");
  wrapAt = config->readNumEntry("WrapAt",79);
*/
  readConfig(config);

  kWriteView->xPos = config->readNumEntry("XPos");
  kWriteView->yPos = config->readNumEntry("YPos");
  cursor.x = config->readNumEntry("CursorX");
  cursor.y = config->readNumEntry("CursorY");
  kWriteView->updateCursor(cursor);

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

void KantView::writeSessionConfig(KConfig *config) {
  int z;
  char s1[16];
  char s2[64];
  KWBookmark *b;

/*
  config->writeEntry("SearchFlags",searchFlags);
  config->writeEntry("ConfigFlags",configFlags);
  config->writeEntry("WrapAt",wrapAt);
*/
  writeConfig(config);

  config->writeEntry("XPos",kWriteView->xPos);
  config->writeEntry("YPos",kWriteView->yPos);
  config->writeEntry("CursorX",kWriteView->cursor.x);
  config->writeEntry("CursorY",kWriteView->cursor.y);

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

void KantView::hlDlg() {
  HighlightDialog *dlg;
  HlManager *hlManager;
  HlDataList hlDataList;
  ItemStyleList defaultStyleList;
  ItemFont defaultFont;

  hlManager = kWriteDoc->hlManager;

  defaultStyleList.setAutoDelete(true);
  hlManager->getDefaults(defaultStyleList,defaultFont);

  hlDataList.setAutoDelete(true);
  //this gets the data from the KConfig object
  hlManager->getHlDataList(hlDataList);

  dlg = new HighlightDialog(hlManager, &defaultStyleList, &defaultFont, &hlDataList,
    kWriteDoc->highlightNum(), this);
//  dlg->hlChanged(kWriteDoc->highlightNum());
  if (dlg->exec() == QDialog::Accepted) {
    //this stores the data into the KConfig object
    hlManager->setHlDataList(hlDataList);
    hlManager->setDefaults(defaultStyleList,defaultFont);
  }
  delete dlg;
}

int KantView::getHl() {
  return kWriteDoc->highlightNum();
}

void KantView::setHl(int n) {
  kWriteDoc->setHighlight(n);
  kWriteDoc->updateViews();
}

int KantView::getEol() {
  return kWriteDoc->eolMode;
}

void KantView::setEol(int eol) {
  if (isReadOnly())
    return;

  kWriteDoc->eolMode = eol;
  kWriteDoc->setModified(true);
}



void KantView::paintEvent(QPaintEvent *e) {
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

void KantView::resizeEvent(QResizeEvent *) {

//  debug("Resize %d, %d",e->size().width(),e->size().height());

//kWriteView->resize(width() -20, height() -20);
  kWriteView->tagAll();
  kWriteView->updateView(0/*ufNoScroll*/);
}



//  Spellchecking methods

void KantView::spellcheck()
{
  if (isReadOnly())
    return;

  kspell.kspell= new KSpell (this, "KantView: Spellcheck", this,
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

void KantView::spellcheck2(KSpell *)
{
  kWriteDoc->setReadOnly (TRUE);

  // this is a hack, setPseudoModal() has been hacked to recognize 0x01
  // as special (never tries to delete it)
  // this should either get improved (with a #define or something),
  // or kspell should provide access to the spell widget.
  kWriteDoc->setPseudoModal((QWidget*)0x01);

  kspell.spell_tmptext = text();


  kspell.kspellon = TRUE;
  kspell.kspellMispellCount = 0;
  kspell.kspellReplaceCount = 0;
  kspell.kspellPristine = ! kWriteDoc->isModified();

  kspell.kspell->setProgressResolution (1);

  kspell.kspell->check(kspell.spell_tmptext);
}

void KantView::misspelling (QString origword, QStringList *, unsigned pos)
{
  int line;
  unsigned int cnt;

  // Find pos  -- CHANGEME: store the last found pos's cursor
  //   and do these searched relative to that to
  //   (significantly) increase the speed of the spellcheck

  for (cnt = 0, line = 0 ; line <= kWriteDoc->lastLine() && cnt <= pos ; line++)
    cnt += kWriteDoc->getTextLine(line)->length()+1;

  // Highlight the mispelled word
  PointStruc cursor;
  line--;
  cursor.x = pos - (cnt - kWriteDoc->getTextLine(line)->length()) + 1;
  cursor.y = line;
//  deselectAll(); // shouldn't the spell check be allowed within selected text?
  kspell.kspellMispellCount++;
  kWriteView->updateCursor(cursor); //this does deselectAll() if no persistent selections
  kWriteDoc->markFound(cursor,origword.length());
  kWriteDoc->updateViews();
}

void KantView::corrected (QString originalword, QString newword, unsigned pos)
{
  //we'll reselect the original word in case the user has played with
  //the selection

  int line;
  unsigned int cnt=0;

  if(newword != originalword)
    {

      // Find pos
      for (line = 0 ; line <= kWriteDoc->lastLine() && cnt <= pos ; line++)
        cnt += kWriteDoc->getTextLine(line)->length() + 1;

      // Highlight the mispelled word
      PointStruc cursor;
      line--;
      cursor.x = pos - (cnt-kWriteDoc->getTextLine(line)->length()) + 1;
      cursor.y = line;
      kWriteView->updateCursor(cursor);
      kWriteDoc->markFound(cursor, newword.length());

      kWriteDoc->recordStart(this, cursor, configFlags,
        KWActionGroup::ugSpell, true, kspell.kspellReplaceCount > 0);
      kWriteDoc->recordReplace(cursor, originalword.length(), newword);
      kWriteDoc->recordEnd(this, cursor, configFlags | KantView::cfGroupUndo);

      kspell.kspellReplaceCount++;
    }

}

void KantView::spellResult (const QString &)
{
  deselectAll(); //!!! this should not be done with persistent selections

//  if (kspellReplaceCount) kWriteDoc->recordReset();

  // we know if the check was cancelled
  // we can safely use the undo mechanism to backout changes
  // in case of a cancel, because we force the entire spell check
  // into one group (record)
  if (kspell.kspell->dlgResult() == 0) {
    if (kspell.kspellReplaceCount) {
      // backout the spell check
      VConfig c;
      kWriteView->getVConfig(c);
      kWriteDoc->undo(c);
      // clear the redo list, so the cancelled spell check can't be redo'ed <- say that word ;-)
      kWriteDoc->clearRedo();
      // make sure the modified flag is turned back off
      // if we started with a clean buffer
      if (kspell.kspellPristine)
        kWriteDoc->setModified(false);
    }
  }

  kWriteDoc->setPseudoModal(0L);
  kWriteDoc->setReadOnly (FALSE);

  // if we marked up the text, clear it now
  if (kspell.kspellMispellCount)
    kWriteDoc->unmarkFound();

  kWriteDoc->updateViews();

  kspell.kspell->cleanUp();
}

void KantView::spellCleanDone ()
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
     kWriteDoc->setPseudoModal(0L);
     kWriteDoc->setReadOnly (FALSE);

     // if we marked up the text, clear it now
     if (kspell.kspellMispellCount)
        kWriteDoc->unmarkFound();

     kWriteDoc->updateViews();
     KMessageBox::sorry(this, i18n("ISpell seems to have crashed."));
  }
  else
  {
     emit spellcheck_done();
  }
}

void KantView::init()
{
}

void KantView::dropEventPassEmited (QDropEvent* e)
{
  emit dropEventPass(e);
}

void KantView::printDlg ()
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
    while (  lineCount <= kWriteDoc->lastLine()  )
    {



       if (y+fm.ascent()+fm.descent() >= max )
      {
        printer->newPage();
        y=0;
      }

     y += fm.ascent();
     paint.drawText( 10, y, kWriteDoc-> textLine(lineCount) );
     y += fm.descent();





      lineCount++;
    }
    }
}

// Applies a new pattern to the search context.
void SConfig::setPattern(QString &newPattern) {
  bool regExp = (flags & KantView::sfRegularExpression);

  m_pattern = newPattern;
  if (regExp) {
    m_regExp.setCaseSensitive(flags & KantView::sfCaseSensitive);
    m_regExp.setPattern(m_pattern);
  }
}

// Applies the search context to the given string, and returns whether a match was found. If one is,
// the length of the string matched is also returned.
int SConfig::search(QString &text, int index) {
  bool regExp = (flags & KantView::sfRegularExpression);
  bool caseSensitive = (flags & KantView::sfCaseSensitive);

  if (flags & KantView::sfBackward) {
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

void KantView::setActive (bool b)
{
  active = b;
}

bool KantView::isActive ()
{
  return active;
}

void KantView::setFocus ()
{
  QWidget::setFocus ();

  emit gotFocus (this);
}

bool KantView::eventFilter (QObject *object, QEvent *event)
{
  if ( (event->type() == 8) )
    emit gotFocus (this);

  return QWidget::eventFilter (object, event);
}

void KantView::searchAgain (bool back)
{
  bool b= (searchFlags & sfBackward) > 0;
  initSearch(s, (searchFlags & ((b==back)?~sfBackward:~0))  // clear flag for forward searching
                | sfFromCursor | sfPrompt | sfAgain | ((b!=back)?sfBackward:0) );
  if (s.flags & sfReplace)
    replaceAgain();
  else
    KantView::searchAgain(s);
}
