/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include "katedockcontainer.h"
#include <qwidgetstack.h>
#include <qlayout.h>
#include <kmultitabbar.h>
#include <kdebug.h>
#include <kiconloader.h>                       
#include <kapplication.h>
#include <kconfig.h>
#include "katemainwindow.h"
#include <qtimer.h>

KateDockContainer::KateDockContainer(QWidget *parent, class KateMainWindow *win, int position):QWidget(parent),KDockContainer()
{         
	m_block=false;
	m_inserted=-1;
	m_mainWin = win;
	oldtab=-1;
	mTabCnt=0;
	m_position = position;
	
	QBoxLayout *l;
	m_vertical=!((position==KDockWidget::DockTop) || (position==KDockWidget::DockBottom));

	if (!m_vertical)
	l=new QVBoxLayout(this);
	else
	l=new QHBoxLayout(this);
	
	l->setAutoAdd(false);

	m_tb=new KMultiTabBar(this,((position==KDockWidget::DockTop) || (position==KDockWidget::DockBottom))?
		KMultiTabBar::Horizontal:KMultiTabBar::Vertical);
	m_tb->showActiveTabTexts(true);
	m_tb->setPosition((position==KDockWidget::DockLeft)?KMultiTabBar::Left:
		(position==KDockWidget::DockBottom)?KMultiTabBar::Bottom:
		(position==KDockWidget::DockTop)?KMultiTabBar::Top:KMultiTabBar::Right);

	m_ws=new QWidgetStack(this);

	m_ws->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
 
	if ( (position==KDockWidget::DockLeft) || (position==KDockWidget::DockTop))
	{
		l->add(m_tb);
		l->add(m_ws);
	}
	else
	{
		l->add(m_ws);
		l->add(m_tb);
	}

	l->activate();
	m_ws->hide();

}

KateDockContainer::~KateDockContainer()
{
}


void KateDockContainer::init()
{
	if (m_vertical)
	parentDockWidget()->setForcedFixedWidth(m_tb->width());	
	else
	parentDockWidget()->setForcedFixedHeight(m_tb->height());

}


KDockWidget *KateDockContainer::parentDockWidget(){return ((KDockWidget*)parent());}
    
void KateDockContainer::insertWidget (KDockWidget *w, QPixmap pixmap, const QString &text, int &)
{
	int tab;
	bool alreadyThere=m_map.contains(w);
	if (alreadyThere)
	{
		tab=m_map[w];
		if (m_ws->addWidget(w,tab)!=tab) kdDebug()<<"ERROR COULDN'T READD WIDGET************"<<endl;
		kdDebug()<<"READDED WIDGET***********************************"<<endl;
		m_tb->setTab(tab,true);
		tabClicked(tab);
	}
	else
	{
		tab=m_ws->addWidget(w);
		m_map.insert(w,tab);
		m_revMap.insert(tab,w);
		m_tb->appendTab(pixmap.isNull()?SmallIcon("misc"):pixmap,tab,w->tabPageLabel());
		kdDebug()<<"NAMENAMENAMENAME:===========================:"<<w->tabPageLabel()<<endl;
		m_tb->setTab(tab,true);
		connect(m_tb->tab(tab),SIGNAL(clicked(int)),this,SLOT(tabClicked(int)));
		kdDebug()<<"KateDockContainer::insertWidget()"<<endl;
		m_tb->setTab(oldtab,false);
		mTabCnt++;
		m_inserted=tab;
		int dummy=0;
		tabClicked(tab);
		KDockContainer::insertWidget(w,pixmap,text,dummy);
		itemNames.append(w->name());
	}
        m_ws->raiseWidget(tab);
		
}

void KateDockContainer::removeWidget(KDockWidget* w)
{
	if (!m_map.contains(w)) return;
	int id=m_map[w];
	m_tb->setTab(id,false);
	tabClicked(id);
	m_tb->removeTab(id);
	m_map.remove(w);
	m_revMap.remove(id);
	KDockContainer::removeWidget(w);
	itemNames.remove(w->name());
}

void KateDockContainer::undockWidget(KDockWidget *w)
{
	if (!m_map.contains(w)) return;
	kdDebug()<<"Wiget has been undocked, setting tab down"<<endl;
	int id=m_map[w];
	m_tb->setTab(id,false);
	tabClicked(id);
}

void KateDockContainer::tabClicked(int t)
{

	kdDebug()<<"KateDockContainer::tabClicked()"<<endl;

	if (m_tb->isTabRaised(t))
	{         
    if (m_ws->isHidden())
    {                        
       m_ws->show ();
	parentDockWidget()->restoreFromForcedFixedSize();
    }
  		if (!m_ws->widget(t))
		{
			m_revMap[t]->manualDock(parentDockWidget(),KDockWidget::DockCenter,20);
			return;
		}
		m_ws->raiseWidget(t);
		if (oldtab!=t) m_tb->setTab(oldtab,false);
		oldtab=t;	
	}
	else
	{
//		oldtab=-1;
    if (m_block) return;
    m_block=true;
    if (m_ws->widget(t)) 
    {
//		((KDockWidget*)m_ws->widget(t))->undock();
    }
    m_block=false;
    m_ws->hide ();
	kdDebug()<<"Fixed Width:"<<m_tb->width()<<endl;
	if (m_vertical)
	parentDockWidget()->setForcedFixedWidth(m_tb->width());
	else
	parentDockWidget()->setForcedFixedHeight(m_tb->height());
 	}
}

void KateDockContainer::setToolTip (KDockWidget *, QString &s)
{
	kdDebug()<<"***********************************Setting tooltip for a widget: "<<s<<endl;
	;
}

void KateDockContainer::save(KConfig*)
{
	KConfig *cfg=kapp->config();
	QString grp=cfg->group();
	cfg->deleteGroup(QString("KateDock::%1").arg(parent()->name()));
	cfg->setGroup(QString("KateDock::%1").arg(parent()->name()));
	
	QPtrList<KMultiTabBarTab>* tl=m_tb->tabs();
	QPtrListIterator<KMultiTabBarTab> it(*tl);
	QStringList::Iterator it2=itemNames.begin();
	int i=0;
	for (;it.current()!=0;++it,++it2)
	{
//		cfg->writeEntry(QString("widget%1").arg(i),m_ws->widget(it.current()->id())->name());	
		cfg->writeEntry(QString("widget%1").arg(i),(*it2));
//		kdDebug()<<"****************************************Saving: "<<m_ws->widget(it.current()->id())->name()<<endl;
		if (m_tb->isTabRaised(it.current()->id()))
			cfg->writeEntry(m_ws->widget(it.current()->id())->name(),true);
	++i;
	}	
	cfg->sync();
	cfg->setGroup(grp);

}
  
void KateDockContainer::load(KConfig*)
{
	KConfig *cfg=kapp->config();
	QString grp=cfg->group();	
	cfg->setGroup(QString("KateDock::%1").arg(parent()->name()));
	int i=0;
	QString raise;
	while (true)
	{
		QString dwn=cfg->readEntry(QString("widget%1").arg(i));
		if (dwn.isEmpty()) break;
		kdDebug()<<"*************************************************************Configuring dockwidget :"<<dwn<<endl;
		KDockWidget *dw=((KDockWidget*)parent())->dockManager()->getDockWidgetFromName(dwn);
		if (dw)
		{
			dw->manualDock((KDockWidget*)parent(),KDockWidget::DockCenter);
		}
		if (cfg->readBoolEntry(dwn,false)) raise=dwn;
		i++;
		
	}
	
	QPtrList<KMultiTabBarTab>* tl=m_tb->tabs();
	QPtrListIterator<KMultiTabBarTab> it1(*tl);
	m_ws->hide();
	if (m_vertical)
	parentDockWidget()->setForcedFixedWidth(m_tb->width());
	else
	parentDockWidget()->setForcedFixedHeight(m_tb->height());
	for (;it1.current()!=0;++it1)
	{
		m_tb->setTab(it1.current()->id(),false);
	}
	kapp->syncX();
	m_delayedRaise=-1;
	if (!raise.isEmpty())
	{
		for (QMap<KDockWidget*,int>::iterator it=m_map.begin();it!=m_map.end();++it)
		{

			if (it.key()->name()==raise)
			{
/*				tabClicked(it.data());	
				m_tb->setTab(it.data(),true);
				tabClicked(it.data());	
				m_ws->raiseWidget(it.key());
				kapp->sendPostedEvents();
				kapp->syncX();*/
				m_delayedRaise=it.data();
				QTimer::singleShot(0,this,SLOT(delayedRaise()));
				kdDebug()<<"************** raising *******: "<<it.key()->name()<<endl;
				break;
			}
		}
		
	}
	if (m_delayedRaise==-1) 	QTimer::singleShot(0,this,SLOT(init()));
	cfg->setGroup(grp);
	
}

void KateDockContainer::delayedRaise()
{
				m_tb->setTab(m_delayedRaise,true);
				tabClicked(m_delayedRaise);
}

#include "katedockcontainer.moc"
