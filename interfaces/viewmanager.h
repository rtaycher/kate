/***************************************************************************
                          viewmanager.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
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

#ifndef _KATE_VIEWMANAGER_INCLUDE_
#define _KATE_VIEWMANAGER_INCLUDE_

#include <qwidget.h>
#include <kurl.h>

namespace Kate
{

class ViewManager : public QWidget
{
  Q_OBJECT

  public:
    ViewManager (QWidget *parent = 0L);
    virtual ~ViewManager ();

    // current active view
    virtual class View *getActiveView() { return 0L; };

    // open a file
    virtual void openURL (KURL) { ; };
};

};

#endif
