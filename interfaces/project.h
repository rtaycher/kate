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

#include <ksharedptr.h>
#include <qobject.h>
#include <kurl.h>
#include <qstringlist.h>
#include <qvaluelist.h>

class KConfig;

namespace Kate
{

class Project;

/**
 * Interface to the project
 */
class ProjectDirFile : public QObject, public KShared
{
  friend class PrivateProjectDirFile;

  Q_OBJECT
 
  public:
    typedef KSharedPtr<ProjectDirFile> Ptr;
    typedef QValueList<Ptr> List;
   
  public:
    /**
     * Construtor, should not interest, internal usage
     */
    ProjectDirFile (void *projectDirFile);
    
    /**
     * Desctructor
     */
    virtual ~ProjectDirFile ();
    
    Project *project ();
    
    /**
     * Return the filename of the project file
     * @return QString project filename
     */
    QString fileName () const;
    
    /**
     * Return the dir of the project
     * @return QString project dir
     */
    QString dir () const;
    
    /**
     * Raw access to config file
     * @return KConfig config data
     */
    KConfig *data ();
     
    QStringList dirs () const;
     
    QStringList files () const;
     
    /**
     * ProjectDirFile object for the dir dir file in the given dir, QString::null for this dir !
     * @param dir dir name
     * @return ProjectDirFile for given dir
     */
    ProjectDirFile::Ptr dirFile (const QString &dir = QString::null);
    
    /**
     * ProjectDirFile objects for all direct subdirs
     * @return ProjectDirFile::List for all direct subdirs
     */
    ProjectDirFile::List dirFiles ();

  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateProjectDirFile *d;
};

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
    class ProjectPlugin *plugin ();
   
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
     * Return the filename of the project file
     * @return QString project filename
     */
    QString fileName () const;
    
    /**
     * Return the dir of the project
     * @return QString project dir
     */
    QString dir () const;
    
    /**
     * Return the name of the dir project files
     * @return QString dir project file naming
     */
    QString dirFilesName () const;
    
    /**
     * Saves the project
     * @return bool success
     */
    bool save ();
    
    /**
     * ProjectDirFile object for the dir dir file in the given dir, QString::null for toplevel dir !
     * @param dir dir name
     * @return ProjectDirFile for given dir
     */
    ProjectDirFile::Ptr dirFile (const QString &dir = QString::null);
    
    /**
     * Raw access to config file
     * @return KConfig config data
     */
    KConfig *data ();
    
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
