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
#include "kateprojectmanager.h"
#include "katepluginmanager.h"

#include <kuniqueapplication.h>

class KateApp : public KUniqueApplication
{
  Q_OBJECT

  public:
    KateApp (bool forcedNewProcess, bool oldState);
    ~KateApp ();

    Kate::Application *application () { return m_application; };

  public:
    int newInstance();

    KatePluginManager *katePluginManager() { return m_pluginManager; };
    KateDocManager *kateDocumentManager () { return m_docManager; };

    class KateMainWindow *newMainWindow ();
    class KateMainWindow *newMainWindow (bool visible);
    
    void removeMainWindow (KateMainWindow *mainWindow);

    void raiseCurrentMainWindow ();

    Kate::DocumentManager *documentManager () { return m_docManager->documentManager(); };
    Kate::ProjectManager *projectManager () { return m_projectManager->projectManager(); };
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
    
    static KConfig *kateSessionConfig () { return m_sessionConfig; }

  signals:
    void onEventLoopEnter();

  private:
    Kate::Application *m_application;
    Kate::InitPluginManager *m_initPluginManager;
    KateDocManager *m_docManager;
    KateProjectManager *m_projectManager;
    KatePluginManager *m_pluginManager;
    QPtrList<class KateMainWindow> m_mainWindows;
    bool m_firstStart;
    Kate::InitPlugin *m_initPlugin;
    int m_doNotInitialize;
    KURL m_initURL;
    QString m_initLib;
    QString m_oldInitLib;
    class KateAppDCOPIface *m_obj;
    KMdi::MdiMode m_restoreGUIMode;
    static KConfig *m_sessionConfig;
    bool m_sessionConfigDelete;
    
  protected slots:
    void performInit();
    void callOnEventLoopEnter();
};

#endif
