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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_APPLICATION_INCLUDE_
#define _KATE_APPLICATION_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{

class DocumentManager;
class PluginManager;
class InitPluginManager;
class MainWindow;

/**
 * Interface to the application, beside some global methodes to access
 * other objects like document/projectmanager, ... no way goes around this
 * central interface
 */
class KDE_EXPORT Application : public QObject
{
  friend class PrivateApplication;

  Q_OBJECT

  public:
    /**
     * Construtor, should not interest, internal usage
     */
    Application (void *application);

    /**
     * Desctructor
     */
    virtual ~Application ();

  public:
    /** Returns a pointer to the document manager
    */
    Kate::DocumentManager *documentManager ();

    Kate::PluginManager *pluginManager ();

    Kate::MainWindow *activeMainWindow ();

    uint mainWindows ();
    Kate::MainWindow *mainWindow (uint n = 0);

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

/**
 * Returns the application object
 * @return Application application object
 */
Application *application ();

}

#endif
