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
#include "../interfaces/docmanager.h"
#include "../interfaces/viewmanager.h"

#include <qptrlist.h>

class KateApp : public Kate::Application
{
  Q_OBJECT

  friend class KateViewManager;
  friend class KatePluginManager;

  public:
    KateApp ();
    ~KateApp ();         
    
    int newInstance();
    
    KatePluginManager *getPluginManager(){return pluginManager;};

    class KateMainWindow *newMainWindow ();
    void removeMainWindow (KateMainWindow *mainWindow);
    uint mainWindowsCount ();
    
    void raiseCurrentMainWindow ();

    Kate::ViewManager *getViewManager ();
    Kate::DocManager *getDocManager ();
    Kate::MainWindow *getMainWindow ();

  private:
    KateDocManager *docManager;
    KatePluginManager *pluginManager;
    QPtrList<KateMainWindow> mainWindows;
    bool m_firstStart;
    
  public:
    void openURL (const QString &name=0L);
};

#endif
