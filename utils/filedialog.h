/* This file is part of the KDE project
   Copyright (C) 2002 Christoph Cullmann <cullmann@kde.org>

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

#ifndef _KATEFILEDIALOG_H_
#define _KATEFILEDIALOG_H_

#include <kfiledialog.h>
#include <kurl.h>
#include <kglobal.h>
#include <klocale.h>

namespace Kate
{

class FileDialogData
{
  public:
    KURL::List urls;
    KURL url;
    QString encoding;
};

class FileDialog : public KFileDialog
{
  friend class PrivateFileDialog;

  Q_OBJECT

  public:
    FileDialog (const QString& startDir = QString::null,
                    const QString& encoding = QString::fromLatin1(KGlobal::locale()->encoding()),
                    QWidget *parent= 0,
                    const QString& caption = QString::null, int type = FileDialog::openDialog);

    ~FileDialog ();

    FileDialogData exec ();

    enum FileDialogType
    {
      openDialog,
      saveDialog
    };

  protected slots:
    void slotApply();

  private:
    class PrivateFileDialog *d;
};

};

#endif
