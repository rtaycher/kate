/***************************************************************************
                          katefiledialog.cpp  -  description
                             ------------------- 
    begin                : Mon Feb 5 2001 
    copyright            : (C) 2001 by Christoph Cullmann 
    email                : crossfire@babylon2k.de 
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
			              QWidget *parent,
			              const QString& caption, int type, const QString& encoding) : KFileDialog (startDir, QString::null, parent, "", true)
{
  toolBar()->insertCombo(KGlobal::charsets()->availableEncodingNames(), 33333, false, "",
	        this, "", true);

	this->encoding = toolBar()->getCombo(33333);
  this->encoding->setCurrentItem (KGlobal::charsets()->availableEncodingNames().findIndex(encoding));
}

KateFileDialog::~KateFileDialog ()
{

}

KateFileDialogData *KateFileDialog::exec()
{
	int n = KDialogBase::exec();

  if (n)
	{
    KateFileDialogData *data = new KateFileDialogData ();
    data->encoding = this->encoding->currentText();
		data->url = selectedURL ();
		data->urls = selectedURLs ();

		return data;
	}

	return 0L;
}

void KateFileDialog::slotApply()
{

}