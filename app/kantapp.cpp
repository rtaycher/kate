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

#include "../mainwindow/kantIface.h"
#include "../pluginmanager/kantpluginmanager.h"
#include "../mainwindow/kantmainwindow.h"
#include "../view/kantviewmanager.h"

#include <kcmdlineargs.h>

KantApp::KantApp () : KUniqueApplication ()
{
  pluginManager=new KantPluginManager(this);
  mainWindow = new KantMainWindow (pluginManager);
  mainWindow->show ();

  connect(this, SIGNAL(lastWindowClosed()), SLOT(quit()));
}

KantApp::~KantApp ()
{
}

int KantApp::newInstance ()
{
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

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
