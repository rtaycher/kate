/***************************************************************************
                          kantproject.h  -  description
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

#ifndef kant_project_h
#define kant_project_h

#include <qstring.h>

class KantProject
{
	public:
		KantProject();
		~KantProject();

		void compile();
		void run();

	private:
		QString workdir;
		QString compilestr;
		QString runstr;
};

#endif
