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

#ifndef _KWRITEDIALOGS_H_
#define _KWRITEDIALOGS_H_

#include <kdialogbase.h>

namespace KTextEditor {
	class EditorChooser;
}

class KWriteEditorChooser: public KDialogBase {

Q_OBJECT

public:
	KWriteEditorChooser(QWidget *parent);
	virtual ~KWriteEditorChooser();
private:
	KTextEditor::EditorChooser *m_chooser;

protected slots:
	virtual void slotOk();
};


#endif
