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

#include <kate/view.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qtextcodec.h>
#include <qprogressdialog.h>

KateDocManager::KateDocManager (QObject *parent) : QObject (parent)
{
  m_documentManager = new Kate::DocumentManager (this);
  m_docList.setAutoDelete(true);
  m_docDict.setAutoDelete(false);
  m_docInfos.setAutoDelete(true);
  m_currentDoc = 0L;

  m_dcop = new KateDocManagerDCOPIface (this);

  createDoc ();
}

KateDocManager::~KateDocManager ()
{
  delete m_dcop;

  m_docList.setAutoDelete(false);
}

Kate::Document *KateDocManager::createDoc ()
{
  KTextEditor::Document *doc = KTextEditor::createDocument ("libkatepart", this, "Kate::Document");
  m_docList.append((Kate::Document *)doc);
  m_docDict.insert (doc->documentNumber(), (Kate::Document *)doc);
  m_docInfos.insert (doc, new KateDocumentInfo ());

  if (m_docList.count() < 2)
    ((Kate::Document *)doc)->readConfig(kapp->config());

  emit documentCreated ((Kate::Document *)doc);
  emit m_documentManager->documentCreated ((Kate::Document *)doc);

  connect(doc,SIGNAL(modifiedOnDisc(Kate::Document *, bool, unsigned char)),this,SLOT(slotModifiedOnDisc(Kate::Document *, bool, unsigned char)));

  return (Kate::Document *)doc;
}

void KateDocManager::deleteDoc (Kate::Document *doc)
{
  uint id = doc->documentNumber();

  if (m_docList.count() < 2)
    doc->writeConfig(kapp->config());

  m_docInfos.remove (doc);
  m_docDict.remove (id);
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
  return m_docDict[id];
}

uint KateDocManager::documentID(Kate::Document *doc)
{
  return doc->documentNumber();
}

const KateDocumentInfo *KateDocManager::documentInfo (Kate::Document *doc)
{
  return m_docInfos[doc];
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

Kate::Document *KateDocManager::openURL (const KURL& url,const QString &encoding, uint *id)
{
  // special handling if still only the first initial doc is there
  if (!documentList().isEmpty() && (documentList().count() == 1) && (!documentList().at(0)->isModified() && documentList().at(0)->url().isEmpty()))
  {
    Kate::Document* doc = documentList().getFirst();

    doc->setEncoding(encoding.isNull() ? QString::fromLatin1(QTextCodec::codecForLocale()->name()) : encoding);

    doc->openURL (url);

    if (id)
      *id=documentID(doc);

    return doc;
 }

  if ( !isOpen( url ) )
  {
    Kate::Document *doc = (Kate::Document *)createDoc ();


    doc->setEncoding(encoding.isNull() ? QString::fromLatin1(QTextCodec::codecForLocale()->name()) : encoding);

    doc->openURL(url);

    if (id)
      *id=documentID(doc);

    return doc;
  }
  else
  {
    Kate::Document *doc1=findDocumentByUrl(url);

    if (id)
      *id=documentID(doc1);

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
  bool res = true;

  while (!m_docList.isEmpty() && res)
    if (! closeDocument(m_docList.at(0)) )
      res = false;

  return res;
}

void KateDocManager::saveDocumentList (KConfig* config)
{
  config->setGroup ("Open Documents");
  QString grp = config->group();

  config->writeEntry ("Count", m_docList.count());

  int i=0;
  for ( Kate::Document *doc = m_docList.first(); doc; doc = m_docList.next() )
  {
    config->writeEntry( QString("Document %1").arg(i), doc->url().prettyURL() );

    config->setGroup(QString ("Document ")+doc->url().prettyURL() );
    doc->writeSessionConfig(config);
    config->setGroup(grp);

    i++;
  }
}

void KateDocManager::restoreDocumentList (KConfig* config)
{
  config->setGroup ("Open Documents");
  QString grp = config->group();

  int count = config->readNumEntry("Count");

  QProgressDialog *pd=new QProgressDialog(
        i18n("Reopening files from the last session..."),
        QString::null,
        count,
        0,
        "openprog");

  int i = 0;
  bool first = true;
  while ((i < count) && config->hasKey(QString("Document %1").arg(i)))
  {
    QString fn = config->readEntry( QString("Document %1").arg( i ) );

    if ( !fn.isEmpty() )
    {
      config->setGroup( QString ("Document ")+fn );
      Kate::Document *doc = 0;

      if (first)
      {
        first = false;
        doc = document (0);
      }
      else
        doc = createDoc ();

      doc->readSessionConfig(config);
      config->setGroup (grp);
    }

    i++;

    pd->setProgress(pd->progress()+1);
    kapp->processEvents();
  }

  delete pd;
}

void KateDocManager::slotModifiedOnDisc (Kate::Document *doc, bool b, unsigned char reason)
{
  if (m_docInfos[doc])
  {
    m_docInfos[doc]->modifiedOnDisc = b;
    m_docInfos[doc]->modifiedOnDiscReason = reason;
  }
}
