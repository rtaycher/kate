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

#include "../../main/kantmain.h"

#include "../../pluginmanager/kantplugin.h"

#include <klibloader.h>
#include <kantpluginIface.h>
#include <kurl.h>

class KantPluginFactory : public KLibFactory
{
  Q_OBJECT

  public:
    KantPluginFactory();
    virtual ~KantPluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

  private:
    static KInstance* s_instance;
};


class KantProjectManager : public KantPlugin
{
  Q_OBJECT

  public:
    KantProjectManager (QObject* parent = 0, const char* name = 0);
    ~KantProjectManager ();

    KantPluginView *createView ();

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
