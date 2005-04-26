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

class KDE_EXPORT KateApp : public KApplication
{
  Q_OBJECT

  public:
    KateApp ();
    ~KateApp ();

    inline static KateApp *self () { return (KateApp *) kapp; }

    Kate::Application *application () { return m_application; };

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

    KatePluginManager *katePluginManager() { return m_pluginManager; }
    KateDocManager *kateDocumentManager () { return m_docManager; }
    KateSessionManager *kateSessionManager () { return m_sessionManager; }

    KateMainWindow *newMainWindow (KConfig *sconfig = 0, const QString &sgroup = "");

    void removeMainWindow (KateMainWindow *mainWindow);

    Kate::DocumentManager *documentManager () { return m_docManager->documentManager(); };
    Kate::PluginManager *pluginManager () { return m_pluginManager->pluginManager(); };
    Kate::MainWindow *activeMainWindow ();
    KateMainWindow *activeKateMainWindow ();

    uint mainWindows () { return m_mainWindows.count(); };
    Kate::MainWindow *mainWindow (uint n) { if (m_mainWindows.at(n)) return m_mainWindows.at(n)->mainWindow(); else return 0; }

    KateMainWindow *kateMainWindow (uint n) { return m_mainWindows.at(n); }

    bool openURL (const KURL &url, const QString &encoding);
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
