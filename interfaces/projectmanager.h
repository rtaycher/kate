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

#ifndef _KATE_PROJECTMANAGER_INCLUDE_
#define _KATE_PROJECTMANAGER_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{

/**
 * Interface to the projectmanager
 */
class ProjectManager : public QObject
{
  friend class PrivateProjectManager;

  Q_OBJECT   
  
  public:
    /**
     * Construtor, should not interest, internal usage
     */
    ProjectManager (void *projectManager);
    
    /**
     * Desctructor
     */
    virtual ~ProjectManager ();
    
    /**
     * Creates a new project file at give url of given type + opens it
     * @param type projecttype
     * @param url url of the new project file
     * @return Project new created project object
     */
    class Project *create (const QString &type, const KURL &url);
    
    /**
     * @param url url of the project file
     * @return Project opened project
     */
    class Project *open (const KURL &url);
       
  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateProjectManager *d;
};

};

#endif
