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

#include "../kantmain.h"

#include <kuniqueapp.h>

class KantApp : public KUniqueApplication
{
    Q_OBJECT

  public:
    KantApp ();
    ~KantApp ();

    virtual int newInstance ();
    KantPluginManager *getPluginManager(){return pluginManager;};
  protected:
    void restore ();

    KantPluginManager *pluginManager;
    KantMainWindow *mainWindow;
};

#endif
