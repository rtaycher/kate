/***************************************************************************
                          katepluginmanager.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katepluginmanager.h"
#include "katepluginmanager.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include <kparts/factory.h>

#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <qstringlist.h>
#include <qfile.h>
#include <kmessagebox.h>

KatePluginManager::KatePluginManager(QObject *parent) : QObject(parent)
{
  setupPluginList ();
  loadConfig ();
}

KatePluginManager::~KatePluginManager()
{
}

void KatePluginManager::setupPluginList ()
{    
  QValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/Plugin");
                                        
  KTrader::OfferList::Iterator it(traderList.begin());
  for( ; it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);        
        
    PluginInfo *info=new PluginInfo;

    info->load = false;
    info->service = ptr;
    info->plugin = 0L;

    myPluginList.append(info);
  }
}

void KatePluginManager::loadConfig ()
{
  KSimpleConfig *config = new KSimpleConfig("katepluginrc", false);
  config->setGroup("Plugins");

  for (uint i=0; i<myPluginList.count(); i++)
  {
    if  (config->readBoolEntry(myPluginList.at(i)->service->library(), false))
      myPluginList.at(i)->load = true;
  }
}

void KatePluginManager::writeConfig ()
{
  KSimpleConfig *config = new KSimpleConfig("katepluginrc", false);
  config->setGroup("Plugins");

  for (uint i=0; i<myPluginList.count(); i++)
  {
    config->writeEntry(myPluginList.at(i)->service->library(), myPluginList.at(i)->load);
  }

  config->sync();
}


void KatePluginManager::loadAllEnabledPlugins ()
{
  for (uint i=0; i<myPluginList.count(); i++)
  {
    if  (myPluginList.at(i)->load)
      loadPlugin (myPluginList.at(i));
  }
}

void KatePluginManager::enableAllPluginsGUI (KateMainWindow *win)
{
  for (uint i=0; i<myPluginList.count(); i++)
  {
    if  (myPluginList.at(i)->load)
      enablePluginGUI (myPluginList.at(i), win);
  }
}

void KatePluginManager::loadPlugin (PluginInfo *item)
{
  KLibFactory *factory = KLibLoader::self()->factory( QFile::encodeName(item->service->library()) );
  
  if (!factory)
  {
     KMessageBox::sorry(0,KLibLoader::self()->lastErrorMessage());
     item->load=false;
     return;
  }
  
  item->plugin = (Kate::Plugin *)factory->create( (Kate::Application *)parent(), "", "Kate::Plugin" );
  item->load = true;
}

void KatePluginManager::unloadPlugin (PluginInfo *item)
{
  disablePluginGUI (item);
  if (item->plugin) delete item->plugin;
  item->plugin = 0L;
  item->load = false;
}

void KatePluginManager::enablePluginGUI (PluginInfo *item, KateMainWindow *win)
{
  if (!item->plugin) return;
  if (!item->plugin->hasView()) return;

  win->guiFactory()->addClient( item->plugin->createView(win) );
}

void KatePluginManager::enablePluginGUI (PluginInfo *item)
{
  if (!item->plugin) return;
  if (!item->plugin->hasView()) return;

  for (uint i=0; i< ((KateApp*)parent())->mainWindows.count(); i++)
  {
    ((KateApp*)parent())->mainWindows.at(i)->guiFactory()->addClient( item->plugin->createView(((KateApp*)parent())->mainWindows.at(i)) );
  }
}

void KatePluginManager::disablePluginGUI (PluginInfo *item)
{
  if (!item->plugin) return;
  for (uint i=0; i< ((KateApp*)parent())->mainWindows.count(); i++)
  {
    for (uint z=0; z< item->plugin->viewList.count(); z++)
    {
      ((KateApp*)parent())->mainWindows.at(i)->guiFactory()->removeClient( item->plugin->viewList.at (z) );
    }
  }

  item->plugin->viewList.setAutoDelete (true);
  item->plugin->viewList.clear ();
}
