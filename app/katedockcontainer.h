/***************************************************************************
                          katedockcontainer.h
                          Dock Container implementation for kate
                             -------------------
    begin                : June 2002
    copyright            : (C) 2002 by Joseph Wenninger <jowenn@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATE_DOCK_CONTAINER_
#define _KATE_DOCK_CONTAINER_

#include <qwidget.h>
#include <kdockwidget.h>
#include <kdockwidget_p.h>
#include <qmap.h>

class QWidgetStack;
class KMultiTabBar;

class KateDockContainer: public QWidget, public KDockContainer
{
        Q_OBJECT
public:
        KateDockContainer(QWidget *parent, class KateMainWindow *win, int position);
        virtual ~KateDockContainer();
        KDockWidget *parentDockWidget();
        virtual void insertWidget (KDockWidget *w, QPixmap, const QString &, int &);
        virtual void setToolTip (KDockWidget *, QString &);
	virtual void undockWidget(KDockWidget*);
	virtual void removeWidget(KDockWidget*);	
	virtual void save(KConfig *);
	virtual void load(KConfig *);
public slots:
	void init();
private:                    
  class KateMainWindow *m_mainWin;
	QWidgetStack *m_ws;
	KMultiTabBar *m_tb;
	int mTabCnt;
	int oldtab;
  	int m_position; 
	QMap<KDockWidget*,int> m_map;
	QMap<int,KDockWidget*> m_revMap;
	QStringList itemNames;
	int m_inserted;
	int m_delayedRaise;
	bool m_vertical;
        bool m_block;
protected slots:
	void tabClicked(int);
	void delayedRaise();
};

#endif
