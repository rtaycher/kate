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

#include "../app/kateproject.h"

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
    }          
        
    KateProject *project; 
  };
            
unsigned int Project::globalProjectNumber = 0;
  
Project::Project (void *project) : QObject ((KateProject*) project)
{
  globalProjectNumber++;
  myProjectNumber = globalProjectNumber; 
  d = new PrivateProject ();
  d->project = (KateProject*) project;
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
  return d->project->plugin ();
}

QString Project::type () const
{
  return d->project->type ();
}

QString Project::name () const
{
  return d->project->name ();
}

KURL Project::url () const
{
  return d->project->url ();
}

bool Project::save ()
{
  return d->project->save ();
}

QStringList Project::subdirs (const QString &dir) const
{
  return d->project->subdirs (dir);
}

QStringList Project::files (const QString &dir) const
{
  return d->project->files (dir);
}

};

