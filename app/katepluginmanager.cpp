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
#include <kglobal.h>
#include <kstandarddirs.h>
#include <qstringlist.h>
#include <qfile.h>
#include <kmessagebox.h>

KatePluginManager::KatePluginManager(QObject *parent) : QObject (parent)
{
  m_pluginManager = new Kate::PluginManager (this);
  setupPluginList ();
  loadConfig ();
}

KatePluginManager::~KatePluginManager()
{
}

void KatePluginManager::setupPluginList ()
{
  QValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/Plugin", "(not ('Kate/ProjectPlugin' in ServiceTypes)) and (not ('Kate/InitPlugin' in ServiceTypes))");

  KTrader::OfferList::Iterator it(traderList.begin());
  for( ; it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);

    KatePluginInfo *info=new KatePluginInfo;

    info->load = false;
    info->service = ptr;
    info->plugin = 0L;
    info->name=info->service->property("X-KATE-InternalName").toString();
    if (info->name.isEmpty()) info->name=info->service->library();
    m_pluginList.append(info);
  }
}

void KatePluginManager::loadConfig ()
{
  kapp->config()->setGroup("Kate Plugins");

  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if  (kapp->config()->readBoolEntry(m_pluginList.at(i)->name, false))
      m_pluginList.at(i)->load = true;
  }
}

void KatePluginManager::writeConfig ()
{
  kapp->config()->setGroup("Kate Plugins");

  for (uint i=0; i<m_pluginList.count(); i++)
  {
    kapp->config()->writeEntry(m_pluginList.at(i)->name, m_pluginList.at(i)->load);
  }
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

void KatePluginManager::loadPlugin (KatePluginInfo *item)
{
  item->load = (item->plugin = Kate::createPlugin (QFile::encodeName(item->service->library()), Kate::application()));
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
    if  (m_pluginList.at(i)->name==name)
    {
	if (m_pluginList.at(i)->plugin) return m_pluginList.at(i)->plugin; else break;
    }
  }
  return 0;
}


bool KatePluginManager::pluginAvailable(const QString &){return false;}
class Kate::Plugin *KatePluginManager::loadPlugin(const QString &,bool ){return 0;}
void KatePluginManager::unloadPlugin(const QString &,bool){;}

