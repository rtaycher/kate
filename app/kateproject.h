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
#include <ktempfile.h>

//
// INTERNAL CLASS FOR PROJECT MANAGER ONLY !!!!!!!!!!!!!
//
class KateProject : public QObject
{
  Q_OBJECT

  public:
    KateProject (class KateProjectManager *proMan, QObject *parent, const KURL &url);
    ~KateProject ();
    
    Kate::Project *project () { return m_project; };
    
    /**
     * Returns the project plugin of this project object
     * @return ProjectPlugin project plugin of this project
     */
    Kate::ProjectPlugin *plugin () const { return m_plugin; };
   
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
     * Return the url of the project dir
     * @return KURL project dir url
     */
    KURL baseurl (bool _strip_trailing_slash_from_result = true) const;
    
    /**
     * Saves the project
     * @return bool success
     */
    bool save ();
    
    /**
     * subdirs of given dir
     * @return QStringList list with subdirs
     */
    QStringList subdirs (const QString &dir = QString (".")) const;
    
    /**
     * files of given dir
     * @return QStringList list with files
     */
    QStringList files (const QString &dir = QString (".")) const;

  private:
    class KateProjectManager *m_projectMan;
    Kate::Project *m_project;
    Kate::ProjectPlugin *m_plugin;
    KConfig *m_data;
    KURL m_url;
    KURL m_baseurl;
    KURL m_baseurlWithoutSlash;
    KTempFile *m_tmpFile;
};

#endif
