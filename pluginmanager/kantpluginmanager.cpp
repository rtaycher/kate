/***************************************************************************
                          kantpluginmanager.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantpluginmanager.h"
#include "kantpluginmanager.moc"

#include <kglobal.h>
#include <kstddirs.h>
#include <qstringlist.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <ksimpleconfig.h>

KantPluginManager::KantPluginManager(QObject *parent) : QObject(parent)
{
  setupPluginList ();
  loadAllEnabledPlugins ();
}

KantPluginManager::~KantPluginManager()
{
}

void KantPluginManager::setupPluginList ()
{
  KStandardDirs *dirs = KGlobal::dirs();

  QStringList list=dirs->findAllResources("appdata","plugins/*.desktop",false,true);

  KSimpleConfig *confFile;
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    confFile=new KSimpleConfig(*it,true);

    PluginListItem *info=new PluginListItem;
    info->load = (confFile->readEntry("load","no") =="1");

    info->config = (*it);

    info->relp = QFileInfo(*it).fileName();
    info->relp = info->relp=info->relp.left(info->relp.length()-8);
    info->relp=dirs->findResource("appdata","plugins/"+info->relp+"/ui.rc");

    info->name = confFile->readEntry("name","no");
    info->description = confFile->readEntry("description","no");
    info->author = confFile->readEntry("author","no");
    myPluginList.append(info);

    delete confFile;
  }
}

void KantPluginManager::loadAllEnabledPlugins ()
{
  for (int i=0; i<myPluginList.count(); i++)
  {
    if  (myPluginList.at(i)->load)
      loadPlugin (myPluginList.at(i));
  }
}

bool KantPluginManager::loadPlugin (PluginListItem *item)
{
  bool val = false;

  KParts::Plugin::PluginInfo plInf;
  plInf.m_absXMLFileName=item->relp;

  QFile f( item->relp );
  if ( f.open( IO_ReadOnly ) )
  {
    if ( plInf.m_document.setContent( &f ) )
    {
      plugins.append(plInf);
      val = true;
    }

    f.close();
  }

  return val;
}
