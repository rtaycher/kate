/***************************************************************************
                          kateapp.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
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

#include "kateIface.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <kcmdlineargs.h>
#include <dcopclient.h>
#include <kconfig.h>
#include <kwin.h>

KateApp::KateApp () : Kate::Application (),DCOPObject ("KateApp" )
{
  mainWindows.setAutoDelete (false);

  config()->setGroup("startup");
  _singleInstance=config()->readBoolEntry("singleinstance",true);
  _isSDI=config()->readBoolEntry("sdi",false);
  
  KCmdLineArgs* args = KCmdLineArgs::parsedArgs();

  if (args->isSet ("s"))
    _isSDI = true;

  DCOPClient *client = dcopClient();
  client->attach();
  client->registerAs("kate");

  docManager = new KateDocManager ();

  pluginManager = new KatePluginManager (this);
  pluginManager->loadAllEnabledPlugins ();

  newMainWindow ();

  connect(this, SIGNAL(lastWindowClosed()), SLOT(quit()));

  processEvents();
  
  if ( KMainWindow::canBeRestored(1) )
    mainWindows.first()->restore( true );
  else
    mainWindows.first()->restore( false );

  if (!isRestored())
  {
    for (int z=0; z<args->count(); z++)
    {
      mainWindows.first()->viewManager->openURL( args->url(z) );
    }
  }

  if ( mainWindows.first()->viewManager->viewCount () == 0 )
    mainWindows.first()->viewManager->openURL( KURL() );
}

KateApp::~KateApp ()
{
  pluginManager->writeConfig ();
}

KateMainWindow *KateApp::newMainWindow ()
{
  KateMainWindow *mainWindow = new KateMainWindow (docManager, pluginManager);
  mainWindows.append (mainWindow);

  if ((mainWindowsCount() > 1) && mainWindows.at(mainWindows.count()-2)->viewManager->activeView())
    mainWindow->viewManager->activateView ( mainWindows.at(mainWindows.count()-2)->viewManager->activeView()->getDoc()->documentNumber() );
  else if ((mainWindowsCount() > 1) && (docManager->docCount() > 0))
    mainWindow->viewManager->activateView ( (docManager->nthDoc(docManager->docCount()-1))->documentNumber() );
  else if ((mainWindowsCount() > 1) && (docManager->docCount() < 1))
    mainWindow->viewManager->openURL ( KURL() );

  mainWindow->show ();
  
  return mainWindow;
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

  mainWindows.at(n)->raise();
  KWin::setActiveWindow (mainWindows.at(n)->winId());
}

void KateApp::raiseCurrentMainWindow ()
{
  int n = mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  mainWindows.at(n)->raise();
  KWin::setActiveWindow (mainWindows.at(n)->winId());
}

Kate::ViewManager *KateApp::getViewManager ()
{
  int n = mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return ((Kate::ViewManager *)mainWindows.at(n)->viewManager);
}

Kate::DocManager *KateApp::getDocManager ()
{
  return ((Kate::DocManager *)docManager);
}

Kate::MainWindow *KateApp::getMainWindow ()
{
  int n = mainWindows.find ((KateMainWindow *)activeWindow());

  if (n < 0)
    n=0;

  return ((Kate::MainWindow *)mainWindows.at(n));
}
