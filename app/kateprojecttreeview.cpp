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

//BEGIN Includes
#include "kateprojecttreeview.h"
#include "kateprojecttreeview.moc"

#include "kateprojectdirview.h"
#include "katemainwindow.h"

#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <klineedit.h>

#include <kdebug.h>

#include <qlabel.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qevent.h>
//END

//BEGIN KateProjectTreeViewItem
KateProjectTreeViewItem::KateProjectTreeViewItem (QDict<KateProjectTreeViewItem> *dict, KateProjectTreeView * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir)
 : KListViewItem (parent)
{
  m_name = name;
  m_fullName = fullname;
  m_dir = dir;
  m_project = prj;
  m_dict = dict;

  init ();
}

KateProjectTreeViewItem::KateProjectTreeViewItem (QDict<KateProjectTreeViewItem> *dict, KateProjectTreeViewItem * parent, Kate::Project *prj, const QString &name, const QString &fullname, bool dir)
 : KListViewItem (parent)
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
//END KateProjectTreeViewItem

//BEGIN KateProjectTreeView
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

  KateProjectTreeViewItem *item = new KateProjectTreeViewItem (&m_dirDict, this, m_project, i18n("Project Directory"), QString::null, true);
  addDir (item, QString::null);

  setOpen (item, true);

  connect(this,SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),this,SLOT(slotDoubleClicked(QListViewItem *, const QPoint &, int)));
  connect( this, SIGNAL(returnPressed(QListViewItem*)), SLOT(execute(QListViewItem*)) );
  connect(this, SIGNAL( contextMenuRequested( QListViewItem *, const QPoint& , int ) ),
            this, SLOT( slotContextMenuRequested( QListViewItem *, const QPoint &, int ) ) );

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
  execute( i );
}

void KateProjectTreeView::execute( QListViewItem *i )
{
  KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) i;

  if (!item)
    return;

  if (item->isDir())
    item->setOpen (!item->isOpen());
  else
    m_mainWin->viewManager()->openURL (KURL (m_project->dir() + QString ("/") + item->fullName()));
}

void KateProjectTreeView::dirsAdded (const QString &dir, const QStringList &dirs)
{
  KateProjectTreeViewItem *item = m_dirDict [QString("/")+dir];

  if (!item)
    return;

  QString fullname = dir;
  if (!fullname.isNull())
    fullname += QString ("/");

  for (uint z=0; z < dirs.size(); z++)
  {
    // add dir recursive
    KateProjectTreeViewItem *i = new KateProjectTreeViewItem (&m_dirDict, item, m_project, dirs[z], fullname + dirs[z], true);
    addDir (i, fullname+dirs[z]);
  }
}

void KateProjectTreeView::dirsRemoved (const QString &dir, const QStringList &dirs)
{
  KateProjectTreeViewItem *item = m_dirDict [QString("/")+dir];

  if (!item)
    return;

  QPtrList<KateProjectTreeViewItem> l;
  l.setAutoDelete (true);

  KateProjectTreeViewItem *myChild = (KateProjectTreeViewItem *) item->firstChild();
  while( myChild )
  {
    if (dirs.findIndex (myChild->name()) != -1)
      l.append (myChild);

    myChild = (KateProjectTreeViewItem *) myChild->nextSibling();
  }
}

void KateProjectTreeView::filesAdded (const QString &dir, const QStringList &files)
{
  KateProjectTreeViewItem *item = m_dirDict [QString("/")+dir];

  if (!item)
    return;

  QString fullname = dir;
  if (!fullname.isNull())
    fullname += QString ("/");

  for (uint z=0; z < files.size(); z++)
  {
    new KateProjectTreeViewItem (&m_dirDict, item, m_project, files[z], fullname + files[z], false);
  }
}

void KateProjectTreeView::filesRemoved (const QString &dir, const QStringList &files)
{
  KateProjectTreeViewItem *item = m_dirDict [QString("/")+dir];

  if (!item)
    return;

  QPtrList<KateProjectTreeViewItem> l;
  l.setAutoDelete (true);

  KateProjectTreeViewItem *myChild = (KateProjectTreeViewItem *) item->firstChild();
  while( myChild )
  {
    if (files.findIndex (myChild->name()) != -1)
      l.append (myChild);

    myChild = (KateProjectTreeViewItem *) myChild->nextSibling();
  }
}

void KateProjectTreeView::slotContextMenuRequested ( QListViewItem * item, const QPoint & pos, int )
{
  if (!item)
    return;

  KateProjectTreeViewItem *i = (KateProjectTreeViewItem *) item;

  QPopupMenu *menu = new QPopupMenu (this);

  if (i->isDir())
    menu->insertItem (i18n("Add Directories/Files"), this, SLOT(addIt()));

  if (!i->fullName().isNull())
    menu->insertItem (i->isDir() ? i18n("Remove Directory") : i18n("Remove File"), this, SLOT(removeIt()));

  menu->exec(pos);
}

void KateProjectTreeView::removeIt ()
{
  KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) selectedItem();

  if (!item)
    return;

  if (item->fullName().isNull())
    return;

  QString dir = ((KateProjectTreeViewItem *) item->parent())->fullName();
  QStringList liste (item->name());

  if (item->isDir())
    m_project->removeDirs (dir, liste);
  else
    m_project->removeFiles (dir, liste);
}

void KateProjectTreeView::addIt ()
{
  KateProjectTreeViewItem *item = (KateProjectTreeViewItem *) selectedItem();

  if (!item)
    return;

  if (item->isDir())
    KateProjectDirView::addDialog (m_project, item->fullName(), this);
}
//END KateProjectTreeView

//BEGIN KateProjectTreeViewContainer
KateProjectTreeViewContainer::KateProjectTreeViewContainer(
     Kate::Project *project, KateMainWindow *mainwin,
     QWidget *parent, const char *name )
     : QVBox( parent, name )
{
  // quick find entry
  QHBox *b = new QHBox( this, "quickfind entry" );
  QLabel *l = new QLabel( i18n("F&ind:"), b );
  m_leQF = new KLineEdit( b );
  m_leQF->installEventFilter( this );
  l->setBuddy( m_leQF );
  connect( m_leQF, SIGNAL(textChanged(const QString &)),
                   SLOT(qfTextChanged(const QString &)) );

  // tree view
  m_tree = new KateProjectTreeView( project, mainwin, this );
}

KateProjectTreeViewContainer::~KateProjectTreeViewContainer()
{}

KateProjectTreeView *KateProjectTreeViewContainer::tree()
{
  return m_tree;
}

void KateProjectTreeViewContainer::qfTextChanged( const QString &t )
{
  QListViewItem *i ( m_tree->currentItem() );
  if ( ! i ) i = m_tree->firstChild();

  bool found ( false );
  QListViewItemIterator it ( i );
/*  if ( oldtext < t )
  {*/
    while ( it.current() )
    {
      if ( it.current()->text(0).startsWith( t ) )
      {
        found = true;
        break;
      }
      ++it;
    }
//   }
  if ( ! found )
  {
    QListViewItemIterator it ( i );
    while ( it.current() )
    {
      if ( it.current()->text(0).startsWith( t ) )
      {
        found = true;
        break;
      }
      --it;
    }
  }
  if ( it.current() )
  {
    i = it.current();
    if ( i->parent() && ! i->parent()->isOpen() )
      i->parent()->setOpen( true );
    m_tree->ensureItemVisible( i );

    m_tree->setCurrentItem( i );
    m_tree->setSelected(i, true);
  }
  oldtext = t;
}

bool KateProjectTreeViewContainer::eventFilter( QObject *o, QEvent *e )
{
  if ( o == m_leQF )
  {
    if ( e->type() == QEvent::KeyPress &&
         ( ((QKeyEvent*)e)->key() == Qt::Key_Return ||
           ((QKeyEvent*)e)->key() == Qt::Key_Enter ) )
    {
      return kapp->sendEvent( m_tree, e );
    }
    if ( e->type() == QEvent::KeyPress &&
         ((QKeyEvent*)e)->key() == Qt::Key_Tab )
    {
      m_tree->setFocus();
      return true;
    }
  }
  return QVBox::eventFilter( o, e );
}

//END KateProjectTreeViewContainer
