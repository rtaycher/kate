/* This file is part of the KDE project
   Copyright (C) 2003 Ian Reinhart Geiser <geiseri@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _katedocmanager_Iface_h_
#define _katedocmanager_Iface_h_

#include <dcopobject.h>
#include <kurl.h>
class KateDocManagerDCOPIface : virtual public DCOPObject
{
  K_DCOP

  k_dcop:
    virtual bool closeDocument(uint)=0;
    virtual bool closeDocumentWithID(uint)=0;
    virtual bool closeAllDocuments()=0;
    virtual bool isOpen(KURL)=0;
    virtual uint documents ()=0;
    virtual int findDocument (KURL)=0;
    virtual KURL activeDocumentURL()=0;

};
#endif
