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
#include "katedocmanageriface.h"

#include <kate/view.h>

#include <kparts/factory.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kapplication.h>
#include <klibloader.h>

#include <qtextcodec.h>
#include <qprogressdialog.h>
#include <kmessagebox.h>
#include <kencodingfiledialog.h>
#include <ktexteditor/encodinginterface.h>

KateDocManager::KateDocManager (QObject *parent) : QObject (parent)
{
  m_factory = (KParts::Factory *) KLibLoader::self()->factory ("libkatepart");

  m_documentManager = new Kate::DocumentManager (this);
  m_docList.setAutoDelete(true);
  m_docDict.setAutoDelete(false);
  m_docInfos.setAutoDelete(true);

  m_dcop = new KateDocManagerDCOPIface (this);

  createDoc ();
}

KateDocManager::~KateDocManager ()
{
  delete m_dcop;
}

Kate::Document *KateDocManager::createDoc ()
{
  KTextEditor::Document *doc = (KTextEditor::Document *) m_factory->createPart (0, "", this, "", "KTextEditor::Document");
  
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
  m_currentDoc = doc;
  
  emit documentChanged ();
  emit m_documentManager->documentChanged ();
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
  for (QPtrListIterator<Kate::Document> it(m_docList); it.current(); ++it)
  {
    if ( it.current()->url() == url)
      return it.current();
  }
  
  return 0L;
}

bool KateDocManager::isOpen(KURL url)
{
  // return just if we found some document with this url
  return findDocumentByUrl (url) != 0;
}

Kate::Document *KateDocManager::openURL (const KURL& url,const QString &encoding, uint *id)
{
  // special handling if still only the first initial doc is there
  if (!documentList().isEmpty() && (documentList().count() == 1) && (!documentList().at(0)->isModified() && documentList().at(0)->url().isEmpty()))
  {
    Kate::Document* doc = documentList().getFirst();

    doc->setEncoding(encoding.isNull() ? Kate::Document::defaultEncoding() : encoding);

    doc->openURL (url);

    if (id)
      *id=doc->documentNumber();

    return doc;
 }

  Kate::Document *doc = findDocumentByUrl (url);
  if ( !doc )
  {
    doc = (Kate::Document *)createDoc ();

    doc->setEncoding(encoding.isNull() ? Kate::Document::defaultEncoding() : encoding);

    doc->openURL(url);
  }
  
  if (id)
    *id=doc->documentNumber();

  return doc;
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

bool KateDocManager::queryCloseDocuments(KateMainWindow *w)
{  
  Kate::Document  *doc;
  for (QPtrListIterator<Kate::Document> it(m_docList); (doc=it.current())!=0; ++it)
  {
    if (doc->url().isEmpty() && doc->isModified())
    {
      int msgres=KMessageBox::warningYesNoCancel( w,
                  i18n("<p>The document '%1' has been modified, but not saved."
                       "<p>Do you want to keep it?").arg( doc->docName() ),
                    i18n("Unsaved Document") );
  
      if (msgres==KMessageBox::Cancel)
        return false;
      
      if (msgres==KMessageBox::Yes)
      {
        KEncodingFileDialog::Result r=KEncodingFileDialog::getSaveURLAndEncoding(
              KTextEditor::encodingInterface(doc)->encoding(),QString::null,QString::null,w,i18n("Save As"));

        doc->setEncoding( r.encoding );
        
        if (!r.URLs.isEmpty())
        {
          KURL tmp = r.URLs.first();
          
          if ( !doc->saveAs( tmp ) )
            return false;
        }
        else
          return false;
      }
    }
    else
    {
      if (!doc->queryClose())
        return false;
    }
  }
  
  return true;
}


void KateDocManager::saveDocumentList (KConfig* config)
{
  QString prevGrp=config->group();
  config->setGroup ("Open Documents");
  QString grp = config->group();

  config->writeEntry ("Count", m_docList.count());

  int i=0;
  for ( Kate::Document *doc = m_docList.first(); doc; doc = m_docList.next() )
  {
    config->setGroup(QString("Document %1").arg(i));
    doc->writeSessionConfig(config);
    config->setGroup(grp);

    i++;
  }
  
  config->setGroup(prevGrp);
}

void KateDocManager::restoreDocumentList (KConfig* config)
{
  QString prevGrp=config->group();
  config->setGroup ("Open Documents");
  QString grp = config->group();

  int count = config->readNumEntry("Count");

  QProgressDialog *pd=new QProgressDialog(
        i18n("Reopening files from the last session..."),
        QString::null,
        count,
        0,
        "openprog");

  bool first = true;
  for (int i=0; i < count; i++)
  {
    config->setGroup(QString("Document %1").arg(i));
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

    pd->setProgress(pd->progress()+1);
    kapp->processEvents();
  }

  delete pd;

  config->setGroup(prevGrp);
}

void KateDocManager::slotModifiedOnDisc (Kate::Document *doc, bool b, unsigned char reason)
{
  if (m_docInfos[doc])
  {
    m_docInfos[doc]->modifiedOnDisc = b;
    m_docInfos[doc]->modifiedOnDiscReason = reason;
  }
}
