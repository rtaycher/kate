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

namespace Kate
{

class DocManager : public QObject
{
  Q_OBJECT

  public:
    DocManager () : QObject () {;};
    virtual ~DocManager () {;};

 public:
    virtual class Document *getNthDoc (uint n)=0;
    virtual class Document *getCurrentDoc ()=0;
    virtual class Document *getFirstDoc ()=0;
    virtual class Document *getNextDoc ()=0;

    virtual class Document *getDocWithID (uint id)=0;

    virtual int findDoc (KURL url)=0;
    virtual bool isOpen (KURL url)=0;

    virtual uint docCount ()=0;
};

};

#endif
