/***************************************************************************
                          kantapp.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantapp.h"
#include "kantapp.moc"

KantApp::KantApp () : KUniqueApplication ()
{
  //JoWenn -- Begin
  pluginManager=new KantPluginManager(this);
  //JoWenn -- End
  mainWindow = new KantMainWindow (pluginManager);
  mainWindow->show ();

  connect(this, SIGNAL(lastWindowClosed()), SLOT(quit()));


  if (isRestored())
    restore();
}

KantApp::~KantApp ()
{
}
int KantApp::newInstance ()
{
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  // Anders: reopen documents
  mainWindow->reopenDocuments(isRestored());
  if (!isRestored())
  {

    for (int z=0; z<args->count(); z++)
    {
      mainWindow->viewManager->openURL( args->url(z) );
    }


    mainWindow->raise();
  }
    if ( mainWindow->viewManager->viewCount () == 0 )
      mainWindow->viewManager->openURL( KURL() );

  return 0;
}

void KantApp::restore ()
{
 /* KConfig *config;
  int docs, windows, z;
  char buf[16];
  KWriteDoc *doc;
  TopLevel *t;

  config = kapp->sessionConfig();
  if (!config) return;

  config->setGroup("Number");
  docs = config->readNumEntry("NumberOfDocuments");
  windows = config->readNumEntry("NumberOfWindows");

  for (z = 1; z <= docs; z++) {
     sprintf(buf,"Document%d",z);
     config->setGroup(buf);
     doc = new KWriteDoc(HlManager::self());
     doc->readSessionConfig(config);
     docList.append(doc);
  }

  for (z = 1; z <= windows; z++) {
    sprintf(buf,"%d",z);
    config->setGroup(buf);
    t = new TopLevel(docList.at(config->readNumEntry("DocumentNumber") - 1));
    t->restore(config,z);
  } */
}
