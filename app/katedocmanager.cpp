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

KateDocManager::KateDocManager () : Kate::DocManager ()
{
  docList.setAutoDelete(true);
  myCurrentDoc = 0L;

  createDoc ();
  myfirstDoc = true;
}

KateDocManager::~KateDocManager ()
{
}

Kate::Document *KateDocManager::createDoc ()
{
  KTextEditor::Document *doc = KTextEditor::createDocument ("katepart");
  docList.append((Kate::Document *)doc);

  emit documentCreated ((Kate::Document *)doc);
  return (Kate::Document *)doc;
}

void KateDocManager::deleteDoc (Kate::Document *doc)
{
  uint id = doc->documentNumber();

  if (docList.find(doc) > -1)
    docList.remove (doc);

 emit documentDeleted (id);
}

Kate::Document *KateDocManager::nthDoc (uint n)
{
  return docList.at(n);
}

Kate::Document *KateDocManager::currentDoc ()
{
  return myCurrentDoc;
}

void KateDocManager::setCurrentDoc (Kate::Document *doc)
{
  myCurrentDoc = doc;
}

Kate::Document *KateDocManager::firstDoc ()
{
  return docList.first();
}

Kate::Document *KateDocManager::nextDoc ()
{
  return docList.next();
}

Kate::Document *KateDocManager::docWithID (uint id)
{
  QPtrListIterator<Kate::Document> it(docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->documentNumber()  == id )
      return it.current();
  }

  return 0L;
}

int KateDocManager::findDoc (Kate::Document *doc)
{
  return docList.find (doc);
}

uint KateDocManager::docCount ()
{
  return docList.count ();
}

int KateDocManager::findDoc( KURL url )
{
  QPtrListIterator<Kate::Document> it(docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current()->documentNumber();
  }
  return -1;
}

Kate::Document *KateDocManager::findDocByUrl( KURL url )
{
  QPtrListIterator<Kate::Document> it(docList);
  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current();
  }
  return 0L;
}

bool KateDocManager::isOpen(KURL url)
{
  QPtrListIterator<Kate::Document> it(docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return true;
  }
  return false;
}

void KateDocManager::checkAllModOnHD(bool forceReload)
{
  QPtrListIterator<Kate::Document> it(docList);

  for (; it.current(); ++it) {
    it.current()->isModOnHD(forceReload);
  }
}

