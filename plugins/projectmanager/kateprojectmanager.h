/***************************************************************************
                          kateprojectmanager.h  -  description
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

#ifndef kate_projectmanager_h
#define kate_projectmanager_h

#include "../../interfaces/plugin.h"
#include "../../interfaces/application.h"
#include "../../interfaces/view.h"
#include "../../interfaces/document.h"
#include "../../interfaces/docmanager.h"
#include "../../interfaces/viewmanager.h"

#include <klibloader.h>
#include <kurl.h>

class KAction;

class KatePluginFactory : public KLibFactory
{
  Q_OBJECT

  public:
    KatePluginFactory();
    virtual ~KatePluginFactory();

    virtual QObject* createObject( QObject* parent = 0, const char* pname = 0, const char* name = "QObject", const QStringList &args = QStringList() );

  private:
    static KInstance* s_instance;
};

class KateProjectManagerView : public Kate::PluginView
{
  Q_OBJECT

  public:
    KateProjectManagerView (QObject *parent=0);
    ~KateProjectManagerView ();

    KAction *projectNew;
    KAction *projectOpen;
    KAction *projectSave;
    KAction *projectSaveAs;
    KAction *projectConfigure;
    KAction *projectCompile;
    KAction *projectRun;

  public slots:
    void projectMenuAboutToShow();
};

class KateProjectManager : public Kate::Plugin
{
  Q_OBJECT

  public:
    KateProjectManager (QObject* parent = 0, const char* name = 0);
    ~KateProjectManager ();

    Kate::PluginView *createView ();

    KURL projectFile;

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
