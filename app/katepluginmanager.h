/***************************************************************************
                          katepluginmanager.h  -  description
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

#ifndef __KATE_PLUGINMANAGER_H__
#define __KATE_PLUGINMANAGER_H__

#include "katemain.h"
#include "../interfaces/plugin.h"

#include <qobject.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <ktrader.h>

class PluginInfo
{
  public:
    bool load;
    KService::Ptr service;
    Kate::Plugin *plugin;
};

typedef QPtrList<PluginInfo> PluginList;

class KatePluginManager : public QObject
{
  Q_OBJECT

  public:
    KatePluginManager(QObject *parent);
    ~KatePluginManager();

    void loadAllEnabledPlugins ();
    void enableAllPluginsGUI (KateMainWindow *win);

    void loadConfig ();
    void writeConfig ();

    void loadPlugin (PluginInfo *item);
    void unloadPlugin (PluginInfo *item);
    void enablePluginGUI (PluginInfo *item, KateMainWindow *win);
    void enablePluginGUI (PluginInfo *item);
    void disablePluginGUI (PluginInfo *item);      
    
    inline PluginList & pluginList () { return m_pluginList; };
    
  private:
    void setupPluginList ();
    
    PluginList m_pluginList;
};

#endif
