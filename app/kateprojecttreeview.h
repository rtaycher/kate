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

#ifndef __KATE_PROJECTTREEVIEW_H__
#define __KATE_PROJECTTREEVIEW_H__

#include "katemain.h"

#include "../interfaces/project.h"

#include <klistview.h>
#include <qstringlist.h>

class KateProjectTreeView;

class KateProjectTreeViewItem : public QObject, public KListViewItem
{
  Q_OBJECT

  public:
    KateProjectTreeViewItem (KateProjectTreeView * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir = false);
    KateProjectTreeViewItem (KateProjectTreeViewItem * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir = false);
    ~KateProjectTreeViewItem ();
    
    void init ();
    
    bool isDir () { return m_dir; };
    
    QString name () { return m_name; };
    
    QString fullName () { return m_fullName; };
    
    int compare ( QListViewItem *i, int, bool ) const;
    
  private slots:
    void dirsAdded (const QStringList &dirs);
    void dirsRemoved (const QStringList &dirs);
    
    void filesAdded (const QStringList &files);
    void filesRemoved (const QStringList &files);

  private:
    QString m_name;
    QString m_fullName;
    Kate::Project *m_project;
    Kate::ProjectDirFile::Ptr m_dirFile;
    bool m_dir;
};

class KateProjectTreeView : public KListView
{
  Q_OBJECT

  public:
    KateProjectTreeView (Kate::Project *project, class KateMainWindow *mainwin, QWidget *parent);
    ~KateProjectTreeView ();
    
    void addDir (KateProjectTreeViewItem *parent, const QString &dir);
    
  private slots:
    void slotDoubleClicked( QListViewItem *i, const QPoint &pos, int c );
    
  private:
    Kate::Project *m_project;
    class KateMainWindow *m_mainWin;
};

#endif
