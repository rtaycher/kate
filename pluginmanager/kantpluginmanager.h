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

#include <qobject.h>
#include <kdebug.h>
#include <qvaluelist.h>
#include <kparts/plugin.h>
 
class KantPluginManager : public QObject
  {
    Q_OBJECT
    public:
    KantPluginManager(QObject *parent);
    ~KantPluginManager(){qDebug("KantPluginManager destroyed");};
    QValueList<KParts::Plugin::PluginInfo> plugins;

  };

#endif
