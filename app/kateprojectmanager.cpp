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

// $Id$

#include "kateprojectmanager.h"
#include "kateprojectmanager.moc"

#include "katemainwindow.h"

#include <kconfig.h>

#include <qfile.h>

KateProjectManager::KateProjectManager (QObject *parent) : QObject (parent)
{
  m_projects.setAutoDelete (true);
  m_projectManager = new Kate::ProjectManager (this);
  setupPluginList ();
}

KateProjectManager::~KateProjectManager()
{
}

void KateProjectManager::setupPluginList ()
{
  QValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/ProjectPlugin");

  KTrader::OfferList::Iterator it(traderList.begin());
  for( ; it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);

    ProjectPluginInfo *info=new ProjectPluginInfo;

    info->service = ptr;
    info->name=info->service->property("X-KATE-InternalName").toString();
    if (info->name.isEmpty()) info->name=info->service->library();
    
    info->projectType=info->service->property("X-KATE-ProjectType").toString();
    
    m_pluginList.append(info);
  }
}

Kate::Project *KateProjectManager::create (const QString &type, const QString &name, const QString &filename)
{
  KConfig *c = new KConfig (filename);
  c->setGroup("General");
  c->writeEntry ("Type", type);
  c->writeEntry ("Name", name);
  c->sync ();
  delete c;

  return open (filename);
}
    
Kate::Project *KateProjectManager::open (const QString &filename)
{
  KateProject *project = new KateProject (this, this, filename);
  
  m_projects.append (project);
  
  return project->project();
}

bool KateProjectManager::close (Kate::Project *project)
{
  if (project)
  {
    if (project->plugin()->close())
    {
      m_projects.removeRef ((KateProject *)project);
    }
  }

  return false;
}

Kate::ProjectPlugin *KateProjectManager::createPlugin (Kate::Project *project)
{
  ProjectPluginInfo *def = 0;
  ProjectPluginInfo *info = 0;

  for (uint i=0; i<m_pluginList.count(); i++)
  {
    if (m_pluginList.at(i)->projectType == project->type())
    {
      info = m_pluginList.at(i);
      break;
    }
    else if (m_pluginList.at(i)->projectType == QString ("Default"))
      def = m_pluginList.at(i);
  }
  
  if (!info)
    info = def;
  
  return Kate::createProjectPlugin (QFile::encodeName(info->service->library()), project);
}

void KateProjectManager::enableProjectGUI (Kate::Project *project, KateMainWindow *win)
{
  if (!project->plugin()) return;
  if (!Kate::pluginViewInterface(project->plugin())) return;

  Kate::pluginViewInterface(project->plugin())->addView(win->mainWindow());
}

void KateProjectManager::disableProjectGUI (Kate::Project *project, KateMainWindow *win)
{
  if (!project->plugin()) return;
  if (!Kate::pluginViewInterface(project->plugin())) return;

  Kate::pluginViewInterface(project->plugin())->removeView(win->mainWindow());
}
