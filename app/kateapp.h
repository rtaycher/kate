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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kate_app_h__
#define __kate_app_h__

#include "katemain.h"
#include "../interfaces/application.h"
#include "../interfaces/mainwindow.h"
#include "../interfaces/documentmanager.h"
#include "../interfaces/viewmanager.h"
#include "../interfaces/plugin.h"
#include <qptrlist.h>

#include "katemainwindow.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"

#include <kapplication.h>

class KateSessionManager;

class KCmdLineArgs;

/**
 * Kate Application
 * This class represents the core kate application object
 */
class KDE_EXPORT KateApp : public KApplication
{
  Q_OBJECT

  /**
   * constructors & accessor to app object + plugin interface for it
   */
  public:
    /**
     * application constructor
     */
    KateApp ();

    /**
     * application destructor
     */
    ~KateApp ();

    /**
     * static accessor to avoid casting ;)
     * @return app instance
     */
    static KateApp *self () { return (KateApp *) kapp; }

    /**
     * accessor to the Kate::Application plugin interface
     * @return application plugin interface
     */
    Kate::Application *application () { return m_application; };

  /**
   * event loop stuff
   */
  private slots:
    /**
     * HACK: this slot is connected to a singleshot timer, to
     * get start of event processing
     */
    void callOnEventLoopEnter();

  signals:
    /**
     * emited as soon as kate's event loop has started
     */
    void onEventLoopEnter();

  /**
   * restore/start/shutdown
   */
  public:
    /**
     * restore a old kate session
     */
    void restoreKate ();

    /**
     * try to start kate with given args
     * @param args command line args
     * @return success, if false, kate should exit
     */
    bool startupKate (KCmdLineArgs *args);

    /**
     * shutdown kate application
     * @param win mainwindow which is used for dialogs
     */
    void shutdownKate (KateMainWindow *win);

  /**
   * other accessors for global unique instances
   */
  public:
    /**
     * accessor to plugin manager
     * @return plugin manager instance
     */
    KatePluginManager *pluginManager() { return m_pluginManager; }

    /**
     * accessor to document manager
     * @return document manager instance
     */
    KateDocManager *documentManager () { return m_docManager; }

    /**
     * accessor to session manager
     * @return session manager instance
     */
    KateSessionManager *sessionManager () { return m_sessionManager; }

  /**
   * window management
   */
  public:
    /**
     * create a new main window, use given config if any for restore
     * @param sconfig session config object
     * @param sgroup session group for this window
     * @return new constructed main window
     */
    KateMainWindow *newMainWindow (KConfig *sconfig = 0, const QString &sgroup = "");

    /**
     * removes the mainwindow given, DOES NOT DELETE IT
     * @param mainWindow window to remove
     */
    void removeMainWindow (KateMainWindow *mainWindow);

    /**
     * give back current active main window
     * can only be 0 at app start or exit
     * @return current active main window
     */
    KateMainWindow *activeMainWindow ();

    /**
     * give back number of existing main windows
     * @return number of main windows
     */
    uint mainWindows () { return m_mainWindows.count(); };

    /**
     * give back the window you want
     * @param n window index
     * @return requested main window
     */
    KateMainWindow *mainWindow (uint n) { return m_mainWindows.at(n); }

  /**
   * some stuff for the dcop API
   */
  public:
    /**
     * open url with given encoding
     * used by kate if --use given
     * @param url filename
     * @param encoding encoding name
     * @return success
     */
    bool openURL (const KURL &url, const QString &encoding);

    /**
     * position cursor in current active view
     * @param line line to set
     * @param column column to set
     * @return success
     */
    bool setCursor (int line, int column);

  private:
    Kate::Application *m_application;
    KateDocManager *m_docManager;
    KatePluginManager *m_pluginManager;
    KateSessionManager *m_sessionManager;
    QPtrList<KateMainWindow> m_mainWindows;
    class KateAppDCOPIface *m_obj;
};

#endif
