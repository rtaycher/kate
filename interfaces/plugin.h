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
#include <qpixmap.h>
#include <qwidget.h>

namespace Kate
{

class PluginConfigPage : public QWidget
{
  Q_OBJECT

  friend class Plugin;
  friend class PluginView;

  public:
    PluginConfigPage (QObject* parent = 0L, QWidget *parentWidget = 0L);
    virtual ~PluginConfigPage ();

    // apply the config of the page to the plugin / save it
    virtual void applyConfig () { ; };

    class Plugin *myPlugin;
};

class PluginView : public QObject, virtual public KXMLGUIClient
{
  Q_OBJECT

  friend class Plugin;
  friend class PluginConfigPage;

  public:
    PluginView (class Plugin *plugin = 0L, class MainWindow *win = 0L);
    virtual ~PluginView ();

    // set xmlGUI rc file
    void setXML (QString filename);

    class Plugin *myPlugin;
    class MainWindow *myMainWindow;
};

class Plugin : public QObject
{
  Q_OBJECT

  friend class PluginView;
  friend class PluginConfigPage;

  public:
    Plugin (QObject* parent = 0L, const char* name = 0L);
    virtual ~Plugin ();

    // create a view / can plugin create a view ?
    virtual PluginView *createView (class MainWindow *) { return 0L; };
    virtual bool hasView () { return true; };

    // create a configpage / can plugin create a configpage ?
    virtual PluginConfigPage *createConfigPage (QWidget *) { return 0L; };
     virtual bool hasConfigPage () { return false; };

     // name / title / icon of the configpage (if you have a page, you must have these stuff too)
     virtual class QString configPageName() { return 0L; };
    virtual class QString configPageTitle() { return 0L; };
    virtual class QPixmap configPageIcon() { return 0L; };

    QList<PluginView> viewList;
    class Application *myApp;
};

};

#endif
