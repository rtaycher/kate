/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002,2003 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

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

enum TabWidgetVisibility {
    AlwaysShowTabs         = 0,
    ShowWhenMoreThanOneTab = 1,
    NeverShowTabs          = 2
  };

class TabWidget : public KTabWidget
{
  Q_OBJECT

  public:
    TabWidget(QWidget* parent, const char* name=0);
    virtual ~TabWidget();

    virtual void addTab ( QWidget * child, const QString & label );

    virtual void addTab ( QWidget * child, const QIconSet & iconset, const QString & label );

    virtual void addTab ( QWidget * child, QTab * tab );

    virtual void insertTab ( QWidget * child, const QString & label, int index = -1 );

    virtual void insertTab ( QWidget * child, const QIconSet & iconset, const QString & label, int index = -1 );

    virtual void insertTab ( QWidget * child, QTab * tab, int index = -1 );

    virtual void removePage ( QWidget * w );

    KateMDI::TabWidgetVisibility tabWidgetVisibility() const;

    void setTabWidgetVisibility( KateMDI::TabWidgetVisibility );

  private slots:
    void closeTab(QWidget* w);

  private:
    void maybeShow();
    void setCornerWidgetVisibility(bool visible);

  private:
    KateMDI::TabWidgetVisibility m_visibility;
};

class ToolView : public QVBox
{
  friend class Sidebar;
  friend class MainWindow;

  protected:
    ToolView (class MainWindow *mainwin, class Sidebar *sidebar, QWidget *parent);

  public:
    virtual ~ToolView ();

    MainWindow *mainWindow () { return m_mainWin; }
    Sidebar *sidebar () { return m_sidebar; }

  private:
    MainWindow *m_mainWin;
    Sidebar *m_sidebar;

    QString id;
    bool visible;
    QPixmap icon;
    QString text;
};

class Sidebar : public KMultiTabBar
{
  Q_OBJECT

  public:
    Sidebar (KMultiTabBar::KMultiTabBarPosition pos, class MainWindow *mainwin, QWidget *parent);
    virtual ~Sidebar ();

    void setSplitter (QSplitter *sp);

  public:
    ToolView *addWidget (const QPixmap &icon, const QString &text, ToolView *widget);
    bool removeWidget (ToolView *widget);

    bool showWidget (ToolView *widget);
    bool hideWidget (ToolView *widget);

    KMultiTabBar::KMultiTabBarPosition position () const { return m_pos; }

    void setLastSize (int s) { m_lastSize = s; }
    int lastSize () { return m_lastSize; }

    bool splitterVisible () { return m_ownSplit->isVisible(); }

  private slots:
    void tabClicked(int);

  protected:
    bool eventFilter(QObject *obj, QEvent *ev);

  private slots:
    void buttonPopupActivate (int id);

  private:
    MainWindow *m_mainWin;

    KMultiTabBar::KMultiTabBarPosition m_pos;
    QSplitter *m_splitter;
    KMultiTabBar *m_tabBar;
    QSplitter *m_ownSplit;

    QIntDict<ToolView> m_idToWidget;
    QMap<ToolView*, int> m_widgetToId;

    int m_lastSize;

    int m_popupButton;
};

class MainWindow : public KParts::MainWindow
{
  Q_OBJECT

  friend class ToolView;

  //
  // Constructor area
  //
  public:
    /**
     * Constructor
     */
    MainWindow (QWidget* parentWidget = 0, const char* name = 0);

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
    TabWidget *tabWidget ();

    /**
     * add a given widget to the given sidebar if possible, name is very important
     * @param identifier unique identifier for this toolview
     * @param pos position for the toolview, if we are in session restore, this is only a preference
     * @param icon icon to use for the toolview
     * @param text text to use in addition to icon
     * @return created toolview on success or 0
     */
    ToolView *createToolView (const QString &identifier, KMultiTabBar::KMultiTabBarPosition pos, const QPixmap &icon, const QString &text);

    /**
     * give you handle to toolview for the given name, 0 if no toolview around
     * @param identifier toolview name
     * @return toolview if existing, else 0
     */
    ToolView *toolView (const QString &identifier);

  protected:
    /**
     * called by toolview destructor
     * @param widget toolview which is destroyed
     */
    void toolViewDeleted (ToolView *widget);

  public:
    // move a toolview to given new pos
    bool moveToolView (ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos);

    // show given toolview
    bool showToolView (ToolView *widget);

    // hide given toolview
    bool hideToolView (ToolView *widget);

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
    QDict<ToolView> m_idToWidget;

    /**
     * list of all toolviews around
     */
    QValueList<ToolView*> m_toolviews;

    /**
     * tab widget, which is the central part of the
     * main window ;)
     */
    TabWidget *m_tabWidget;

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
