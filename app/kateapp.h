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

#include <qptrlist.h>

class KateApp : public Kate::Application
{
  Q_OBJECT

  public:
    KateApp ();
    ~KateApp ();         
    
    int newInstance();
    
    KatePluginManager *katePluginManager() { return m_pluginManager; };
    KateDocManager *kateDocumentManager () { return m_docManager; };
    
    class KateMainWindow *newMainWindow ();
    void removeMainWindow (KateMainWindow *mainWindow);
    
    void raiseCurrentMainWindow ();   
    
    Kate::DocumentManager *documentManager () { return (Kate::DocumentManager*)m_docManager; };
    Kate::MainWindow *activeMainWindow ();
    
    uint mainWindows () { return m_mainWindows.count(); };
    Kate::MainWindow *mainWindow (uint n) { return (Kate::MainWindow *)m_mainWindows.at(n); };
    
    KateMainWindow *kateMainWindow (uint n) { return m_mainWindows.at(n); };
        
  private:
    KateDocManager *m_docManager;
    KatePluginManager *m_pluginManager;
    QPtrList<class KateMainWindow> m_mainWindows;
    bool m_firstStart;
    
  public:
    void openURL (const QString &name=0L);
};

#endif
