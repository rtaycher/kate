/***************************************************************************
                          kantIface.h  -  description
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
#ifndef _kant_App_Iface_h_
#define _kant_App_Iface_h_

#include <dcopobject.h>

class KantAppIface : virtual public DCOPObject
{
  K_DCOP

  k_dcop:
    virtual QString isSingleInstance()=0;
};
#endif
