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

#include "katedocmanageriface.h"

#include "katedocmanager.h"

KateDocManagerDCOPIface::KateDocManagerDCOPIface (KateDocManager *dm) : DCOPObject ("KateDocumentManager"), m_dm (dm)
{

}

/*DCOPRef KateDocManagerDCOPIface::document (uint n)
{
  return DCOPRef (m_dm->document(n));
}*/

bool KateDocManagerDCOPIface::closeDocument(uint n)
{
  return m_dm->closeDocument(n);
}

bool KateDocManagerDCOPIface::closeDocumentWithID(uint n)
{
  return m_dm->closeDocumentWithID (n);
}

bool KateDocManagerDCOPIface::closeAllDocuments()
{
  return m_dm->closeAllDocuments();
}

bool KateDocManagerDCOPIface::isOpen(KURL u)
{
  return m_dm->isOpen (u);
}

uint KateDocManagerDCOPIface::documents ()
{
  return m_dm->documents();
}

int KateDocManagerDCOPIface::findDocument (KURL u)
{
  return m_dm->findDocument (u);
}


