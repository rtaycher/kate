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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "katepluginmanager.h"
#include "katepluginmanager.moc"

#include "kateapp.h"
#include "katemainwindow.h"

#include <kconfig.h>
#include <qstringlist.h>
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
  m_pluginList.setAutoDelete(true);
  m_pluginList.clear();
}

KatePluginManager *KatePluginManager::self()
{
  return KateApp::self()->katePluginManager ();
}

void KatePluginManager::setupPluginList ()
{
  QValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/Plugin", "(not ('Kate/ProjectPlugin' in ServiceTypes)) and (not ('Kate/InitPlugin' in ServiceTypes))");

  for(KTrader::OfferList::Iterator it(traderList.begin()); it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);

    QString pVersion = ptr->property("X-Kate-Version").toString();

    if ((pVersion >= "2.2") && (pVersion <= KATE_VERSION))
    {
      KatePluginInfo *info = new KatePluginInfo;

      info->load = false;
      info->service = ptr;
      info->plugin = 0L;

      m_pluginList.append(info);
    }
  }
}

void KatePluginManager::loadConfig ()
{
  kapp->config()->setGroup("Kate Plugins");

  for (uint i=0; i<m_pluginList.count(); i++)
    m_pluginList.at(i)->load =  kapp->config()->readBoolEntry(m_pluginList.at(i)->service->library(), false) ||
	kapp->config()->readBoolEntry(m_pluginList.at(i)->service->property("X-Kate-PluginName").toString(),false);
}

void KatePluginManager::writeConfig ()
{
  kapp->config()->setGroup("Kate Plugins");

  for (uint i=0; i<m_pluginList.count(); i++) {
	KatePluginInfo *info=m_pluginList.at(i);
	QString saveName=info->service->property("X-Kate-PluginName").toString();
	if (saveName.isEmpty())
		saveName=info->service->library();
    	kapp->config()->writeEntry(saveName, m_pluginList.at(i)->load);
  }
}

void KatePluginManager::loadAllEnabledPlugins ()
{
  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (m_pluginList.at(i)->load)
      loadPlugin (m_pluginList.at(i));
    else
      unloadPlugin (m_pluginList.at(i));
  }
}

void KatePluginManager::unloadAllPlugins ()
{
  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (m_pluginList.at(i)->plugin)
      unloadPlugin (m_pluginList.at(i));
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

void KatePluginManager::disableAllPluginsGUI (KateMainWindow *win)
{
  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (m_pluginList.at(i)->load)
      disablePluginGUI (m_pluginList.at(i), win);
  }
}

void KatePluginManager::loadPlugin (KatePluginInfo *item)
{
  QString pluginName=item->service->property("X-Kate-PluginName").toString();
  if (pluginName.isEmpty())
       pluginName=item->service->library();
  item->load = (item->plugin = Kate::createPlugin (QFile::encodeName(item->service->library()), Kate::application(),0,pluginName));
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
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  Kate::pluginViewInterface(item->plugin)->addView(win->mainWindow());
}

void KatePluginManager::enablePluginGUI (KatePluginInfo *item)
{
  if (!item->plugin) return;
  if (!Kate::pluginViewInterface(item->plugin)) return;

  for (uint i=0; i< ((KateApp*)parent())->mainWindows(); i++)
  {
    Kate::pluginViewInterface(item->plugin)->addView(((KateApp*)parent())->mainWindow(i));
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

  for (uint i=0; i< ((KateApp*)parent())->mainWindows(); i++)
  {
    Kate::pluginViewInterface(item->plugin)->removeView(((KateApp*)parent())->mainWindow(i));
  }
}

Kate::Plugin *KatePluginManager::plugin(const QString &name)
{
 for (uint i=0; i<m_pluginList.count(); i++)
  {
    KatePluginInfo *info=m_pluginList.at(i);
    QString pluginName=info->service->property("X-Kate-PluginName").toString();
    if (pluginName.isEmpty())
       pluginName=info->service->library();
    if  (pluginName==name)
    {
      if (info->plugin)
        return info->plugin;
      else
        break;
    }
  }
  return 0;
}

bool KatePluginManager::pluginAvailable(const QString &){return false;}
class Kate::Plugin *KatePluginManager::loadPlugin(const QString &,bool ){return 0;}
void KatePluginManager::unloadPlugin(const QString &,bool){;}
