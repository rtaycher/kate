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
#ifndef _KANT_DOCMANAGER_IFACE_
#define _KANT_DOCMANAGER_IFACE_

#include <qobject.h>

#include "katedocumentIface.h"

class KateDocManagerIface : public QObject
{
  Q_OBJECT

  public:
    KateDocManagerIface () : QObject () {;};
    virtual ~KateDocManagerIface () {;};

 public:
    virtual KateDocumentIface *getNthDoc (uint n)=0;
    virtual KateDocumentIface *getCurrentDoc ()=0;
    virtual KateDocumentIface *getFirstDoc ()=0;
    virtual KateDocumentIface *getNextDoc ()=0;

    virtual KateDocumentIface *getDocWithID (uint id)=0;

    virtual int findDoc (KURL url)=0;
    virtual bool isOpen (KURL url)=0;

    virtual uint docCount ()=0;
};

#endif
