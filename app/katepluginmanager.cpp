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

#include <kconfig.h>
#include <ksimpleconfig.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <qstringlist.h>
#include <qfile.h>
#include <kmessagebox.h>

KatePluginManager::KatePluginManager(QObject *parent) : Kate::PluginManager(parent)
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

    m_pluginList.append(info);
  }
}

void KatePluginManager::loadConfig ()
{
  KSimpleConfig *config = new KSimpleConfig("katepluginrc", false);
  config->setGroup("Plugins");

  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (config->readBoolEntry(m_pluginList.at(i)->service->library(), false))
      m_pluginList.at(i)->load = true;
  }
}

void KatePluginManager::writeConfig ()
{
  KSimpleConfig *config = new KSimpleConfig("katepluginrc", false);
  config->setGroup("Plugins");

  for (uint i=0; i<m_pluginList.count(); i++)
  {
    config->writeEntry(m_pluginList.at(i)->service->library(), m_pluginList.at(i)->load);
  }

  config->sync();
}


void KatePluginManager::loadAllEnabledPlugins ()
{
  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (m_pluginList.at(i)->load)
      loadPlugin (m_pluginList.at(i));
  }
}

void KatePluginManager::enableAllPluginsGUI (KateMainWindow *win)
{
  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (m_pluginList.at(i)->load)
      enablePluginGUI (m_pluginList.at(i), win);
  }
}

void KatePluginManager::loadPlugin (PluginInfo *item)
{               
  item->load = (item->plugin = Kate::createPlugin (QFile::encodeName(item->service->library()), (Kate::Application *)kapp));
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
  if (!Kate::pluginViewInterface(item->plugin)) return;

  Kate::pluginViewInterface(item->plugin)->addView(win);
}

void KatePluginManager::enablePluginGUI (PluginInfo *item)
{
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  for (uint i=0; i< ((KateApp*)parent())->mainWindows(); i++)
  {
    Kate::pluginViewInterface(item->plugin)->addView(((KateApp*)parent())->mainWindow(i));
  }
}

void KatePluginManager::disablePluginGUI (PluginInfo *item)
{
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;
  
  for (uint i=0; i< ((KateApp*)parent())->mainWindows(); i++)
  {
    Kate::pluginViewInterface(item->plugin)->removeView(((KateApp*)parent())->mainWindow(i));       
  }
}

Kate::Plugin *KatePluginManager::plugin(const QString &name)
{
 for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (m_pluginList.at(i)->service->library()==name)
    {
	if (m_pluginList.at(i)->plugin) return m_pluginList.at(i)->plugin; else break;
    }
  }
  return 0;
}


bool KatePluginManager::pluginAvailable(const QString &name){return false;}
class Kate::Plugin *KatePluginManager::loadPlugin(const QString &name,bool permanent=true){return 0;}
void KatePluginManager::unloadPlugin(const QString &name,bool permanent=true){;}

