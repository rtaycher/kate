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
  KStandardDirs *dirs = KGlobal::dirs();

  QStringList list=dirs->findAllResources("appdata","plugins/*.desktop",false,true);

  KSimpleConfig *confFile;
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    confFile=new KSimpleConfig(*it,true);

    PluginListItem *info=new PluginListItem;
    info->load = (confFile->readEntry("load","no") =="1");
    info->name = confFile->readEntry("name","no");
    info->description = confFile->readEntry("description","no");
    info->author = confFile->readEntry("author","no");
    myPluginList.append(info);

    if (info->load)
    {
      QString relp=QFileInfo(*it).fileName();
      relp=relp.left(relp.length()-8);

      relp=dirs->findResource("appdata","plugins/"+relp+"/ui.rc");

      KParts::Plugin::PluginInfo plInf;
      plInf.m_absXMLFileName=relp;

      QFile f( relp );
      if ( f.open( IO_ReadOnly ) )
      {
        if ( plInf.m_document.setContent( &f ) )
          plugins.append(plInf);

        f.close();
      }
    }

    delete confFile;
  }
}
