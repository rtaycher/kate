/***************************************************************************
                          katedocmanager.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katedocmanager.h"
#include "katedocmanager.moc"

KateDocManager::KateDocManager (QObject *parent) : Kate::DocumentManager (parent)
{
  m_docList.setAutoDelete(true);
  m_currentDoc = 0L;

  createDoc ();
  m_firstDoc = true;
}

KateDocManager::~KateDocManager ()
{                                
  m_docList.setAutoDelete(false);
}

Kate::Document *KateDocManager::createDoc ()
{
  KTextEditor::Document *doc = KTextEditor::createDocument ("libkatepart", this, "Kate::Document");
  m_docList.append((Kate::Document *)doc);

  emit documentCreated ((Kate::Document *)doc);
  return (Kate::Document *)doc;
}

void KateDocManager::deleteDoc (Kate::Document *doc)
{
  uint id = doc->documentNumber();

  if (m_docList.find(doc) > -1)
    m_docList.remove (doc);

 emit documentDeleted (id);
}

Kate::Document *KateDocManager::document (uint n)
{
  return m_docList.at(n);
}

Kate::Document *KateDocManager::activeDocument ()
{
  return m_currentDoc;
}

void KateDocManager::setActiveDocument (Kate::Document *doc)
{                      
  if (m_currentDoc != doc)
    emit documentChanged ();

  m_currentDoc = doc;
}

Kate::Document *KateDocManager::firstDocument ()
{
  return m_docList.first();
}

Kate::Document *KateDocManager::nextDocument ()
{
  return m_docList.next();
}

Kate::Document *KateDocManager::documentWithID (uint id)
{
  QPtrListIterator<Kate::Document> it(m_docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->documentNumber()  == id )
      return it.current();
  }

  return 0L;
}

int KateDocManager::findDocument (Kate::Document *doc)
{
  return m_docList.find (doc);
}

uint KateDocManager::documents ()
{
  return m_docList.count ();
}

int KateDocManager::findDocument ( KURL url )
{
  QPtrListIterator<Kate::Document> it(m_docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current()->documentNumber();
  }
  return -1;
}

Kate::Document *KateDocManager::findDocumentByUrl( KURL url )
{
  QPtrListIterator<Kate::Document> it(m_docList);
  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current();
  }
  return 0L;
}

bool KateDocManager::isOpen(KURL url)
{
  QPtrListIterator<Kate::Document> it(m_docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return true;
  }
  return false;
}

void KateDocManager::checkAllModOnHD(bool forceReload)
{
  QPtrListIterator<Kate::Document> it(m_docList);

  for (; it.current(); ++it) {
    it.current()->isModOnHD(forceReload);
  }
}

