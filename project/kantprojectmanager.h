/***************************************************************************
                          kantprojectmanager.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef kant_projectmanager_h
#define kant_projectmanager_h

#include "../kantmain.h"

#include <qobject.h>
#include <kurl.h>

class KantProjectManager : public QObject
{
  Q_OBJECT

  public:
    KantProjectManager (KantDocManager *docManager=0, KantViewManager *viewManager=0, KStatusBar *statusBar=0);
    ~KantProjectManager ();

    KURL projectFile;

  private:
    KantDocManager *docManager;
    KantViewManager *viewManager;
    KStatusBar *statusBar;

  public slots:
    void slotProjectNew();
    void slotProjectOpen();
    void slotProjectSave();
    void slotProjectSaveAs();
    void slotProjectConfigure();
    void slotProjectCompile();
    void slotProjectRun();
};

#endif
