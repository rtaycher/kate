/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_MDI_H__
#define __KATE_MDI_H__

#include <kparts/mainwindow.h>

#include <kmultitabbar.h>
#include <ktabwidget.h>

#include <qdict.h>
#include <qintdict.h>
#include <qmap.h>
#include <qsplitter.h>
#include <qpixmap.h>

namespace KateMDI {

class Sidebar : public KMultiTabBar
{
  Q_OBJECT

  public:
    Sidebar (KMultiTabBar::KMultiTabBarPosition pos, QWidget *parent);
    ~Sidebar ();

    void setSplitter (QSplitter *sp);

    void setResizeMode(QSplitter::ResizeMode mode);

  public:
    bool addWidget (const QPixmap &icon, const QString &text, QWidget *widget);
    bool removeWidget (QWidget *widget);

    bool showWidget (QWidget *widget);
    bool hideWidget (QWidget *widget);

  private slots:
    void tabClicked(int);

  private:
    KMultiTabBar::KMultiTabBarPosition m_pos;
    QSplitter *m_splitter;
    KMultiTabBar *m_tabBar;
    QWidget *m_ownSplit;

    QIntDict<QWidget> m_idToWidget;
    QMap<QWidget*, int> m_widgetToId;
};

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  public:
    MainWindow ();
    ~MainWindow ();

    KTabWidget *tabWidget () { return m_tabWidget; }

    // add a given widget to the given sidebar if possible, name is very important
    bool addToolView (const QString &identifier, QWidget *widget, KMultiTabBar::KMultiTabBarPosition pos, const QPixmap &icon, const QString &text);

    // remove the toolview out of the sidebars + delete it
    bool deleteToolView (QWidget *widget);

    // move a toolview to given new pos
    bool moveToolView (QWidget *widget, KMultiTabBar::KMultiTabBarPosition pos);

    // show given toolview
    bool showToolView (QWidget *widget);

    // hide given toolview
    bool hideToolView (QWidget *widget);

    // set the sidebar's resize mode.
    void setSidebarResizeMode(KMultiTabBar::KMultiTabBarPosition pos, QSplitter::ResizeMode mode);

  private:
    class WidgetData
    {
      public:
        QPixmap icon;
        QString text;
    };

    QDict<QWidget> m_idToWidget;
    QMap<QWidget*, QString> m_widgetToId;
    QMap<QWidget*, WidgetData> m_widgetToData;

    QMap<QWidget*, int> m_widgetToSide;

    KTabWidget *m_tabWidget;
    QSplitter *m_hSplitter;
    QSplitter *m_vSplitter;

    Sidebar *m_sidebars[4];
};

}

#endif
