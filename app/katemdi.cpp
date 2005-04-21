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

#include "katemdi.h"
#include "katemdi.moc"

#include <dcopclient.h>
#include <kurldrag.h>
#include <kencodingfiledialog.h>
#include <kdiroperator.h>
#include <kiconloader.h>
#include <kaboutdata.h>
#include <kstatusbar.h>
#include <kstdaction.h>
#include <kaction.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kapplication.h>
#include <klocale.h>
#include <kurl.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>
#include <kkeydialog.h>
#include <kedittoolbar.h>
#include <kparts/event.h>
#include <kmenubar.h>
#include <kiconloader.h>

#include <qvbox.h>
#include <qhbox.h>
#include <qtabbar.h>

namespace KateMDI {

//BEGIN TABWIDGET

TabWidget::TabWidget(QWidget* parent, const char* name)
 : KTabWidget(parent,name)
 , m_visibility (KateMDI::ShowWhenMoreThanOneTab)
{
  tabBar()->hide();

  setHoverCloseButton(true);

  connect(this, SIGNAL(closeRequest(QWidget*)), this, SLOT(closeTab(QWidget*)));
}

TabWidget::~TabWidget()
{
}

void TabWidget::closeTab(QWidget* w)
{
  w->close();
}

void TabWidget::addTab ( QWidget * child, const QString & label )
{
  KTabWidget::addTab(child,label);
  showPage(child);
  maybeShow();
}

void TabWidget::addTab ( QWidget * child, const QIconSet & iconset, const QString & label )
{
  KTabWidget::addTab(child,iconset,label);
  showPage(child);
  maybeShow();
}

void TabWidget::addTab ( QWidget * child, QTab * tab )
{
  KTabWidget::addTab(child,tab);
  showPage(child);
  maybeShow();
}

void TabWidget::insertTab ( QWidget * child, const QString & label, int index)
{
  KTabWidget::insertTab(child,label,index);
  showPage(child);
  maybeShow();
  tabBar()->repaint();
}

void TabWidget::insertTab ( QWidget * child, const QIconSet & iconset, const QString & label, int index )
{
  KTabWidget::insertTab(child,iconset,label,index);
  showPage(child);
  maybeShow();
  tabBar()->repaint();
}

void TabWidget::insertTab ( QWidget * child, QTab * tab, int index)
{
  KTabWidget::insertTab(child,tab,index);
  showPage(child);
  maybeShow();
  tabBar()->repaint();
}

void TabWidget::removePage ( QWidget * w )
{
  KTabWidget::removePage(w);
  maybeShow();
}

void TabWidget::maybeShow()
{
  switch (m_visibility)
  {
    case KateMDI::AlwaysShowTabs:
      tabBar()->show();

      // show/hide corner widgets
      if (count() == 0)
        setCornerWidgetVisibility(false);
      else
        setCornerWidgetVisibility(true);

      break;

    case KateMDI::ShowWhenMoreThanOneTab:
      if (count()<2) tabBar()->hide();
      else tabBar()->show();

      // show/hide corner widgets
      if (count() < 2)
        setCornerWidgetVisibility(false);
      else
        setCornerWidgetVisibility(true);

      break;

    case KateMDI::NeverShowTabs:
      tabBar()->hide();
      break;
  }
}

void TabWidget::setCornerWidgetVisibility(bool visible)
{
  // there are two corner widgets: on TopLeft and on TopTight!

  if (cornerWidget(Qt::TopLeft) ) {
    if (visible)
      cornerWidget(Qt::TopLeft)->show();
    else
      cornerWidget(Qt::TopLeft)->hide();
  }

  if (cornerWidget(Qt::TopRight) ) {
    if (visible)
      cornerWidget(Qt::TopRight)->show();
    else
      cornerWidget(Qt::TopRight)->hide();
  }
}

void TabWidget::setTabWidgetVisibility( KateMDI::TabWidgetVisibility visibility )
{
  m_visibility = visibility;
  maybeShow();
}

KateMDI::TabWidgetVisibility TabWidget::tabWidgetVisibility( ) const
{
  return m_visibility;
}

//END TABWIDGET


//BEGIN SIDEBAR

Sidebar::Sidebar (KMultiTabBar::KMultiTabBarPosition pos, QWidget *parent, QMap<QWidget*, WidgetData> &widgetToData)
  : KMultiTabBar ((pos == KMultiTabBar::Top || pos == KMultiTabBar::Bottom) ? KMultiTabBar::Horizontal : KMultiTabBar::Vertical, parent)
  , m_pos (pos)
  , m_splitter (0)
  , m_ownSplit (0)
  , m_widgetToData (widgetToData)
  , m_lastSize (0)
{
  hide ();
}

Sidebar::~Sidebar ()
{
}

void Sidebar::setSplitter (QSplitter *sp)
{
  m_splitter = sp;
  m_ownSplit = new QSplitter ((m_pos == KMultiTabBar::Top || m_pos == KMultiTabBar::Bottom) ? Qt::Horizontal : Qt::Vertical, m_splitter);
  m_ownSplit->hide ();
}

void Sidebar::setResizeMode(QSplitter::ResizeMode mode)
{
  m_splitter->setResizeMode(m_ownSplit, mode);
}

bool Sidebar::addWidget (const QPixmap &icon, const QString &text, QWidget *widget)
{
  static int id = 0;

  int newId = ++id;

  appendTab (icon, newId, text);

  widget->hide ();
  widget->reparent (m_ownSplit, 0, QPoint());

  m_idToWidget.insert (newId, widget);
  m_widgetToId.insert (widget, newId);

  show ();

  connect(tab(newId),SIGNAL(clicked(int)),this,SLOT(tabClicked(int)));

  return true;
}

bool Sidebar::removeWidget (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  removeTab(m_widgetToId[widget]);

  m_idToWidget.remove (m_widgetToId[widget]);
  m_widgetToId.remove (widget);

  if (m_idToWidget.isEmpty())
  {
    m_ownSplit->hide ();
    hide ();
  }

  return true;
}

bool Sidebar::showWidget (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  QIntDictIterator<QWidget> it( m_idToWidget );
  for ( ; it.current(); ++it )
    if (it.current() != widget)
    {
      it.current()->hide();
      setTab (it.currentKey(), false);
      m_widgetToData[it.current()].visible = false;
    }

  setTab (m_widgetToId[widget], true);

  m_ownSplit->show ();
  widget->show ();

  m_widgetToData[widget].visible = true;

  return true;
}

bool Sidebar::hideWidget (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  bool anyVis = false;

  QValueList<int> s = m_splitter->sizes ();

  int i = 0;
  if ((m_pos == KMultiTabBar::Right || m_pos == KMultiTabBar::Bottom))
    i = 2;

  // little threshold
  if (s[i] > 2)
    m_lastSize = s[i];

  QIntDictIterator<QWidget> it( m_idToWidget );
  for ( ; it.current(); ++it )
  {
    if (it.current() == widget)
    {
      it.current()->hide();
      continue;
    }

    if (!anyVis)
      anyVis =  it.current()->isVisible();
  }

  // lower tab
  setTab (m_widgetToId[widget], false);

  if (!anyVis)
    m_ownSplit->hide ();

  m_widgetToData[widget].visible = false;

  return true;
}

void Sidebar::tabClicked(int i)
{
  kdDebug () << "MUHHH" << endl;

  QWidget *w = m_idToWidget[i];

  if (!w)
    return;

  if (isTabRaised(i))
    showWidget (w);
  else
    hideWidget (w);
}


//END SIDEBAR


//BEGIN MAIN WINDOW

MainWindow::MainWindow (QWidget* parentWidget, const char* name)
 : KParts::MainWindow( parentWidget, name)
 , m_restoreConfig (0)
{
  // init the internal widgets
  QHBox *hb = new QHBox (this);
  setCentralWidget(hb);

  m_sidebars[KMultiTabBar::Left] = new Sidebar (KMultiTabBar::Left, hb, m_widgetToData);

  m_hSplitter = new QSplitter (Qt::Horizontal, hb);

  m_sidebars[KMultiTabBar::Left]->setSplitter (m_hSplitter);

  QVBox *vb = new QVBox (m_hSplitter);

  m_sidebars[KMultiTabBar::Top] = new Sidebar (KMultiTabBar::Top, vb, m_widgetToData);

  m_vSplitter = new QSplitter (Qt::Vertical, vb);

  m_sidebars[KMultiTabBar::Top]->setSplitter (m_vSplitter);

  m_tabWidget = new TabWidget (m_vSplitter);

  m_sidebars[KMultiTabBar::Bottom] = new Sidebar (KMultiTabBar::Bottom, vb, m_widgetToData);
  m_sidebars[KMultiTabBar::Bottom]->setSplitter (m_vSplitter);

  m_sidebars[KMultiTabBar::Right] = new Sidebar (KMultiTabBar::Right, hb, m_widgetToData);
  m_sidebars[KMultiTabBar::Right]->setSplitter (m_hSplitter);
}

MainWindow::~MainWindow ()
{
  // seems like we really should delete this by hand ;)
  delete m_tabWidget;

  for (unsigned int i=0; i < 4; ++i)
    delete m_sidebars[i];
}

TabWidget *MainWindow::tabWidget ()
{
  return m_tabWidget;
}

bool MainWindow::addToolView (const QString &identifier, QWidget *widget, KMultiTabBar::KMultiTabBarPosition pos, const QPixmap &icon, const QString &text)
{
  if (m_idToWidget[identifier])
    return false;

  // try the restore config to figure out real pos
  if (m_restoreConfig)
  {
    m_restoreConfig->setGroup (m_restoreGroup);
    pos = (KMultiTabBar::KMultiTabBarPosition) m_restoreConfig->readNumEntry (QString ("Kate-MDI-ToolView-%1-Position").arg(identifier), pos);
  }

  m_idToWidget.insert (identifier, widget);

  WidgetData d;
  d.id = identifier;
  d.pos = pos;
  d.visible = false;
  d.icon = icon;
  d.text = text;
  m_widgetToData.insert (widget, d);

  return m_sidebars[pos]->addWidget (icon, text, widget);
}

bool MainWindow::deleteToolView (QWidget *widget)
{
  if (!m_widgetToData.contains(widget))
    return false;

  m_sidebars[m_widgetToData[widget].pos]->removeWidget (widget);
  delete widget;

  m_idToWidget.remove (m_widgetToData[widget].id);
  m_widgetToData.remove (widget);

  return true;
}

bool MainWindow::moveToolView (QWidget *widget, KMultiTabBar::KMultiTabBarPosition pos)
{
  if (!m_widgetToData.contains(widget))
    return false;

  if (m_widgetToData[widget].pos == pos)
    return true;

  m_sidebars[pos]->addWidget (m_widgetToData[widget].icon, m_widgetToData[widget].text, widget);
  m_sidebars[m_widgetToData[widget].pos]->removeWidget (widget);

  m_widgetToData[widget].pos = pos;

  return true;
}

bool MainWindow::showToolView (QWidget *widget)
{
  if (!m_widgetToData.contains(widget))
    return false;

  return m_sidebars[m_widgetToData[widget].pos]->showWidget (widget);
}

bool MainWindow::hideToolView (QWidget *widget)
{
  if (!m_widgetToData.contains(widget))
    return false;

  return m_sidebars[m_widgetToData[widget].pos]->hideWidget (widget);
}

void MainWindow::setSidebarResizeMode(KMultiTabBar::KMultiTabBarPosition pos, QSplitter::ResizeMode mode)
{
  m_sidebars[pos]->setResizeMode(mode);
}

void MainWindow::startRestore (KConfig *config, const QString &group)
{
  // first save this stuff
  m_restoreConfig = config;
  m_restoreGroup = group;

  if (!m_restoreConfig)
    return;

  m_restoreConfig->setGroup (m_restoreGroup);

  // get main splitter sizes ;)
  QValueList<int> hs = m_restoreConfig->readIntListEntry ("Kate-MDI-H-Splitter");
  QValueList<int> vs = m_restoreConfig->readIntListEntry ("Kate-MDI-V-Splitter");

  m_sidebars[0]->setLastSize (hs[0]);
  m_sidebars[1]->setLastSize (hs[2]);
  m_sidebars[2]->setLastSize (vs[0]);
  m_sidebars[3]->setLastSize (vs[2]);

  m_hSplitter->setSizes(hs);
  m_vSplitter->setSizes(vs);
}

void MainWindow::finishRestore ()
{
  if (!m_restoreConfig)
    return;

  // reshuffle toolviews only if needed
  for ( QMap<QWidget*, WidgetData>::Iterator it = m_widgetToData.begin(); it != m_widgetToData.end(); ++it )
  {
    KMultiTabBar::KMultiTabBarPosition newPos = (KMultiTabBar::KMultiTabBarPosition) m_restoreConfig->readNumEntry (QString ("Kate-MDI-ToolView-%1-Position").arg(it.data().id), it.data().pos);

    if (it.data().pos != newPos)
    {
      moveToolView (it.key(), newPos);
    }
  }

  // hide toolviews
  m_restoreConfig->setGroup (m_restoreGroup);
  for ( QMap<QWidget*, WidgetData>::Iterator it = m_widgetToData.begin(); it != m_widgetToData.end(); ++it )
  {
    if (!m_restoreConfig->readBoolEntry (QString ("Kate-MDI-ToolView-%1-Visible").arg(it.data().id), false))
      hideToolView (it.key());
  }

  // restore visible toolviews
  m_restoreConfig->setGroup (m_restoreGroup);
  for ( QMap<QWidget*, WidgetData>::Iterator it = m_widgetToData.begin(); it != m_widgetToData.end(); ++it )
  {
    if (m_restoreConfig->readBoolEntry (QString ("Kate-MDI-ToolView-%1-Visible").arg(it.data().id), false))
      showToolView (it.key());
  }

  // clear this stuff, we are done ;)
  m_restoreConfig = 0;
  m_restoreGroup = "";
}

void MainWindow::saveSession (KConfig *config, const QString &group)
{
  config->setGroup (group);

  // save main splitter sizes ;)
  QValueList<int> hs = m_hSplitter->sizes();
  QValueList<int> vs = m_vSplitter->sizes();

  if (hs[0] <= 2 && !m_sidebars[0]->splitterVisible ())
    hs[0] = m_sidebars[0]->lastSize();
  if (hs[2] <= 2 && !m_sidebars[1]->splitterVisible ())
    hs[2] = m_sidebars[1]->lastSize();
  if (vs[0] <= 2 && !m_sidebars[2]->splitterVisible ())
    vs[0] = m_sidebars[2]->lastSize();
  if (vs[2] <= 2 && !m_sidebars[3]->splitterVisible ())
    vs[2] = m_sidebars[3]->lastSize();

  config->writeEntry ("Kate-MDI-H-Splitter", hs);
  config->writeEntry ("Kate-MDI-V-Splitter", vs);

  // now save the state of the toolviews ;)
  for ( QMap<QWidget*, WidgetData>::Iterator it = m_widgetToData.begin(); it != m_widgetToData.end(); ++it )
  {
    config->writeEntry (QString ("Kate-MDI-ToolView-%1-Position").arg(it.data().id), it.data().pos);
    config->writeEntry (QString ("Kate-MDI-ToolView-%1-Visible").arg(it.data().id), it.data().visible);
  }
}

//END MAIN WINDOW

} // namespace KateMDI
