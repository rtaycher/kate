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
#include "../document/kantdocmanager.h"
#include "../pluginmanager/kantpluginmanager.h"
#include "../mainwindow/kantmainwindow.h"
#include "../view/kantviewmanager.h"

#include <kcmdlineargs.h>

KantApp::KantApp () : KUniqueApplication ()
{
  mainWindows.setAutoDelete (false);

  docManager = new KantDocManager ();
  pluginManager=new KantPluginManager(this);

  newMainWindow ();

  connect(this, SIGNAL(lastWindowClosed()), SLOT(quit()));
}

KantApp::~KantApp ()
{
}

void KantApp::newMainWindow ()
{
  KantMainWindow *mainWindow = new KantMainWindow (docManager, pluginManager);
  mainWindows.append (mainWindow);

  mainWindow->viewManager->openURL( KURL() );
  mainWindow->show ();
}

void KantApp::removeMainWindow (KantMainWindow *mainWindow)
{
  mainWindows.remove (mainWindow);

  if (mainWindows.count() == 0)
    quit();
}

long KantApp::mainWindowsCount ()
{
  return mainWindows.count();
}

int KantApp::newInstance ()
{
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  mainWindows.first()->reopenDocuments(isRestored());

  if (!isRestored())
  {

    for (int z=0; z<args->count(); z++)
    {
      mainWindows.first()->viewManager->openURL( args->url(z) );
    }

    mainWindows.first()->raise();
  }

  if ( mainWindows.first()->viewManager->viewCount () == 0 )
    mainWindows.first()->viewManager->openURL( KURL() );

  return 0;
}
