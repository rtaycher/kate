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

#include <kuniqueapplication.h>

namespace Kate
{
/** This interface provides access to the central Kate objects */
class Application : public KUniqueApplication
{
  Q_OBJECT

  protected:
    Application ();
    virtual ~Application ();
    
  public:
    /** Returns a pointer to the document manager
    */
    virtual class DocumentManager *documentManager () = 0;

    virtual class PluginManager *pluginManager () = 0;
    
    virtual class MainWindow *activeMainWindow () = 0;
    
    virtual uint mainWindows () = 0;
    virtual class MainWindow *mainWindow (uint n) = 0;
  
    virtual void *topLevelInterfaces(const QString& name)=0; // for later easier BC interface extension.
	//At the moment there are "documents","plugins","activeWindow" which behave like the methods above
};

class InitPluginManager
{
   public:
    InitPluginManager();
    virtual ~InitPluginManager();
    virtual void performInit(const QString &libname, const KURL &initScript)=0;
    virtual class InitPlugin *initPlugin() const =0;
    virtual class KURL  initScript() const =0;

};

InitPluginManager *initPluginManager(Application *app);

};

#endif
