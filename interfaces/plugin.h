/***************************************************************************
                          plugin.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
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

#ifndef _KATE_PLUGIN_INCLUDE_
#define _KATE_PLUGIN_INCLUDE_

#include <qobject.h>
#include <kxmlguiclient.h>
#include <qlist.h>
#include <qstring.h>
#include <qwidget.h>

namespace Kate
{

class PluginConfigPage : public QWidget
{
  Q_OBJECT

  friend class Plugin;

  public:
    PluginConfigPage (QObject* parent = 0L, QWidget *parentWidget=0L) : QWidget (parentWidget, 0L)
     { myPlugin = (Plugin *) parent; };

    ~PluginConfigPage () {;};

    virtual void applyConfig () { ; };

    class Plugin *myPlugin;
};

class PluginView : public QObject, virtual public KXMLGUIClient
{
  Q_OBJECT

  friend class Plugin;

  public:
    PluginView (class Plugin *plugin, class MainWindow *win) : QObject ((QObject *)win)
     { myPlugin = plugin; myMainWindow = win; };

    ~PluginView () {;};

    void setXML (QString filename)
      { setXMLFile( filename ); };

    class Plugin *myPlugin;
    class MainWindow *myMainWindow;
};

class Plugin : public QObject
{
  Q_OBJECT

  friend class PluginView;
  friend class PluginConfigPage;

  public:
    Plugin (QObject* parent = 0L, const char* name = 0L) : QObject (parent, name)
    { myApp = (class Application *) parent; };

    virtual ~Plugin () {;};

    virtual PluginView *createView (class MainWindow *win)=0;
    virtual bool hasView () { return true; };

    virtual PluginConfigPage *createConfigPage (QWidget *) { return 0L; };
    virtual bool hasConfigPage () { return false; };

    QList<PluginView> viewList;
    class Application *myApp;
};

};

#endif
