/***************************************************************************
                          kantdocmanager.cpp  -  description
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

#include "kantdocmanager.h"
#include "kantdocmanager.moc"

#include "kantdocument.h"

#include <qfileinfo.h>

KantDocManager::KantDocManager () : KantDocManagerIface ()
{
  docList.setAutoDelete(true);
  myDocID = 0;
}

KantDocManager::~KantDocManager ()
{
}

KantDocument *KantDocManager::createDoc (QFileInfo* fi)
{
  KantDocument *doc = new KantDocument (myDocID, fi);
  docList.append(doc);
  myDocID++;

  emit documentCreated (doc);
  return doc;
}

void KantDocManager::deleteDoc (KantDocument *doc)
{
  long id = doc->docID();

  if (docList.find(doc) > -1)
    docList.remove (doc);

 emit documentDeleted (id);
}

KantDocument *KantDocManager::nthDoc (long n)
{
  return docList.at(n);
}

KantDocument *KantDocManager::currentDoc ()
{
  return docList.current();
}

KantDocument *KantDocManager::firstDoc ()
{
  return docList.first();
}

KantDocument *KantDocManager::nextDoc ()
{
  return docList.next();
}

KantDocument *KantDocManager::docWithID (long id)
{
  QListIterator<KantDocument> it(docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->docID()  == id )
      return it.current();
  }

  return 0L;
}

long KantDocManager::findDoc (KantDocument *doc)
{
  return docList.find (doc);
}

long KantDocManager::docCount ()
{
  return docList.count ();
}

long KantDocManager::findDoc( KURL url )
{
  QListIterator<KantDocument> it(docList);
  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current()->docID();
  }
  return -1;
}

bool KantDocManager::isOpen(KURL url)
{
  QListIterator<KantDocument> it(docList);
  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return true;
  }
  return false;
}

void KantDocManager::checkAllModOnHD(bool forceReload)
{
  QListIterator<KantDocument> it(docList);
  for (; it.current(); ++it) {
    it.current()->isModOnHD(forceReload);
  }
}

