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

#ifndef _DEFAULT_PROJECT_KATE_HEADER_H_
#define _DEFAULT_PROJECT_KATE_HEADER_H_

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

#endif
