 /***************************************************************************
                          kateplugin.h  -  description
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
#ifndef _KANT_plugin_
#define _KANT_plugin_

#include "kateappIface.h"

#include <qobject.h>
#include <kxmlguiclient.h>
#include <qlist.h>
#include <qstring.h>

class KatePluginViewIface : public QObject, virtual public KXMLGUIClient
{
  Q_OBJECT

  friend class KatePluginIface;

  public:
    KatePluginViewIface (QObject* parent = 0) : QObject (0L, 0L)
     { pluginIface = (KatePluginIface *) parent; };

    ~KatePluginViewIface () {;};

    void setXML (QString filename)
      { setXMLFile( filename ); }

      class KatePluginIface *pluginIface;;
};

class KatePluginIface : public QObject
{
  Q_OBJECT

  friend class KatePluginViewIface;

  public:
    KatePluginIface (QObject* parent = 0, const char* name = 0) : QObject (parent, name)
    { appIface = (KateAppIface *) parent; };

    virtual ~KatePluginIface () {;};

    virtual KatePluginViewIface *createView ()=0;

    QList<KatePluginViewIface> viewList;

    KateAppIface *appIface;
};

#endif
