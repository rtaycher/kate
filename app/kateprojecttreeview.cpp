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

// $Id$

#include "kateprojecttreeview.h"
#include "kateprojecttreeview.moc"

#include <klocale.h>

#include <qheader.h>

KateProjectTreeView::KateProjectTreeView (Kate::Project *project, QWidget *parent) : KListView (parent)
{
  m_project = project;
  
  header()->setStretchEnabled (true);
  addColumn(i18n("Project Tree"));

  QStringList sections = project->sections ();
  
  KListViewItem *last = 0;
  for (uint z=0; z < sections.count(); z++)
  {
    last = new KListViewItem (this, sections[z]);
  }
}

KateProjectTreeView::~KateProjectTreeView ()
{
}
