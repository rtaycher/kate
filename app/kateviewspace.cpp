/***************************************************************************
                          kateviewspace.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kateviewspace.h"
#include "kateviewspace.moc"

#include "katemainwindow.h"

#include <kiconloader.h>
#include <klocale.h>
#include <qwidgetstack.h>
#include <ksimpleconfig.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qcursor.h>

KateViewSpace::KateViewSpace(QWidget* parent, const char* name)
  : QVBox(parent, name)
{
   mViewList.setAutoDelete(false);

  stack = new QWidgetStack( this );
  setStretchFactor(stack, 1);
  stack->installEventFilter( this );
  stack->setFocus();
  mStatusBar = new KateVSStatusBar(this);

  mIsActiveSpace = false;
  mViewCount = 0;
}

KateViewSpace::~KateViewSpace()
{
}

void KateViewSpace::addView(Kate::View* v, bool show)
{
  uint id = mViewList.count();
  stack->addWidget(v, id);
  if (show) {
    mViewList.append(v);
    showView( v );
  }
  else {
    Kate::View* c = mViewList.current();
    mViewList.prepend( v );
    showView( c );
  }
}

void KateViewSpace::removeView(Kate::View* v)
{
  mStatusBar->slotClear ();
  mViewList.remove (v);
  stack->removeWidget (v);

  if (currentView() != 0L)
    stack->raiseWidget(mViewList.current());
  else if (mViewList.count() > 0)
    stack->raiseWidget(mViewList.last());
}

bool KateViewSpace::showView(Kate::View* v)
{
  Kate::Document* d = v->getDoc();
  QPtrListIterator<Kate::View> it (mViewList);

  it.toLast();
  for( ; it.current(); --it ) {
    if (it.current()->getDoc() == d) {
      Kate::View* kv = it.current();
      mViewList.removeRef( kv );
      mViewList.append( kv );
      kv->show();
      stack->raiseWidget( kv );
      return true;
    }
  }
   return false;
}

bool KateViewSpace::showView(uint documentNumber)
{
  QPtrListIterator<Kate::View> it (mViewList);

  it.toLast();
  for( ; it.current(); --it ) {
    if (((Kate::Document*)it.current()->getDoc())->documentNumber() == documentNumber) {
      Kate::View* kv = it.current();
      mViewList.removeRef( kv );
      mViewList.append( kv );
      kv->show();
      stack->raiseWidget( kv );
      return true;
    }
  }
   return false;
}


Kate::View* KateViewSpace::currentView()
{
  if (mViewList.count() > 0)
    return (Kate::View*)stack->visibleWidget();

  return 0L;
}

bool KateViewSpace::isActiveSpace()
{
  return mIsActiveSpace;
}

void KateViewSpace::setActive(bool b,  bool showled)
{
  mIsActiveSpace = b;

  // enable the painting of the icon and repaint it ;-)
  mStatusBar->showActiveViewIndicator( showled );
}

bool KateViewSpace::eventFilter(QObject* o, QEvent* e)
{
  if(o == stack && e->type() == QEvent::ChildRemoved) {
    if (mViewList.count() > 0)
      mViewList.current()->setFocus(); // trigger gotFocus sig!
  }
  return QWidget::eventFilter(o, e);
}

void KateViewSpace::slotStatusChanged (Kate::View *view, int r, int c, int ovr, bool block, int mod, QString msg)
{
  if ((QWidgetStack *)view->parentWidget() != stack)
    return;
  //if (!mIsActiveSpace) return;
  QString s1 = i18n("Line: %1").arg(KGlobal::locale()->formatNumber(r+1, 0));
  QString s2 = i18n("Col: %1").arg(KGlobal::locale()->formatNumber(c, 0));

  QString ovrstr;
  if (ovr == 0)
    ovrstr = i18n(" R/O ");
  if (ovr == 1)
     ovrstr = i18n(" OVR ");
  if (ovr == 2)
    ovrstr = i18n(" INS ");

  QString modstr;
  if (mod == 1)
    modstr = QString (" * ");
  else
    modstr = QString ("   ");
  QString blockstr;
  blockstr=block ? i18n(" BLK ") : i18n("NORM");
  mStatusBar->slotDisplayStatusText (" " + s1 + " " + s2 + " " + ovrstr + " " + blockstr+ " " + modstr + " " + msg);
}

void KateViewSpace::saveFileList( KSimpleConfig* config, int myIndex )
{
  config->setGroup( QString("viewspace%1").arg( myIndex ) );

  // Save file list, includeing cursor position in this instance.
  QPtrListIterator<Kate::View> it(mViewList);

  QStringList l;
  int idx = 0;
  for (; it.current(); ++it) {
    l.clear();
    if ( !it.current()->getDoc()->url().isEmpty() )
    {
      l << it.current()->getDoc()->url().prettyURL();
      l << QString("%1").arg( it.current()->cursorLine());
      l << QString("%1").arg( it.current()->cursorColumn() );
      config->writeEntry(QString("file%1").arg( idx ), l);
    }
    idx++;
  }
}


///////////////////////////////////////////////////////////
// KateVSStatusBar implementation
///////////////////////////////////////////////////////////

KateVSStatusBar::KateVSStatusBar ( KateViewSpace *parent, const char *name ) : QWidget( parent, name )
,m_yOffset(0)
,m_showLed( false )
{
   viewspace = parent;
   installEventFilter( this );
   m_pStatusLabel = new QLabel( this );
   m_pStatusLabel->show();
   m_pStatusLabel->installEventFilter(this);

   int h=fontMetrics().height()+2;
   if (h<13 ) h=13;
   setFixedHeight(h);
   m_yOffset=(h-13)/2;

   m_pStatusLabel->setGeometry(40,0,50,h);
}

KateVSStatusBar::~KateVSStatusBar ()
{
}


void KateVSStatusBar::showMenu()
{
   QPopupMenu *menu = (QPopupMenu*) ((KMainWindow *)topLevelWidget ())->factory()->container("viewspace_popup", (KMainWindow *)topLevelWidget ());
   menu->exec(QCursor::pos());
}

bool KateVSStatusBar::eventFilter(QObject*,QEvent *e)
{
   if (e->type()==QEvent::MouseButtonPress)
   {
      emit clicked();
      update();

      if ( ((KateViewSpace*)parentWidget())->currentView() )
        ((KateViewSpace*)parentWidget())->currentView()->setFocus();

      if ( ((QMouseEvent*)e)->button()==RightButton)
         showMenu();

      return true;
   }
   return false;
}

void KateVSStatusBar::slotDisplayStatusText (const QString& text)
{
   m_pStatusLabel->resize(fontMetrics().width(text),fontMetrics().height());
   m_pStatusLabel->setText(text);
}

void KateVSStatusBar::slotClear()
{
  slotDisplayStatusText( "" );
}

void KateVSStatusBar::showActiveViewIndicator( bool b )
{
    m_showLed = b;
    repaint();
}

void KateVSStatusBar::paintEvent(QPaintEvent* e)
{
   static QPixmap indicator_viewactive( UserIcon( "indicator_viewactive" ) );
   static QPixmap indicator_empty( UserIcon( "indicator_empty" ) );

   if (!isVisible()) return;

   QWidget::paintEvent(e);

   if (m_showLed)
   {
     QPainter p(this);
     p.drawPixmap(4,m_yOffset,viewspace->isActiveSpace() ? indicator_viewactive : indicator_empty);
   }
}
