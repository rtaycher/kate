/***************************************************************************
                          mainwindow.h -  description
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

#ifndef _KATE_MAINWINDOW_INCLUDE_
#define _KATE_MAINWINDOW_INCLUDE_

#include <kdockwidget.h>

namespace Kate
{

class MainWindow : public KDockMainWindow
{
  Q_OBJECT

  public:
    MainWindow () : KDockMainWindow () {;};
    virtual ~MainWindow () {;};

    virtual class ViewManager *getViewManager ()=0;
    virtual class DocManager *getDocManager ()=0;
};

};

#endif
