#include "katedockcontainer.h"
#include <qwidgetstack.h>
#include <qlayout.h>
#include <kmultiverttabbar.h>
#include <kdebug.h>
#include <kiconloader.h>                       

#include "katemainwindow.h"

KateDockContainer::KateDockContainer(QWidget *parent, class KateMainWindow *win, int position):QWidget(parent),KDockContainer()
{         
  	m_mainWin = win;
	oldtab=-1;
	mTabCnt=0;
	m_position = position;
	
	QHBoxLayout *l=new QHBoxLayout(this);
	l->setAutoAdd(false);

	m_tb=new KMultiVertTabBar(this);
	m_tb->setPosition((position==KDockWidget::DockLeft)?KMultiVertTabBar::Left:KMultiVertTabBar::Right);

	m_ws=new QWidgetStack(this);

	m_ws->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
 
	if (position==KDockWidget::DockLeft)
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


KDockWidget *KateDockContainer::parentDockWidget(){return ((KDockWidget*)parent());}
    
void KateDockContainer::insertWidget (KDockWidget *w, QPixmap pixmap, const QString &text, int &)
{
	int tab;
	tab=m_ws->addWidget(w);
        m_ws->raiseWidget(tab);
	m_map.insert(w,tab);
	m_tb->insertTab(pixmap.isNull()?SmallIcon("misc"):pixmap,tab);
	m_tb->setTab(tab,true);
	connect(m_tb->getTab(tab),SIGNAL(clicked(int)),this,SLOT(tabClicked(int)));
	kdDebug()<<"KateDockContainer::insertWidget()"<<endl;
	m_tb->setTab(oldtab,false);
	oldtab=tab;
	mTabCnt++;
}

void KateDockContainer::removeWidget(KDockWidget* w)
{
	int id=m_map[w];
	m_tb->setTab(id,false);
	tabClicked(id);
	m_tb->removeTab(id);
//	m_ws->removeWidget(w);
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
		m_tb->setTab(oldtab,false);
		oldtab=t;	
	}
	else
	{
		oldtab=-1;
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
#include "katedockcontainer.moc"
