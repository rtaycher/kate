/***************************************************************************
                          kantstacktabwidget.h  -  description
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

#ifndef _K_STACK_WIDGET_H_
#define _K_STACK_WIDGET_H_

#include <qwidget.h>
#include <qlist.h>
#include <qstring.h>
#include <qpushbutton.h>

class KantStackTabWidgetButton;

class KantStackTabWidget : public QWidget
{
	Q_OBJECT
	public:
		KantStackTabWidget(QWidget *parent, const char* name,bool stacked=true);
                void addPage(QWidget *wid, QString header);
                void showPage(QWidget *wid);
                void showPage(int id);
	        void setMode(bool mode);
	private:
		class QWidgetStack *internalView;
		QList<KantStackTabWidgetButton> buttons;
                class QVBox *topWidget;
                class QWidget *bottomBox;
                class QVBoxLayout *BottomLayout;
		class QTabBar *tabbar;
		bool _stacking;
                int lastID;
                bool internal_updated;
        private slots:
                void selected_tabbar(int id);
                void selected_button(int id);
};

class KantStackTabWidgetButton : public QPushButton
{
        Q_OBJECT
        public:
                KantStackTabWidgetButton(const QString &, QWidget *,int);
                int getID();
        private:
                int _id;
        private slots:
                void _clicked();
        signals:
                void clicked(int);
};

#endif
