/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#ifndef _KATE_APPLICATION_INCLUDE_
#define _KATE_APPLICATION_INCLUDE_

#include <qobject.h>
#include <kurl.h>


namespace KTextEditor
{
  class Editor;
}

/**
 * \brief Namespace for Kate application interfaces.
 *
 * This namespace contains interfaces for plugin developers for the Kate
 * application.
 */
namespace Kate
{

class DocumentManager;
class PluginManager;
class InitPluginManager;
class MainWindow;

/**
 * \brief Central interface to the application.
 *
 * The class Application is central as it provides access to the core
 * application, which includes the
 * - document manager, access it with documentManager()
 * - plugin manager, access it with pluginManager()
 * - main windows, access a main window with mainWindow() or activeMainWindow()
 * - used editor component, access it with editor().
 *
 * To access the application use the global accessor function application().
 * You should never have to create an instance of this class yourself.
 *
 * \author Christoph Cullmann \<cullmann@kde.org\>
 */
class KDE_EXPORT Application : public QObject
{
  friend class PrivateApplication;

  Q_OBJECT

  public:
    /**
     * Construtor.
     *
     * The constructor is internally used by the Kate application, so it is
     * of no interest for plugin developers. Plugin developers should use the
     * global accessor application() instead.
     *
     * \internal
     */
    Application (void *application);

    /**
     * Virtual desctructor.
     */
    virtual ~Application ();

  public:
    /**
     * Accessor to the document manager.
     * \return a pointer to the document manager
     */
    Kate::DocumentManager *documentManager ();

    /**
     * Accessor to the plugin manager.
     * \return a pointer to the plugin manager
     */
    Kate::PluginManager *pluginManager ();

    /**
     * Accessor to the active mainwindow.
     * Usually there always is an active mainwindow, so it is not necessary
     * to test the returned value for NULL.
     * \return a pointer to the active mainwindow
     */
    Kate::MainWindow *activeMainWindow ();

    /**
     * Get the amount of mainwindows.
     * \return amount of mainwindows
     * \see activeMainWindow(), mainWindow()
     */
    uint mainWindows ();

    /**
     * Accessor to the mainwindow with index @p n.
     * \return mainwindow with index @p n
     * \see mainWindows()
     */
    Kate::MainWindow *mainWindow (uint n = 0);

    /**
     * Accessor to the global editor part.
     * \return KTextEditor component
     */
    KTextEditor::Editor *editor();

  private:
    class PrivateApplication *d;
};

/**
 * Global accessor to the application object.
 * \return application object
 */
Application *application ();

}

#endif
