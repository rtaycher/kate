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
/** This interface provides access to the Kate Project .
*/
class Project : public QObject
{
  friend class PrivateProject;

  Q_OBJECT   
  
  public:
    Project ( void *project  );
    virtual ~Project ();
    
    class ProjectPlugin *plugin ();
   
    QString type () const;
    
    bool save ();
    
    bool close ();
    
  private:
    class PrivateProject *d;
};

};

#endif
