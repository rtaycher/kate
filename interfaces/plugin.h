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

#include <kdebase_export.h>

#include <QWidget>
#include <QPixmap>
#include <kicontheme.h>

#include <kurl.h>

namespace Kate
{

  class Application;
  class MainWindow;
  class PluginView;

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
   * When Kate loads a session it calls loadGeneralConfig(), so if you have
   * config settings use this function to load them. To save config settings
   * for a session use storeGeneralConfig(), as it will be called whenever a
   * session is saved/closed.
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
  class KATEINTERFACES_EXPORT Plugin : public QObject
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
       * Accessor to the Kate application.
       * \return the application object
       */
      Application *application() const;

      /**
       * Create a new View for this plugin for the given Kate MainWindow
       * This may be called arbitary often by the application to create as much
       * views as mainwindows are around, the application will take care to delete
       * this views if mainwindows close, you don't need to handle this yourself in
       * the plugin.
       * The default implementation just doesn't create any view and returns a NULL
       * pointer
       * \param mainWindow the MainWindow for which a view should be created
       * \return the new created view or NULL
       */
      virtual PluginView *createView (MainWindow *mainWindow);

      /**
       * Load session specific settings here.
       * This function is called whenever a Kate session is loaded. You
       * should use the given \p config and prefix \p groupPrefix to store the
       * data. The group prefix exist so that the group does not clash with
       * other applications that use the same config file.
       * \param config the KConfig object which is to be used
       * \param groupPrefix the group prefix which is to be used
       * \see writeSessionConfig()
       */
      virtual void readSessionConfig (KConfig* config, const QString& groupPrefix);

      /**
       * Store session specific settings here.
       * This function is called whenever a Kate session is saved. You
       * should use the given \p config and prefix \p groupPrefix to store the
       * data. The group prefix exists so that the group does not clash with
       * other applications that use the same config file.
       * \param config the KConfig object which is to be used
       * \param groupPrefix the group prefix which is to be used
       * \see readSessionConfig()
       */
      virtual void writeSessionConfig (KConfig* config, const QString& groupPrefix);

    private:
      class PrivatePlugin *d;
  };

  /**
   * Helper function for the Kate application to create new plugins.
   * \param libname the plugin/library name
   * \param application the application
   * \param args arguments
   * \return the plugin on success, otherwise NULL
   */
  KATEINTERFACES_EXPORT Plugin *createPlugin ( const char* libname, Application *application = 0,
      const QStringList &args = QStringList() );

  /**
   * \brief PluginView interface.
   *
   * Topics:
   *  - \ref intro
   *  - \ref views
   *  - \ref example
   *
   * \section intro Introduction
   *
   * The class PluginView is a interface for the view of a plugin.
   *
   * \section views Plugin Views
   *
   * The Kate application supports multiple mainwindows (Window > New Window).
   * For every Kate MainWindow createView() of the plugin is called, i.e. overwrite addView()
   * and hook your view into the given mainwindow's KXMLGUIFactory. That means
   * you have to create an own KXMLGUIClient derived \e PluginView class and
   * create an own instance for \e every mainwindow. One PluginView then is
   * bound to this specific MainWindow.
   *
   * removeView() is called for every MainWindow whenever a plugin view client
   * is to be removed.
   *
   * As already mentioned above, loadViewConfig() and storeViewConfig() is
   * called to load and save session data.
   *
   * \section example Basic PluginView Example
   *
   * A PluginView is bound to a single MainWindow. To add GUI elements KDE's
   * GUI XML frameworks is used, i.e. the MainWindow provides a KXMLGUIFactory
   * into which the KXMLGUIClient is to be hooked. So the plugin view must
   * inherit from KXMLGUIClient, the following example shows the basic skeleton
   * of the PluginView.
   * \code
   *   class PluginView : public QObject, public KXMLGUIClient
   *   {
   *       Q_OBJECT
   *   public:
   *       // Constructor and other methods
   *       PluginView( Kate::MainWindow* mainwindow )
   *         : QObject( mainwindow ), KXMLGUIClient( mainwindow ),
   *           m_mainwindow(mainwindow)
   *       { ... }
   *       // ...
   *   private:
   *       Kate::MainWindow* m_mainwindow;
   *   };
   * \endcode
   *
   * \see Plugin, KXMLGUIClient, MainWindow
   * \author Christoph Cullmann \<cullmann@kde.org\>
   */
  class KATEINTERFACES_EXPORT PluginView : public QObject
  {
      friend class PrivatePluginView;

      Q_OBJECT

    public:
      /**
       * Constructor.
       */
      PluginView (MainWindow *mainWindow);

      /**
       * Virtual destructor.
       */
      virtual ~PluginView ();

      /**
       * Accessor to the Kate mainwindow of this view.
       * \return the mainwindow object
       */
      MainWindow *mainWindow() const;

      /**
       * Load session specific settings here.
       * This function is called whenever a Kate session is loaded. You
       * should use the given \p config and prefix \p groupPrefix to store the
       * data. The group prefix exist so that the group does not clash with
       * other applications that use the same config file.
       * \param config the KConfig object which is to be used
       * \param groupPrefix the group prefix which is to be used
       * \see writeSessionConfig()
       */
      virtual void readSessionConfig (KConfig* config, const QString& groupPrefix);

      /**
       * Store session specific settings here.
       * This function is called whenever a Kate session is saved. You
       * should use the given \p config and prefix \p groupPrefix to store the
       * data. The group prefix exists so that the group does not clash with
       * other applications that use the same config file.
       * \param config the KConfig object which is to be used
       * \param groupPrefix the group prefix which is to be used
       * \see readSessionConfig()
       */
      virtual void writeSessionConfig (KConfig* config, const QString& groupPrefix);

    private:
      class PrivatePluginView *d;
  };

}

#endif
