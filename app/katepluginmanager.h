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

#ifndef __KATE_PLUGINMANAGER_H__
#define __KATE_PLUGINMANAGER_H__

#include "katemain.h"
#include "../interfaces/plugin.h"
#include "../interfaces/pluginmanager.h"

#include <qobject.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <ktrader.h>

class KatePluginInfo
{
  public:
    bool load;
    KService::Ptr service;
    Kate::Plugin *plugin;
};

typedef QPtrList<KatePluginInfo> KatePluginList;

class KatePluginManager : public QObject
{
  Q_OBJECT

  public:
    KatePluginManager(QObject *parent);
    ~KatePluginManager();

    Kate::PluginManager *pluginManager () const { return m_pluginManager; };

    void loadAllEnabledPlugins ();
    void unloadAllPlugins ();
    
    void enableAllPluginsGUI (KateMainWindow *win);
    void disableAllPluginsGUI (KateMainWindow *win);

    void loadConfig ();
    void writeConfig ();

    void loadPlugin (KatePluginInfo *item);
    void unloadPlugin (KatePluginInfo *item);
    
    void enablePluginGUI (KatePluginInfo *item, KateMainWindow *win);
    void enablePluginGUI (KatePluginInfo *item);
    
    void disablePluginGUI (KatePluginInfo *item, KateMainWindow *win);
    void disablePluginGUI (KatePluginInfo *item);

    inline KatePluginList & pluginList () { return m_pluginList; };

    virtual Kate::Plugin *plugin(const QString &name);
    virtual bool pluginAvailable(const QString &name);
    virtual class Kate::Plugin *loadPlugin(const QString &name,bool permanent=true);
    virtual void unloadPlugin(const QString &name,bool permanent=true);

  private:
    Kate::PluginManager *m_pluginManager;

    void setupPluginList ();

    KatePluginList m_pluginList;
};

#endif
