/***************************************************************************
                          katestacktabwidget.cpp  -  description
                             -------------------
    begin                : Sat 31 March 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katestacktabwidget.h"
#include "katestacktabwidget.moc"

#include <qtabbar.h>
#include <qwidgetstack.h>
#include <qlayout.h>
#include <qvbox.h>

KateStackTabWidgetButton::KateStackTabWidgetButton(const QString& text, QWidget *parent, int id):QPushButton(text,parent)
  {
    _id=id;
    connect(this,SIGNAL(clicked()),this,SLOT(_clicked()));
    setFixedHeight(fontMetrics().height() + 4);
  }

void KateStackTabWidgetButton::_clicked()
  {
    emit clicked(_id);
  }

int KateStackTabWidgetButton::getID(){return _id;}

KateStackTabWidget::KateStackTabWidget(QWidget *parent,const char *name,bool stacked) : QWidget(parent,name)
{
  lastID=0;
  _stacking=stacked;
  internal_updated=false;
  QVBoxLayout *l=(new QVBoxLayout(this));//->setAutoAdd(true);
      l->insertWidget(-1,tabbar=new QTabBar(this));
      connect(tabbar,SIGNAL(selected(int)),this,SLOT(selected_tabbar(int)));
      l->insertWidget(-1,topWidget=new QVBox(this));
      l->insertWidget(-1,internalView=new QWidgetStack(this),1);
      l->insertWidget(-1,bottomBox=new QWidget(this));
      BottomLayout=new QVBoxLayout(bottomBox);
      BottomLayout->setAutoAdd(true);
      BottomLayout->setDirection(QBoxLayout::BottomToTop);
      bottomBox->setSizePolicy( QSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum ) );
  if (_stacking)
    {
      tabbar->hide();
    }
  else
    {
      bottomBox->hide();
      topWidget->hide();
    }

}

void KateStackTabWidget::addPage(QWidget *wid,QString header)
{
   int id;
      internalView->addWidget(wid,id=tabbar->addTab(new QTab(header)));
        KateStackTabWidgetButton *b;
        BottomLayout->setAutoAdd(false);
        buttons.append(b=new KateStackTabWidgetButton(header,bottomBox,id));
        BottomLayout->setAutoAdd(true);
        connect(b,SIGNAL(clicked(int)),this,SLOT(selected_button(int)));
	showPage(wid);
}

void KateStackTabWidget::selected_tabbar(int id)
{
  internalView->raiseWidget(id);
}

void KateStackTabWidget::selected_button(int id)
{
  bool undefined_top=false;
  if (!internal_updated) internalView->raiseWidget(id);
  if (buttons.current()==0) {undefined_top=true; buttons.first();}
  if (buttons.current()==0) return;
  if ((id<(buttons.current())->getID()) && (undefined_top)) return;
  if (id<buttons.current()->getID())
    {
      for (;(buttons.current()!=0) && (id!=buttons.current()->getID());buttons.prev())
        {
          buttons.current()->reparent(bottomBox,QPoint(0,0),true);
        }
      return;
    }
  for (;(buttons.current()!=0) && (buttons.current()->getID()<=id); buttons.next())
    {
      buttons.current()->reparent(topWidget,QPoint(0,0),true);
      if (buttons.current()->getID()==id) return;
    }
}

void KateStackTabWidget::showPage(QWidget *wid)
  {
    internalView->raiseWidget(wid);
    internal_updated=true;
    if (!_stacking) tabbar->setCurrentTab(internalView->id(wid));
      else selected_button(internalView->id(wid));
    internal_updated=false;
  }

void KateStackTabWidget::showPage(int id)
  {
	internalView->raiseWidget(id);
	internal_updated=true;
        if (!_stacking) tabbar->setCurrentTab(id);
	  else selected_button(id);
        internal_updated=false;
  }

void KateStackTabWidget::setMode(bool mode)
{
  if (mode==_stacking) return;
  _stacking=mode;
  if (_stacking)
  {
	tabbar->hide();
	topWidget->show();
	bottomBox->show();
	showPage(tabbar->currentTab());
  }
  else
  {
        tabbar->show();
        topWidget->hide();
        bottomBox->hide();
        if (buttons.current()==0) return;
	showPage(buttons.current()->getID());

  }
}
