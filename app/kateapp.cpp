/***************************************************************************
                          kateapp.cpp  -  description
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

#include "kateapp.h"
#include "kateapp.moc"

#include "../mainwindow/kateIface.h"
#include "../document/katedocmanager.h"
#include "../document/katedocument.h"
#include "../pluginmanager/katepluginmanager.h"
#include "../mainwindow/katemainwindow.h"
#include "../view/kateviewmanager.h"

#include <kcmdlineargs.h>
#include <kdebug.h>
#include <dcopclient.h>

KateApp::KateApp () : KateAppIface (),DCOPObject ("KateApp" )
{
  myWinID = 0;
  mainWindows.setAutoDelete (false);

  config()->setGroup("startup");
  _singleInstance=config()->readBoolEntry("singleinstance",true);

  DCOPClient *client = dcopClient();
  client->attach();
  client->registerAs("kate");

  docManager = new KateDocManager ();

  pluginManager = new KatePluginManager (this);
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

KateApp::~KateApp ()
{
  pluginManager->writeConfig ();
}

void KateApp::newMainWindow ()
{
  KateMainWindow *mainWindow = new KateMainWindow (docManager, pluginManager, myWinID, (QString("KateMainWindow%1").arg(myWinID)).latin1());
  mainWindows.append (mainWindow);
  myWinID++;

  if ((mainWindowsCount() > 1) && mainWindows.at(mainWindows.count()-2)->viewManager->activeView())
    mainWindow->viewManager->activateView ( mainWindows.at(mainWindows.count()-2)->viewManager->activeView()->doc()->docID() );
  else if ((mainWindowsCount() > 1) && (docManager->docCount() > 0))
    mainWindow->viewManager->activateView ( (docManager->nthDoc(docManager->docCount()-1))->docID() );
  else if ((mainWindowsCount() > 1) && (docManager->docCount() < 1))
    mainWindow->viewManager->openURL ( KURL() );

  mainWindow->show ();
}

void KateApp::removeMainWindow (KateMainWindow *mainWindow)
{
  mainWindows.remove (mainWindow);

  if (mainWindows.count() == 0)
    quit();
}

uint KateApp::mainWindowsCount ()
{
  return mainWindows.count();
}

void KateApp::openURL (const QString &name)
{
  int n = mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  mainWindows.at(n)->viewManager->openURL (KURL(name));
}

KateViewManagerIface *KateApp::viewManagerIface ()
{
  int n = mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return ((KateViewManagerIface *)mainWindows.at(n)->viewManager);
}

KateDocManagerIface *KateApp::docManagerIface ()
{
  return ((KateDocManagerIface *)docManager);
}

KStatusBar *KateApp::statusBar ()
{
  int n = mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return mainWindows.at(n)->statusBar();
}
