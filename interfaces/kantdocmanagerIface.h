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
#ifndef _KANT_DOCMANAGER_IFACE_
#define _KANT_DOCMANAGER_IFACE_

#include "../kantmain.h"

#include <qobject.h>

class KantDocManagerIface : public QObject
{
  Q_OBJECT

  public:
    KantDocManagerIface () : QObject () {;};
    ~KantDocManagerIface () {;};
};

#endif
