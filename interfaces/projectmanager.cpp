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

#include "projectmanager.h"
#include "projectmanager.moc"

#include "application.h"

#include "../app/kateprojectmanager.h"
#include "../app/kateproject.h"

namespace Kate
{

class PrivateProjectManager
  {
  public:
    PrivateProjectManager ()
    {
    }

    ~PrivateProjectManager ()
    {    
    }          
        
    KateProjectManager *projectMan; 
  };
            
ProjectManager::ProjectManager (void *projectManager) : QObject ((KateProjectManager*) projectManager)
{
  d = new PrivateProjectManager ();
  d->projectMan = (KateProjectManager*) projectManager;
}

ProjectManager::~ProjectManager ()
{
  delete d;
}

Project *ProjectManager::create (const QString &type, const QString &name, const KURL &url)
{
  return d->projectMan->create (type, name, url);
}
    
Project *ProjectManager::open (const const KURL &url)
{
  return d->projectMan->open (url);
}

bool ProjectManager::close (Kate::Project *project)
{
  d->projectMan->close (project);
}

Project *ProjectManager::project (uint n)
{
  return d->projectMan->project (n);
}

uint ProjectManager::projects ()
{
  return d->projectMan->projects ();
}

ProjectManager *projectManager ()
{
  return application()->projectManager ();
}

};

