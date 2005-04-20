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

/**
  * internal data holder class, used to have some
  * additional infos to the toolview widgets ;)
  */
class WidgetData
{
  public:
    QString id;
    KMultiTabBar::KMultiTabBarPosition pos;
    bool visible;
    QPixmap icon;
    QString text;
};

class Sidebar : public KMultiTabBar
{
  Q_OBJECT

  public:
    Sidebar (KMultiTabBar::KMultiTabBarPosition pos, QWidget *parent, QMap<QWidget*, WidgetData> &widgetToData);
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

    // more widget data
    QMap<QWidget*, WidgetData> &m_widgetToData;
};

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  //
  // Constructor area
  //
  public:
    /**
     * Constructor
     */
    MainWindow ();

    /**
     * Destructor
     */
    virtual ~MainWindow ();

  //
  // public interfaces
  //
  public:
    /**
     * central tabwidget ;)
     * @return tab widget
     */
    KTabWidget *tabWidget ();

    /**
     * add a given widget to the given sidebar if possible, name is very important
     * @param identifier unique identifier for this toolview
     * @param widget widget to insert as toolview
     * @param pos position for the toolview, if we are in session restore, this is only a preference
     * @param icon icon to use for the toolview
     * @param test text to use in addition to icon
     * @return success
     */
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

  //
  // session saving and restore stuff
  //
  public:
    /**
     * start the restore
     * @param config config object to use
     * @param group config group to use
     */
    void startRestore (KConfig *config, const QString &group);

    /**
     * finish the restore
     */
    void finishRestore ();

    /**
     * save the current session config to given object and group
     * @param config config object to use
     * @param group config group to use
     */
    void saveSession (KConfig *config, const QString &group);

  //
  // internal data ;)
  //
  private:
    /**
     * map identifiers to widgets
     */
    QDict<QWidget> m_idToWidget;

    /**
     * map widgets to the additional data set ;)
     */
    QMap<QWidget*, WidgetData> m_widgetToData;

    /**
     * tab widget, which is the central part of the
     * main window ;)
     */
    KTabWidget *m_tabWidget;

    /**
     * horizontal splitter
     */
    QSplitter *m_hSplitter;

    /**
     * vertical splitter
     */
    QSplitter *m_vSplitter;

    /**
     * sidebars for the four sides
     */
    Sidebar *m_sidebars[4];

    /**
     * config object for session restore, only valid between
     * start and finish restore calls
     */
    KConfig *m_restoreConfig;

    /**
     * restore group
     */
    QString m_restoreGroup;
};

}

#endif
