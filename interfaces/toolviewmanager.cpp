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

// $Id$

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

KDockWidget *ToolViewManager::addToolViewWidget(KDockWidget::DockPosition position,QWidget *widget, const class QPixmap&icon, const class QString&sname)
{
  return d->toolViewMan->addToolViewWidget (position, widget, icon, sname);
}

bool ToolViewManager::removeToolViewWidget(QWidget *widget)
{
  return d->toolViewMan->removeToolViewWidget (widget);
}

KDockWidget *ToolViewManager::addToolView(KDockWidget::DockPosition position,const char *name,const QPixmap &icon,const QString&sname)
{
  return d->toolViewMan->addToolView (position, name, icon, sname);
}

bool ToolViewManager::removeToolView(KDockWidget *dockwidget)
{
  return d->toolViewMan->removeToolView (dockwidget);
}

bool ToolViewManager::hideToolView(KDockWidget*dockwidget)
{
  return d->toolViewMan->hideToolView (dockwidget);
}

bool ToolViewManager::showToolView(KDockWidget*dockwidget)
{
  return d->toolViewMan->showToolView (dockwidget);
}
    
bool ToolViewManager::hideToolView(const QString& sname)
{
  return d->toolViewMan->hideToolView (sname);
}

bool ToolViewManager::showToolView(const QString& sname)
{
  return d->toolViewMan->showToolView (sname);
}

}

