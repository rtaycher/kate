
/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002,2003 Joseph Wenninger <jowenn@kde.org>

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

#ifndef _KATE_DOCK_CONTAINER_
#define _KATE_DOCK_CONTAINER_

#include <qwidget.h>
#include <kdockwidget.h>
#include <kdockwidget_p.h>
#include <qmap.h>

#include <qpushbutton.h>

class QWidgetStack;
class KMultiTabBar;
class KDockButton_Private;

class KateDockContainer: public QWidget, public KDockContainer
{
        Q_OBJECT
public:
        KateDockContainer(QWidget *parent, QWidget *win, int position);
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
        void collapseOverlapped();
private:                    
  	QWidget *m_mainWin;
	QWidgetStack *m_ws;
	KMultiTabBar *m_tb;
	int mTabCnt;
	int oldtab;
  	int m_position; 
	QMap<KDockWidget*,int> m_map;
	QMap<int,KDockWidget*> m_revMap;
	QMap<KDockWidget*,KDockButton_Private*> m_overlapButtons;
	QStringList itemNames;
	int m_inserted;
	int m_delayedRaise;
	bool m_vertical;
        bool m_block;
protected slots:
	void tabClicked(int);
	void delayedRaise();
	void changeOverlapMode();
};


/* THIS IS GOING TO BE REMOVED ONCE THAT CONTAINER IS IN KDELIBS. It's a copy of a private header
*/
class KDockButton_Private : public QPushButton
{
  Q_OBJECT
public:
  KDockButton_Private( QWidget *parent=0, const char *name=0 );
  ~KDockButton_Private();

protected:
  virtual void drawButton( QPainter * );
  virtual void enterEvent( QEvent * );
  virtual void leaveEvent( QEvent * );

private:
  bool moveMouse;
};

#endif
