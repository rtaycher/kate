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

#include "projectiface.h"

#include "plugin.h"

#include "../app/kateprojectmanager.h"

#include <kconfig.h>

KateProjectDCOPIface::KateProjectDCOPIface (Kate::Project *p)
 : DCOPObject ((QString("KateProject#%1").arg(p->projectNumber())).latin1()), m_p (p)
{
}

namespace Kate
{

class PrivateProject
{
  public:
    PrivateProject ()
    {
    }

    ~PrivateProject ()
    {
      delete m_dcop;
      delete m_data;
      delete m_config;
      delete m_plugin;
    }

    KateInternalProjectData *m_data;
    Kate::ProjectPlugin *m_plugin;
    KConfig *m_config;
    QString m_dir;
    KateProjectDCOPIface *m_dcop;
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

  d->m_dcop = new KateProjectDCOPIface (this);

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

DCOPObject *Project::dcopObject ()
{
  return 0;
}

ProjectPlugin *Project::plugin () const
{
  return d->m_plugin;
}

KConfig *Project::data ()
{
  return d->m_config;
}

KConfig *Project::dirData (const QString &dir)
{
  if (dir.isNull())
    d->m_config->setGroup("Project Dir");
  else
    d->m_config->setGroup ("Dir "+dir);

  return d->m_config;
}

KConfig *Project::fileData (const QString &file)
{
  if (file.isNull())
    d->m_config->setGroup("Project File");
  else
    d->m_config->setGroup ("File "+file);

  return d->m_config;
}

QString Project::type ()
{
  return fileData()->readEntry ("Type", "Default");
}

QString Project::name ()
{
  return fileData()->readEntry ("Name", "Untitled");
}

QString Project::fileName ()
{
  return d->m_data->fileName;
}

QString Project::dir ()
{
  return d->m_dir;
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

QStringList Project::dirs (const QString &dir)
{
  return dirData(dir)->readListEntry ("Dirs", '/');
}

QStringList Project::files (const QString &dir)
{
  return dirData(dir)->readListEntry ("Files", '/');
}

void Project::addDirs (const QString &dir, QStringList &dirs)
{
  QStringList existing = this->dirs (dir);
  for (uint z=0; z < existing.count(); z++)
  {
    dirs.remove (existing[z]);
  }

  plugin()->addDirs (dir, dirs);

  dirData (dir);
  d->m_config->writeEntry ("Dirs", existing + dirs, '/');
  d->m_config->sync ();

  emit dirsAdded (dir, dirs);
}

void Project::removeDirs (const QString &dir, QStringList &dirs)
{
  QStringList toRemove;
  QStringList existing = this->dirs (dir);
  for (uint z=0; z < dirs.count(); z++)
  {
    if (existing.findIndex(dirs[z]) != -1)
      toRemove.append (dirs[z]);
  }

  dirs = toRemove;

  plugin()->removeDirs (dir, dirs);

  for (uint z=0; z < dirs.count(); z++)
  {
    existing.remove (dirs[z]);
  }

  dirData (dir);
  d->m_config->writeEntry ("Dirs", existing, '/');
  d->m_config->sync ();

  emit dirsRemoved (dir, dirs);
}

void Project::addFiles (const QString &dir, QStringList &files)
{
  QStringList existing = this->files (dir);
  for (uint z=0; z < existing.count(); z++)
  {
    files.remove (existing[z]);
  }

  plugin()->addFiles (dir, files);

  dirData (dir);
  d->m_config->writeEntry ("Files", existing + files, '/');
  d->m_config->sync ();

  emit filesAdded (dir, files);
}

void Project::removeFiles (const QString &dir, QStringList &files)
{
  QStringList toRemove;
  QStringList existing = this->files (dir);
  for (uint z=0; z < files.count(); z++)
  {
    if (existing.findIndex(files[z]) != -1)
      toRemove.append (files[z]);
  }

  files = toRemove;

  plugin()->removeDirs (dir, files);

  for (uint z=0; z < files.count(); z++)
  {
    existing.remove (files[z]);
  }

  dirData (dir);
  d->m_config->writeEntry ("Files", existing, '/');
  d->m_config->sync ();

  emit filesRemoved (dir, files);
}

}

