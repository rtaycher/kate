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

#ifndef _KATE_PROJECT_INCLUDE_
#define _KATE_PROJECT_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{

/**
 * Interface to the project
 */
class Project : public QObject
{
  friend class PrivateProject;

  Q_OBJECT   
  
  public:
    /**
     * Construtor, should not interest, internal usage
     */
    Project (void *project);
    
    /**
     * Desctructor
     */
    virtual ~Project ();
    
    /**
     * Returns the project plugin of this project object
     * @return ProjectPlugin project plugin of this project
     */
    class ProjectPlugin *plugin () const;
   
    /**
     * Return the project type
     * @return QString project type
     */
    QString type () const;
    
    /**
     * Return the filename of the project file
     * @return QString project filename
     */
    QString fileName () const;
    
    /**
     * Saves the project
     * @return bool success
     */
    bool save ();
    
    /**
     * Closes the project
     * @return bool success
     */
    bool close ();
    
  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateProject *d;
};

};

#endif
