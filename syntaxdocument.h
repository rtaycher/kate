/***************************************************************************
   $Id$
                          syntaxdocument.h  -  description
                             -------------------
    begin                : Thu Nov 16 2000
    copyright            : (C) 2000 by Maniac
    email                : Maniac@Alltel.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SYNTAXDOCUMENT_H
#define SYNTAXDOCUMENT_H

#include <qdom.h>
#include <qstringlist.h>
#include <qstring.h>

/**
  *@author Maniac
  */

class SyntaxDocument : public QDomDocument  {
  public:
	  SyntaxDocument();
	  ~SyntaxDocument();

	  QStringList& finddata(const QString& langName,const QString& type=QString("Keywords"));

    QStringList data() { return m_data;};
    QString name() { return m_name;}
	private:
	  QStringList m_data;
    QString m_name;

};

#endif
