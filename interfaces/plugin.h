/***************************************************************************
                          plugin.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/* **************************************************************************
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

#ifndef _KATE_PLUGIN_INCLUDE_
#define _KATE_PLUGIN_INCLUDE_

#include <qwidget.h>
#include <qpixmap.h>
#include <kicontheme.h>

namespace Kate
{
                           
class Application;
class MainWindow;
                  
class Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    Plugin (Application *application = 0, const char *name = 0 );
    virtual ~Plugin ();
    
    unsigned int pluginNumber () const;
      
    Application *application() const;
    
  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};
   
Plugin *createPlugin ( const char* libname, Application *application = 0, const char *name = 0 );

/*
 * view plugin class
 * this plugin will be bound to a ktexteditor::view
 */
class PluginViewInterface
{
  friend class PrivatePluginViewInterface;

  public:
    PluginViewInterface ();
    virtual ~PluginViewInterface ();
    
    unsigned int pluginViewInterfaceNumber () const;
  
    /*
     * will be called from the part to bound the plugin to a view
     */
    virtual void addView (MainWindow *) = 0;
    virtual void removeView (MainWindow *) = 0;

  private:
    class PrivatePluginViewInterface *d;
    static unsigned int globalPluginViewInterfaceNumber;
    unsigned int myPluginViewInterfaceNumber;
};         

PluginViewInterface *pluginViewInterface (Plugin *plugin);

};

#endif
