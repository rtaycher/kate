#ifndef _KATE_DOCK_CONTAINER_
#define _KATE_DOCK_CONTAINER_

#include <qwidget.h>
#include <kdockwidget.h>

class QWidgetStack;
class KMultiVertTabBar;

class KateDockContainer: public QWidget, public KDockContainer
{
        Q_OBJECT
public:
        KateDockContainer(QWidget *parent, class KateMainWindow *win, int position);
        virtual ~KateDockContainer();
        KDockWidget *parentDockWidget();
        void insertWidget (KDockWidget *w, QPixmap, const QString &, int &);
        void setToolTip (KDockWidget *, QString &);
private:                    
  class KateMainWindow *m_mainWin;
	QWidgetStack *m_ws;
	KMultiVertTabBar *m_tb;
	int mTabCnt;
	int oldtab;
  int m_position; 
protected slots:
	void tabClicked(int);
};

#endif
