 /***************************************************************************
                          katepluginiface.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _KANT_DOCUMENT_IFACE_
#define _KANT_DOCUMENT_IFACE_

#include <ktexteditor.h>
#include <qstring.h>

class KateDocumentIface : public KTextEditor::Document
{
  Q_OBJECT

  public:
    KateDocumentIface( ) : KTextEditor::Document (0L, 0L) {;};
    virtual ~KateDocumentIface () {;};
};

#endif
