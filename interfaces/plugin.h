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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
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

/**
 * \brief Kate plugin interface.
 *
 * Topics:
 *  - \ref intro
 *  - \ref config
 *  - \ref views
 *  - \ref configpages
 *
 * \section intro Introduction
 *
 * The Plugin class is the central part of a Kate plugin. If you want your
 * plugin to be present in the GUI as well you have to also derive it from
 * the class PluginViewInterface.
 *
 * \section config Configuration Management
 *
 * When Kate loads a plugin it calls loadGeneralConfig(), so if you have
 * config settings use this function to load them. To save config settings
 * fill storeGeneralConfig(), as it will be called right before the plugin
 * is unloaded.
 *
 * \section views Plugin Views
 *
 * If your plugin needs to be present in the GUI (e.g. menu or toolbar
 * entries) you have to additionally derive your plugin from the class
 * PluginViewInterface, like this:
 * \code
 * class MyPlugin : public Kate::Plugin,
 *                  public Kate::PluginViewInterface
 * {
 *     Q_OBJECT
 *     Q_INTERFACES(Kate::PluginViewInterface) // important for qobject_cast!
 *
 * public:
 *     // other methods etc...
 * };
 * \endcode
 * Now there are several other methods like addView() and removeView() which
 * are to be used to attach new elements into the GUI. Read the documentation
 * about the PluginViewInterface for further details.
 *
 * \section configpages Config Pages
 *
 * If your plugin is configurable it makes sense to have config pages which
 * appear in Kate's settings dialog. To tell the plugin that loader your
 * plugin supports config pages you have to additionally derive your plugin
 * from the class PluginConfigPageInterface, like this:
 * \code
 * class MyPlugin : public Kate::Plugin,
 *                  public Kate::PluginConfigPageInterface
 * {
 *     Q_OBJECT
 *     Q_INTERFACES(Kate::PluginConfigPageInterface) // important for qobject_cast!
 *
 * public:
 *     // other methods etc...
 * };
 * \endcode
 * Now there are several new methods which you have to reimplement, for
 * example to tell Kate how many config pages the plugin supports. Read the
 * documentation about the PluginConfigPageInterface for further details.
 * 
 * \see PluginViewInterface, PluginConfigPageInterface
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KDE_EXPORT Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    /**
     * Constructor.
     * \param application the Kate application
     * \param name identifier
     */
    Plugin (Application *application = 0, const char *name = 0 );
    /**
     * Virtual destructor.
     */
    virtual ~Plugin ();

    /**
     * For internal reason every plugin has a unique global number.
     * \return unique identifier
     */
    unsigned int pluginNumber () const;

    /**
     * Accessor to the Kate application.
     * \return the application object
     */
    Application *application() const;

    /**
     * Store general config settings here.
     * This function is called right before a plugin is unloaded. You should
     * use the given \p config and prefix \p groupPrefix to store the data.
     * The group prefix exist so that the group does not clash with other
     * applications that use the same config file.
     * \param config the KConfig object which is to be used
     * \param groupPrefix the group prefix which is to be used
     * \see loadGeneralConfig()
     */
    virtual void storeGeneralConfig(KConfig* config,const QString& groupPrefix)=0;
    /**
     * Load general config settings here.
     * This function is called right after a plugin was loaded. You should
     * use the given \p config and prefix \p groupPrefix to store the data.
     * The group prefix exist so that the group does not clash with other
     * applications that use the same config file.
     * \param config the KConfig object which is to be used
     * \param groupPrefix the group prefix which is to be used
     * \see storeGeneralConfig()
     */
    virtual void loadGeneralConfig(KConfig* config,const QString& groupPrefix)=0;

  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};

/**
 * Helper function for the Kate application to create new plugins.
 * \param libname the plugin/library name
 * \param application the application
 * \param name identifier
 * \param args arguments
 * \return the plugin on success, otherwise NULL 
 */
Plugin *createPlugin ( const char* libname, Application *application = 0, const char *name = 0,const QStringList &args = QStringList() );

/**
 * \brief Plugin view extension interface.
 *
 * \section intro Introduction
 *
 * @todo description
 * view plugin class
 * this plugin will be bound to a MainWindow
 */
class KDE_EXPORT PluginViewInterface
{
  friend class PrivatePluginViewInterface;

  public:
    /**
     * Constructor.
     */
    PluginViewInterface ();
    /**
     * Virtual destructor.
     */
    virtual ~PluginViewInterface ();

    /**
     * For internal reason every plugin view has a unique global number.
     * \return unique identifier
     */
    unsigned int pluginViewInterfaceNumber () const;

    /**
     * This function is called from Kate for every MainWindow, so that a plugin
     * can register its own GUI in the given \p mainwindow.
     * \see removeView()
     */
    virtual void addView (MainWindow *mainwindow) = 0;
    /**
     * This function is called from Kate for every MainWindow, so that a plugin
     * can cleanly unregister its own GUI from the given \p mainwindow.
     * \see addView()
     */
    virtual void removeView (MainWindow *mainwindow) = 0;
    /**
     * Store \p mainwindow specific config settings here.
     * This function is called right before a plugin is unloaded. You should
     * use the given \p config and prefix \p groupPrefix to store the data.
     * The group prefix exist so that the group does not clash with other
     * applications that use the same config file.
     * \param config the KConfig object which is to be used
     * \param mainwindow the MainWindow
     * \param groupPrefix the group prefix which is to be used
     * \see loadViewConfig()
     */
    virtual void storeViewConfig(KConfig* config, MainWindow* mainwindow, const QString& groupPrefix)=0;
    /**
     * Load \p mainwindow specific config settings here.
     * This function is called right after a plugin was loaded. You should
     * use the given \p config and prefix \p groupPrefix to store the data.
     * The group prefix exist so that the group does not clash with other
     * applications that use the same config file.
     * \param config the KConfig object which is to be used
     * \param mainwindow the MainWindow
     * \param groupPrefix the group prefix which is to be used
     * \see loadViewConfig()
     */
    virtual void loadViewConfig(KConfig* config, MainWindow* mainwindow, const QString& groupPrefix)=0;

  private:
    class PrivatePluginViewInterface *d;
    static unsigned int globalPluginViewInterfaceNumber;
    unsigned int myPluginViewInterfaceNumber;
};

/**
 * Helper function that returns the PluginViewInterface of the \p plugin
 * or NULL if the \p plugin does not support the interface.
 * \param plugin the plugin for which the view interface be returned
 * \return the view interface or NULL if the plugin does not
 *         support the interface
 */
PluginViewInterface *pluginViewInterface (Plugin *plugin);

}

Q_DECLARE_INTERFACE(Kate::PluginViewInterface,"org.kde.Kate.PluginViewInterface")

#endif
