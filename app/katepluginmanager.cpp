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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katepluginmanager.h"
#include "katepluginmanager.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include "../interfaces/application.h"

#include <kconfig.h>
#include <qstringlist.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <kmessagebox.h>
#include <kdebug.h>
#include <qfile.h>

KatePluginManager::KatePluginManager(QObject *parent) : QObject (parent)
{
  m_pluginManager = new Kate::PluginManager (this);
  setupPluginList ();

  loadConfig ();
  loadAllEnabledPlugins ();
}

KatePluginManager::~KatePluginManager()
{
  // first write config
  writeConfig ();

  // than unload the plugins
  unloadAllPlugins ();
}

KatePluginManager *KatePluginManager::self()
{
  return KateApp::self()->pluginManager ();
}

void KatePluginManager::setupPluginList ()
{
  Q3ValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/Plugin", "(not ('Kate/ProjectPlugin' in ServiceTypes)) and (not ('Kate/InitPlugin' in ServiceTypes))");

  for(KTrader::OfferList::Iterator it(traderList.begin()); it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);

    QString pVersion = ptr->property("X-Kate-Version").toString();

    if ((pVersion >= "2.5") && (pVersion <= KATE_VERSION))
    {
      KatePluginInfo info;

      info.load = false;
      info.service = ptr;
      info.plugin = 0L;

      m_pluginList.push_back (info);
    }
  }
}

void KatePluginManager::loadConfig ()
{
  KateApp::self()->config()->setGroup("Kate Plugins");

  foreach (const KatePluginInfo &plugin,m_pluginList)
    plugin.load =  KateApp::self()->config()->readBoolEntry (plugin.service->library(), false) ||
                            KateApp::self()->config()->readBoolEntry (plugin.service->property("X-Kate-PluginName").toString(),false);
}

void KatePluginManager::writeConfig ()
{
  KateApp::self()->config()->setGroup("Kate Plugins");

  foreach(const KatePluginInfo &plugin,m_pluginList)
  {
    QString saveName=plugin.service->property("X-Kate-PluginName").toString();

    if (saveName.isEmpty())
      saveName = plugin.service->library();

    KateApp::self()->config()->writeEntry (saveName, plugin.load);
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

void KatePluginManager::enableAllPluginsGUI (KateMainWindow *win)
{
  for (KatePluginList::iterator it=m_pluginList.begin();it!=m_pluginList.end(); ++it)
  {
    if (it->load)
      enablePluginGUI(&(*it),win);
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

  item->load = (item->plugin = Kate::createPlugin (QFile::encodeName(item->service->library()), Kate::application(), 0, QStringList(pluginName)));
}

void KatePluginManager::unloadPlugin (KatePluginInfo *item)
{
  disablePluginGUI (item);
  if (item->plugin) delete item->plugin;
  item->plugin = 0L;
  item->load = false;
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item, KateMainWindow *win)
{
  kdDebug(13000)<<"Checking if the GUI of a plugin should be enabled"<<endl;
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;
  //BEGIN DEBUG
  QString pluginName=item->service->property("X-Kate-PluginName").toString();

  if (pluginName.isEmpty())
    pluginName=item->service->library();

  kdDebug(13000)<<"Enabling GUI for plugin: "<<pluginName<<endl;
  kdDebug()<<item->plugin<<"--"<<Kate::pluginViewInterface(item->plugin)<<endl;
  //END DEBUG
  Kate::pluginViewInterface(item->plugin)->addView(win->mainWindow());
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item)
{
  kdDebug(13000)<<"Checking if the GUI of a plugin should be enabled"<<endl;

  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  //BEGIN DEBUG
  QString pluginName=item->service->property("X-Kate-PluginName").toString();

  if (pluginName.isEmpty())
    pluginName=item->service->library();

  kdDebug(13000)<<"Enabling GUI for plugin: "<<pluginName<<endl;
  kdDebug()<<item->plugin<<"--"<<Kate::pluginViewInterface(item->plugin)<<endl;

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
