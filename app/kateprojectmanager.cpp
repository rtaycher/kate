/***************************************************************************
                          katepluginmanager.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001,2002 by Joseph Wenninger
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
