/* This file is part of the KDE project
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

#include <kwritedialogs.h>
#include <kwritedialogs.moc>
#include <klocale.h>
#include <ktexteditor/editorchooser.h>
#include <qlayout.h>

KWriteEditorChooser::KWriteEditorChooser(QWidget *):
	KDialogBase(KDialogBase::Plain,i18n("Choose Editor Component"),KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Cancel) 
{
	(new QVBoxLayout(plainPage()))->setAutoAdd(true);
	m_chooser=new KTextEditor::EditorChooser(plainPage(),"Editor Chooser");
	setMainWidget(m_chooser);
	m_chooser->readAppSetting();
}

KWriteEditorChooser::~KWriteEditorChooser() {
;
}

void KWriteEditorChooser::slotOk() {
	m_chooser->writeAppSetting();
	KDialogBase::slotOk();
}
