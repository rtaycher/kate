/***************************************************************************
                          plugin_katetextfilter.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katedefaultproject.h"
#include "katedefaultproject.moc"

#include <qfileinfo.h>
#include <kgenericfactory.h>
#include <kaction.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <kio/netaccess.h>
                                     
class PluginView : public KXMLGUIClient
{             
  friend class PluginKateDefaultProject;

  public:
    Kate::MainWindow *win;
};

K_EXPORT_COMPONENT_FACTORY( katedefaultprojectplugin, KGenericFactory<PluginKateDefaultProject>( "katedefaultproject" ) )

PluginKateDefaultProject::PluginKateDefaultProject( QObject* parent, const char* name, const QStringList& )
    : Kate::ProjectPlugin ( (Kate::Project *)parent, name )
{
}

PluginKateDefaultProject::~PluginKateDefaultProject()
{
}

void PluginKateDefaultProject::addView(Kate::MainWindow *win)
{
    // TODO: doesn't this have to be deleted?
    PluginView *view = new PluginView ();
             
    view->setInstance (new KInstance("kate"));
    view->setXMLFile( "plugins/katedefaultproject/ui.rc" );
    win->guiFactory()->addClient (view);
    view->win = win; 
    
   m_views.append (view);
}   

void PluginKateDefaultProject::removeView(Kate::MainWindow *win)
{
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.remove (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }  
}
