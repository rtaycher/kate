/* This file is part of the KDE project
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include "toolviewmanager.h"
#include "toolviewmanager.moc"

#include "plugin.h"
#include "documentmanager.h"
#include "pluginmanager.h"

#include "../app/katemainwindow.h"

namespace Kate
{

class PrivateToolViewManager
  {
  public:
    PrivateToolViewManager ()
    {
    }

    ~PrivateToolViewManager ()
    {
    }

    KateMainWindow *toolViewMan;
  };

ToolViewManager::ToolViewManager (void *toolViewManager) : QObject ((KateMainWindow*) toolViewManager)
{
  d = new PrivateToolViewManager ();
  d->toolViewMan = (KateMainWindow*) toolViewManager;
}

ToolViewManager::~ToolViewManager ()
{
  delete d;
}

KMDI::ToolViewAccessor *ToolViewManager::addToolView(KDockWidget::DockPosition position, QWidget *widget, const QPixmap &icon, const QString &sname, const QString &tabToolTip, const QString &tabCaption)
{
  return d->toolViewMan->addToolView (position, widget, icon, sname, tabToolTip, tabCaption);
}

bool ToolViewManager::removeToolView(QWidget *widget)
{
  return d->toolViewMan->removeToolView (widget);
}

bool ToolViewManager::removeToolView(KMDI::ToolViewAccessor *accessor)
{
  return d->toolViewMan->removeToolView (accessor);
}

bool ToolViewManager::showToolView(QWidget *widget)
{
  return d->toolViewMan->showToolView (widget);
}

bool ToolViewManager::showToolView(KMDI::ToolViewAccessor *accessor)
{
  return d->toolViewMan->showToolView (accessor);
}

bool ToolViewManager::hideToolView(QWidget *widget)
{
  return d->toolViewMan->hideToolView (widget);
}

bool ToolViewManager::hideToolView(KMDI::ToolViewAccessor *accessor)
{
  return d->toolViewMan->hideToolView (accessor);
}

}
