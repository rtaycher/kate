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

struct PluginListItem
{
  bool load;
  QString libname;
  QString name;
  QString description;
  QString author;
  Kate::Plugin *plugin;
};

typedef QPtrList<PluginListItem> PluginList;

class KatePluginManager : public QObject
{
  Q_OBJECT

  friend class KateConfigPluginPage;
  friend class KateConfigDialog;
  friend class KateMainWindow;
  friend class KateApp;

  public:
    KatePluginManager(QObject *parent);
    ~KatePluginManager();

    void loadAllEnabledPlugins ();
    void enableAllPluginsGUI (KateMainWindow *win);

  private:
    void setupPluginList ();
    void loadConfig ();
    void writeConfig ();

    void loadPlugin (PluginListItem *item);
    void unloadPlugin (PluginListItem *item);
    void enablePluginGUI (PluginListItem *item, KateMainWindow *win);
    void enablePluginGUI (PluginListItem *item);
    void disablePluginGUI (PluginListItem *item);

    PluginList myPluginList;
};

#endif
