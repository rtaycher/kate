/***************************************************************************
                          kantpartdocument.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
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

#ifndef KANTPARTDOCUMENT_H
#define KANTPARTDOCUMENT_H

#include "../kantmain.h"

#include "../document/kantdocument.h"

class KantPartDocument : public KantDocument
{
  public:
    KantPartDocument ();
    ~KantPartDocument ();
};

#endif
