/***************************************************************************
                          katedocmanager.cpp  -  description
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

#include "katedocmanager.h"
#include "katedocmanager.moc"

#include "katedocument.h"

#include <qfileinfo.h>

uint KateDocManager::myDocID = 0;

KateDocManager::KateDocManager () : KateDocManagerIface ()
{
  docList.setAutoDelete(true);
}

KateDocManager::~KateDocManager ()
{
}

KateDocument *KateDocManager::createDoc (QFileInfo* fi)
{
  KateDocument *doc = new KateDocument (myDocID, fi, false, false, false, 0L, 0L, 0L, (QString("KateDocument%1").arg(myDocID)).latin1());
  docList.append(doc);
  myDocID++;

  emit documentCreated (doc);
  return doc;
}

void KateDocManager::deleteDoc (KateDocument *doc)
{
  uint id = doc->docID();

  if (docList.find(doc) > -1)
    docList.remove (doc);

 emit documentDeleted (id);
}

KateDocument *KateDocManager::nthDoc (uint n)
{
  return docList.at(n);
}

KateDocument *KateDocManager::currentDoc ()
{
  return docList.current();
}

KateDocument *KateDocManager::firstDoc ()
{
  return docList.first();
}

KateDocument *KateDocManager::nextDoc ()
{
  return docList.next();
}

KateDocument *KateDocManager::docWithID (uint id)
{
  QListIterator<KateDocument> it(docList);

  for (; it.current(); ++it)
  {
    if ( it.current()->docID()  == id )
      return it.current();
  }

  return 0L;
}

int KateDocManager::findDoc (KateDocument *doc)
{
  return docList.find (doc);
}

uint KateDocManager::docCount ()
{
  return docList.count ();
}

int KateDocManager::findDoc( KURL url )
{
  QListIterator<KateDocument> it(docList);
  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current()->docID();
  }
  return -1;
}

bool KateDocManager::isOpen(KURL url)
{
  QListIterator<KateDocument> it(docList);
  for (; it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return true;
  }
  return false;
}

void KateDocManager::checkAllModOnHD(bool forceReload)
{
  QListIterator<KateDocument> it(docList);
  for (; it.current(); ++it) {
    it.current()->isModOnHD(forceReload);
  }
}

