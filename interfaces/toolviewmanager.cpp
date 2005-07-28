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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "toolviewmanager.h"
#include "toolviewmanager.moc"

#include "plugin.h"
#include "documentmanager.h"
#include "pluginmanager.h"

#include "../app/katemainwindow.h"
//Added by qt3to4:
#include <QPixmap>

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

QWidget *ToolViewManager::createToolView (const QString &identifier, ToolViewManager::Position pos, const QPixmap &icon, const QString &text)
{
  return d->toolViewMan->createToolView (identifier, (KMultiTabBar::KMultiTabBarPosition)pos, icon, text);
}

bool ToolViewManager::moveToolView (QWidget *widget, ToolViewManager::Position  pos)
{
  if (!widget || !qobject_cast<KateMDI::ToolView*>(widget))
    return false;

  return d->toolViewMan->moveToolView (qobject_cast<KateMDI::ToolView*>(widget), (KMultiTabBar::KMultiTabBarPosition)pos);
}

bool ToolViewManager::showToolView(QWidget *widget)
{
  if (!widget || !qobject_cast<KateMDI::ToolView*>(widget))
    return false;

  return d->toolViewMan->showToolView (qobject_cast<KateMDI::ToolView*>(widget));
}

bool ToolViewManager::hideToolView(QWidget *widget)
{
  if (!widget || !qobject_cast<KateMDI::ToolView*>(widget))
    return false;

  return d->toolViewMan->hideToolView (qobject_cast<KateMDI::ToolView*>(widget));
}

}
