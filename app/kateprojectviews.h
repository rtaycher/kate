/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_ProjectViews_H__
#define __KATE_ProjectViews_H__

#include "katemain.h"

#include "../interfaces/project.h"

#include <qwidget.h>
#include <qmap.h>
#include <qframe.h>
#include <qwidgetstack.h>

class KateMainWindow;

class KateProjectViews : public QWidget
{
  Q_OBJECT

  public:
    KateProjectViews (class KateProjectManager *_projectManager, class KateMainWindow *_mainWindow, QWidget * parent = 0, const char * name = 0 );
    ~KateProjectViews ();
    
  private slots:
    void projectChanged ();
    void projectCreated (Kate::Project *project);
    void projectDeleted (uint projectNumber);

  private:
    QWidgetStack *m_stack;
    class KateProjectManager *m_projectManager;
    class KateMainWindow *m_mainWindow;

    QMap<uint, QWidget*> m_wMap;
};

#endif
