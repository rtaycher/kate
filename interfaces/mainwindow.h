/***************************************************************************
                          mainwindow.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

#ifndef _KATE_MAINWINDOW_INCLUDE_
#define _KATE_MAINWINDOW_INCLUDE_

#include <kparts/dockmainwindow.h>

namespace Kate
{

/**
 I want to stay source compatible, so I don't remove the KParts::DockMainWindow. This will happen in KDE 4.0 though
 NEVER CALL A KParts::DockMainWindow method directly. Use the ToolViewManager !!!!!
 **/
class MainWindow : public KParts::DockMainWindow
{
  Q_OBJECT

  protected:
    MainWindow ();
    virtual ~MainWindow ();    
    
  public:
    virtual class ViewManager *viewManager () = 0;

    enum DockWidgetMode {ClassicStyle,IDEAlStyle};
    virtual class ToolViewManager *toolViewManager() = 0;
    virtual void* interfaces(const QString &name)=0; //"views" and "toolViews" at the moment
};

};

#endif
