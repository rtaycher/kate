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
#include <qstringlist.h>

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
    
    unsigned int projectNumber () const;
    
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
     * Return the project name
     * @return QString project name
     */
    QString name () const;
    
    /**
     * Return the url of the project file
     * @return KURL project file url
     */
    KURL url () const;
    
    /**
     * Saves the project
     * @return bool success
     */
    bool save ();
    
    /**
     * subdirs of given dir
     * @return QStringList list with subdirs
     */
    QStringList subdirs (const QString &dir = QString::null) const;
    
    /**
     * files of given dir
     * @return QStringList list with files
     */
    QStringList files (const QString &dir = QString::null) const;
    
  #undef signals
  #define signals public
  signals:
  #undef signals
  #define signals protected
  
    void dirInserted (const QString &dir);
    void dirDeleted (const QString &dir);
    
    void fileInserted (const QString &file);
    void fileRemoved (const QString &file);
    
  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateProject *d;
    static unsigned int globalProjectNumber;
    unsigned int myProjectNumber;
};

};

#endif
