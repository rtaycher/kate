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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_PARTMANAGER_H__
#define __KATE_PARTMANAGER_H__

#include "katemain.h"

#include <kxmlguiclient.h>

#include <kparts/part.h>

#include <QPointer>
#include <QList>
#include <QObject>
#include <QByteArray>
#include <QHash>

class KatePartProxy : public QWidget, KXMLGUIClient
{
  Q_OBJECT

  public:
    KatePartProxy (QWidget *parent);
    ~KatePartProxy ();

    void setPart (KParts::Part *part);

    KParts::Part *part ();

    bool isReadWrite ();

  private:
    KParts::Part *m_part;
};

class KatePartManager : public QObject
{
  Q_OBJECT

  public:
    KatePartManager (QObject *parent);
    ~KatePartManager ();

    /**
     * singleton accessor to the part manager
     * @return instance of the part manager
     */
    static KatePartManager *self ();

    /**
     * create a new part object (or better a proxy containing it)
     * @param libname library to open
     * @param widget widget to use as parent
     * @param classname classname of the part
     * @return proxy to access the part
     */
    KatePartProxy *createPart (const char *libname, QWidget *parent = 0, const char *classname = "KParts::Part");

    /** 
     * Number of parts
     * @return number of open parts
     */
    int parts ();

    /** 
     * return part with given index
     * @param index part index
     * @return part proxy
     */
    KatePartProxy *part (int index);

    /**
     * move the part's widget over to the cool store
     * @parent part proxy part
     */
    void moveToStore (KatePartProxy *part);

  private:
    /**
     * hack, dummy parent widget for the parts ;)
     */
    QWidget *m_coolStore;
    
    /**
     * list of all alive parts
     */
    QList<KatePartProxy*> m_partList;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
