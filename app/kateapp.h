/***************************************************************************
                          kateapp.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
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
    void removeMainWindow (KateMainWindow *mainWindow);
    
    void raiseCurrentMainWindow ();   
    
    Kate::DocumentManager *documentManager () { return m_docManager->documentManager(); };
    Kate::PluginManager *pluginManager () { return m_pluginManager->pluginManager(); };
    Kate::InitPluginManager *initPluginManager () { return m_initPluginManager; };
    Kate::MainWindow *activeMainWindow ();
    
    uint mainWindows () { return m_mainWindows.count(); };
    Kate::MainWindow *mainWindow (uint n) { return m_mainWindows.at(n)->mainWindow(); };
    
    KateMainWindow *kateMainWindow (uint n) { return m_mainWindows.at(n); };
                 
    void openURL (const QString &name=0L);  
    
    virtual void performInit(const QString &, const KURL &);
    virtual Kate::InitPlugin *initPlugin() const;
    virtual KURL initScript() const;
    
    int doNotInitialize(){return m_doNotInitialize;}

  private:
    Kate::Application *m_application;
    Kate::InitPluginManager *m_initPluginManager;
    KateDocManager *m_docManager;
    KatePluginManager *m_pluginManager;
    QPtrList<class KateMainWindow> m_mainWindows;
    bool m_firstStart;  
    bool m_noEventLoop;
    Kate::InitPlugin *m_initPlugin;
    int m_doNotInitialize;
    KURL m_initURL;
    QString m_initLib;
    QString m_oldInitLib;

    signals:
	void onEventLoopEnter();

  protected slots:
    void performInit();
    void callOnEventLoopEnter();
};

#endif
