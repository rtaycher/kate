/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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
#include "kateview.h"
#include "kateviewinternal.moc"
#include "katedocument.h"   
#include "katecodefoldinghelpers.h"
#include "kateiconborder.h"
#include "katehighlight.h"

#include <kcursor.h>
#include <kapplication.h>
#include <kglobalsettings.h>

#include <qstyle.h>
#include <qdragobject.h>
#include <qdropsite.h>
#include <qtimer.h>
#include <qpainter.h>
#include <qclipboard.h>

KateViewInternal::KateViewInternal(KateView *view, KateDocument *doc)
  : QScrollView(view, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )    
  , myView (view)
  , myDoc (doc)
{
  // this will prevent the yScrollbar from jumping around on appear of the xScrollbar 
  setCornerWidget (new QWidget (this));
  cornerWidget()->hide ();
  cornerWidget()->setFixedSize (style().scrollBarExtent().width(), style().scrollBarExtent().width());
  cornerWidget()->setFocusPolicy ( NoFocus );
                                
  // iconborder ;)
  leftBorder = new KateIconBorder( this );
  leftBorder->setFocusPolicy( NoFocus );
  connect( leftBorder, SIGNAL(sizeHintChanged()),
           this, SLOT(updateIconBorder()) );
  updateIconBorder();                            
  
  connect( leftBorder, SIGNAL(toggleRegionVisibility(unsigned int)),
           myDoc->regionTree, SLOT(toggleRegionVisibility(unsigned int)));  
           
  connect( doc->regionTree, SIGNAL(regionVisibilityChangedAt(unsigned int)),
           this, SLOT(slotRegionVisibilityChangedAt(unsigned int)));
//  connect( doc->regionTree, SIGNAL(regionBeginEndAddedRemoved(unsigned int)),
//           this, SLOT(slotRegionBeginEndAddedRemoved(unsigned int)) );
  connect( doc, SIGNAL(codeFoldingUpdated()),
           this, SLOT(slotCodeFoldingChanged()) );
  
  displayCursor.line=0;
  displayCursor.col=0;

  scrollTimer = 0;

  cursor.col = 0;
  cursor.line = 0;
  cursorOn = true;
  cursorTimer = 0;
  cXPos = 0;         
  yPos = 0;

  possibleTripleClick = false;
  
  bm.sXPos = 0;
  bm.eXPos = -1;

  setFocusPolicy( StrongFocus );    
  viewport()->setFocusProxy( this );   
  viewport()->setAcceptDrops( true );
  viewport()->setBackgroundMode( NoBackground );
  setDragAutoScroll( true );
  setFrameStyle( NoFrame );
  viewport()->setCursor( KCursor::ibeamCursor() );
  KCursor::setAutoHideCursor( viewport(), true );

  dragInfo.state = diNone;
    
  verticalScrollBar()->setLineStep( myDoc->viewFont.fontHeight );

  connect( this, SIGNAL( contentsMoving(int, int) ),
           this, SLOT( slotContentsMoving(int, int) ) );
}

void KateViewInternal::updateIconBorder()
{
  int width = leftBorder->sizeHint().width();
  setMargins( width, 0, 0, 0 );
  leftBorder->resize( width, visibleHeight() );
  leftBorder->update();
}       

void KateViewInternal::slotRegionVisibilityChangedAt(unsigned int)
{
  kdDebug(13030) << "slotRegionVisibilityChangedAt()" << endl;
  updateView();
  updateContents();
  leftBorder->update();
}

void KateViewInternal::slotCodeFoldingChanged()
{
  leftBorder->update();
}

void KateViewInternal::slotRegionBeginEndAddedRemoved(unsigned int)
{
  kdDebug(13030) << "slotRegionBeginEndAddedRemoved()" << endl;
//  myViewInternal->repaint();   
  // FIXME: performance problem
//  if (myDoc->getVirtualLine(line)<=myViewInternal->endLine)
  leftBorder->update();
}

int KateViewInternal::yPosition () const
{
   return yPos;
}

uint KateViewInternal::contentsYToLine( int y ) const
{
  return y / myDoc->viewFont.fontHeight;
}

int KateViewInternal::lineToContentsY( uint line ) const
{
  return line * myDoc->viewFont.fontHeight;
}

uint KateViewInternal::linesDisplayed() const
{
  int h = height();
  int fh = myDoc->viewFont.fontHeight;
  return (h % fh) == 0 ? h / fh : h / fh + 1;
}

QPoint KateViewInternal::cursorCoordinates()
{
   return contentsToViewport( QPoint( cXPos, lineToContentsY( displayCursor.line ) ) );
}

void KateViewInternal::doReturn()
{
  if( myDoc->configFlags() & KateDocument::cfDelOnInput )
    myDoc->removeSelectedText();
  KateTextCursor c = cursor;
  myDoc->newLine( c );
  updateCursor( c );
  updateView();
}

void KateViewInternal::doDelete()
{
  if ( (myDoc->configFlags() & KateDocument::cfDelOnInput) && myDoc->hasSelection() ) {
    myDoc->removeSelectedText();
  } else {
    myDoc->del( cursor );
  }
}

void KateViewInternal::doBackspace()
{
  if( (myDoc->configFlags() & KateDocument::cfDelOnInput) && myDoc->hasSelection() ) {
    myDoc->removeSelectedText();
  } else {
    myDoc->backspace( cursor );
  }
}

void KateViewInternal::doPaste()
{
   if( myDoc->configFlags() & KateDocument::cfDelOnInput )
     myDoc->removeSelectedText();
   myDoc->paste( cursor, myView );
}

void KateViewInternal::doTranspose()
{
  myDoc->transpose( cursor );
  cursorRight();
  cursorRight();
}

class CalculatingCursor : public KateTextCursor {
public:
  CalculatingCursor( const Kate::Document& doc )
    : KateTextCursor(), m_doc( doc )
    { Q_ASSERT( valid() ); }
  CalculatingCursor( const Kate::Document& doc, const KateTextCursor& c )          
    : KateTextCursor( c ), m_doc( doc )
    { Q_ASSERT( valid() ); }
  // This one constrains its arguments to valid positions
  CalculatingCursor( const Kate::Document& doc, uint line, uint col )
    : KateTextCursor( line, col ), m_doc( doc )
    { makeValid(); }
  virtual CalculatingCursor& operator+=( int n ) = 0;
  virtual CalculatingCursor& operator-=( int n ) = 0;
  CalculatingCursor& operator++() { return operator+=( 1 ); }
  CalculatingCursor& operator--() { return operator-=( 1 ); }
  void makeValid() {
    line = QMAX( 0, QMIN( int( m_doc.numLines() - 1 ), line ) );
    col  = QMAX( 0, QMIN( m_doc.lineLength( line ), col ) );
    Q_ASSERT( valid() );
  }
  void toEdge( Bias bias ) {
    if( bias == left ) col = 0;
    else if( bias == right ) col = m_doc.lineLength( line );
  }
  bool atEdge() const { return atEdge( left ) || atEdge( right ); }
  bool atEdge( Bias bias ) const {
    switch( bias ) {
    case left:  return col == 0;
    case none:  return atEdge();
    case right: return col == m_doc.lineLength( line );
    default: Q_ASSERT(false); return false;
    }
  }
protected:
  bool valid() const { return line >= 0 && uint( line ) < m_doc.numLines() &&
                              col  >= 0 && col <= m_doc.lineLength( line ); }
  const Kate::Document& m_doc;
};

class BoundedCursor : public CalculatingCursor {
public:
  BoundedCursor( const Kate::Document& doc )
    : CalculatingCursor( doc ) {};
  BoundedCursor( const Kate::Document& doc, const KateTextCursor& c )          
    : CalculatingCursor( doc, c ) {};
  BoundedCursor( const Kate::Document& doc, uint line, uint col )
    : CalculatingCursor( doc, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    col += n;
    col = QMAX( 0, col );
    col = QMIN( col, m_doc.lineLength( line ) );
    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    return operator+=( -n );
  }
};

class WrappingCursor : public CalculatingCursor {
public:
  WrappingCursor( const Kate::Document& doc )
    : CalculatingCursor( doc ) {};
  WrappingCursor( const Kate::Document& doc, const KateTextCursor& c )          
    : CalculatingCursor( doc, c ) {};
  WrappingCursor( const Kate::Document& doc, uint line, uint col )
    : CalculatingCursor( doc, line, col ) {};
  virtual CalculatingCursor& operator+=( int n ) {
    if( n < 0 ) return operator-=( -n );
    int len = m_doc.lineLength( line );
    if( col + n <= len ) {
      col += n;
    } else if( uint( line ) < m_doc.numLines() - 1 ) {
      n -= len - col + 1;
      col = 0;
      line++;
      operator+=( n );
    } else {
      col = len;
    }
    Q_ASSERT( valid() );
    return *this;
  }
  virtual CalculatingCursor& operator-=( int n ) {
    if( n < 0 ) return operator+=( -n );
    if( col - n >= 0 ) {
      col -= n;
    } else if( line > 0 ) {
      n -= col + 1;
      line--;
      col = m_doc.lineLength( line );
      operator-=( n );
    } else {
      col = 0;
    }
    Q_ASSERT( valid() );
    return *this;
  }
};

void KateViewInternal::moveChar( Bias bias, bool sel )
{
  KateTextCursor c;
  if( myDoc->configFlags() & KateDocument::cfWrapCursor ) {
    c = WrappingCursor( *myDoc, cursor ) += bias;
  } else {
    c = BoundedCursor( *myDoc, cursor ) += bias;
  }
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorLeft(  bool sel ) { moveChar( left,  sel ); }
void KateViewInternal::cursorRight( bool sel ) { moveChar( right, sel ); }

void KateViewInternal::moveWord( Bias bias, bool sel )
{
  WrappingCursor c( *myDoc, cursor );
  if( !c.atEdge( bias ) ) {
    Highlight* h = myDoc->highlight();   
    c += bias;
    while( !c.atEdge( bias ) && !h->isInWord( myDoc->textLine( c.line )[ c.col - (bias == left ? 1 : 0) ] ) )
      c += bias;
    while( !c.atEdge( bias ) &&  h->isInWord( myDoc->textLine( c.line )[ c.col - (bias == left ? 1 : 0) ] ) )
      c += bias;
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
  BoundedCursor c( *myDoc, cursor );
  c.toEdge( bias );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::home( bool sel )
{      
  if( !(myDoc->configFlags() & KateDocument::cfSmartHome) ) {
    moveEdge( left, sel );
    return;
  }

  KateTextCursor c = cursor;
  int lc = myDoc->kateTextLine( c.line )->firstChar();
  
  if( lc < 0 || c.col == lc ) {
    c.col = 0;
  } else {
    c.col = lc;
  }

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::end( bool sel ) { moveEdge( right, sel ); }

void KateViewInternal::cursorUp(bool sel)
{
  if( displayCursor.line == 0 )
    return;
  
  KateTextCursor c( myDoc->getRealLine(displayCursor.line-1), cursor.col );
  myDoc->textWidth( c, cXPos );

  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::cursorDown(bool sel)
{
  KateTextCursor c = cursor;

  int x;
  
  if( c.line >= (int)myDoc->lastLine() ) {
    x = myDoc->textLength( c.line );
    if( c.col >= x )
      return;
    c.col = x;
  } else {
    c.line = myDoc->getRealLine( displayCursor.line + 1 );
    x = myDoc->textLength( c.line );
    if( c.col > x )
      c.col = x;
  }
  
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::topOfView( bool sel )
{
  WrappingCursor c( *myDoc, myDoc->getRealLine( firstLine() ), 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottomOfView( bool sel )
{
  WrappingCursor c( *myDoc, myDoc->getRealLine( lastLine() ), 0 );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::scrollLines( int lines, bool sel )
{
  scrollBy( 0, lines * myDoc->viewFont.fontHeight );
  
  int line = int( displayCursor.line ) + lines;
  line = QMAX( 0, QMIN( line, int(myDoc->numVisLines()) ) );
  
  KateTextCursor c( myDoc->getRealLine( line ), cursor.col );
  myDoc->textWidth( c, cXPos );

  updateSelection( c, sel );
  updateCursor( c );
}

// Maybe not the right thing to do for these actions, but
// better than what was here before!
void KateViewInternal::scrollUp()   { scrollLines( -1, false ); }
void KateViewInternal::scrollDown() { scrollLines(  1, false ); }

void KateViewInternal::pageUp( bool sel )
{
  scrollLines( -QMAX( linesDisplayed() - 1, 0 ), sel );
}

void KateViewInternal::pageDown( bool sel )
{
  scrollLines( QMAX( linesDisplayed() - 1, 0 ), sel );
}

void KateViewInternal::top( bool sel )
{
  KateTextCursor c( 0, cursor.col );  
  myDoc->textWidth( c, cXPos );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::bottom( bool sel )
{
  KateTextCursor c( myDoc->lastLine(), cursor.col );
  myDoc->textWidth( c, cXPos );
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
  KateTextCursor c( myDoc->lastLine(), myDoc->textLength( myDoc->lastLine() ) );
  updateSelection( c, sel );
  updateCursor( c );
}

void KateViewInternal::slotContentsMoving( int, int y)
{                         
  yPos = y;

  updateView();              
  
  //kdDebug ()<< "now Y: "<<contentsY() << " new Y: "<<y<<endl;

  if (contentsY() != yPos)  
    leftBorder->scroll (0, contentsY()-yPos);
}

void KateViewInternal::updateSelection( const KateTextCursor& newCursor, bool keepSel )
{
  if( keepSel ) {
    myDoc->selectTo( cursor, newCursor );
  } else if( !(myDoc->configFlags() & KateDocument::cfPersistent) ) {
    myDoc->clearSelection();
  }
}

void KateViewInternal::updateCursor( const KateTextCursor& newCursor )
{
  if( cursor == newCursor )
    return;

  KateTextCursor oldDisplayCursor = displayCursor;

  cursor = newCursor;
  displayCursor.line = myDoc->getVirtualLine(cursor.line);
  displayCursor.col = cursor.col;

  kdDebug(13030) << "updateCursor():" << endl;
  kdDebug(13030) << "Virtual: " << displayCursor.line << endl;
  kdDebug(13030) << "Real: " << cursor.line << endl;

  cXPos = myDoc->textWidth( cursor );
  ensureVisible( cXPos, lineToContentsY( displayCursor.line   ), 0, 0 );
  ensureVisible( cXPos, lineToContentsY( displayCursor.line+1 ), 0, 0 );

  if( bm.sXPos < bm.eXPos ) {
    tagLines( bm.cursor.line, bm.cursor.line );
  }

  myDoc->newBracketMark( cursor, bm );

  tagLines( oldDisplayCursor.line, oldDisplayCursor.line );
  tagLines( displayCursor.line, displayCursor.line );

  QPoint cursorP = cursorCoordinates();
  setMicroFocusHint( cursorP.x(), cursorP.y(), 0, myDoc->viewFont.fontHeight );
  
  emit myView->cursorPositionChanged();
}

void KateViewInternal::tagRealLines( int start, int end )
{
  kdDebug(13030) << "tagRealLines( " << start << ", " << end << " )\n";
  tagLines( myDoc->getVirtualLine( start ), myDoc->getVirtualLine( end ) );
}

void KateViewInternal::tagLines( int start, int end )
{
  kdDebug(13030) << "tagLines( " << start << ", " << end << " )\n";
  int y = lineToContentsY( start );
  int h = (end - start + 1) * myDoc->viewFont.fontHeight;
  updateContents( contentsX(), y, visibleWidth(), h );  
  leftBorder->update (0, y-contentsY(), leftBorder->width(), h);
}

void KateViewInternal::tagAll()
{
  kdDebug(13030) << "tagAll()" << endl;
  updateContents();      
  leftBorder->update ();
}

void KateViewInternal::centerCursor()
{
  center( cXPos, lineToContentsY( displayCursor.line ) );
}

void KateViewInternal::updateView()
{
  uint maxLen = 0;
  uint endLine = lastLine();
  for( uint line = firstLine(); line <= endLine; line++ ) {
    maxLen = QMAX( maxLen, myDoc->textWidth( myDoc->kateTextLine( myDoc->getRealLine( line ) ), -1 ) );
  }

  // Nice bit of extra space
  maxLen += 8;

  resizeContents( maxLen, myDoc->visibleLines() * myDoc->viewFont.fontHeight );
}

void KateViewInternal::paintCursor()
{
  tagLines( displayCursor.line, displayCursor.line );
}

void KateViewInternal::paintBracketMark()
{
//   int y = myDoc->viewFont.fontHeight*( bm.cursor.line );
// 
//   QPainter paint( this );
//   paint.setPen(myDoc->cursorCol(bm.cursor.col, bm.cursor.line));
//   paint.drawLine( bm.sXPos - contentsX(), y, bm.eXPos - contentsX() -1, y );
}

// Point in content coordinates
void KateViewInternal::placeCursor( const QPoint& p, bool keepSelection )
{
  int line = contentsYToLine( p.y() );

  line = QMAX( 0, QMIN( line, int(myDoc->numVisLines()) - 1 ) );

  KateTextCursor c;
  c.line = myDoc->getRealLine( line );
  myDoc->textWidth( c, p.x() );

  updateSelection( c, keepSelection );
  updateCursor( c );
}

// Point in content coordinates
bool KateViewInternal::isTargetSelected( const QPoint& p )
{
  int line = contentsYToLine( p.y() );

  TextLine::Ptr textLine = myDoc->kateTextLine( myDoc->getRealLine( line ) );
  if( !textLine )
    return false;

  int col = myDoc->textPos( textLine, p.x() );

  return myDoc->lineColSelected( line, col );
}

void KateViewInternal::focusInEvent( QFocusEvent* )
{
  cursorTimer = startTimer( KApplication::cursorFlashTime() / 2 );
  paintCursor();
}

void KateViewInternal::focusOutEvent( QFocusEvent* )
{
  if( cursorTimer ) {
    killTimer( cursorTimer );
    cursorTimer = 0;
  }
  paintCursor();
}

void KateViewInternal::keyPressEvent( QKeyEvent* e )
{
  if( !myDoc->isReadWrite() ) {
    e->ignore();
    return;
  }

  if( myDoc->configFlags() & KateDocument::cfTabIndents && myDoc->hasSelection() ) {
    KKey key(e);
    if( key == Qt::Key_Tab ) {
      myDoc->indent( cursor.line );
      return;
    } else if (key == SHIFT+Qt::Key_Backtab || key == Qt::Key_Backtab) {
      myDoc->unIndent( cursor.line );
      return;
    }
  }
  if ( !(e->state() & ControlButton) && !(e->state() & AltButton)
         && myDoc->insertChars( cursor.line, cursor.col, e->text(), myView ) )
  {
    e->accept();
    return;
  }
  
  e->ignore();
}

void KateViewInternal::contentsMousePressEvent( QMouseEvent* e )
{
  if (e->button() == LeftButton) {
    if (possibleTripleClick) {
      possibleTripleClick = false;
      myDoc->selectLine( cursor );
      cursor.col = 0;
      updateCursor( cursor );
      return;
    }

    if( isTargetSelected( e->pos() ) ) {
      dragInfo.state = diPending;
      dragInfo.start = contentsToViewport( e->pos() );
    } else {
      dragInfo.state = diNone;

      placeCursor( e->pos(), e->state() & ShiftButton );
      scrollX = 0;
      scrollY = 0;
      if( !scrollTimer )
        scrollTimer = startTimer(50);
    }
  }

  if( myView->popup() && e->button() == RightButton ) {
    myView->popup()->popup( mapToGlobal( contentsToViewport( e->pos() ) ) );
  }
  myView->mousePressEvent(e); // this doesn't do anything, does it?
  // it does :-), we need this for KDevelop, so please don't uncomment it again -Sandy
}

void KateViewInternal::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
  if (e->button() == LeftButton) {

    myDoc->selectWord( cursor );

    // Move cursor to end of selected word
    if (myDoc->hasSelection())
    {
      cursor.col = myDoc->selectEnd.col;
      cursor.line = myDoc->selectEnd.line;
      updateCursor( cursor );
    }

    possibleTripleClick = true;
    QTimer::singleShot( QApplication::doubleClickInterval(),
      this, SLOT(tripleClickTimeout()) );
  }
}

void KateViewInternal::tripleClickTimeout()
{
  possibleTripleClick = false;
}

void KateViewInternal::contentsMouseReleaseEvent( QMouseEvent* e )
{
  if (e->button() == LeftButton) {
    if (dragInfo.state == diPending) {
      // we had a mouse down in selected area, but never started a drag
      // so now we kill the selection
      placeCursor( e->pos() );
    } else if (dragInfo.state == diNone) {
      QApplication::clipboard()->setSelectionMode( true );
      myDoc->copy();
      QApplication::clipboard()->setSelectionMode( false );

      killTimer(scrollTimer);
      scrollTimer = 0;
    }
    dragInfo.state = diNone;
  } else if (e->button() == MidButton) {
    placeCursor( e->pos() );
    if( myDoc->isReadWrite() )
    {
      QApplication::clipboard()->setSelectionMode( true );
      doPaste();
      QApplication::clipboard()->setSelectionMode( false );
    }
  }
}

void KateViewInternal::contentsMouseMoveEvent( QMouseEvent* e )
{
  if( e->state() & LeftButton ) {
    
    if (dragInfo.state == diPending) {
      // we had a mouse down, but haven't confirmed a drag yet
      // if the mouse has moved sufficiently, we will confirm
      QPoint p( e->pos() - dragInfo.start );
      if( p.manhattanLength() > KGlobalSettings::dndEventDelay() ) {
        // we've left the drag square, we can start a real drag operation now
        doDrag();
      }
      return;
    }
   
    contentsToViewport (e->x(), e->y(), mouseX, mouseY);
    
    scrollX = 0;
    scrollY = 0;
    int d = myDoc->viewFont.fontHeight;
    if (mouseX < 0) {
      mouseX = 0;
      scrollX = -d;
    }
    if (mouseX > visibleWidth()) {
      mouseX = visibleWidth();
      scrollX = d;
    }
    if (mouseY < 0) {
      mouseY = 0;
      scrollY = -d;
    }
    if (mouseY > visibleHeight()) {
      mouseY = visibleHeight();
      scrollY = d;
    }
    placeCursor( viewportToContents( QPoint( mouseX, mouseY ) ), true );
  }
}

void KateViewInternal::drawContents( QPainter *paint, int cx, int cy, int cw, int ch )
{
  uint h = myDoc->viewFont.fontHeight;

  uint startline = contentsYToLine( cy );
  uint endline   = contentsYToLine( cy + ch - 1 );
  
  kdDebug(13030) << "drawContents(): y = " << cy << " h = " << ch << endl; 
  kdDebug(13030) << "drawContents(): x = " << cx << " w = " << cw << endl;
  kdDebug(13030) << "Repainting " << startline << " - " << endline << endl;

  for( uint line = startline; line <= endline; line++ ) {
    uint realLine = myDoc->getRealLine( line );
    if( realLine > myDoc->lastLine() ) {
      paint->fillRect( cx, line*h, cw, h, myDoc->colors[0] );
    } else {
      myDoc->paintTextLine( *paint, realLine, 0, -1,
                            cx, line*h, cx, cx+cw,
                            (cursorOn && hasFocus() && (realLine == uint(cursor.line))) ? cursor.col : -1,
                            myView->isOverwriteMode(), cXPos, true,
                            myDoc->configFlags() & KateDocument::cfShowTabs,
                            KateDocument::ViewFont, realLine == uint(cursor.line) );
    }
  }

  if (bm.eXPos > bm.sXPos)
    paintBracketMark();
}

void KateViewInternal::viewportResizeEvent( QResizeEvent* )
{ 
  updateIconBorder();
  updateView();
}

void KateViewInternal::timerEvent( QTimerEvent* e )
{
  if (e->timerId() == cursorTimer)
  {
    cursorOn = !cursorOn;
    paintCursor();
  }
  else if (e->timerId() == scrollTimer && (scrollX | scrollY))
  {   
     scrollBy( scrollX, scrollY );
    placeCursor( viewportToContents( QPoint( mouseX, mouseY ) ), true );
  }
}

void KateViewInternal::doDrag()
{
  dragInfo.state = diDragging;
  dragInfo.dragObject = new QTextDrag(myDoc->selection(), this);
  dragInfo.dragObject->dragCopy();
}

void KateViewInternal::contentsDragEnterEvent( QDragEnterEvent* event )
{
  event->accept( (QTextDrag::canDecode(event) && myDoc->isReadWrite()) ||
                  QUriDrag::canDecode(event) );
}

void KateViewInternal::contentsDropEvent( QDropEvent* event )
{
  if ( QUriDrag::canDecode(event) ) {

      emit dropEventPass(event);

  } else if ( QTextDrag::canDecode(event) && myDoc->isReadWrite() ) {

    QString text;

    if (!QTextDrag::decode(event, text))
      return;
    
    // is the source our own document?
    bool priv = myDoc->ownedView( (KateView*)(event->source()) );
    // dropped on a text selection area?
    bool selected = isTargetSelected( event->pos() );

    if( priv && selected ) {
      // this is a drag that we started and dropped on our selection
      // ignore this case
      return;
    }

    if( priv && event->action() == QDropEvent::Move ) {
      // this is one of mine (this document), not dropped on the selection
      myDoc->removeSelectedText();
    }
    placeCursor( event->pos() );
    myDoc->insertText( cursor.line, cursor.col, text );

    updateView();
  }
}

void KateViewInternal::clear()
{
  cursor.col        = cursor.line        = 0;
  displayCursor.col = displayCursor.line = 0;
}
