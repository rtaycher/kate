#include "katedockcontainer.h"
#include <qwidgetstack.h>
#include <qlayout.h>
#include <kmultiverttabbar.h>
#include <kdebug.h>
#include <kiconloader.h>                       
#include <kapplication.h>
#include <kconfig.h>
#include "katemainwindow.h"
#include <qtimer.h>

KateDockContainer::KateDockContainer(QWidget *parent, class KateMainWindow *win, int position):QWidget(parent),KDockContainer()
{         
	m_inserted=-1;
	m_mainWin = win;
	oldtab=-1;
	mTabCnt=0;
	m_position = position;
	
	QBoxLayout *l;
	if ((position==KDockWidget::DockTop) || (position==KDockWidget::DockBottom))
	l=new QVBoxLayout(this);
	else
	l=new QHBoxLayout(this);
	
	l->setAutoAdd(false);

	m_tb=new KMultiTabBar(this,((position==KDockWidget::DockTop) || (position==KDockWidget::DockBottom))?
		KMultiTabBar::Horizontal:KMultiTabBar::Vertical);
	m_tb->setPosition((position==KDockWidget::DockLeft)?KMultiTabBar::Left:
		(position==KDockWidget::DockBottom)?KMultiTabBar::Bottom:KMultiTabBar::Right);

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
//	parentDockWidget()->setForcedFixedWidth(m_tb->width());

}

KateDockContainer::~KateDockContainer()
{
}


void KateDockContainer::init()
{
	parentDockWidget()->setForcedFixedWidth(m_tb->width());	
	
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
	}
	else
	{
		tab=m_ws->addWidget(w);
		m_map.insert(w,tab);
		m_tb->insertTab(pixmap.isNull()?SmallIcon("misc"):pixmap,tab);
		m_tb->setTab(tab,true);
		connect(m_tb->getTab(tab),SIGNAL(clicked(int)),this,SLOT(tabClicked(int)));
		kdDebug()<<"KateDockContainer::insertWidget()"<<endl;
		m_tb->setTab(oldtab,false);
		mTabCnt++;
		m_inserted=tab;
		int dummy=0;
		tabClicked(tab);
		KDockContainer::insertWidget(w,pixmap,text,dummy);
	}
        m_ws->raiseWidget(tab);
	
	//if (!alreadyThere)
	//{
	///}
}

void KateDockContainer::removeWidget(KDockWidget* w)
{
	if (!m_map.contains(w)) return;
	int id=m_map[w];
	m_tb->setTab(id,false);
	tabClicked(id);
	m_tb->removeTab(id);
	m_map.remove(w);
	KDockContainer::removeWidget(w);
//	m_ws->removeWidget(w);
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
//      parentDockWidget()->manualDock(m_mainWin->centralDock(), KDockWidget::DockLeft,20);
    }
  
		m_ws->raiseWidget(t);
		if (oldtab!=t) m_tb->setTab(oldtab,false);
		oldtab=t;	
	}
	else
	{
//		oldtab=-1;
    m_ws->hide ();
//    parentDockWidget()->manualDock(m_mainWin->centralDock(), KDockWidget::DockLeft,0);
	kdDebug()<<"Fixed Width:"<<m_tb->width()<<endl;
	parentDockWidget()->setForcedFixedWidth(m_tb->width());

 	}
}

void KateDockContainer::setToolTip (KDockWidget *, QString &)
{
	;
}

void KateDockContainer::save(KConfig*)
{
	KConfig *cfg=kapp->config();
	QString grp=cfg->group();
	cfg->deleteGroup(QString("BLAH::%1").arg(parent()->name()));
	cfg->setGroup(QString("BLAH::%1").arg(parent()->name()));
	
	QPtrList<KMultiTabBarTab>* tl=m_tb->tabs();
	QPtrListIterator<KMultiTabBarTab> it(*tl);
	int i=0;
	for (;it.current()!=0;++it)
	{
		cfg->writeEntry(QString("widget%1").arg(i),m_ws->widget(it.current()->id())->name());	
		kdDebug()<<"****************************************Saving: "<<m_ws->widget(it.current()->id())->name()<<endl;
		if (m_tb->isTabRaised(it.current()->id()))
			cfg->writeEntry(m_ws->widget(it.current()->id())->name(),true);
	++i;
	}	
	cfg->sync();
	cfg->setGroup(grp);

}
  
void KateDockContainer::load(KConfig*)
{
	//m_map.clear();
	//m_tb->
	KConfig *cfg=kapp->config();
	QString grp=cfg->group();	
	cfg->setGroup(QString("BLAH::%1").arg(parent()->name()));
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
	parentDockWidget()->setForcedFixedWidth(m_tb->width());
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
				//QTimer::singleShot(0,parentDockWidget(),SLOT(restoreFromForcedFixedSize()));
				//parentDockWidget()->setForcedFixedWidth(100);
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
//	m_ws->raiseWidget(m_delayedRaise);
//	m_ws->show();
//	parentDockWidget()->setForcedFixedWidth(100);
}

#include "katedockcontainer.moc"
