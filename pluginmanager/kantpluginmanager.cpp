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

KantPluginManager::KantPluginManager(QObject *parent):QObject(parent)
  {
    kdDebug(13040)<<"Constructing KantPluginManager"<<endl;
    KStandardDirs *dirs = KGlobal::dirs();

    QStringList list=dirs->findAllResources("appdata","plugins/*.desktop",false,true);

    KSimpleConfig *confFile;    
    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
      {
	confFile=new KSimpleConfig(*it,true);
	if (confFile->readEntry("load","no").upper()=="YES")
          {
	    kdDebug(13040)<<"Cool one  plugin should be loaded"<<endl;
            QString relp=QFileInfo(*it).fileName();
	    relp=relp.left(relp.length()-8);
	    kdDebug(13040)<<"Found plugin: "<<relp<<endl;
	    relp=dirs->findResource("appdata","plugins/"+relp+"/ui.rc");
	    KParts::Plugin::PluginInfo plInf;
	    plInf.m_absXMLFileName=relp;
            //        plInf.m_document=
            //  QDomDocument doc( "mydocument" );
            QFile f( relp );
            if ( f.open( IO_ReadOnly ) )
	      {
                if ( plInf.m_document.setContent( &f ) ) 
                  {
	            plugins.append(plInf);
                  }
                f.close();
              }
	    loadedPlugins<<(*it);
          }
	else
          { 
		kdDebug(13040)<<"Plugin skipped"<<endl;
	       availablePlugins<<(*it);
	  }
	delete confFile;
      }
     //       qDebug((*it).latin1());}

   kdDebug(13040)<<"KantPluginManager initializes"<<endl;
  }
