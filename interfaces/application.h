/***************************************************************************
                          application.h -  description
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

#ifndef _KATE_APPLICATION_INCLUDE_
#define _KATE_APPLICATION_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{

/** This interface provides access to the central Kate objects */
class Application : public QObject
{
  friend class PrivateApplication;

  Q_OBJECT

  public:
    Application (void *application);
    virtual ~Application ();
    
  public:
    /** Returns a pointer to the document manager
    */
    class DocumentManager *documentManager ();

    class ProjectManager *projectManager ();
    
    class PluginManager *pluginManager ();
    
    class InitPluginManager *initPluginManager ();
    
    class MainWindow *activeMainWindow ();
    
    uint mainWindows ();
    class MainWindow *mainWindow (uint n = 0);

  //invention of public signals, like in kparts/browserextension.h
  #undef signals
  #define signals public
  signals:
  #undef signals
  #define signals protected
  
    void onEventLoopEnter();  
    
  private:
    class PrivateApplication *d;
};

Application *application ();

};

#endif
