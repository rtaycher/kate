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
#include <ktrader.h>

class ProjectPluginInfo
{
  public:
    KService::Ptr service;
    QString projectType;
    QString name;
};

typedef QPtrList<ProjectPluginInfo> ProjectPluginList;

class KateProjectManager : public QObject
{
  Q_OBJECT

  public:
    KateProjectManager(QObject *parent);
    ~KateProjectManager();
    
    Kate::ProjectManager *projectManager () { return m_projectManager; };
    
    Kate::Project *create (const QString &type, const KURL &url);
    
    Kate::Project *open (const KURL &url);
  
  private:
    Kate::ProjectManager *m_projectManager;
    
    void setupPluginList ();
    
    ProjectPluginList m_pluginList;
};

#endif
