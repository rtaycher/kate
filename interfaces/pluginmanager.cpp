/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#include "pluginmanager.moc"

#include "plugin.h"
#include "documentmanager.h"
#include "toolviewmanager.h"
#include "pluginmanager.h"

#include "../app/katepluginmanager.h"
#include "../app/kateapp.h"

namespace Kate
{

class PrivatePluginManager
  {
  public:
    PrivatePluginManager ()
    {
    }

    ~PrivatePluginManager ()
    {
    }

    KatePluginManager *pluginMan;
  };

PluginManager::PluginManager (void *pluginManager) : QObject ((KatePluginManager*) pluginManager)
{
  d = new PrivatePluginManager ();
  d->pluginMan = (KatePluginManager*) pluginManager;
}

PluginManager::~PluginManager ()
{
  delete d;
}

Plugin *PluginManager::plugin(const QString &name)
{
	return d->pluginMan->plugin(name);
}

bool PluginManager::pluginAvailable(const QString &name)
{
  return d->pluginMan->pluginAvailable (name);
}

Plugin *PluginManager::loadPlugin(const QString &name,bool permanent)
{
  return d->pluginMan->loadPlugin (name, permanent);
}

void PluginManager::unloadPlugin(const QString &name,bool permanent)
{
  d->pluginMan->unloadPlugin (name, permanent);
}

class PrivateInitPluginManager
  {
  public:
    PrivateInitPluginManager ()
    {
    }

    ~PrivateInitPluginManager ()
    {
    }

    KateApp *initPluginMan;
  };

InitPluginManager::InitPluginManager (void *initPluginManager) : QObject ((KateApp*) initPluginManager)
{
  d = new PrivateInitPluginManager ();
  d->initPluginMan = (KateApp*) initPluginManager;
}

InitPluginManager::~InitPluginManager ()
{
  delete d;
}

void InitPluginManager::performInit(const QString &libname, const KURL &initScript)
{
  d->initPluginMan->performInit (libname, initScript);
}

InitPlugin *InitPluginManager::initPlugin() const
{
  return d->initPluginMan->initPlugin();
}

KURL InitPluginManager::initScript() const
{
  return d->initPluginMan->initScript();
}

}

