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
#ifndef _KANT_PLUGIN_IFACE_
#define _KANT_PLUGIN_IFACE_

#include "../kantmain.h"
#include "../kwrite/kwview.h"

#include <qwidget.h>

class KantPluginIface : public QWidget
  {
    Q_OBJECT
    public:
      KantPluginIface(QWidget *parent):QWidget(parent){;};
      ~KantPluginIface(){;};
      virtual KWrite *getActiveView()=0;
  };  

#endif
