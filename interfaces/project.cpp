/***************************************************************************
                          interfaces.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

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
            
Project::Project (void *project) : QObject ((KateProject*) project)
{
  d = new PrivateProject ();
  d->project = (KateProject*) project;
}

Project::~Project ()
{
  delete d;
}

ProjectPlugin *Project::plugin ()
{
  return d->project->plugin ();
}

QString Project::type () const
{
  return d->project->type ();
}

bool Project::save ()
{
  return d->project->save ();
}

bool Project::close ()
{
  return d->project->close ();
}

};

