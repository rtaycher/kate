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
#include <qstringlist.h>

#include <kdemacros.h>

class KConfig;
class DCOPObject;

namespace Kate
{

  class Plugin;

/**
 * Interface to the project
 */
class KDE_EXPORT Project : public QObject
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

    DCOPObject *dcopObject ();

    /**
     * Returns the project plugin of this project object
     * @return ProjectPlugin project plugin of this project
     */
    class ProjectPlugin *plugin () const;

    /**
     * Raw access to config file
     * @return KConfig config data
     */
    KConfig *data ();

    KConfig *dirData (const QString &dir = QString::null);

    KConfig *fileData (const QString &file = QString::null);

    KConfig *pluginData(Plugin *plugin,const QString& group=QString::null);

    /**
     * Return the project type
     * @return QString project type
     */
    QString type ();

    /**
     * Return the project name
     * @return QString project name
     */
    QString name ();

    /**
     * Return the filename of the project file
     * @return QString project filename
     */
    QString fileName ();

    /**
     * Return the dir of the project
     * @return QString project dir
     */
    QString dir ();

    /**
     * Saves the project
     * @return bool success
     */
    bool save ();

    /**
     * Query if the project can be closed now, don't close it, only ask questions or
     * save it, or ...
     * @return bool success
     */
    bool queryClose ();
    
    /**
     * Close the project (says the project plugin the app requests a close, prepare for deletion)
     * @return bool success
     */
    bool close ();

    QStringList dirs (const QString &dir = QString::null);

    QStringList files (const QString &dir = QString::null);

    void addDirs (const QString &dir, QStringList &dirs);
    void removeDirs (const QString &dir, QStringList &dirs);

    void addFiles (const QString &dir, QStringList &files);
    void removeFiles (const QString &dir, QStringList &files);

  #undef signals
  #define signals public
  signals:
  #undef signals
  #define signals protected

    void dirsAdded (const QString &dir, const QStringList &dirs);
    void dirsRemoved (const QString &dir, const QStringList &dirs);

    void filesAdded (const QString &dir, const QStringList &files);
    void filesRemoved (const QString &dir, const QStringList &files);

  private:
    /**
     * REALLY PRIVATE ;)
     */
    class PrivateProject *d;
    static unsigned int globalProjectNumber;
    unsigned int myProjectNumber;
};

}

#endif
