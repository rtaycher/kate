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

#include <kapp.h>
#include <qlist.h>

class KantApp : public KApplication
{
  Q_OBJECT

  friend class KantViewManager;

  public:
    KantApp ();
    ~KantApp ();

    KantPluginManager *getPluginManager(){return pluginManager;};

    void newMainWindow ();
    void removeMainWindow (KantMainWindow *mainWindow);
    long mainWindowsCount ();

  private:
    KantDocManager *docManager;
    KantPluginManager *pluginManager;

    QList<KantMainWindow> mainWindows;
};

#endif
