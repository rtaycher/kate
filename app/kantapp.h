/***************************************************************************
                          kantapp.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __kant_app_h__
#define __kant_app_h__

#include "../main/kantmain.h"
#include "kantappIface.h"
#include "../interfaces/kantpluginIface.h"

#include <kapp.h>
#include <qlist.h>

class KantMyPluginIface : public KantPluginIface
{
  Q_OBJECT

  public:
    KantMyPluginIface(QObject *parent):KantPluginIface(parent){;};
    ~KantMyPluginIface(){;};

    KantViewManagerIface *viewManagerIface ();
    KantDocManagerIface *docManagerIface ();
};

class KantApp : public KApplication, public KantAppIface
{
  Q_OBJECT

  friend class KantViewManager;
  friend class KantMyPluginIface;

  public:
    KantApp ();
    ~KantApp ();

    KantPluginManager *getPluginManager(){return pluginManager;};

    void newMainWindow ();
    void removeMainWindow (KantMainWindow *mainWindow);
    long mainWindowsCount ();
    virtual QString  isSingleInstance(){if (_singleInstance) return "true"; else return "false";};

    KantMyPluginIface *pluginIface;

    KantViewManagerIface *viewManagerIface ();
    KantDocManagerIface *docManagerIface ();

  private:
    bool _singleInstance;
    KantDocManager *docManager;
    KantPluginManager *pluginManager;

    QList<KantMainWindow> mainWindows;
};

#endif
