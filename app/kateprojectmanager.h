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

#ifndef __KATE_PROJECTMANAGER_H__
#define __KATE_PROJECTMANAGER_H__

#include "katemain.h"
#include "../interfaces/project.h"
#include "../interfaces/projectmanager.h"

#include <qobject.h>

class KateProjectManager : public QObject
{
  Q_OBJECT

  public:
    KateProjectManager(QObject *parent);
    ~KateProjectManager();
    
    Kate::ProjectManager *projectManager () { return m_projectManager; };

  private:
    Kate::ProjectManager *m_projectManager;  
};

#endif
