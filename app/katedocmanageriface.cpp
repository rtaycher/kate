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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katedocmanageriface.h"

#include "katedocmanager.h"

#include <kdebug.h>

KateDocManagerDCOPIface::KateDocManagerDCOPIface (KateDocManager *dm) : DCOPObject ("KateDocumentManager"), m_dm (dm)
{

}

// bit more error save than the forcing c cast ;()
DCOPRef KateDocManagerDCOPIface::document (uint n)
{
  KTextEditor::Document *doc = m_dm->document(n);

  if (!doc || !doc->inherits ("DCOPObject"))
    return DCOPRef ();

  return DCOPRef ((DCOPObject *) doc);
}

DCOPRef KateDocManagerDCOPIface::activeDocument ()
{
  KTextEditor::Document *doc = m_dm->activeDocument();

  if (!doc || !doc->inherits ("DCOPObject"))
    return DCOPRef ();

  return DCOPRef ((DCOPObject *) doc);
}

uint KateDocManagerDCOPIface::activeDocumentNumber ()
{
  KTextEditor::Document *doc = m_dm->activeDocument();

  if (doc)
    return doc->documentNumber ();
  
  return 0;
}

DCOPRef KateDocManagerDCOPIface::openURL (KURL url, QString encoding)
{
  KTextEditor::Document *doc = m_dm->openURL (url, encoding);

  if (!doc || !doc->inherits ("DCOPObject"))
    return DCOPRef ();

  return DCOPRef ((DCOPObject *) doc);
}

bool KateDocManagerDCOPIface::closeDocument(uint n)
{
  return m_dm->closeDocument(n);
}

bool KateDocManagerDCOPIface::closeAllDocuments()
{
  return m_dm->closeAllDocuments();
}

bool KateDocManagerDCOPIface::isOpen(KURL url)
{
  return m_dm->isOpen (url);
}

uint KateDocManagerDCOPIface::documents ()
{
  return m_dm->documents();
}


