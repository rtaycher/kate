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

// $Id$

 #include "application.h"
 #include "project.h"

#include "plugin.h"
#include "plugin.moc"

#include <kparts/componentfactory.h>

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

  class PrivateProjectPlugin
  {
  public:
    PrivateProjectPlugin ()
    {
    }

    ~PrivateProjectPlugin ()
    {
    }

    Project *m_project;
  };

  class PrivateInitPlugin
  {
  public:
    PrivateInitPlugin ()
    {
    }

    ~PrivateInitPlugin ()
    {
    }

    KURL m_configScript;
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
unsigned int ProjectPlugin::globalProjectPluginNumber = 0;
unsigned int InitPlugin::globalInitPluginNumber = 0;
unsigned int PluginViewInterface::globalPluginViewInterfaceNumber = 0;

Plugin::Plugin( Application *application, const char *name ) : QObject (application, name )
{
  globalPluginNumber++;
  myPluginNumber = globalPluginNumber;
}

Plugin::~Plugin()
{
}

ProjectPlugin::ProjectPlugin( Project *project, const char *name ) : Plugin (Kate::application(), name )
{
  globalProjectPluginNumber++;
  myProjectPluginNumber = globalProjectPluginNumber;

  d = new PrivateProjectPlugin ();
  d->m_project = project;
}

ProjectPlugin::~ProjectPlugin()
{
  delete d;
}

bool ProjectPlugin::save ()
{
  return true;
}

bool ProjectPlugin::close ()
{
  return true;
}

void ProjectPlugin::addDirs (const QString &dir, QStringList &)
{
}

void ProjectPlugin::removeDirs (const QString &dir, QStringList &)
{
}

void ProjectPlugin::addFiles (const QString &dir, QStringList &)
{
}

void ProjectPlugin::removeFiles (const QString &dir, QStringList &)
{
}

InitPlugin :: InitPlugin(Application *application, const char *name):Plugin(application,name)
{
  globalInitPluginNumber++;
  myInitPluginNumber = globalInitPluginNumber;

  d = new PrivateInitPlugin ();
  d->m_configScript = KURL();
}

InitPlugin::~InitPlugin()
{
  delete d;
}

unsigned int InitPlugin::initPluginNumber () const
{
  return myInitPluginNumber;
}

void InitPlugin::activate(const KURL &initScript)
{
  d->m_configScript=initScript;
}

int InitPlugin::actionsKateShouldNotPerformOnRealStartup()
{
  return 0;
}

const KURL InitPlugin::configScript() const
{
  return d->m_configScript;
}


int InitPlugin::initKate()
{
return 0;
}

unsigned int Plugin::pluginNumber () const
{
  return myPluginNumber;
}

 Application *Plugin::application () const
{
  return Kate::application();
}

unsigned int ProjectPlugin::projectPluginNumber () const
{
  return myProjectPluginNumber;
}

 Project *ProjectPlugin::project () const
{
  return d->m_project;
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
  return KParts::ComponentFactory::createInstanceFromLibrary<Plugin>( libname, application, name, args);
}

ProjectPlugin *createProjectPlugin ( const char* libname, Project *project, const char *name, const QStringList &args )
{
  return KParts::ComponentFactory::createInstanceFromLibrary<ProjectPlugin>( libname, project, name, args);
}

PluginViewInterface *pluginViewInterface (Plugin *plugin)
{
  if (!plugin)
    return 0;

  return static_cast<PluginViewInterface*>(plugin->qt_cast("Kate::PluginViewInterface"));
}

}

