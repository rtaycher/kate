/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#include "project.h"
#include "project.moc"

#include "plugin.h"

#include "../app/kateprojectmanager.h"

#include <kconfig.h>

#include <qfile.h>

namespace Kate
{

class PrivateProjectDirFileData
{
  public:
    QString fileName;
    Project *project;
};

class PrivateProject
  {
  public:
    PrivateProject ()
    {
    }

    ~PrivateProject ()
    {    
      delete m_data;
      delete m_config;
    }          
        
    KateInternalProjectData *m_data;
    Kate::ProjectPlugin *m_plugin;
    KConfig *m_config;
    QString m_dir;
  };
  
class PrivateProjectDirFile
  {
  public:
    PrivateProjectDirFile ()
    {
    }

    ~PrivateProjectDirFile ()
    {
      delete m_data;
    }
    
    PrivateProjectDirFileData *m_data;
    KConfig *m_config;
    QString m_dir;
  };

            
unsigned int Project::globalProjectNumber = 0;
  
Project::Project (void *project) : QObject (((KateInternalProjectData*) project)->proMan)
{
  globalProjectNumber++;
  myProjectNumber = globalProjectNumber; 

  d = new PrivateProject ();
  d->m_data = ((KateInternalProjectData*) project);
  
  d->m_config = new KConfig (d->m_data->fileName);
  d->m_dir = d->m_data->fileName.left (d->m_data->fileName.findRev (QChar ('/')));
  
  // LAST STEP, IMPORTANT, LOAD PLUGIN AFTER ALL OTHER WORK IS DONE !
  d->m_plugin = d->m_data->proMan->createPlugin (this);
}

Project::~Project ()
{
  delete d;
}

unsigned int Project::projectNumber () const
{
  return myProjectNumber;
}     

ProjectPlugin *Project::plugin ()
{
  return d->m_plugin;
}

QString Project::type () const
{
  d->m_config->setGroup("General");
  return d->m_config->readEntry ("Type", "Default");
}

QString Project::name () const
{
  d->m_config->setGroup("General");
  return d->m_config->readEntry ("Name", "Untitled");
}

QString Project::fileName () const
{
  return d->m_data->fileName;
}

QString Project::dir () const
{
  return d->m_dir;
}

QString Project::dirFilesName () const
{
  d->m_config->setGroup("General");
  return d->m_config->readEntry ("DirFilesName", ".katedir");
}

bool Project::save ()
{
  d->m_config->sync();

  return d->m_plugin->save ();
}

ProjectDirFile::Ptr Project::dirFile (const QString &dir)
{
  QString fname = d->m_dir + QString ("/");
  
  if (!dir.isNull ())
    fname += dir + QString ("/") + dirFilesName ();
   else
    fname += dirFilesName ();
    
  if (!QFile::exists (fname))
    return 0;
    
  PrivateProjectDirFileData *data = new PrivateProjectDirFileData ();
  data->fileName = fname;
  data->project = this;
  
  ProjectDirFile::Ptr p = new ProjectDirFile ((void *)data);
  
  return p;
}

KConfig *Project::data ()
{
  return d->m_config;
}

ProjectDirFile::ProjectDirFile (void *projectDirFile) : QObject ()
{
  d = new PrivateProjectDirFile ();
  d->m_data = (PrivateProjectDirFileData *) projectDirFile;
  
  d->m_config = new KConfig (d->m_data->fileName);
  d->m_dir = d->m_data->fileName.left (d->m_data->fileName.findRev (QChar ('/')));
}

ProjectDirFile::~ProjectDirFile ()
{
  delete d;
}

Project *ProjectDirFile::project ()
{
  return d->m_data->project;
}

KConfig *ProjectDirFile::data ()
{
  return d->m_config;
}

QStringList ProjectDirFile::dirs () const
{
  d->m_config->setGroup("General");
  return d->m_config->readListEntry ("Dirs", '/');
}
     
QStringList ProjectDirFile::files () const
{
  d->m_config->setGroup("General");
  return d->m_config->readListEntry ("Files", '/');
}

QString ProjectDirFile::fileName () const
{
  return d->m_data->fileName;
}

QString ProjectDirFile::dir () const
{
  return d->m_dir;
}

ProjectDirFile::Ptr ProjectDirFile::dirFile (const QString &dir)
{
  QString fname = d->m_dir + QString ("/");
  
  if (!dir.isNull ())
    fname += dir + QString ("/") + d->m_data->project->dirFilesName ();
   else
    fname += d->m_data->project->dirFilesName ();
    
  if (!QFile::exists (fname))
    return 0;
    
  PrivateProjectDirFileData *data = new PrivateProjectDirFileData ();
  data->fileName = fname;
  data->project = d->m_data->project;
  
  ProjectDirFile::Ptr p = new ProjectDirFile ((void *)data);
  
  return p;
}

ProjectDirFile::List ProjectDirFile::dirFiles ()
{
  QStringList d = dirs ();
  ProjectDirFile::List list;
  
  for (uint i=0; i < d.count(); i++)
  {
    ProjectDirFile::Ptr p = dirFile (d[i]);
    
    if (p)
      list.push_back (p);
  }
  
  return list;
}

};

