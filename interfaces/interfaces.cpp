/***************************************************************************
                          interfaces.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
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
 ***************************************************************************/

#include "application.h"
#include "application.moc"

#include "documentmanager.h"
#include "documentmanager.moc"

#include "mainwindow.h"
#include "mainwindow.moc"

#include "plugin.h"
#include "plugin.moc"

#include "viewmanager.h"
#include "viewmanager.moc"     

#include "toolviewmanager.h"

#include "pluginmanager.h"
#include "pluginmanager.moc"

#include <kparts/componentfactory.h>

namespace Kate
{
            
Application::Application () : KUniqueApplication (true,true,true)
{
}

Application::~Application ()
{
}

DocumentManager::DocumentManager (QObject *parent, const char *name) : QObject (parent, name)
{
}

DocumentManager::~DocumentManager ()
{
}


MainWindow::MainWindow () : KParts::DockMainWindow ()
{
}

MainWindow::~MainWindow ()
{
}

ViewManager::ViewManager (QWidget *parent, const char *name) : QWidget(parent, name)
{
}

ViewManager::~ViewManager ()
{
}   

ToolViewManager::ToolViewManager()
{
}

ToolViewManager::~ToolViewManager()
{
}

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

Plugin::Plugin( Application *application, const char *name ) : QObject (application, name )
{
  globalPluginNumber++;
  myPluginNumber = globalPluginNumber; 
}

Plugin::~Plugin()
{
}


InitPlugin :: InitPlugin(Application *application, const char *name):Plugin(application,name),m_configScript(KURL())
{
}

void InitPlugin::activate(const KURL &initScript)
{
	m_configScript=initScript;
}


InitPlugin::~InitPlugin()
{
}

int InitPlugin::actionsKateShouldNotPerformOnRealStartup()
{
  return 0;
}

const KURL InitPlugin::configScript() const
{
  return m_configScript;
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
   return ((Kate::Application *)kapp);
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

Plugin *createPlugin ( const char* libname, Application *application, const char *name )
{
  return KParts::ComponentFactory::createInstanceFromLibrary<Plugin>( libname, application, name );
}

PluginViewInterface *pluginViewInterface (Plugin *plugin)
{       
  if (!plugin)
    return 0;

  return static_cast<PluginViewInterface*>(plugin->qt_cast("Kate::PluginViewInterface"));
}

InitPluginManager::InitPluginManager(){;}
InitPluginManager::~InitPluginManager(){;}


InitPluginManager *initPluginManager(Application *app)
{
	if (!app) return 0;
	return static_cast<InitPluginManager*>(app->qt_cast("Kate::InitPluginManager"));
}


PluginManager::PluginManager (QObject *parent, const char *name) : QObject (parent, name)
{
}

PluginManager::~PluginManager ()
{
}



};

