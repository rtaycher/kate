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

#include "kateprojectdirview.h"
#include "kateprojectdirview.moc"

#include <kdialogbase.h>
#include <kicontheme.h>
#include <klocale.h>

class KateProjectDirViewDialog : public KDialogBase
{
  public:
    KateProjectDirViewDialog (Kate::Project *project, const QString &dir, QWidget *parent);
    ~KateProjectDirViewDialog ();

    int exec();

  private:
    Kate::Project *m_project;
    QString m_dir;
    KateProjectDirView *m_view;
};

KateProjectDirView::KateProjectDirView (Kate::Project *project, const QString &dir, QWidget *parent) : KFileIconView (parent, "projectdirview")
{
  m_project = project;
  m_relDir = dir;
  m_dir = KURL (m_project->dir() + QString ("/") + m_relDir);
  m_dirs = m_project->dirs (dir);
  m_files = m_project->files (dir);

  setSelectionMode (KFile::Extended);
  setIconSize( KIcon::SizeMedium );

  m_listJob = KIO::listDir (m_dir, false, true);
  connect (m_listJob, SIGNAL(entries( KIO::Job *, const KIO::UDSEntryList&)), this, SLOT(entries( KIO::Job *, const KIO::UDSEntryList&)));
}

KateProjectDirView::~KateProjectDirView ()
{
}

void KateProjectDirView::entries( KIO::Job *, const KIO::UDSEntryList& list)
{
  for (uint z=0; z < list.count(); z++)
  {
    KFileItem *item = new KFileItem (list[z], m_dir, true, true);

    if (item->isDir())
    {
      if ((item->name() != QString (".")) && (item->name() != QString ("..")) && (m_dirs.findIndex (item->name()) == -1))
        insertItem (item);
    }
    else
    {
      if (m_files.findIndex (item->name()) == -1)
        insertItem (item);
    }
  }
}

void KateProjectDirView::addDialog (Kate::Project *project, const QString &dir, QWidget *parent)
{
  KateProjectDirViewDialog* dlg = new KateProjectDirViewDialog (project, dir, parent);
  dlg->exec();
  delete dlg;
}

KateProjectDirViewDialog::KateProjectDirViewDialog (Kate::Project *project, const QString &dir, QWidget *parent) : KDialogBase (parent, "dirviewdialog", true, i18n ("Add Directories/Files to Project"), KDialogBase::Ok|KDialogBase::Cancel)
{
  m_project = project;
  m_dir = dir;
  m_view = new KateProjectDirView (project, dir, this);
  setMainWidget(m_view);
}

KateProjectDirViewDialog::~KateProjectDirViewDialog ()
{
}

int KateProjectDirViewDialog::exec()
{
  int n = 0;

  if ((n = KDialogBase::exec()))
  {
    QStringList dirs, files;
    for (KFileItem *item = m_view->firstFileItem(); item != 0; item = m_view->nextItem (item))
    {
      if (m_view->isSelected (item))
      {
        if (item->isDir())
          dirs.push_back (item->name());
        else
          files.push_back (item->name());
      }
    }

    m_project->addDirs (m_dir, dirs);
    m_project->addFiles (m_dir, files);
  }

  return n;
}
