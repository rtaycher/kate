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

#include <qvbox.h>
#include <qstringlist.h>
#include <qdict.h>

class KateProjectTreeView;

class KateProjectTreeViewItem : public KListViewItem
{
  public:
    KateProjectTreeViewItem (QDict<KateProjectTreeViewItem> *dict, KateProjectTreeView * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir = false);
    KateProjectTreeViewItem (QDict<KateProjectTreeViewItem> *dict, KateProjectTreeViewItem * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir = false);
    ~KateProjectTreeViewItem ();

    void init ();

    bool isDir () { return m_dir; };

    QString name () { return m_name; };

    QString fullName () { return m_fullName; };

    int compare ( QListViewItem *i, int, bool ) const;

  private:
    QString m_name;
    QString m_fullName;
    Kate::Project *m_project;
    bool m_dir;
    QDict<KateProjectTreeViewItem> *m_dict;
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

    void dirsAdded (const QString &dir, const QStringList &dirs);
    void dirsRemoved (const QString &dir, const QStringList &dirs);

    void filesAdded (const QString &dir, const QStringList &files);
    void filesRemoved (const QString &dir, const QStringList &files);

    void slotContextMenuRequested ( QListViewItem * item, const QPoint & pos, int col );

    void removeIt ();
    void addIt ();

    // doubleclicked or return pressed
    void execute( QListViewItem * );

  private:
    Kate::Project *m_project;
    class KateMainWindow *m_mainWin;
    QDict<KateProjectTreeViewItem> m_dirDict;
};

class KateProjectTreeViewContainer : public QVBox
{
  Q_OBJECT
  public:
    KateProjectTreeViewContainer( Kate::Project *project, class KateMainWindow *mainwin, QWidget*, const char* name=0 );
    ~KateProjectTreeViewContainer();

     KateProjectTreeView * tree();

  protected:
    bool eventFilter( QObject *, QEvent * );

  private slots:
    void qfTextChanged( const QString & );

  private:
    QString oldtext; //??
    class KLineEdit *m_leQF;
    KateProjectTreeView *m_tree;
};

#endif
