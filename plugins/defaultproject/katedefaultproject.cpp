/* This file is part of the KDE project
   Copyright (C) 2003 Christoph Cullmann <cullmann@kde.org>

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

#include "katedefaultproject.h"
#include "katedefaultproject.moc"

#include <kgenericfactory.h>
#include <kaction.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>

#include <qfileinfo.h>

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
