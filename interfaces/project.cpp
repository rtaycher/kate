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
#include <qdir.h>

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
    QString m_absdir;
    QString m_absfilename;
  };

            
unsigned int Project::globalProjectNumber = 0;
  
Project::Project (void *project) : QObject (((KateInternalProjectData*) project)->proMan)
{
  globalProjectNumber++;
  myProjectNumber = globalProjectNumber; 

  d = new PrivateProject ();
  d->m_data = ((KateInternalProjectData*) project);
  
  d->m_config = new KConfig (d->m_data->fileName, false, false);
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

bool Project::close ()
{
  return d->m_plugin->close ();
}

ProjectDirFile::Ptr Project::dirFile (const QString &dir)
{
  QString fname;
  
  if (!dir.isNull ())
    fname = dir + QString ("/") + dirFilesName ();
   else
    fname = dirFilesName ();
    
  if (!QFile::exists (d->m_dir + QString ("/") + fname))
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
  
  d->m_absfilename = d->m_data->project->dir() + QString ("/") + d->m_data->fileName;
  d->m_config = new KConfig (d->m_absfilename, false, false);
  
  int pos = d->m_data->fileName.findRev (QChar ('/'));
  
  if (pos == -1)
    d->m_dir = QString::null;
  else
    d->m_dir = d->m_data->fileName.left (pos);
    
  if (d->m_dir.isNull())
    d->m_absdir = d->m_data->project->dir ();
  else
    d->m_absdir = d->m_data->project->dir () + QString ("/") + d->m_dir;
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

QString ProjectDirFile::absFileName () const
{
  return d->m_absfilename;
}

QString ProjectDirFile::absDir () const
{
  return d->m_absdir;
}

ProjectDirFile::Ptr ProjectDirFile::dirFile (const QString &dir)
{
  QString fname = d->m_dir;
  
  if (!fname.isNull())
    fname += QString ("/");
  
  if (!dir.isNull ())
    fname += dir + QString ("/") + d->m_data->project->dirFilesName ();
   else
    fname += d->m_data->project->dirFilesName ();
    
  if (!QFile::exists (d->m_data->project->dir() + QString ("/") + fname))
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

QStringList ProjectDirFile::addDirs (const QStringList &dirs)
{
  QStringList existingDirs = this->dirs();
  
  QStringList newDirs;
  QDir dirCheck;
  for (uint z=0; z < dirs.count(); z++)
  {  
    if (existingDirs.findIndex (dirs[z]) == -1)
    {
      dirCheck.setPath (absDir() + QString ("/") + dirs[z]);
      
      if (dirCheck.exists())
        newDirs.push_back (dirs[z]);
    }
  }
      
  project()->plugin()->addDirs (this, newDirs);
  
  d->m_config->setGroup("General");
  d->m_config->writeEntry ("Dirs", newDirs + existingDirs, '/');
  d->m_config->sync ();
  
  for (uint z=0; z < newDirs.count(); z++)
  {
    KConfig config (absDir() + QString ("/") + newDirs[z] + QString ("/") + d->m_data->project->dirFilesName (), false, false);
    config.setGroup ("General");
    config.writeEntry ("Dirs", QStringList(), '/');
    config.writeEntry ("Files", QStringList(), '/');
    config.sync ();
  }
   
  emit d->m_data->project->dirsAdded (dir(), newDirs);
  
  return newDirs;
}

QStringList ProjectDirFile::removeDirs (const QStringList &dirs)
{
  QStringList existingDirs = this->dirs();
  
  QStringList removeDirs;
  for (uint z=0; z < dirs.count(); z++)
  {  
    if (existingDirs.findIndex (dirs[z]) != -1)
    {      
      removeDirs.push_back (dirs[z]);
    }
  }
      
  project()->plugin()->removeDirs (this, removeDirs);
  
  QStringList saveList;
  for (uint z=0; z < existingDirs.count(); z++)
  {  
    if (removeDirs.findIndex (existingDirs[z]) == -1)
    {      
      saveList.push_back (existingDirs[z]);
    }
  }
  
  d->m_config->setGroup("General");
  d->m_config->writeEntry ("Dirs", saveList, '/');
  d->m_config->sync ();
  
   emit d->m_data->project->dirsRemoved (dir(), removeDirs);
  
  return removeDirs;
}

QStringList ProjectDirFile::addFiles (const QStringList &files)
{
  QStringList existingFiles = this->files();
  
  QStringList newFiles;
  QFile fileCheck;
  for (uint z=0; z < files.count(); z++)
  {  
    if (existingFiles.findIndex (files[z]) == -1)
    {
      fileCheck.setName (absDir() + QString ("/") + files[z]);
      
      if (fileCheck.exists())
        newFiles.push_back (files[z]);
    }
  }
      
  project()->plugin()->addFiles (this, newFiles);
  
  d->m_config->setGroup("General");
  d->m_config->writeEntry ("Files", newFiles + existingFiles, '/');
  d->m_config->sync ();
   
   emit d->m_data->project->filesAdded (dir(), newFiles);
  
  return newFiles;
}

QStringList ProjectDirFile::removeFiles (const QStringList &files)
{
  QStringList existingFiles = this->files();
  
  QStringList removeFiles;
  for (uint z=0; z < files.count(); z++)
  {  
    if (existingFiles.findIndex (files[z]) != -1)
    {      
      removeFiles.push_back (files[z]);
    }
  }
      
  project()->plugin()->removeFiles (this, removeFiles);
  
  QStringList saveList;
  for (uint z=0; z < existingFiles.count(); z++)
  {  
    if (removeFiles.findIndex (existingFiles[z]) == -1)
    {      
      saveList.push_back (existingFiles[z]);
    }
  }
  
  d->m_config->setGroup("General");
  d->m_config->writeEntry ("Files", saveList, '/');
  d->m_config->sync ();
  
  emit d->m_data->project->filesRemoved (dir(), removeFiles);
  
  return removeFiles;
}

};

