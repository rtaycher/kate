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

#include <qlabel.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qgrid.h>
#include <iostream.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <klineedit.h>

KantProjectDialog::KantProjectDialog(QWidget* parent, const char* name)
	: KDialog(parent, name, true, 0)
{
	setCaption(i18n("Project configuration"));

	QHBox* hbox = new QHBox(this);
	hbox->setMargin(10);
	hbox->setSpacing(20);
	QGrid* grid = new QGrid(4, QGrid::Vertical, hbox);
	grid->setSpacing(5);

	new QLabel(i18n("Name:"), grid);
	new QLabel(i18n("Workdir:"), grid);
	new QLabel(i18n("Compile:"), grid);
	new QLabel(i18n("Run:"), grid);

	e_name = new KLineEdit(grid);
	e_workdir = new KLineEdit(grid);
	e_compile = new KLineEdit(grid);
	e_run = new KLineEdit(grid);

	QVBox* vbox = new QVBox(hbox);
	vbox->setSpacing(5);

	b_ok = new KPushButton(i18n("OK"), vbox);
	b_cancel = new KPushButton(i18n("Cancel"), vbox);

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

