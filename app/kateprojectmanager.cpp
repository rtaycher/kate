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

// $Id$

#include "kateprojectmanager.h"
#include "kateprojectmanager.moc"

KateProjectManager::KateProjectManager (QObject *parent) : QObject (parent)
{
  m_projectManager = new Kate::ProjectManager (this);
  setupPluginList ();
}

KateProjectManager::~KateProjectManager()
{
}

void KateProjectManager::setupPluginList ()
{
  QValueList<KService::Ptr> traderList= KTrader::self()->query("Kate/ProjectPlugin");

  KTrader::OfferList::Iterator it(traderList.begin());
  for( ; it != traderList.end(); ++it)
  {
    KService::Ptr ptr = (*it);

    ProjectPluginInfo *info=new ProjectPluginInfo;

    info->service = ptr;
    info->name=info->service->property("X-KATE-InternalName").toString();
    if (info->name.isEmpty()) info->name=info->service->library();
    
    info->projectType=info->service->property("X-KATE-ProjectType").toString();
    
    m_pluginList.append(info);
  }
}

Kate::Project *KateProjectManager::create (const QString &type, const KURL &url)
{
  return 0;
}
    
Kate::Project *KateProjectManager::open (const KURL &url)
{
  return 0;
}
