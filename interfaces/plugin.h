/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#ifndef _KATE_PLUGIN_INCLUDE_
#define _KATE_PLUGIN_INCLUDE_

#include <qwidget.h>
#include <qpixmap.h>
#include <kicontheme.h>

#include <kurl.h>

namespace Kate
{

class Application;
class Project;
class MainWindow;


class KDE_EXPORT Plugin : public QObject
{
  friend class PrivatePlugin;

  Q_OBJECT

  public:
    Plugin (Application *application = 0, const char *name = 0 );
    virtual ~Plugin ();

    unsigned int pluginNumber () const;

    Application *application() const;

  private:
    class PrivatePlugin *d;
    static unsigned int globalPluginNumber;
    unsigned int myPluginNumber;
};

class KDE_EXPORT ProjectPlugin : public Plugin
{
  friend class PrivateProjectPlugin;

  Q_OBJECT

  public:
    ProjectPlugin (Project *project = 0, const char *name = 0 );
    virtual ~ProjectPlugin ();

    unsigned int projectPluginNumber () const;

    Project *project() const;

    // default implementations returns true of the following bool methodes

    virtual bool save ();
    virtual bool queryClose ();
    virtual bool close ();

    // default implementations don't modify the given list at all

    virtual void addDirs (const QString &dir, QStringList &dirs);
    virtual void removeDirs (const QString &dir, QStringList &dirs);

    virtual void addFiles (const QString &dir, QStringList &files);
    virtual void removeFiles (const QString &dir, QStringList &files);

  private:
    class PrivateProjectPlugin *d;
    static unsigned int globalProjectPluginNumber;
    unsigned int myProjectPluginNumber;
};

class KDE_EXPORT InitPlugin : public Plugin
{
  friend class PrivateInitPlugin;

  Q_OBJECT

  public:

    /**
     * Please never instanciate this class yourself from a plugin. Use the Applications performInit(pluginname,configscript) method instead
     * You must neither  assume that theol init  object still exist after control returned to the main event loop after a call to performInit nor that the init library is still loaded
     */
    InitPlugin(Application *application=0, const char *name = 0);
    virtual ~InitPlugin();

    unsigned int initPluginNumber () const;

    /* This is called whenever a new config script should be opened */
    virtual void activate( const KURL &configScript=KURL());

    /**
     * I don't create an enum, because I want this to be freely extensible
     * Please return the or'ed values from the list, you don't need initialized by kate.
     * That speeds up appliaction startup. Be aware though, that you have to unload plugins
     * or clear view/document lists yourself anyway. This is needed, because There could be
     * a reinitialisation during the application runtime. (eg if another config script is opened)
     *
     * 0x1: restoreDocuments
     * 0x2: restoreViews;
     * 0x4: loadPlugins
     */
    virtual int actionsKateShouldNotPerformOnRealStartup();

    /**
     *This should initiate the real kate initialisation. Please always return "0". The return value
     *is for later extenstion
     *
     */
    virtual int initKate();

    const KURL configScript() const;

  private:
    class PrivateInitPlugin *d;
    static unsigned int globalInitPluginNumber;
    unsigned int myInitPluginNumber;
};

Plugin *createPlugin ( const char* libname, Application *application = 0, const char *name = 0,const QStringList &args = QStringList() );
ProjectPlugin *createProjectPlugin ( const char* libname, Project *project = 0, const char *name = 0,const QStringList &args = QStringList() );

/*
 * view plugin class
 * this plugin will be bound to a ktexteditor::view
 */
class KDE_EXPORT PluginViewInterface
{
  friend class PrivatePluginViewInterface;

  public:
    PluginViewInterface ();
    virtual ~PluginViewInterface ();

    unsigned int pluginViewInterfaceNumber () const;

    /*
     * will be called from the part to bound the plugin to a view
     */
    virtual void addView (MainWindow *) = 0;
    virtual void removeView (MainWindow *) = 0;

  private:
    class PrivatePluginViewInterface *d;
    static unsigned int globalPluginViewInterfaceNumber;
    unsigned int myPluginViewInterfaceNumber;
};

PluginViewInterface *pluginViewInterface (Plugin *plugin);

}

#endif
