/***************************************************************************
                          kateapp.h  -  description
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
#ifndef __kate_app_h__
#define __kate_app_h__

#include "../main/katemain.h"
#include "kateappIface.h"
#include "../interfaces/kateappIface.h"

#include <qlist.h>

class KateApp : public KateAppIface, public KateAppDCOPIface
{
  Q_OBJECT

  friend class KateViewManager;
  friend class KateMyPluginIface;
  friend class KatePluginManager;

  public:
    KateApp ();
    ~KateApp ();

    KatePluginManager *getPluginManager(){return pluginManager;};

    void newMainWindow ();
    void removeMainWindow (KateMainWindow *mainWindow);
    long mainWindowsCount ();
    virtual QString  isSingleInstance(){if (_singleInstance) return "true"; else return "false";};

    KateViewManagerIface *viewManagerIface ();
    KateDocManagerIface *docManagerIface ();
    KStatusBar *statusBar ();

  private:
    bool _singleInstance;
    KateDocManager *docManager;
    KatePluginManager *pluginManager;

    QList<KateMainWindow> mainWindows;
};

#endif
