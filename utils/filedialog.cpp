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

// $Id$

#include "filedialog.h"
#include "filedialog.moc"

#include <kcombobox.h>
#include <ktoolbar.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <qstringlist.h>

#include <kdebug.h>

namespace Kate
{

  class PrivateFileDialog
  {
    public:
      KComboBox *encoding;
  };
};

Kate::FileDialog::FileDialog (const QString& startDir,
                    const QString& encoding,
                    QWidget *parent,
                    const QString& caption,
                    int type) : KFileDialog (startDir, QString::null, parent, "", true)
{
  d = new Kate::PrivateFileDialog ();

  QString sEncoding (encoding);

  setCaption (caption);

  QStringList availableEncodingNames(KGlobal::charsets()->availableEncodingNames());

  toolBar()->insertCombo(availableEncodingNames, 33333, false, 0L,
          0L, 0L, true);

  QStringList filter;
  filter << "all/allfiles";
  filter << "text/plain";

  if (type == Kate::FileDialog::openDialog) {
    setMode(KFile::Files);
    setMimeFilter (filter, "all/allfiles");
    }
  else {
    setMode(KFile::File);
    setOperationMode( Saving );
    setMimeFilter (filter, "text/plain");
    }

  d->encoding = toolBar()->getCombo(33333);

        // Set default encoding to the locale one, if a different default wasn't requested
        if (encoding.isNull())
          sEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());

        // This is a bit inefficient, but it's the only way to match
        // KCharsets encodings and QTextCodec encodings (e.g. KCharsets say 'utf8'
        // while QTextCodec says 'UTF-8')
        QStringList::ConstIterator it;
        QTextCodec *codecForEnc;
        int iIndex = -1;
        for (it = availableEncodingNames.begin(); it != availableEncodingNames.end(); ++it) {
          ++iIndex;
          codecForEnc = KGlobal::charsets()->codecForName(*it);
          if ( (codecForEnc->name() == sEncoding) || (*it == sEncoding) )
             break;
        }

        if (iIndex >= 0)
          d->encoding->setCurrentItem(iIndex);
}

Kate::FileDialog::~FileDialog ()
{
  delete d;
}

Kate::FileDialogData Kate::FileDialog::exec()
{
  int n = KDialogBase::exec();

  Kate::FileDialogData data = Kate::FileDialogData ();

  if (n)
  {
    data.encoding = d->encoding->currentText();
    data.url = selectedURL ();
    data.urls = selectedURLs ();
  }

  return data;
}

void Kate::FileDialog::slotApply()
{

}
