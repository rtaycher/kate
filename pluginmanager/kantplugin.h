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

#include "../main/kantmain.h"

#include <qobject.h>
#include <kxmlguiclient.h>
#include <qlist.h>
#include <qstring.h>

class KantPluginView : virtual public KXMLGUIClient
{
  friend class KantPluginManager;

  public:
    KantPluginView () {;};
    ~KantPluginView () {;};

    void setXML (QString filename)
      { setXMLFile( filename ); };
};

class KantPlugin : public QObject
{
  Q_OBJECT

  friend class KantPluginManager;

  public:
    KantPlugin (QObject* parent = 0, const char* name = 0) : QObject (parent, name) {;};
    ~KantPlugin () {;};

    virtual KantPluginView *createView ()=0;

    QList<KantPluginView> viewList;
};

#endif
