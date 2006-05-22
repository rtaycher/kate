/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "katepluginmanager.h"
#include "katepluginmanager.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include "../interfaces/application.h"

#include <kconfig.h>
#include <QStringList>
//Added by qt3to4:
#include <QList>
#include <kmessagebox.h>
#include <kservicetypetrader.h>
#include <kdebug.h>
#include <QFile>

QString KatePluginInfo::saveName() const {
    QString saveName=service->property("X-Kate-PluginName").toString();

    if (saveName.isEmpty())
      saveName = service->library();
    return saveName;
}

KatePluginManager::KatePluginManager(QObject *parent) : QObject (parent)
{
  m_pluginManager = new Kate::PluginManager (this);
  setupPluginList ();

  loadConfig (KGlobal::config());
  loadAllEnabledPlugins ();
}

KatePluginManager::~KatePluginManager()
{
  // first write config
  storeGeneralConfig (KGlobal::config());

  // than unload the plugins
  unloadAllPlugins ();
}

KatePluginManager *KatePluginManager::self()
{
  return KateApp::self()->pluginManager ();
}

void KatePluginManager::setupPluginList ()
{
  KService::List traderList= KServiceTypeTrader::self()->query("Kate/Plugin", "(not ('Kate/ProjectPlugin' in ServiceTypes)) and (not ('Kate/InitPlugin' in ServiceTypes))");

  foreach(const KService::Ptr &ptr, traderList)
  {
    QString pVersion = ptr->property("X-Kate-Version").toString();

    // don't use plugins out of 3.x release series
    if ((pVersion >= "2.9") && (pVersion <= KateApp::kateVersion(false)))
    {
      KatePluginInfo info;

      info.load = false;
      info.service = ptr;
      info.plugin = 0L;

      m_pluginList.push_back (info);
    }
  }
}

void KatePluginManager::loadConfig (KConfig* config)
{
  config->setGroup("Kate Plugins");

  foreach (const KatePluginInfo &plugin,m_pluginList)
    plugin.load =  config->readEntry (plugin.service->library(), false) ||
                            config->readEntry (plugin.service->property("X-Kate-PluginName").toString(),false);
}

void KatePluginManager::storeGeneralConfig(KConfig* config) {

  foreach(const KatePluginInfo &plugin,m_pluginList)
  {
    config->setGroup("Kate Plugins");
    QString saveName=plugin.saveName();

    config->writeEntry (saveName, plugin.load);

    if (plugin.load) {
      plugin.plugin->storeGeneralConfig(config,QString("Plugin:%1:").arg(saveName));
    }
  }
}

void KatePluginManager::storeViewConfig(KConfig* config,uint id) {
  Kate::MainWindow *mw=KateApp::self()->mainWindow(id)->mainWindow();
  foreach(const KatePluginInfo &plugin,m_pluginList) {
	if (!plugin.load) continue;
	Kate::PluginViewInterface *vi=qobject_cast<Kate::PluginViewInterface*>(plugin.plugin);
	if (vi) {
		vi->storeViewConfig(config,mw,QString("Plugin:%1:MainWindow:%2").arg(plugin.saveName()).arg(id));
	}
  }

}





void KatePluginManager::loadAllEnabledPlugins ()
{
  for (KatePluginList::iterator it=m_pluginList.begin();it!=m_pluginList.end(); ++it)
  {
    if (it->load)
      loadPlugin(&(*it));
    else
      unloadPlugin(&(*it));
  }
}

void KatePluginManager::unloadAllPlugins ()
{
  for (KatePluginList::iterator it=m_pluginList.begin();it!=m_pluginList.end(); ++it)
  {
    if (it->plugin)
      unloadPlugin(&(*it));
  }
}

void KatePluginManager::enableAllPluginsGUI (KateMainWindow *win,KConfig *config)
{
  for (KatePluginList::iterator it=m_pluginList.begin();it!=m_pluginList.end(); ++it)
  {
    if (it->load)
      enablePluginGUI(&(*it),win,config);
  }
}

void KatePluginManager::disableAllPluginsGUI (KateMainWindow *win)
{
  for (KatePluginList::iterator it=m_pluginList.begin();it!=m_pluginList.end(); ++it)
  {
    if (it->load)
      disablePluginGUI(&(*it),win);
  }
}

void KatePluginManager::loadPlugin (KatePluginInfo *item)
{
  QString pluginName=item->service->property("X-Kate-PluginName").toString();

  if (pluginName.isEmpty())
    pluginName=item->service->library();

  item->load = (item->plugin = Kate::createPlugin (QFile::encodeName(item->service->library()), Kate::application(), QStringList(pluginName)));
}

void KatePluginManager::unloadPlugin (KatePluginInfo *item)
{
  disablePluginGUI (item);
  if (item->plugin) delete item->plugin;
  item->plugin = 0L;
  item->load = false;
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item, KateMainWindow *win, KConfig *config)
{
  kDebug(13000)<<"Checking if the GUI of a plugin should be enabled"<<endl;
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;
  //BEGIN DEBUG
  QString pluginName=item->service->property("X-Kate-PluginName").toString();

  if (pluginName.isEmpty())
    pluginName=item->service->library();

  kDebug(13000)<<"Enabling GUI for plugin: "<<pluginName<<endl;
  kDebug()<<item->plugin<<"--"<<Kate::pluginViewInterface(item->plugin)<<endl;
  //END DEBUG
  Kate::pluginViewInterface(item->plugin)->addView(win->mainWindow());
  int winID = KateApp::self()->mainWindowID(win);
  if (config && winID >= 0)
  	Kate::pluginViewInterface(item->plugin)->loadViewConfig(config,win->mainWindow(),QString("Plugin:%1:MainWindow:%2").arg(item->saveName()).arg(winID));
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item)
{
  kDebug(13000)<<"Checking if the GUI of a plugin should be enabled"<<endl;

  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  //BEGIN DEBUG
  QString pluginName=item->service->property("X-Kate-PluginName").toString();

  if (pluginName.isEmpty())
    pluginName=item->service->library();

  kDebug(13000)<<"Enabling GUI for plugin: "<<pluginName<<endl;
  kDebug()<<item->plugin<<"--"<<Kate::pluginViewInterface(item->plugin)<<endl;

  //END DEBUG

  for (int i=0; i< KateApp::self()->mainWindows(); i++)
  {
    Kate::pluginViewInterface(item->plugin)->addView(KateApp::self()->mainWindow(i)->mainWindow());
  }
}

void KatePluginManager::disablePluginGUI (KatePluginInfo *item, KateMainWindow *win)
{
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  Kate::pluginViewInterface(item->plugin)->removeView(win->mainWindow());
}

void KatePluginManager::disablePluginGUI (KatePluginInfo *item)
{
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  for (int i=0; i< KateApp::self()->mainWindows(); i++)
  {
    Kate::pluginViewInterface(item->plugin)->removeView(KateApp::self()->mainWindow(i)->mainWindow());
  }
}

Kate::Plugin *KatePluginManager::plugin(const QString &name)
{
  foreach(const KatePluginInfo &info,m_pluginList)
  {
    QString pluginName=info.service->property("X-Kate-PluginName").toString();
    if (pluginName.isEmpty())
       pluginName=info.service->library();
    if  (pluginName==name)
    {
      if (info.plugin)
        return info.plugin;
      else
        break;
    }
  }
  return 0;
}

bool KatePluginManager::pluginAvailable(const QString &){return false;}
class Kate::Plugin *KatePluginManager::loadPlugin(const QString &,bool ){return 0;}
void KatePluginManager::unloadPlugin(const QString &,bool){;}
