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

#include <kuniqueapplication.h>

class KateSessionManager;

class KDE_EXPORT KateApp : public KUniqueApplication
{
  Q_OBJECT

  public:
    KateApp (bool forcedNewProcess, bool oldState);
    ~KateApp ();

    inline static KateApp *self () { return (KateApp *) kapp; }

    Kate::Application *application () { return m_application; };

  public:
    int newInstance();

    void shutdownKate (KateMainWindow *win);

    KatePluginManager *katePluginManager() { return m_pluginManager; }
    KateDocManager *kateDocumentManager () { return m_docManager; }
    KateSessionManager *kateSessionManager () { return m_sessionManager; }

    class KateMainWindow *newMainWindow ();
    class KateMainWindow *newMainWindow (bool visible, KConfig *sconfig = 0, const QString &sgroup = "");

    void removeMainWindow (KateMainWindow *mainWindow);

    Kate::DocumentManager *documentManager () { return m_docManager->documentManager(); };
    Kate::PluginManager *pluginManager () { return m_pluginManager->pluginManager(); };
    Kate::InitPluginManager *initPluginManager () { return m_initPluginManager; };
    Kate::MainWindow *activeMainWindow ();
    KateMainWindow *activeKateMainWindow ();

    uint mainWindows () { return m_mainWindows.count(); };
    Kate::MainWindow *mainWindow (uint n) { if (m_mainWindows.at(n)) return m_mainWindows.at(n)->mainWindow(); else return 0; }

    KateMainWindow *kateMainWindow (uint n) { return m_mainWindows.at(n); }

    void openURL (const QString &name=0L);

    virtual void performInit(const QString &, const KURL &);
    virtual Kate::InitPlugin *initPlugin() const;
    virtual KURL initScript() const;

  signals:
    void onEventLoopEnter();

  private:
    Kate::Application *m_application;
    Kate::InitPluginManager *m_initPluginManager;
    KateDocManager *m_docManager;
    KatePluginManager *m_pluginManager;
    KateSessionManager *m_sessionManager;
    QPtrList<class KateMainWindow> m_mainWindows;
    bool m_firstStart;
    Kate::InitPlugin *m_initPlugin;
    int m_doNotInitialize;
    KURL m_initURL;
    QString m_initLib;
    QString m_oldInitLib;
    class KateAppDCOPIface *m_obj;
    KConfig *m_sessionConfig;

  protected slots:
    void performInit();
    void callOnEventLoopEnter();
};

#endif
