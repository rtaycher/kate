/***************************************************************************
                          katepluginmanager.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_PROJECT_H__
#define __KATE_PROJECT_H__

#include "katemain.h"
#include "../interfaces/project.h"
#include "../interfaces/plugin.h"

#include <qobject.h>
#include <kconfig.h>

class KateProject : public QObject
{
  Q_OBJECT

  public:
    KateProject (QObject *parent);
    ~KateProject ();
    
    Kate::Project *project () { return m_project; };
    
    Kate::ProjectPlugin *plugin () { return m_plugin; };
    
    QString type () const;
    
    bool save ();
    
    bool close ();

  private:
    Kate::Project *m_project;
    Kate::ProjectPlugin *m_plugin;
    KConfig *m_data;
};

#endif
