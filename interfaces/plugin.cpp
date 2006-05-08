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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

 #include "application.h"

#include "plugin.h"
#include "plugin.moc"

#include <kparts/componentfactory.h>
#include <klibloader.h>

namespace Kate
{

  class PrivatePlugin
  {
  public:
    PrivatePlugin ()
    {
    }

    ~PrivatePlugin ()
    {
    }
  };

  class PrivatePluginViewInterface
  {
  public:
    PrivatePluginViewInterface ()
    {
    }

    ~PrivatePluginViewInterface ()
    {
    }

  };

unsigned int Plugin::globalPluginNumber = 0;
unsigned int PluginViewInterface::globalPluginViewInterfaceNumber = 0;

Plugin::Plugin( Application *application, const char *name ) : QObject (application )
{
  setObjectName( name );
  globalPluginNumber++;
  myPluginNumber = globalPluginNumber;
}

Plugin::~Plugin()
{
}

unsigned int Plugin::pluginNumber () const
{
  return myPluginNumber;
}

Application *Plugin::application () const
{
  return Kate::application();
}

PluginViewInterface::PluginViewInterface()
{
  globalPluginViewInterfaceNumber++;
  myPluginViewInterfaceNumber = globalPluginViewInterfaceNumber;
}

PluginViewInterface::~PluginViewInterface()
{
}

unsigned int PluginViewInterface::pluginViewInterfaceNumber () const
{
  return myPluginViewInterfaceNumber;
}

Plugin *createPlugin ( const char* libname, Application *application, const char *name, const QStringList &args )
{
  Plugin* plugin = KLibLoader::createInstance<Plugin>( libname, application, args );

  if (plugin && name)
  {
    plugin->setObjectName( name );
  }

  return plugin;
}

PluginViewInterface *pluginViewInterface (Plugin *plugin)
{
// doesn't work with abstract methods:  return qobject_cast<Kate::PluginViewInterface>(plugin);
   
  return (PluginViewInterface*)(plugin->qt_metacast("Kate::PluginViewInterface"));
}

}

