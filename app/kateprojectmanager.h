/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_PROJECTMANAGER_H__
#define __KATE_PROJECTMANAGER_H__

#include "katemain.h"
#include "../interfaces/project.h"
#include "../interfaces/projectmanager.h"

#include <qobject.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <ktrader.h>
#include <qguardedptr.h>
#include <kdialogbase.h>

class KComboBox;
class KLineEdit;
class KURLRequester;

class KateInternalProjectData
{
  public:
    class KateProjectManager *proMan;
    QString fileName;
};

class ProjectPluginInfo
{
  public:
    KService::Ptr service;
    QString projectType;
    QString name;
};

class ProjectInfo
{
  public:
    QString type;
    QString name;
    QString fileName;
};

typedef QPtrList<ProjectPluginInfo> ProjectPluginList;

class KateProjectManager : public QObject
{
  Q_OBJECT

  public:
    KateProjectManager(QObject *parent);
    ~KateProjectManager();

    Kate::ProjectManager *projectManager ()const { return m_projectManager; };

    Kate::Project *create (const QString &type, const QString &name, const QString &filename);

    Kate::Project *open (const QString &filename);

    bool close (Kate::Project *project);

    Kate::Project *project (uint n = 0);

    uint projects ();

    Kate::ProjectPlugin *createPlugin (Kate::Project *project);

    void enableProjectGUI (Kate::Project *project, class KateMainWindow *win);
    void disableProjectGUI (Kate::Project *project, class KateMainWindow *win);

    ProjectInfo *newProjectDialog (QWidget *parent);

    QStringList pluginStringList ();

    void setCurrentProject (Kate::Project *project);

    bool queryCloseAll ();
    bool closeAll ();

    void saveProjectList (class KConfig *config);
    void restoreProjectList (class KConfig *config);

  private:
    Kate::ProjectManager *m_projectManager;

    void setupPluginList ();

    ProjectPluginList m_pluginList;

    // INTERNAL USE OF KateProject !!!
    QPtrList<Kate::Project> m_projects;
    QGuardedPtr<Kate::Project> m_currentProject;
};

class KateProjectDialogNew : public KDialogBase
{
    Q_OBJECT
  public:
    KateProjectDialogNew (QWidget *parent, KateProjectManager *projectMan);
    ~KateProjectDialogNew ();

    int exec();

private slots:
    void slotTextChanged();

  private:
    KateProjectManager *m_projectMan;

    KComboBox *m_typeCombo;
    KLineEdit *m_nameEdit;
    KURLRequester *m_urlRequester;

  public:
    QString type;
    QString name;
    QString fileName;
};

#endif
