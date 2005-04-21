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
#include <kpopupmenu.h>

#include <qvbox.h>
#include <qhbox.h>
#include <qtabbar.h>
#include <qevent.h>

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


//BEGIN TOOLVIEW

ToolView::ToolView (MainWindow *mainwin, Sidebar *sidebar, QWidget *parent)
 : QVBox (parent)
 , m_mainWin (mainwin)
 , m_sidebar (sidebar)
{
}

ToolView::~ToolView ()
{
  m_mainWin->toolViewDeleted (this);
}

//END TOOLVIEW


//BEGIN SIDEBAR

Sidebar::Sidebar (KMultiTabBar::KMultiTabBarPosition pos, MainWindow *mainwin, QWidget *parent)
  : KMultiTabBar ((pos == KMultiTabBar::Top || pos == KMultiTabBar::Bottom) ? KMultiTabBar::Horizontal : KMultiTabBar::Vertical, parent)
  , m_mainWin (mainwin)
  , m_pos (pos)
  , m_splitter (0)
  , m_ownSplit (0)
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
  m_ownSplit->setOpaqueResize( KGlobalSettings::opaqueResize() );
  m_ownSplit->hide ();
}

void Sidebar::setResizeMode(QSplitter::ResizeMode mode)
{
  m_splitter->setResizeMode(m_ownSplit, mode);
}

ToolView *Sidebar::addWidget (const QPixmap &icon, const QString &text, ToolView *widget)
{
  static int id = 0;

  if (widget)
  {
    if (widget->sidebar() == this)
      return widget;

    widget->sidebar()->removeWidget (widget);
  }

  int newId = ++id;

  appendTab (icon, newId, text);

  if (!widget)
  {
    widget = new ToolView (m_mainWin, this, m_ownSplit);
    widget->hide ();
    widget->visible = false;
    widget->icon = icon;
    widget->text = text;
  }
  else
  {
    widget->hide ();
    widget->reparent (m_ownSplit, 0, QPoint());
    widget->m_sidebar = this;
  }

  m_idToWidget.insert (newId, widget);
  m_widgetToId.insert (widget, newId);

  show ();

  connect(tab(newId),SIGNAL(clicked(int)),this,SLOT(tabClicked(int)));
  tab(newId)->installEventFilter(this);

  return widget;
}

bool Sidebar::removeWidget (ToolView *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  removeTab(m_widgetToId[widget]);

  m_idToWidget.remove (m_widgetToId[widget]);
  m_widgetToId.remove (widget);

  bool anyVis = false;
  QIntDictIterator<ToolView> it( m_idToWidget );
  for ( ; it.current(); ++it )
  {
    if (!anyVis)
      anyVis =  it.current()->isVisible();
  }

  if (m_idToWidget.isEmpty())
  {
    m_ownSplit->hide ();
    hide ();
  }
  else if (!anyVis)
    m_ownSplit->hide ();

  return true;
}

bool Sidebar::showWidget (ToolView *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  QIntDictIterator<ToolView> it( m_idToWidget );
  for ( ; it.current(); ++it )
    if (it.current() != widget)
    {
      it.current()->hide();
      setTab (it.currentKey(), false);
      it.current()->visible = false;
    }

  setTab (m_widgetToId[widget], true);

  m_ownSplit->show ();
  widget->show ();

  widget->visible = true;

  return true;
}

bool Sidebar::hideWidget (ToolView *widget)
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

  QIntDictIterator<ToolView> it( m_idToWidget );
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

  widget->visible = false;

  return true;
}

void Sidebar::tabClicked(int i)
{
  ToolView *w = m_idToWidget[i];

  if (!w)
    return;

  if (isTabRaised(i))
    showWidget (w);
  else
    hideWidget (w);
}

bool Sidebar::eventFilter(QObject *obj, QEvent *ev)
{
  if (ev->type()==QEvent::ContextMenu)
  {
    QContextMenuEvent *e = (QContextMenuEvent *) ev;
    KMultiTabBarTab *bt = dynamic_cast<KMultiTabBarTab*>(obj);
    if (bt)
    {
      kdDebug()<<"Request for popup"<<endl;

      m_popupButton = bt->id();

      KPopupMenu *p = new KPopupMenu (this);
      p->insertTitle(SmallIcon("move"), i18n("Move to..."), 50);

      if (position() != 0)
        p->insertItem(SmallIconSet("back"), i18n("Left Sidebar"),0);

      if (position() != 1)
        p->insertItem(SmallIconSet("forward"), i18n("Right Sidebar"),1);

      if (position() != 2)
        p->insertItem(SmallIconSet("up"), i18n("Top Sidebar"),2);

      if (position() != 3)
        p->insertItem(SmallIconSet("down"), i18n("Bottom Sidebar"),3);

      connect(p, SIGNAL(activated(int)),
            this, SLOT(buttonPopupActivate(int)));

      p->exec(e->globalPos());
      delete p;

      return true;
    }
  }

  return false;
}

void Sidebar::buttonPopupActivate (int id)
{
  ToolView *w = m_idToWidget[m_popupButton];

  if (!w)
    return;

  // move ids
  if (id < 4)
  {
    // move + show ;)
    m_mainWin->moveToolView (w, (KMultiTabBar::KMultiTabBarPosition) id);
    m_mainWin->showToolView (w);
  }
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

  m_sidebars[KMultiTabBar::Left] = new Sidebar (KMultiTabBar::Left, this, hb);

  m_hSplitter = new QSplitter (Qt::Horizontal, hb);
  m_hSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );

  m_sidebars[KMultiTabBar::Left]->setSplitter (m_hSplitter);

  QVBox *vb = new QVBox (m_hSplitter);

  m_sidebars[KMultiTabBar::Top] = new Sidebar (KMultiTabBar::Top, this, vb);

  m_vSplitter = new QSplitter (Qt::Vertical, vb);
  m_vSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );

  m_sidebars[KMultiTabBar::Top]->setSplitter (m_vSplitter);

  m_tabWidget = new TabWidget (m_vSplitter);

  m_sidebars[KMultiTabBar::Bottom] = new Sidebar (KMultiTabBar::Bottom, this, vb);
  m_sidebars[KMultiTabBar::Bottom]->setSplitter (m_vSplitter);

  m_sidebars[KMultiTabBar::Right] = new Sidebar (KMultiTabBar::Right, this, hb);
  m_sidebars[KMultiTabBar::Right]->setSplitter (m_hSplitter);
}

MainWindow::~MainWindow ()
{
  // cu toolviews
  while (!m_toolviews.isEmpty())
    delete m_toolviews[0];

  // seems like we really should delete this by hand ;)
  delete m_tabWidget;

  for (unsigned int i=0; i < 4; ++i)
    delete m_sidebars[i];
}

TabWidget *MainWindow::tabWidget ()
{
  return m_tabWidget;
}

ToolView *MainWindow::createToolView (const QString &identifier, KMultiTabBar::KMultiTabBarPosition pos, const QPixmap &icon, const QString &text)
{
  if (m_idToWidget[identifier])
    return 0;

  // try the restore config to figure out real pos
  if (m_restoreConfig)
  {
    m_restoreConfig->setGroup (m_restoreGroup);
    pos = (KMultiTabBar::KMultiTabBarPosition) m_restoreConfig->readNumEntry (QString ("Kate-MDI-ToolView-%1-Position").arg(identifier), pos);
  }

  ToolView *v  = m_sidebars[pos]->addWidget (icon, text, 0);
  v->id = identifier;

  m_idToWidget.insert (identifier, v);
  m_toolviews.push_back (v);

  return v;
}

ToolView *MainWindow::toolView (const QString &identifier)
{
  return m_idToWidget[identifier];
}

void MainWindow::toolViewDeleted (ToolView *widget)
{
  if (!widget)
    return;

  if (widget->mainWindow() != this)
    return;

  widget->sidebar()->removeWidget (widget);

  m_idToWidget.remove (widget->id);
  m_toolviews.remove (widget);
}

bool MainWindow::moveToolView (ToolView *widget, KMultiTabBar::KMultiTabBarPosition pos)
{
  if (!widget || widget->mainWindow() != this)
    return false;

  m_sidebars[pos]->addWidget (widget->icon, widget->text, widget);

  return true;
}

bool MainWindow::showToolView (ToolView *widget)
{
  if (!widget || widget->mainWindow() != this)
    return false;

  return widget->sidebar()->showWidget (widget);
}

bool MainWindow::hideToolView (ToolView *widget)
{
  if (!widget || widget->mainWindow() != this)
    return false;

  return widget->sidebar()->hideWidget (widget);
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
  for ( unsigned int i=0; i < m_toolviews.size(); ++i )
  {
    KMultiTabBar::KMultiTabBarPosition newPos = (KMultiTabBar::KMultiTabBarPosition) m_restoreConfig->readNumEntry (QString ("Kate-MDI-ToolView-%1-Position").arg(m_toolviews[i]->id), m_toolviews[i]->sidebar()->position());

    if (m_toolviews[i]->sidebar()->position() != newPos)
    {
      moveToolView (m_toolviews[i], newPos);
    }
  }

  // hide toolviews
  m_restoreConfig->setGroup (m_restoreGroup);
  for ( unsigned int i=0; i < m_toolviews.size(); ++i )
  {
    if (!m_restoreConfig->readBoolEntry (QString ("Kate-MDI-ToolView-%1-Visible").arg(m_toolviews[i]->id), false))
      hideToolView (m_toolviews[i]);
  }

  // restore visible toolviews
  m_restoreConfig->setGroup (m_restoreGroup);
  for ( unsigned int i=0; i < m_toolviews.size(); ++i )
  {
    if (m_restoreConfig->readBoolEntry (QString ("Kate-MDI-ToolView-%1-Visible").arg(m_toolviews[i]->id), false))
      showToolView (m_toolviews[i]);
  }

  // clear this stuff, we are done ;)
  m_restoreConfig = 0;
  m_restoreGroup = "";
}

void MainWindow::saveSession (KConfig *config, const QString &group)
{
  if (!config)
    return;

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
  for ( unsigned int i=0; i < m_toolviews.size(); ++i)
  {
    config->writeEntry (QString ("Kate-MDI-ToolView-%1-Position").arg(m_toolviews[i]->id), m_toolviews[i]->sidebar()->position());
    config->writeEntry (QString ("Kate-MDI-ToolView-%1-Visible").arg(m_toolviews[i]->id), m_toolviews[i]->visible);
  }
}

//END MAIN WINDOW

} // namespace KateMDI
