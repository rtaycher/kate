/***************************************************************************
                          pluginmanager.h -  description
                             -------------------
    begin                : Mon July 14 2002
    copyright            : (C) 2002 by Joseph Wenninger
    email                : jowenn@kde.org
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

#ifndef _KATE_PLUGINMANAGER_INCLUDE_
#define _KATE_PLUGINMANAGER_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{
/** This interface provides access to the Kate Plugin Manager.
*/
class PluginManager : public QObject
{
  Q_OBJECT   
  
  protected:
    PluginManager ( QObject *parent = 0, const char *name = 0  );
    virtual ~PluginManager ();

  public:
    /** if the plugin with the library name "name" is loaded, a pointer to that plugin is returned */
    virtual class Kate::Plugin *plugin(const QString &name)=0;

    /** return true, if plugin is known to kate (either loaded or not loaded)
     * 
     * This method is not used yet
     */
    virtual bool pluginAvailable(const QString &name)=0;

    /** tries loading the specified plugin and returns a pointer to the new plugin or 0
     *  if permanent is true (default value) the plugin will be loaded at the next kate startup
     * 
     * This method is not used yet
     */
    virtual class Kate::Plugin *loadPlugin(const QString &name,bool permanent=true)=0;
	
    /** unload the specified plugin. If the value permanent is true (default value), the plugin will not be
     * loaded on kate's next startup. Even if it had been loaded with permanent=true.
     * 
     * This method is not used yet
     */
    virtual void unloadPlugin(const QString &name,bool permanent=true)=0;
   
};

};

#endif
