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
#include <kdebug.h>
#include <dcopclient.h>

KantApp::KantApp () : KantAppIface (),DCOPObject ("KantappIface" )
{
  mainWindows.setAutoDelete (false);

  config()->setGroup("startup");
  _singleInstance=config()->readBoolEntry("singleinstance",true);

  DCOPClient *client = dcopClient();
  client->attach();
  client->registerAs("kant");

  docManager = new KantDocManager ();

  pluginManager = new KantPluginManager (this);
  pluginManager->loadAllEnabledPlugins ();

  newMainWindow ();

  connect(this, SIGNAL(lastWindowClosed()), SLOT(quit()));

  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  mainWindows.first()->restore(isRestored());

  if (isRestored())
    kdDebug(13000)<<"restored app anybody?"<<endl;
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
}

KantApp::~KantApp ()
{
  pluginManager->writeConfig ();
}

void KantApp::newMainWindow ()
{
  KantMainWindow *mainWindow = new KantMainWindow (docManager, pluginManager);
  mainWindows.append (mainWindow);
  // anders: do not force an "Untitled" document on first window!
  // q: do we want to force it at all if documents are open??
  if (mainWindowsCount() > 1)
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


KantViewManagerIface *KantApp::viewManagerIface ()
{
  return ((KantViewManagerIface *)mainWindows.at(0)->viewManager);
}

KantDocManagerIface *KantApp::docManagerIface ()
{
  return ((KantDocManagerIface *)docManager);
}


KStatusBar *KantApp::statusBar ()
{
  return mainWindows.at(0)->statusBar();
}