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


namespace KateMDI {

Sidebar::Sidebar (KMultiTabBar::KMultiTabBarPosition pos, QWidget *parent)
  : KMultiTabBar ((pos == KMultiTabBar::Top || pos == KMultiTabBar::Bottom) ? KMultiTabBar::Horizontal : KMultiTabBar::Vertical, parent)
  , m_pos (pos)
  , m_splitter (0)
  , m_ownSplit (0)
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
    }

  setTab (m_widgetToId[widget], true);

  m_ownSplit->show ();
  widget->show ();

  return true;
}

bool Sidebar::hideWidget (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  bool anyVis = false;

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

MainWindow::MainWindow ()
{
  // init the internal widgets
  QHBox *hb = new QHBox (this);
  setCentralWidget(hb);

  m_sidebars[KMultiTabBar::Left] = new Sidebar (KMultiTabBar::Left, hb);

  m_hSplitter = new QSplitter (Qt::Horizontal, hb);

  m_sidebars[KMultiTabBar::Left]->setSplitter (m_hSplitter);

  QVBox *vb = new QVBox (m_hSplitter);

  m_sidebars[KMultiTabBar::Top] = new Sidebar (KMultiTabBar::Top, vb);

  m_vSplitter = new QSplitter (Qt::Vertical, vb);

  m_sidebars[KMultiTabBar::Top]->setSplitter (m_vSplitter);

  m_tabWidget = new KTabWidget (m_vSplitter);

  m_sidebars[KMultiTabBar::Bottom] = new Sidebar (KMultiTabBar::Bottom, vb);
  m_sidebars[KMultiTabBar::Bottom]->setSplitter (m_vSplitter);


  m_sidebars[KMultiTabBar::Right] = new Sidebar (KMultiTabBar::Right, hb);
  m_sidebars[KMultiTabBar::Right]->setSplitter (m_hSplitter);
}

MainWindow::~MainWindow ()
{
}

bool MainWindow::addToolView (const QString &identifier, QWidget *widget, KMultiTabBar::KMultiTabBarPosition pos, const QPixmap &icon, const QString &text)
{
  if (m_idToWidget[identifier])
    return false;

  m_idToWidget.insert (identifier, widget);
  m_widgetToId.insert (widget, identifier);
  m_widgetToSide.insert (widget, pos);

  m_sidebars[pos]->addWidget (icon, text, widget);

  WidgetData d;
  d.icon = icon;
  d.text = text;
  m_widgetToData.insert (widget, d);

  return true;
}

bool MainWindow::deleteToolView (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  m_idToWidget.remove (m_widgetToId[widget]);
  m_widgetToId.remove (widget);
  m_widgetToData.remove (widget);

  m_sidebars[m_widgetToSide[widget]]->removeWidget (widget);
  m_widgetToSide.remove (widget);

  delete widget;

  return true;
}

bool MainWindow::moveToolView (QWidget *widget, KMultiTabBar::KMultiTabBarPosition pos)
{
  if (!m_widgetToId.contains(widget))
    return false;

  if (m_widgetToSide[widget] == pos)
    return true;

  m_sidebars[pos]->addWidget (m_widgetToData[widget].icon, m_widgetToData[widget].text, widget);
  m_sidebars[m_widgetToSide[widget]]->removeWidget (widget);

  m_widgetToSide[widget] = pos;

  return true;
}

bool MainWindow::showToolView (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  return m_sidebars[m_widgetToSide[widget]]->showWidget (widget);
}

bool MainWindow::hideToolView (QWidget *widget)
{
  if (!m_widgetToId.contains(widget))
    return false;

  return m_sidebars[m_widgetToSide[widget]]->hideWidget (widget);
}

void MainWindow::setSidebarResizeMode(KMultiTabBar::KMultiTabBarPosition pos, QSplitter::ResizeMode mode)
{
  m_sidebars[pos]->setResizeMode(mode);
}

} // namespace KateMDI
