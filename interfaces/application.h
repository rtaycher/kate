/***************************************************************************
                          application.h -  description
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

#ifndef _KATE_APPLICATION_INCLUDE_
#define _KATE_APPLICATION_INCLUDE_

#include <kapp.h>

namespace Kate
{

class Application : public KApplication
{
  Q_OBJECT

  public:
    Application ();
    virtual ~Application ();

    // get the current active doc/viewmanager, mainwindow
    virtual class ViewManager *getViewManager () { return 0L; };
    virtual class DocManager *getDocManager () { return 0L; };
    virtual class MainWindow *getMainWindow () { return 0L; };
};

};

#endif
