/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_PROJECT_H__
#define __KATE_PROJECT_H__

#include "katemain.h"
#include "../interfaces/project.h"
#include "../interfaces/plugin.h"

#include <qobject.h>
#include <kconfig.h>

//
// INTERNAL CLASS FOR PROJECT MANAGER ONLY !!!!!!!!!!!!!
//
class KateProject : public QObject
{
  Q_OBJECT

  public:
    KateProject (class KateProjectManager *proMan, QObject *parent, const QString &filename);
    ~KateProject ();
    
    Kate::Project *project () { return m_project; };
    
    /**
     * Returns the project plugin of this project object
     * @return ProjectPlugin project plugin of this project
     */
    Kate::ProjectPlugin *plugin () { return m_plugin; } const;
   
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
     * Return the project dir
     * @return QString project dir
     */
    QString dir () const;
    
    /**
     * Saves the project
     * @return bool success
     */
    bool save ();
    
    /**
     * top subdirs
     * @return QStringList list with subdirs
     */
    QStringList subdirs () const;
    
    /**
     * subdirs of given dir
     * @return QStringList list with subdirs
     */
    QStringList subdirs (const QString &dir) const;

  private:
    class KateProjectManager *m_projectMan;
    Kate::Project *m_project;
    Kate::ProjectPlugin *m_plugin;
    KConfig *m_data;
    QString m_filename;
    QString m_path;
};

#endif
