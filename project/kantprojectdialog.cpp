/***************************************************************************
                          kantprojectmanager.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
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

#include "kantprojectdialog.h"
#include "kantprojectdialog.moc"

#include <iostream.h>

KantProjectDialog::KantProjectDialog(QWidget* parent, const char* name) 
	: KDialog(parent, name, true, 0)
{
	setCaption("Project configuration");
	
	QHBox* hbox = new QHBox(this);
	hbox->setMargin(10);
	hbox->setSpacing(20);
	QGrid* grid = new QGrid(4, QGrid::Vertical, hbox);
	grid->setSpacing(5);

	new QLabel("Name:", grid);
	new QLabel("Workdir:", grid);
	new QLabel("Compile:", grid);
	new QLabel("Run:", grid);

	e_name = new KLineEdit(grid);
	e_workdir = new KLineEdit(grid);
	e_compile = new KLineEdit(grid);
	e_run = new KLineEdit(grid);
	
	QVBox* vbox = new QVBox(hbox);
	vbox->setSpacing(5);

	b_ok = new KPushButton("OK", vbox);
	b_cancel = new KPushButton("Cancel", vbox);

	vbox->adjustSize();
	grid->adjustSize();
	hbox->adjustSize();
	adjustSize();

	connect(b_ok, SIGNAL(clicked()), SLOT(accept()));
	connect(b_cancel, SIGNAL(clicked()), SLOT(reject()));
}

KantProjectDialog::~KantProjectDialog()
{
}

