/***************************************************************************
                          kantpluginmanager.h  -  description
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
#ifndef _kant_pluginmanager_h_
#define _kant_pluginmanager_h_

#include "../main/kantmain.h"

class KantPlugin;

#include <qobject.h>
#include <qvaluelist.h>
#include <qlist.h>

struct PluginListItem
{
  bool load;
  QString libname;
  QString config;
  QString name;
  QString description;
  QString author;
};

typedef QList<PluginListItem> PluginList;

class KantPluginManager : public QObject
{
  Q_OBJECT

  friend class KantConfigPluginPage;
  friend class KantMainWindow;

  public:
    KantPluginManager(QObject *parent);
    ~KantPluginManager();

    void loadAllEnabledPlugins ();
    void enabledAllPluginsGUI (KantMainWindow *win);

  private:
    void setupPluginList ();
    void loadPlugin (PluginListItem *item);

    PluginList myPluginList;
    QList<KantPlugin> loadedPlugins;
};

#endif
