/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
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

#include "kateiconborder.h"
#include "kateiconborder.moc"

#include "kateview.h"
#include "kateviewinternal.h"
#include "katedocument.h"
#include "katecodefoldinghelpers.h"

#include <kdebug.h>
#include <kglobalsettings.h>
#include <klocale.h>

#include <qpainter.h>
#include <qpopupmenu.h>
#include <qcursor.h>

using namespace KTextEditor;

const char * plus_xpm[] = {
"12 16 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"      .     ",
"      .     ",
" .........  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++.+++.  ",
" .+++.+++.  ",
" .+.....+.  ",
" .+++.+++.  ",
" .+++.+++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .........  ",
"      .     ",
"      .     "};

const  char * minus_xpm[] = {
"12 16 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"      .     ",
"      .     ",
" .........  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+.....+.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .+++++++.  ",
" .........  ",
"      .     ",
"      .     "};


const char*bookmark_xpm[]={
"12 16 4 1",
"b c #808080",
"a c #000080",
"# c #0000ff",
". c None",
"............",
"............",
"........###.",
".......#...a",
"......#.##.a",
".....#.#..aa",
"....#.#...a.",
"...#.#.a.a..",
"..#.#.a.a...",
".#.#.a.a....",
"#.#.a.a.....",
"#.#a.a...bbb",
"#...a..bbb..",
".aaa.bbb....",
"............",
"............"};

const int iconPaneWidth = 16;
const int halfIPW = 8;

KateIconBorder::KateIconBorder ( KateViewInternal* internalView )
  : QWidget(internalView, "", Qt::WStaticContents | Qt::WRepaintNoErase | Qt::WResizeNoErase )
  , m_view( internalView->m_view )
  , m_doc( internalView->m_doc )
  , m_viewInternal( internalView )
  , m_iconBorderOn( false )
  , m_lineNumbersOn( false )
  , m_foldingMarkersOn( false )
  , m_cachedLNWidth( 0 )
{                                        
  setBackgroundMode( NoBackground );
  setFont( m_doc->getFont(KateDocument::ViewFont) ); // for line numbers
  
  m_doc->setDescription( MarkInterface::markType01, i18n("Bookmark") );
  m_doc->setPixmap( MarkInterface::markType01, QPixmap(bookmark_xpm) );
}

void KateIconBorder::setIconBorderOn( bool enable )
{
  if( enable == m_iconBorderOn )
    return;

  m_iconBorderOn = enable;
  
  updateGeometry();
}

void KateIconBorder::setLineNumbersOn( bool enable )
{
  if( enable == m_lineNumbersOn )
    return;

  m_lineNumbersOn = enable;
  
  updateGeometry();
}

void KateIconBorder::setFoldingMarkersOn( bool enable )
{
  if( enable == m_foldingMarkersOn )
    return;
  
  m_foldingMarkersOn = enable;
  
  updateGeometry();
}

QSize KateIconBorder::sizeHint() const
{
  int w = 0;
  
  if (m_lineNumbersOn) {
    // FIXME this is not good enough if the font is not fixed-pitch
    w += fontMetrics().width( QString().setNum(m_view->doc()->numLines()) ) + 4;
    w += 2; //  2px leading
  }

  if (m_iconBorderOn)
    w += iconPaneWidth + 1;

  if (m_foldingMarkersOn)
    w += iconPaneWidth;
  else
    w += 4;
  // A little extra makes selecting at the beginning easier and looks nicer
  // Anders: And this belongs in the border? Apart from that job obviously belonging
  // to the view, the icon border may not have the same color as the
  // editor content area...
//  if( !m_foldingMarkersOn )
//    w += 4;

  return QSize( w, 0 );
}

QSize KateIconBorder::minimumSizeHint() const
{
  return sizeHint();
}

void KateIconBorder::paintEvent(QPaintEvent* e)
{      
  QRect rect = e->rect();

  int lnWidth( 0 );
  if ( m_lineNumbersOn ) // avoid calculating unless needed ;-)
  {
    lnWidth = fontMetrics().width( QString().setNum(m_view->doc()->numLines()) ) + 4;
    if ( lnWidth != m_cachedLNWidth ) 
    {
      // we went from n0 ->n9 lines or vice verca
      // this causes an extra updateGeometry() first time the line numbers
      // are displayed, but sizeHint() is supposed to be const so we can't set
      // the cached value there.
      m_cachedLNWidth = lnWidth;
      updateGeometry();
      return;
    }
  }
  
  uint startline = m_viewInternal->contentsYToLine( m_viewInternal->yPosition() + rect.y() );
  uint endline   = m_viewInternal->contentsYToLine( m_viewInternal->yPosition() + rect.y() + rect.height() - 1 );
                                    
  
  int fontHeight = m_doc->viewFont.fontHeight; 
  int w( width() );                     // sane value/calc only once
  int y( startline * fontHeight );      // assuming adding is faster than mult'ing(?)
  int lnbx( 2+lnWidth-1 );              // line nbr pane border position: calc only once 
  QColor bgCol ( colorGroup().light() );
  QColor col ( colorGroup().background().dark() );
  uint currentLine( m_view->cursorLine() );
  
  QPainter p (this);   
  p.setPen( col );
  p.fillRect( rect, bgCol );                 
  
  p.translate (0, -m_viewInternal->yPosition());
  
  for( uint line = startline; line <= endline; line++ )
  {
    uint realLine = m_doc->getRealLine( line );    
    
    //int y = line * fontHeight; // see below
    int lnX( 0 );
  
    if ( realLine == currentLine )
      p.fillRect( 0, y, w, fontHeight, colorGroup().midlight() );    

    // line number
    if( m_lineNumbersOn )
    {
      lnX +=2;
      p.drawLine( lnbx, y, lnbx, y+fontHeight );
      if( realLine <= m_doc->lastLine() )
        p.drawText( lnX + 1, y, lnWidth-4, fontHeight, Qt::AlignRight|Qt::AlignVCenter,
          QString("%1").arg( realLine + 1 ) );

      lnX += lnWidth;
    }

    // icon pane
    if( m_iconBorderOn ) {
      p.drawLine(lnX+iconPaneWidth, y, lnX+iconPaneWidth, y+fontHeight);

      if( realLine <= m_doc->lastLine() ) {
        uint mrk( m_doc->mark( realLine ) ); // call only once
        if ( mrk ) // ;-]]
          for( uint bit = 0; bit < 32; bit++ ) {
            MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)(1<<bit);
            if( mrk & markType ) {
              p.drawPixmap( lnX+2, y, m_doc->markPixmap( markType ) );
            }
          }
      }

      lnX += iconPaneWidth + 1;
    }

    // folding markers
    if( m_foldingMarkersOn )
    {
      if( realLine <= m_doc->lastLine() )
      {
        p.setPen(black);
        KateLineInfo info;
        m_doc->lineInfo(&info,realLine);
        if (!info.topLevel)
        {
          if (info.startsVisibleBlock)
            p.drawPixmap(lnX+2,y,QPixmap(minus_xpm));
          else if (info.startsInVisibleBlock)
            p.drawPixmap(lnX+2,y,QPixmap(plus_xpm));
          else
          {
            p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+fontHeight-1);
            if (info.endsBlock)
              p.drawLine(lnX+halfIPW,y+fontHeight-1,lnX+iconPaneWidth-2,y+fontHeight-1);
          }
        }
        p.setPen( col );
      }
      lnX+=iconPaneWidth;
    }
    y += fontHeight; // faster?   
  }
}

KateIconBorder::BorderArea KateIconBorder::positionToArea( const QPoint& p ) const
{
  int x = 0;
  if( m_lineNumbersOn ) {
    x += fontMetrics().width( QString().setNum(m_view->doc()->numLines()) );
    if( p.x() <= x )
      return LineNumbers;
  }
  if( m_iconBorderOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return IconBorder;
  }
  if( m_foldingMarkersOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return FoldingMarkers;
  }
  return None;
}

void KateIconBorder::mousePressEvent( QMouseEvent* e )
{
  m_lastClickedLine = m_doc->getRealLine(
    (e->y() + m_viewInternal->contentsY()) / m_doc->viewFont.fontHeight );
  
  QMouseEvent forward( QEvent::MouseButtonPress, 
    QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
  m_viewInternal->contentsMousePressEvent( &forward );
}

void KateIconBorder::mouseMoveEvent( QMouseEvent* e )
{
  QMouseEvent forward( QEvent::MouseMove, 
    QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
  m_viewInternal->contentsMouseMoveEvent( &forward );
}

void KateIconBorder::mouseReleaseEvent( QMouseEvent* e )
{
  uint cursorOnLine = m_doc->getRealLine(
    (e->y() + m_viewInternal->contentsY()) / m_doc->viewFont.fontHeight );
  
  BorderArea area = positionToArea( e->pos() );
  if( area == IconBorder &&
      e->button() == LeftButton &&
      cursorOnLine == m_lastClickedLine &&
      cursorOnLine <= m_doc->lastLine() )
  {
    if( m_doc->editableMarks() == MarkInterface::markType01 ) {
      if( m_doc->mark( cursorOnLine ) & MarkInterface::markType01 )
        m_doc->removeMark( cursorOnLine, MarkInterface::markType01 );
      else
        m_doc->addMark( cursorOnLine, MarkInterface::markType01 );
    } else {
      showMarkMenu( cursorOnLine, QCursor::pos() );
    }
  }
  if( area == FoldingMarkers &&
      cursorOnLine == m_lastClickedLine &&
      cursorOnLine <= m_doc->lastLine() )
  {
    kdDebug(13000)<<"The click was within a marker range, is it valid though ?"<<endl;
    KateLineInfo info;
    m_doc->lineInfo(&info,cursorOnLine);
    if ((info.startsVisibleBlock) || (info.startsInVisibleBlock))
    {
      kdDebug(13000)<<"Tell whomever it concerns, that we want a region visibility changed"<<endl;
      emit toggleRegionVisibility(cursorOnLine);
    }
  }
  QMouseEvent forward( QEvent::MouseButtonRelease, 
    QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
  m_viewInternal->contentsMouseReleaseEvent( &forward );
}

void KateIconBorder::mouseDoubleClickEvent( QMouseEvent* e )
{
  QMouseEvent forward( QEvent::MouseButtonDblClick, 
    QPoint( 0, e->y() + m_viewInternal->contentsY() ), e->button(), e->state() );
  m_viewInternal->contentsMouseDoubleClickEvent( &forward );
}

void KateIconBorder::showMarkMenu( uint line, const QPoint& pos )
{
  QPopupMenu markMenu;
  for( uint bit = 0; bit < 32; bit++ ) {
    MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)(1<<bit);
    if( !(m_doc->editableMarks() & markType) )
      continue;
    if( !m_doc->markDescription( markType ).isEmpty() ) {
      markMenu.insertItem( m_doc->markDescription( markType ), markType );
    } else {
      markMenu.insertItem( i18n("Mark Type %1").arg( bit + 1 ), markType );
    }
    if( m_doc->mark( line ) & markType )
      markMenu.setItemChecked( markType, true );
  }
  if( markMenu.count() == 0 )
    return;
  int result = markMenu.exec( pos );
  if( result <= 0 )
    return;
  MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)result;
  if( m_doc->mark( line ) & markType ) {
    m_doc->removeMark( line, markType );
  } else {
    m_doc->addMark( line, markType );
  }
}
