/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KATE_PLUGIN_INCLUDE_
#define _KATE_PLUGIN_INCLUDE_

#include <qwidget.h>
#include <qpixmap.h>
#include <kicontheme.h>

#include <kurl.h>

namespace Kate
{

class Application;
class MainWindow;

class KDE_EXPORT Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    Plugin (Application *application = 0, const char *name = 0 );
    virtual ~Plugin ();

    unsigned int pluginNumber () const;

    Application *application() const;

    virtual void storeGeneralConfig(KConfig*,const QString& groupPrefix)=0;
    virtual void loadGeneralConfig(KConfig*,const QString& groupPrefix)=0;

  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};

Plugin *createPlugin ( const char* libname, Application *application = 0, const char *name = 0,const QStringList &args = QStringList() );

/*
 * view plugin class
 * this plugin will be bound to a MainWindow
 */
class KDE_EXPORT PluginViewInterface
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
    virtual void storeViewConfig(KConfig*, MainWindow* view, const QString& groupPrefix)=0;
    virtual void loadViewConfig(KConfig*, MainWindow* view, const QString& groupPrefix)=0;

  private:
    class PrivatePluginViewInterface *d;
    static unsigned int globalPluginViewInterfaceNumber;
    unsigned int myPluginViewInterfaceNumber;
};

PluginViewInterface *pluginViewInterface (Plugin *plugin);

}

Q_DECLARE_INTERFACE(Kate::PluginViewInterface,"org.kde.Kate.PluginViewInterface");

#endif
