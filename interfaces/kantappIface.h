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
#ifndef _KANT_APP_IFACE_
#define _KANT_APP_IFACE_

#include "../main/kantmain.h"

#include <kapp.h>

#include "kantdocmanagerIface.h"
#include "kantviewmanagerIface.h"

class KantAppIface : public KApplication
{
  Q_OBJECT

  public:
    KantAppIface () : KApplication () {;};
    virtual ~KantAppIface () {;};

    virtual KantViewManagerIface *viewManagerIface ()=0;
    virtual KantDocManagerIface *docManagerIface ()=0;
};

#endif
