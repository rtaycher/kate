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


	m_tb=new KMultiVertTabBar(this);
	m_tb->setPosition(KMultiVertTabBar::Left);
	l->add(m_tb);

	m_ws=new QWidgetStack(this);

	m_ws->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
 
	l->add(m_ws);

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
        m_ws->raiseWidget(tab);
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
    if (m_ws->isHidden())
    {                        
       m_ws->show ();
      parentDockWidget()->manualDock(m_mainWin->centralDock(), KDockWidget::DockLeft,20);
    }
  
		m_ws->raiseWidget(t);
		m_tb->setTab(oldtab,false);
		oldtab=t;	
	}
	else
	{
		oldtab=-1;
    m_ws->hide ();
    parentDockWidget()->manualDock(m_mainWin->centralDock(), KDockWidget::DockLeft,0);
 	}
}

void KateDockContainer::setToolTip (KDockWidget *, QString &)
{
	;
}
#include "katedockcontainer.moc"
