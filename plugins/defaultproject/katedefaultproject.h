 /***************************************************************************
                          plugin_katetextfilter.h  -  description
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

#ifndef _PLUGIN_KANT_HEADER_H
#define _PLUGIN_KANT_HEADER_H

#include <qstring.h>

#include <plugin.h>
#include <application.h>
#include <kate/view.h>
#include <kate/document.h>
#include <documentmanager.h>
#include <mainwindow.h>
#include <viewmanager.h>

class PluginKateDefaultProject : public Kate::ProjectPlugin, Kate::PluginViewInterface
{
  Q_OBJECT

  public:
    PluginKateDefaultProject( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );
    virtual ~PluginKateDefaultProject();

    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

  private:
    QPtrList<class PluginView> m_views;
};

#endif // _PLUGIN_KANT_OPENHEADER_H
