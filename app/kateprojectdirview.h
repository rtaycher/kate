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

#ifndef __KATE_PROJECTDIRVIEW_H__
#define __KATE_PROJECTDIRVIEW_H__

#include "katemain.h"

#include "../interfaces/project.h"

#include <kfileiconview.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

class KateProjectDirView : public KFileIconView
{
  Q_OBJECT

  public:
    KateProjectDirView (Kate::Project *project, const QString &dir, QWidget *parent);
    ~KateProjectDirView ();

    static void addDialog (Kate::Project *project, const QString &dir, QWidget *parent);

  private slots:
    void entries( KIO::Job *job, const KIO::UDSEntryList& list);

  private:
    Kate::Project *m_project;
    QString m_relDir;
    KIO::ListJob *m_listJob;
    KURL m_dir;
    QStringList m_dirs;
    QStringList m_files;
};

#endif
