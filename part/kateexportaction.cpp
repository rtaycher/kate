/***************************************************************************
                          kateexportaction.cpp  -  description
                             -------------------
    begin                : Sat 16 December 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "kateexportaction.h"
#include "kateexportaction.moc"
#include <kpopupmenu.h>
#include <klocale.h>


void KateExportAction::init(QObject *related_)
{
	filter.clear();
	filter<<QString("kate_html_export");
	popupMenu()->insertItem (i18n("&HTML..."),0);
	connect(popupMenu(),SIGNAL(activated(int)),this,SLOT(filterChoosen(int)));
	related=related_;
//	connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateExportAction::filterChoosen(int id)
{
	emit exportAs(*filter.at(id));
}
