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

#include "katemainwindow.h"

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

int KateProjectTreeViewItem::compare ( QListViewItem *i, int, bool ) const
{
  KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) i;
  
  if ((m_name == item->m_name) && (m_dir == item->m_dir))
    return 0;
    
  if (m_dir == 0)
  {
    if (item->m_dir == 1)
      return 1;
      
    if (m_name < item->m_name)
      return -1;
    else
      return 1;
  }
  else
  {
    if (item->m_dir == 0)
      return -1;
      
    if (m_name < item->m_name)
      return -1;
    else
      return 1;
  }
}

KateProjectTreeView::KateProjectTreeView (Kate::Project *project, KateMainWindow *mainwin, QWidget *parent) : KListView (parent)
{
  m_project = project;
  m_mainWin = mainwin;
  
  setRootIsDecorated (true);
  
  header()->setStretchEnabled (true);
  addColumn(i18n("Project: ") + m_project->name());

  addDir (0, QString ("."));
  
  connect(this,SIGNAL(executed (QListViewItem*)),this,SLOT(slotExecuted (QListViewItem*)));
}

KateProjectTreeView::~KateProjectTreeView ()
{
}

void KateProjectTreeView::addDir (KateProjectTreeViewItem *parent, const QString &dir)
{
  QString base = dir + QString ("/");

  QStringList dirs = m_project->subdirs (dir);
  
  for (uint z=0; z < dirs.count(); z++)
  {
    KateProjectTreeViewItem *item = 0;
    if (parent)
      item = new KateProjectTreeViewItem (parent, dirs[z], base + dirs[z], true);
    else
      item = new KateProjectTreeViewItem (this, dirs[z], base + dirs[z], true);
      
    addDir (item, base + dirs[z]);
  }
  
  QStringList files = m_project->files (dir);
  
  for (uint z=0; z < files.count(); z++)
  {
    if (parent)
      new KateProjectTreeViewItem (parent, files[z], base + files[z], false);
    else
      new KateProjectTreeViewItem (this, files[z], base + files[z], false);
  }
}

void KateProjectTreeView::slotExecuted ( QListViewItem *i )
{
  KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) i;
  
  if (item)
  {
    m_mainWin->viewManager()->openURL (KURL (m_project->baseurl(false), item->fullName()));
  }
}

