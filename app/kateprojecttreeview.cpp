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

KateProjectTreeViewItem::KateProjectTreeViewItem (KateProjectTreeView * parent, const QString &name, const QString &fullname, bool dir)
 : KListViewItem (parent, name)
{
  m_name = name;
  m_fullName = fullname;
  m_dir = dir;
}

KateProjectTreeViewItem::KateProjectTreeViewItem (KateProjectTreeViewItem * parent, const QString &name, const QString &fullname, bool dir)
 : KListViewItem (parent, name)
{
  m_name = name;
  m_fullName = fullname;
  m_dir = dir;
}
    
KateProjectTreeViewItem::~KateProjectTreeViewItem ()
{
}

KateProjectTreeView::KateProjectTreeView (Kate::Project *project, QWidget *parent) : KListView (parent)
{
  m_project = project;
  
  setRootIsDecorated (true);
  
  header()->setStretchEnabled (true);
  addColumn(i18n("Project Tree"));

  addDir (0, QString::null);
}

KateProjectTreeView::~KateProjectTreeView ()
{
}

void KateProjectTreeView::addDir (KateProjectTreeViewItem *parent, const QString &dir)
{
  QStringList dirs = m_project->subdirs (dir);
  
  for (uint z=0; z < dirs.count(); z++)
  {
    KateProjectTreeViewItem *item = 0;
    if (parent)
      item = new KateProjectTreeViewItem (parent, dirs[z], dir + QString ("/") + dirs[z], true);
    else
      item = new KateProjectTreeViewItem (this, dirs[z], dir + QString ("/") + dirs[z], true);
      
    addDir (item, dir + QString ("/") + dirs[z]);
  }
  
  QStringList files = m_project->files (dir);
  
  for (uint z=0; z < files.count(); z++)
  {
    if (parent)
      new KateProjectTreeViewItem (parent, files[z], dir + QString ("/") + files[z], false);
    else
      new KateProjectTreeViewItem (this, files[z], dir + QString ("/") + files[z], false);
  }
}
