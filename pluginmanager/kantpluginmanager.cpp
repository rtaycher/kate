/***************************************************************************
                          kantpluginmanager.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantpluginmanager.h"
#include "kantpluginmanager.moc"

#include "../app/kantapp.h"
#include "../mainwindow/kantmainwindow.h"

#include <kparts/factory.h>

#include <kglobal.h>
#include <kstddirs.h>
#include <qstringlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <ksimpleconfig.h>

KantPluginManager::KantPluginManager(QObject *parent) : QObject(parent)
{
  setupPluginList ();
}

KantPluginManager::~KantPluginManager()
{
}

void KantPluginManager::setupPluginList ()
{
  KStandardDirs *dirs = KGlobal::dirs();

  QStringList list=dirs->findAllResources("appdata","plugins/*.desktop",false,true);

  KSimpleConfig *confFile;
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    confFile=new KSimpleConfig(*it,true);

    PluginListItem *info=new PluginListItem;
    info->load = (confFile->readEntry("load","no") =="1");

    info->libname = confFile->readEntry("libname","no");
    info->config = (*it);

    info->name = confFile->readEntry("name","no");
    info->description = confFile->readEntry("description","no");
    info->author = confFile->readEntry("author","no");

    myPluginList.append(info);

    delete confFile;
  }
}

void KantPluginManager::loadAllEnabledPlugins ()
{
  for (int i=0; i<myPluginList.count(); i++)
  {
    if  (myPluginList.at(i)->load)
      loadPlugin (myPluginList.at(i));
  }
}

void KantPluginManager::enabledAllPluginsGUI (KantMainWindow *win)
{
  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( ((KantApp*)parent())->pluginIface, &ev );

  for (int i=0; i<loadedPlugins.count(); i++)
  {
    win->guiFactory()->addClient( loadedPlugins.at(i) );
  }
}

void KantPluginManager::loadPlugin (PluginListItem *item)
{
  KLibFactory *factory = KLibLoader::self()->factory( item->libname.latin1() );
  KParts::Part *plugin = (KParts::Part *)factory->create( ((KantApp*)parent())->pluginIface, "", "KParts::Part" );
  loadedPlugins.append (plugin);
}
