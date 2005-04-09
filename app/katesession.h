/* This file is part of the KDE project
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __KATE_SESSION_H__
#define __KATE_SESSION_H__

#include "katemain.h"

#include <qobject.h>
#include <qvaluelist.h>

class KateSession
{
  public:
    // session filename, absolute, as found by the findAllResources
    QString sessionFile;

    // session filename, in local location we can write to
    QString sessionFileLocal;

    // session name, extracted from the file, to display to the user
    QString sessionName;
};

typedef QValueList<KateSession *> KateSessionList;

class KateSessionManager : public QObject
{
  Q_OBJECT

  public:
    KateSessionManager(QObject *parent);
    ~KateSessionManager();

    static KateSessionManager *self();

    inline KateSessionList & sessionList () { return m_sessionList; };

  private:
    void setupSessionList ();

    KateSessionList m_sessionList;
};

#endif
