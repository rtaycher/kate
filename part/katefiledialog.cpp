/***************************************************************************
                          katefiledialog.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann 
    email                : cullmann@kde.org
 ***************************************************************************/ 

/***************************************************************************
 *                                                                         * 
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  * 
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
    
#include "katefiledialog.h"

#include <kcombobox.h>
#include <ktoolbar.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <qstringlist.h>

KateFileDialog::KateFileDialog (const QString& startDir,
                    const QString& encoding,
			              QWidget *parent,
			              const QString& caption,
										int type) : KFileDialog (startDir, QString::null, parent, "", true)
{
  setCaption (caption);

  toolBar()->insertCombo(KGlobal::charsets()->availableEncodingNames(), 33333, false, 0L,
	        0L, 0L, true);

	if (type == KateFileDialog::openDialog)
	  setMode(KFile::Files);
	else
	  setMode(KFile::File);

	this->encoding = toolBar()->getCombo(33333);
  
	if (encoding != QString::null)
	  this->encoding->setCurrentItem (KGlobal::charsets()->availableEncodingNames().findIndex(encoding));
	else
    this->encoding->setCurrentItem (KGlobal::charsets()->availableEncodingNames().findIndex(QString::fromLatin1(QTextCodec::codecForLocale()->name())));
}

KateFileDialog::~KateFileDialog ()
{

}

KateFileDialogData KateFileDialog::exec()
{
	int n = KDialogBase::exec();

	KateFileDialogData data = KateFileDialogData ();
  
	if (n)
	{
	  data.encoding = this->encoding->currentText();
	  data.url = selectedURL ();
	  data.urls = selectedURLs ();
  }

	return data;
}

void KateFileDialog::slotApply()
{

}

#include "katefiledialog.moc"
