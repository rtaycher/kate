 /***************************************************************************
                          kantplugin.h  -  description
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

#include "kantappIface.h"

#include <qobject.h>
#include <kxmlguiclient.h>
#include <qlist.h>
#include <qstring.h>

class KantPluginViewIface : virtual public KXMLGUIClient
{
  public:
    KantPluginViewIface () {;};
    ~KantPluginViewIface () {;};

    void setXML (QString filename)
      { setXMLFile( filename ); };
};

class KantPluginIface : public QObject
{
  Q_OBJECT

  public:
    KantPluginIface (QObject* parent = 0, const char* name = 0) : QObject (parent, name)
    { appIface = (KantAppIface *) parent; };

    virtual ~KantPluginIface () {;};

    virtual KantPluginViewIface *createView ()=0;

    QList<KantPluginViewIface> viewList;

    KantAppIface *appIface;
};

#endif
