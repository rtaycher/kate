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

#include "kantplugin.h"

#include "../app/kantapp.h"
#include "../mainwindow/kantmainwindow.h"

#include <kparts/factory.h>

#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <qstringlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <ksimpleconfig.h>

KantPluginManager::KantPluginManager(QObject *parent) : QObject(parent)
{
  setupPluginList ();
  loadConfig ();
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

    info->load = false;
    info->libname = confFile->readEntry("libname","");
    info->config = (*it);

    info->name = confFile->readEntry("name","");
    info->description = confFile->readEntry("description","");
    info->author = confFile->readEntry("author","");

    info->plugin = 0L;

    myPluginList.append(info);

    delete confFile;
  }
}

void KantPluginManager::loadConfig ()
{
  KSimpleConfig *config = new KSimpleConfig("kantpluginrc", false);
  config->setGroup("Plugins");

  for (int i=0; i<myPluginList.count(); i++)
  {
    if  (config->readBoolEntry(myPluginList.at(i)->libname, false))
      myPluginList.at(i)->load = true;
  }
}

void KantPluginManager::writeConfig ()
{
  KSimpleConfig *config = new KSimpleConfig("kantpluginrc", false);
  config->setGroup("Plugins");

  for (int i=0; i<myPluginList.count(); i++)
  {
    config->writeEntry(myPluginList.at(i)->libname, myPluginList.at(i)->load);
  }

  config->sync();
}


void KantPluginManager::loadAllEnabledPlugins ()
{
  for (int i=0; i<myPluginList.count(); i++)
  {
    if  (myPluginList.at(i)->load)
      loadPlugin (myPluginList.at(i));
  }
}

void KantPluginManager::enableAllPluginsGUI (KantMainWindow *win)
{
  for (int i=0; i<myPluginList.count(); i++)
  {
    if  (myPluginList.at(i)->load)
      enablePluginGUI (myPluginList.at(i), win);
  }
}

void KantPluginManager::loadPlugin (PluginListItem *item)
{
  KLibFactory *factory = KLibLoader::self()->factory( item->libname.latin1() );
  item->plugin = (KantPlugin *)factory->create( ((KantApp*)parent())->pluginIface, "", "KantPlugin" );
  item->load = true;
}

void KantPluginManager::unloadPlugin (PluginListItem *item)
{
  delete item->plugin;
  item->plugin = 0L;
  item->load = false;
}

void KantPluginManager::enablePluginGUI (PluginListItem *item, KantMainWindow *win)
{
  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( ((KantApp*)parent())->pluginIface, &ev );

  win->guiFactory()->addClient( item->plugin->createView() );
}

void KantPluginManager::enablePluginGUI (PluginListItem *item)
{
  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( ((KantApp*)parent())->pluginIface, &ev );

  for (int i=0; i< ((KantApp*)parent())->mainWindows.count(); i++)
  {
    ((KantApp*)parent())->mainWindows.at(i)->guiFactory()->addClient( item->plugin->createView() );
  }
}


    /*
void KantPluginManager::disablePluginGUI (PluginListItem *item, KantMainWindow *win)
{
  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( ((KantApp*)parent())->pluginIface, &ev );

  for (int i=0; i< ((KantApp*)parent())->mainWindows.count(); i++)
  {
    ((KantApp*)parent())->mainWindows.at(i)->guiFactory()->addClient( item->plugin->createView() );
  }

  KParts::GUIActivateEvent ev( true );
  QApplication::sendEvent( ((KantApp*)parent())->pluginIface, &ev );
  win->guiFactory()->addClient( item->plugin->createView() );
}   */
