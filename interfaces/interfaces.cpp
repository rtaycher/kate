/***************************************************************************
                          interfaces.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
 ***************************************************************************/

#include "application.h"
#include "application.moc"

#include "docmanager.h"
#include "docmanager.moc"

#include "mainwindow.h"
#include "mainwindow.moc"

#include "plugin.h"
#include "plugin.moc"

#include "viewmanager.h"
#include "viewmanager.moc"

namespace Kate
{

Application::Application () : KUniqueApplication (true,true,true)
{
}

Application::~Application ()
{
}

DocManager::DocManager () : QObject ()
{
}

DocManager::~DocManager ()
{
}

MainWindow::MainWindow () : KDockMainWindow ()
{
}

MainWindow::~MainWindow ()
{
}

PluginConfigPage::PluginConfigPage (QObject* parent, QWidget *parentWidget) : QWidget (parentWidget, 0L)
{
  myPlugin = (Plugin *) parent;
}

PluginConfigPage::~PluginConfigPage ()
{
}

PluginView::PluginView (Plugin *plugin, MainWindow *win) : QObject ((QObject *)win)
{
  myPlugin = plugin;
  myMainWindow = win;
  myPlugin->viewList.append(this);
}

PluginView::~PluginView ()
{
  myPlugin->viewList.remove(this);
}

void PluginView::setXML (QString filename)
{
  setXMLFile( filename );
};

Plugin::Plugin (QObject* parent, const char* name) : QObject (parent, name)
{
  myApp = (class Application *) parent;
  viewList.setAutoDelete(false);
}

Plugin::~Plugin ()
{
}

ViewManager::ViewManager (QWidget *parent) : QWidget(parent)
{
}

ViewManager::~ViewManager ()
{
}

};
