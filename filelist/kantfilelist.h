/***************************************************************************
                          kantfilelist.h  -  description
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

#ifndef KANTFILELIST_H
#define KANTFILELIST_H

#include "../kantmain.h"

#include <klistbox.h>

class KantFileList : public KListBox
{
  Q_OBJECT

  public:
    KantFileList (QWidget * parent = 0, const char * name = 0 );
    ~KantFileList ();
};

#endif
