/***************************************************************************
                          kantviewspace.cpp  -  description
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

#include "kantviewspace.h"
#include "kantviewspace.moc"

#include "../mainwindow/kantmainwindow.h"
#include "../document/kantdocument.h"

#include <kiconloader.h>
#include <klocale.h>
#include <kstatusbar.h>
#include <qwidgetstack.h>
#include <kdebug.h>
#include <ksimpleconfig.h>

KantViewSpace::KantViewSpace(QWidget* parent, const char* name)
  : QVBox(parent, name)
{
   mViewList.setAutoDelete(false);

  stack = new QWidgetStack( this );
  setStretchFactor(stack, 1);
  stack->installEventFilter( this );
  stack->setFocus();
  mStatusBar = new KantVSStatusBar(this);

  mIsActiveSpace = false;
  mViewCount = 0;
}

KantViewSpace::~KantViewSpace()
{
}

void KantViewSpace::addView(KantView* v, bool show)
{
  int id = mViewList.count();
  stack->addWidget(v, id);
  if (show) {
    mViewList.append(v);
    showView( v );
  }
  else {
    KantView* c = mViewList.current();
    //kdDebug()<<"KantViewSpace::addView(): showing current view "<< c->doc()->url().path()<<endl;
    mViewList.prepend( v );
    showView( c );
  }
}

void KantViewSpace::removeView(KantView* v)
{
  mStatusBar->slotClear ();
  mViewList.remove (v);
  stack->removeWidget (v);

//  if (mViewList.current() != 0L)
  if (currentView() != 0L)
    stack->raiseWidget(mViewList.current());
  else if (mViewList.count() > 0)
    stack->raiseWidget(mViewList.last());
}

bool KantViewSpace::showView(KantView* v)
{
  KWriteDoc* d = v->doc();
  QListIterator<KantView> it (mViewList);
  it.toLast();
  for( ; it.current(); --it ) {
    if (it.current()->doc() == d) {
      KantView* kv = it.current();
      mViewList.removeRef( kv );
      mViewList.append( kv );
      kv->show();
      stack->raiseWidget( kv );
      return true;
    }
  }
   return false;
}

bool KantViewSpace::showView(int docID)
{
  QListIterator<KantView> it (mViewList);
  it.toLast();
  for( ; it.current(); --it ) {
    if (((KantDocument*)it.current()->doc())->docID() == docID) {
      KantView* kv = it.current();
      mViewList.removeRef( kv );
      mViewList.append( kv );
      kv->show();
      stack->raiseWidget( kv );
      return true;
    }
  }
   return false;
}


KantView* KantViewSpace::currentView()
{
  if (mViewList.count() > 0) {
    //kdDebug()<<"KantViewSpace::currentView(): "<<((KantView*)stack->visibleWidget())->doc()->url().filename()<<endl;
    return (KantView*)stack->visibleWidget();
  }
  return 0L;
}

bool KantViewSpace::isActiveSpace()
{
  return mIsActiveSpace;
}

void KantViewSpace::setActive(bool b)
{
  mIsActiveSpace = b;

  // enable the painting of the icon and repaint it ;-)
  mStatusBar->showActiveViewIndicator( b );
}

bool KantViewSpace::eventFilter(QObject* o, QEvent* e)
{
  if(o == stack && e->type() == QEvent::ChildRemoved) {
    if (mViewList.count() > 0)
      mViewList.current()->setFocus(); // trigger gotFocus sig!
  }
  return QWidget::eventFilter(o, e);
}

void KantViewSpace::slotStatusChanged (KantView *view, int r, int c, int ovr, int mod, QString msg)
{
  if ((QWidgetStack *)view->parentWidget() != stack)
    return;
  //if (!mIsActiveSpace) return;
  QString s1 = i18n("Line: %1").arg(KGlobal::locale()->formatNumber(r, 0));
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
  mStatusBar->slotDisplayStatusText (" " + s1 + " " + s2 + " " + ovrstr + " " + modstr + " " + msg);
}

void KantViewSpace::saveFileList( KSimpleConfig* config, int myIndex )
{
  config->setGroup( QString("viewspace%1").arg( myIndex ) );
  // Save file list, includeing cursor position in this instance.
  QListIterator<KantView> it(mViewList);
  QStringList l;
  for (; it.current(); ++it) { // FIXME: SHOULD BE REVERSE!!!
    // TODO: Save cursor pos.
    if ( !it.current()->doc()->url().isEmpty() )
      l << it.current()->doc()->url().prettyURL();
  }
  config->writeEntry("documents", l);
}


///////////////////////////////////////////////////////////
// KantVSStatusBar implementation
///////////////////////////////////////////////////////////

KantVSStatusBar::KantVSStatusBar ( KantViewSpace *parent, const char *name ) : QWidget( parent, name )
,m_yOffset(0)
,m_showLed(true)
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

KantVSStatusBar::~KantVSStatusBar ()
{
}


void KantVSStatusBar::showMenu()
{
   QPopupMenu *menu = (QPopupMenu*) ((KMainWindow *)topLevelWidget ())->factory()->container("viewspace_popup", (KMainWindow *)topLevelWidget ());
   menu->exec(QCursor::pos());
}

bool KantVSStatusBar::eventFilter(QObject*,QEvent *e)
{
   if (e->type()==QEvent::MouseButtonPress)
   {
      emit clicked();
      update();

      if ( ((KantViewSpace*)parentWidget())->currentView() )
        ((KantViewSpace*)parentWidget())->currentView()->setFocus();

      if ( ((QMouseEvent*)e)->button()==RightButton)
         showMenu();

      return true;
   }
   return false;
}

void KantVSStatusBar::slotDisplayStatusText (const QString& text)
{
   m_pStatusLabel->resize(fontMetrics().width(text),fontMetrics().height());
   m_pStatusLabel->setText(text);
}

void KantVSStatusBar::slotClear()
{
  slotDisplayStatusText( "" );
}

void KantVSStatusBar::showActiveViewIndicator( bool b )
{
    m_showLed = b;
    repaint();
}

void KantVSStatusBar::paintEvent(QPaintEvent* e)
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
