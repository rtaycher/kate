/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "application.h"
#include "application.moc"

#include "documentmanager.h"
#include "mainwindow.h"
#include "pluginmanager.h"

#include "../app/kateapp.h"

#include <kapplication.h>

namespace Kate
{

class PrivateApplication
  {
  public:
    PrivateApplication ()
    {
    }

    ~PrivateApplication ()
    {

    }

    KateApp *app;
  };

Application::Application (void *application) : QObject ((KateApp *) application)
{
  d = new PrivateApplication;
  d->app = (KateApp *) application;
}

Application::~Application ()
{
  delete d;
}

DocumentManager *Application::documentManager ()
{
  return d->app->documentManager ()->documentManager ();
}

PluginManager *Application::pluginManager ()
{
  return d->app->pluginManager ()->pluginManager ();
}

MainWindow *Application::activeMainWindow ()
{
  if (!d->app->activeMainWindow())
    return 0;

  return d->app->activeMainWindow()->mainWindow();
}

uint Application::mainWindows ()
{
  return d->app->mainWindows ();
}

MainWindow *Application::mainWindow (uint n)
{
  if (n < mainWindows ())
    return d->app->mainWindow (n)->mainWindow();

  return 0;
}

Application *application ()
{
  return KateApp::self()->application ();
}

}

