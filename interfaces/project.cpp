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
      delete data;
      delete m_data;
    }          
        
    KateInternalProjectData *data;
    Kate::ProjectPlugin *m_plugin;
    KConfig *m_data;
  };
            
unsigned int Project::globalProjectNumber = 0;
  
Project::Project (void *project) : QObject (((KateInternalProjectData*) project)->proMan)
{
  globalProjectNumber++;
  myProjectNumber = globalProjectNumber; 
  

  d = new PrivateProject ();
  d->data = ((KateInternalProjectData*) project);
  d->m_plugin = d->data->proMan->createPlugin (this);
}

Project::~Project ()
{
  delete d;
}

unsigned int Project::projectNumber () const
{
  return myProjectNumber;
}     

ProjectPlugin *Project::plugin () const
{
  return d->m_plugin;
}

QString Project::type () const
{
  d->m_data->setGroup("General");
  return d->m_data->readEntry ("Type", "Default");
}

QString Project::name () const
{
  d->m_data->setGroup("General");
  return d->m_data->readEntry ("Name", "");
}

KURL Project::url () const
{
  return KURL ();
}

KURL Project::baseurl (bool _strip_trailing_slash_from_result) const
{
  return KURL ();
}

bool Project::save ()
{
  d->m_data->sync();

  return d->m_plugin->save ();
}

QStringList Project::subdirs (const QString &dir) const
{
  return QStringList ();
}

QStringList Project::files (const QString &dir) const
{
  return QStringList ();
}

};

