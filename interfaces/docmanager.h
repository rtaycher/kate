/***************************************************************************
                          docmanager.h -  description
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

#ifndef _KATE_DOCMANAGER_INCLUDE_
#define _KATE_DOCMANAGER_INCLUDE_

#include <qobject.h>
#include <kurl.h>

namespace Kate
{

class DocManager : public QObject
{
  Q_OBJECT

  public:
     DocManager ();
    virtual ~DocManager ();

  public:
    // get doc number n/current/first/next/doc by id
    virtual class Document *getNthDoc (uint) { return 0L; };
    virtual class Document *getCurrentDoc () { return 0L; };
     virtual class Document *getFirstDoc () { return 0L; };
     virtual class Document *getNextDoc () { return 0L; };
     virtual class Document *getDocWithID (uint) { return 0L; };

     // find do with URL / is doc with URL open ?
     virtual int findDoc (KURL) { return 0L; };
     virtual bool isOpen (KURL) { return 0L; };

     // how much docs open ?
     virtual uint docCount () { return 0L; };
};

};

#endif
