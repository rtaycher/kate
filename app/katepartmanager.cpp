/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katepartmanager.h"
#include "katepartmanager.moc"
#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <kparts/factory.h>

#include <kdebug.h>
#include <kconfig.h>
#include <klibloader.h>

KatePartProxy::KatePartProxy (QWidget *parent)
 : QWidget (parent)
{
}

KatePartProxy::~KatePartProxy ()
{
}

void KatePartProxy::setPart (KParts::Part *part)
{
  m_part = part;
}

KatePartManager::KatePartManager (QObject *parent)
 : QObject (parent)
 , m_coolStore (new QWidget (0))
{
}

KatePartManager::~KatePartManager ()
{
  delete m_coolStore;
}

KatePartManager *KatePartManager::self ()
{
  return KateApp::self()->partManager ();
}

KatePartProxy *KatePartManager::createPart (const char *libname, QWidget *parent, const char *classname)
{
  // create the wrapper :)
  KatePartProxy *part = new KatePartProxy (parent ? parent : m_coolStore);

  KParts::Factory *factory = (KParts::Factory *) KLibLoader::self()->factory( libname );

  KParts::Part *p = (KParts::Part *)factory->createPart(part, "", this, "", classname);
     
  m_partList.append (part);
}

int KatePartManager::parts ()
{
  return m_partList.size ();
}

KatePartProxy *KatePartManager::part (int index)
{
  if (index < 0 || index >= m_partList.size())
    return 0;

  return m_partList[index];
}

// kate: space-indent on; indent-width 2; replace-tabs on;
