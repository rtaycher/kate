/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

// $Id$

#include "katedocmanager.h"
#include "katedocmanager.moc"
#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kate/view.h>

#include <qtextcodec.h>

KateDocManager::KateDocManager (QObject *parent) : QObject (parent)
{
  m_documentManager = new Kate::DocumentManager (this);
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
  emit m_documentManager->documentCreated ((Kate::Document *)doc);
  
  return (Kate::Document *)doc;
}

void KateDocManager::deleteDoc (Kate::Document *doc)
{
  uint id = doc->documentNumber();

  if (m_docList.find(doc) > -1)
    m_docList.remove (doc);

 emit documentDeleted (id);
 emit m_documentManager->documentDeleted (id);
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
  {
    emit documentChanged ();
    emit m_documentManager->documentChanged ();
  }
  
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


uint KateDocManager::documentID(Kate::Document *doc)
{
	return doc->documentNumber();
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



Kate::Document *KateDocManager::openURL(const KURL& url,const QString &encoding,uint *id )
{
  // special handling if still only the first initial doc is there
  if (isFirstDocument() && !documentList().isEmpty())
  {
      Kate::Document* doc = documentList().getFirst();
      doc->setEncoding(encoding.isNull()?
			QString::fromLatin1(QTextCodec::codecForLocale()->name()):
			encoding);


      doc->openURL (url);

      if (!doc->url().filename().isEmpty())
        doc->setDocName (doc->url().filename());
   
      setIsFirstDocument (false);
    
      if (id) *id=documentID(doc);
      return doc;
 }

  if ( !isOpen( url ) )
  {
    setIsFirstDocument (false);
  
    Kate::Document *doc = (Kate::Document *)createDoc ();


        doc->setEncoding(encoding==QString::null?
                        QString::fromLatin1(QTextCodec::codecForLocale()->name()):
                        encoding);

	doc->openURL(url);

      if (doc->url().filename() != "")
      {
	      QString name=doc->url().filename();
	      int hassamename = 0;

                 QPtrListIterator<Kate::Document> it(m_docList);

                 for (; it.current(); ++it)
                 {
		        if ( it.current()->url().filename().compare( name ) == 0 )
		        hassamename++;
		 }
 
              if (hassamename > 1)
                  name = QString(name+"<%1>").arg(hassamename);

              doc->setDocName (name);
      }
      if (id) *id=documentID(doc);
      return doc;
  }
  else
  {
	Kate::Document *doc1=findDocumentByUrl(url);
	if (id) *id=documentID(doc1);
	return doc1;
  }
}

bool KateDocManager::closeDocument(class Kate::Document *doc)
{
  if (!doc) return false;

  if (!doc->closeURL()) return false;

  QPtrList<Kate::View> closeList;
  uint documentNumber = doc->documentNumber();


  for (uint i=0; i < ((KateApp *)kapp)->mainWindows (); i++ )
  {
     ((KateApp *)kapp)->kateMainWindow(i)->kateViewManager()->closeViews(documentNumber);
  }

  deleteDoc (doc);

  return true;
}

bool KateDocManager::closeDocument(uint n)
{
	return closeDocument(document(n));
}

bool KateDocManager::closeDocumentWithID(uint id)
{
	return closeDocument(documentWithID(id));
}

bool KateDocManager::closeAllDocuments()
{
	bool res=true;
//        if (isFirstDocument()) return true;
	while (!m_docList.isEmpty() && res)
		if (! closeDocument(m_docList.at(0)) ) res=false;
	return res;
}

void KateDocManager::saveDocumentList(KConfig* cfg)
{
  QString grp=cfg->group();
  int i=0;
  
  for ( Kate::Document *doc = m_docList.first(); doc; doc = m_docList.next() )
  {
    cfg->writeEntry( QString("File%1").arg(i), doc->url().prettyURL() );
    cfg->setGroup(doc->url().prettyURL() );
    doc->writeSessionConfig(cfg);
    cfg->setGroup(grp);
    
    i++;
  }
}
