/***************************************************************************
                          kateexportaction.h  -  description
                             -------------------
    begin                : Sat 16th December 2001
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

#ifndef _KATE_EXPORTACTION_H_
#define _KATE_EXPORTACTION_H_

#include <kaction.h>
#include <qstringlist.h>

class KateExportAction: public KActionMenu
{
	Q_OBJECT
public:
	KateExportAction( QObject *related, const QString& text, QObject* parent = 0,
		const char* name = 0):KActionMenu(text, parent ,
		name){init(related);}

	KateExportAction( QObject *related, const QString& text, const QIconSet& icon,
		 QObject* parent = 0, const char* name = 0 )
		: KActionMenu(text, icon, parent, name){init(related);}

	KateExportAction( QObject *related,const QString& text, const QString& icon,
                QObject* parent = 0, const char* name = 0 )
		:KActionMenu(text,icon, parent, name ){init(related);}

	KateExportAction( QObject* related, QObject* parent = 0, const char* name = 0 )
    		:KActionMenu( parent, name) {init(related);}

	~KateExportAction(){;}
private:
	QObject *related;
	QStringList filter;
	void init(QObject *related_);
protected slots:
	void filterChoosen(int);

signals:
	void exportAs(const QString&);
};

#endif
