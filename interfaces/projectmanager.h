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
/** This interface provides access to the Kate Project Manager.
*/
class ProjectManager : public QObject
{
  friend class PrivateProjectManager;

  Q_OBJECT   
  
  public:
    ProjectManager ( void *projectManager  );
    virtual ~ProjectManager ();
    
    class Project *create (const QString &type, const KURL &url);
    
    class Project *open (const KURL &url);
       
  private:
    class PrivateProjectManager *d;
};

};

#endif
