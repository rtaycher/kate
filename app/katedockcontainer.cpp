#include "katedockcontainer.h"
#include <qwidgetstack.h>
#include <qlayout.h>
#include <kmultiverttabbar.h>
#include <kdebug.h>
#include <kiconloader.h>

KateDockContainer::KateDockContainer(QWidget *parent):QWidget(parent),KDockContainer()
{
	oldtab=-1;
	mTabCnt=0;
	QHBoxLayout *l=new QHBoxLayout(this);
	m_ws=new QWidgetStack(this);

	m_ws->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));

	l->add(m_ws);
	m_tb=new KMultiVertTabBar(this);
	l->add(m_tb);
	l->activate();
}

KateDockContainer::~KateDockContainer()
{
}


KDockWidget *KateDockContainer::parentDockWidget(){return ((KDockWidget*)parent());}
    
void KateDockContainer::insertWidget (KDockWidget *w, QPixmap pixmap, const QString &text, int &)
{
	int tab;
	tab=m_ws->addWidget(w);
	m_tb->insertTab(pixmap.isNull()?SmallIcon("misc"):pixmap,tab);
	m_tb->setTab(tab,true);
	connect(m_tb->getTab(tab),SIGNAL(clicked(int)),this,SLOT(tabClicked(int)));
	kdDebug()<<"KateDockContainer::insertWidget()"<<endl;
	m_tb->setTab(oldtab,false);
	oldtab=tab;
	mTabCnt++;
}

void KateDockContainer::tabClicked(int t)
{

	kdDebug()<<"KateDockContainer::tabClicked()"<<endl;

	if (m_tb->isTabRaised(t))
	{
		m_ws->raiseWidget(t);
		m_tb->setTab(oldtab,false);
		oldtab=t;	
	}
	else
	{
		oldtab=-1;
		//parentDockWidget()->resize(20,parentDockWidget()->height());
	}
}

void KateDockContainer::setToolTip (KDockWidget *, QString &)
{
	;
}
#include "katedockcontainer.moc"
