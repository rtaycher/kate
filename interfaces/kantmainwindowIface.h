 /***************************************************************************
                          kantpluginiface.h  -  description
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
#ifndef _KANT_MAINWINDOW_IFACE_
#define _KANT_MAINWINDOW_IFACE_

#include "../kantmain.h"
#include "../kwrite/kwview.h"

#include <kdockwidget.h>

class KantMainWindowIface : public KDockMainWindow
{
  Q_OBJECT

  public:
    KantMainWindowIface (QWidget *parent, const char * name = 0) : KDockMainWindow (parent, name) {;};
    ~KantMainWindowIface () {;};
};

#endif
