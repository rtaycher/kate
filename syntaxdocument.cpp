/***************************************************************************
   $Id$
                          syntaxdocument.cpp  -  description
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

#include "syntaxdocument.h"
#include <qfile.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kmessagebox.h>

SyntaxDocument::SyntaxDocument() : QDomDocument()
{
    QString syntaxPath = locate("data", "kwrite/syntax.xml");
    if( !syntaxPath.isEmpty() )
    {
        QFile f( syntaxPath );
        if ( f.open(IO_ReadOnly) )
            setContent(&f);
        else
            KMessageBox::error( 0L, i18n("Can't open %1").arg(syntaxPath) );
        f.close();
    } else
        KMessageBox::error( 0L, i18n("File share/apps/kwrite/syntax.xml not found ! Check your installation!") );
}

SyntaxDocument::~SyntaxDocument()
{

}
/*syntax of xml file whitespace is optional
 <language name=XXX>
  <keywords>
   <keyword> XXX </keyword>
  </keywords>
  <types>
  <type XXX </type>
  </types>
</language>

*/
QStringList& SyntaxDocument::finddata(const QString& langName,const QString& type)
{
	QDomElement docElem = documentElement();
	QDomNode n = docElem.firstChild();
		while ( !n.isNull() ) {
			if ( n.isElement()) {
				QDomElement e = n.toElement(); //e.tagName is language
				QDomNode child=e.firstChild(); // child.toElement().tagname() is keywords/types
				QDomNode grandchild=child.firstChild(); // grandchild.tagname is keyword/type

// at this point e.attribute("name") should equal langName
// and we need the grandchild.tagName() equal to type

				if(e.attribute("name").compare(langName)==0 && grandchild.toElement().tagName().compare(type) == 0 ){
					QDomNodeList childlist=n.childNodes();
					QDomNode childnode=childlist.item(0).firstChild();
					QDomNodeList grandchildlist=childlist.item(0).childNodes();
					for(uint i=0; i< grandchildlist.count();i++)
						m_data+=grandchildlist.item(i).toElement().text().stripWhiteSpace();
					return m_data;
				}
			}
		n = n.nextSibling();
	}
	return m_data;
}
