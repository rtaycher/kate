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

#include "kateprojectdirview.h"
#include "katemainwindow.h"

#include <klocale.h>
#include <kiconloader.h>
#include <kmimetype.h>

#include <qheader.h>

KateProjectTreeViewItem::KateProjectTreeViewItem (QDict<KateProjectTreeViewItem> *dict, KateProjectTreeView * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir)
 : QObject (0), KListViewItem (parent)
{
  m_name = name;
  m_fullName = fullname;
  m_dir = dir;
  m_project = prj;
  m_dict = dict;

  init ();
}

KateProjectTreeViewItem::KateProjectTreeViewItem (QDict<KateProjectTreeViewItem> *dict, KateProjectTreeViewItem * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir)
 : QObject (0), KListViewItem (parent)
{
  m_name = name;
  m_fullName = fullname;
  m_dir = dir;
  m_project = prj;
  m_dict = dict;

  init ();
}

KateProjectTreeViewItem::~KateProjectTreeViewItem ()
{
  if (m_dir)
    m_dict->remove(QString("/")+m_fullName);
}

void KateProjectTreeViewItem::init ()
{
  if (m_dir)
    m_dict->insert(QString("/")+m_fullName, this);

  if (m_dir)
    setPixmap (0, KMimeType::mimeType("inode/directory")->pixmap( KIcon::Small ));
  else
    setPixmap (0, KMimeType::findByPath (m_project->dir() + QString ("/") + m_fullName)->pixmap (KIcon::Small, KIcon::SizeSmall));

  setText (0, m_name);
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
/*
void KateProjectTreeViewItem::dirsAdded (const QStringList &dirs)
{
  QString fullname = m_fullName;
  if (!m_fullName.isNull())
    fullname += QString ("/");

  for (uint z=0; z < dirs.size(); z++)
  {
    KateProjectTreeViewItem *item = new KateProjectTreeViewItem (this, m_project, dirs[z], fullname + dirs[z], true);
    ((KateProjectTreeView *)listView())->addDir (item, fullname + dirs[z]);
  }
}

void KateProjectTreeViewItem::dirsRemoved (const QStringList &dirs)
{
  for (KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) firstChild(); item; item = (KateProjectTreeViewItem *) nextSibling())
  {
    for (uint z=0; z < dirs.size(); z++)
    {
      if (dirs[z] == item->name())
      {
        delete item;
        break;
      }
    }
  }
}

void KateProjectTreeViewItem::filesAdded (const QStringList &files)
{
  QString fullname = m_fullName;
  if (!m_fullName.isNull())
    fullname += QString ("/");

  for (uint z=0; z < files.size(); z++)
  {
    new KateProjectTreeViewItem (this, m_project, files[z], fullname + files[z], false);
  }
}

void KateProjectTreeViewItem::filesRemoved (const QStringList &files)
{
  for (KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) firstChild(); item; item = (KateProjectTreeViewItem *) nextSibling())
  {
    for (uint z=0; z < files.size(); z++)
    {
      if (files[z] == item->name())
      {
        delete item;
        break;
      }
    }
  }
}
*/
KateProjectTreeView::KateProjectTreeView (Kate::Project *project, KateMainWindow *mainwin, QWidget *parent) : KListView (parent)
{
  m_project = project;
  m_mainWin = mainwin;

  m_dirDict.setAutoDelete (false);

  setSelectionModeExt( KListView::Single );
  setRootIsDecorated (false);
  setAlternateBackground (viewport()->colorGroup().base());

  header()->setStretchEnabled (true);
  addColumn(i18n("Project: ") + m_project->name());
  header()->hide ();

  KateProjectTreeViewItem *item = new KateProjectTreeViewItem (&m_dirDict, this, m_project, i18n("Project Dir"), QString::null, true);
  addDir (item, QString::null);

  setOpen (item, true);

  connect(this,SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),this,SLOT(slotDoubleClicked(QListViewItem *, const QPoint &, int)));

  connect (m_project, SIGNAL (dirsAdded (const QString &, const QStringList &)), this, SLOT (dirsAdded (const QString &, const QStringList &)));
  connect (m_project, SIGNAL (filesAdded (const QString &, const QStringList &)), this, SLOT (filesAdded (const QString &, const QStringList &)));
  connect (m_project, SIGNAL (dirsRemoved (const QString &, const QStringList &)), this, SLOT (dirsRemoved (const QString &, const QStringList &)));
  connect (m_project, SIGNAL (filesRemoved (const QString &, const QStringList &)), this, SLOT (filesRemoved (const QString &, const QStringList &)));
}

KateProjectTreeView::~KateProjectTreeView ()
{
}

void KateProjectTreeView::addDir (KateProjectTreeViewItem *parent, const QString &dir)
{
  QString base = dir;

  if (!dir.isNull())
    base += QString ("/");

  QStringList dirs = m_project->dirs (dir);

  for (uint z=0; z < dirs.count(); z++)
  {
    KateProjectTreeViewItem *item = new KateProjectTreeViewItem (&m_dirDict, parent, m_project, dirs[z], base + dirs[z], true);
    addDir (item, base + dirs[z]);
  }

  QStringList files = m_project->files (dir);

  for (uint z=0; z < files.count(); z++)
  {
    new KateProjectTreeViewItem (&m_dirDict, parent, m_project, files[z], base + files[z], false);
  }
}

void KateProjectTreeView::slotDoubleClicked( QListViewItem *i, const QPoint &, int )
{
  KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) i;

  if (!item)
    return;

  if (item->isDir())
  {
    KateProjectDirView::addDialog (m_project, item->fullName(), this);
  }
  else
    m_mainWin->viewManager()->openURL (KURL (m_project->dir() + QString ("/") + item->fullName()));
}


void KateProjectTreeView::dirsAdded (const QString &dir, const QStringList &dirs)
{
}

void KateProjectTreeView::dirsRemoved (const QString &dir, const QStringList &dirs)
{
}

void KateProjectTreeView::filesAdded (const QString &dir, const QStringList &files)
{
}

void KateProjectTreeView::filesRemoved (const QString &dir, const QStringList &files)
{

}

