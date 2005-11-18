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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katedocmanager.h"
#include "katedocmanager.moc"
#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "katedocmanageriface.h"
#include "kateexternaltools.h"
#include "kateviewspacecontainer.h"

#include <ktexteditor/view.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/editorchooser.h>

#include <kparts/factory.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <klibloader.h>
#include <kcodecs.h>
#include <kmessagebox.h>
#include <kencodingfiledialog.h>

#include <qdatetime.h>
#include <qtextcodec.h>
#include <q3progressdialog.h>
#include <QByteArray>
#include <QHash>

KateDocManager::KateDocManager (QObject *parent)
 : QObject (parent)
 , m_saveMetaInfos(true)
 , m_daysMetaInfos(0)
{
  // Constructed the beloved editor ;)
  m_editor = KTextEditor::EditorChooser::editor();

  // read in editor config
  m_editor->readConfig(KGlobal::config());

  m_documentManager = new Kate::DocumentManager (this);

  m_dcop = new KateDocManagerDCOPIface (this);

  m_metaInfos = new KConfig("metainfos", false, false, "appdata");

  createDoc ();
}

KateDocManager::~KateDocManager ()
{
  // write editor config
  m_editor->writeConfig(KGlobal::config());

  // write metainfos?
  if (m_saveMetaInfos)
  {
    // saving meta-infos when file is saved is not enough, we need to do it once more at the end
    foreach (KTextEditor::Document *doc,m_docList)
      saveMetaInfos(doc);

    // purge saved filesessions
    if (m_daysMetaInfos > 0)
    {
      QStringList groups = m_metaInfos->groupList();
      QDateTime *def = new QDateTime(QDate(1970, 1, 1));
      for (QStringList::Iterator it = groups.begin(); it != groups.end(); ++it)
      {
        m_metaInfos->setGroup(*it);
        QDateTime last = m_metaInfos->readDateTimeEntry("Time", def);
        if (last.daysTo(QDateTime::currentDateTime()) > m_daysMetaInfos)
          m_metaInfos->deleteGroup(*it);
      }
      delete def;
    }
  }

  delete m_dcop;
  delete m_metaInfos;
}

KateDocManager *KateDocManager::self ()
{
  return KateApp::self()->documentManager ();
}

KTextEditor::Document *KateDocManager::createDoc ()
{
  KTextEditor::Document *doc = (KTextEditor::Document *) m_editor->createDocument(this);

  if (qobject_cast<KTextEditor::ModificationInterface *>(doc))
    qobject_cast<KTextEditor::ModificationInterface *>(doc)->setModifiedOnDiskWarning (true);

  m_docList.append((KTextEditor::Document *)doc);
  m_docInfos.insert (doc, new KateDocumentInfo ());

  emit documentCreated ((KTextEditor::Document *)doc);
  emit m_documentManager->documentCreated ((KTextEditor::Document *)doc);

  connect(doc,SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
   this,SLOT(slotModifiedOnDisc(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)));
  return (KTextEditor::Document *)doc;
}

void KateDocManager::deleteDoc (KTextEditor::Document *doc)
{
  KTextEditor::Document *activeId = m_currentDoc;

  delete m_docInfos.take (doc);
  delete m_docList.takeAt (m_docList.indexOf(doc));

  emit documentDeleted (doc);
  emit m_documentManager->documentDeleted (doc);

  // ohh, current doc was deleted
  if (activeId == doc)
  {
    // special case of documentChanged, no longer any doc here !
    m_currentDoc = 0;

    emit documentChanged ();
    emit m_documentManager->documentChanged ();
  }
}

KTextEditor::Document *KateDocManager::document (uint n)
{
  return m_docList.at(n);
}

KTextEditor::Document *KateDocManager::activeDocument ()
{
  return m_currentDoc;
}

void KateDocManager::setActiveDocument (KTextEditor::Document *doc)
{
  if (!doc || (m_currentDoc == doc))
    return;

  m_currentDoc = doc;

  emit documentChanged ();
  emit m_documentManager->documentChanged ();
}

const KateDocumentInfo *KateDocManager::documentInfo (KTextEditor::Document *doc)
{
  return m_docInfos[doc];
}

int KateDocManager::findDocument (KTextEditor::Document *doc)
{
  return m_docList.indexOf (doc);
}

uint KateDocManager::documents ()
{
  return m_docList.count ();
}

KTextEditor::Document *KateDocManager::findDocument (KURL url )
{
  foreach (KTextEditor::Document* it,m_docList)
  {
    if ( it->url() == url)
      return it;
  }

  return 0;
}

bool KateDocManager::isOpen(KURL url)
{
  // return just if we found some document with this url
  return findDocument (url) != 0;
}

KTextEditor::Document *KateDocManager::openURL (const KURL& url,const QString &encoding)
{
  // special handling if still only the first initial doc is there
  if (!documentList().isEmpty() && (documentList().count() == 1) && (!documentList().at(0)->isModified() && documentList().at(0)->url().isEmpty()))
  {
    KTextEditor::Document* doc = documentList().first();

    doc->setEncoding(encoding);

    if (!loadMetaInfos(doc, url))
      doc->openURL (url);

    connect(doc, SIGNAL(modifiedChanged(KTextEditor::Document *)), this, SLOT(slotModChanged(KTextEditor::Document *)));

    emit initialDocumentReplaced();

    return doc;
 }

  KTextEditor::Document *doc = findDocument (url);
  if ( !doc )
  {
    doc = (KTextEditor::Document *)createDoc ();

    doc->setEncoding(encoding);

    if (!loadMetaInfos(doc, url))
      doc->openURL (url);
  }

  return doc;
}

bool KateDocManager::closeDocument(class KTextEditor::Document *doc,bool closeURL)
{
  if (!doc) return false;

  saveMetaInfos(doc);
  if (closeURL)
  if (!doc->closeURL()) return false;

  for (int i=0; i < KateApp::self()->mainWindows (); i++ )
    KateApp::self()->mainWindow(i)->viewManager()->closeViews(doc);

  deleteDoc (doc);

  // never ever empty the whole document list
  if (m_docList.isEmpty())
    createDoc ();

  return true;
}

bool KateDocManager::closeDocument(uint n)
{
  return closeDocument(document(n));
}

bool KateDocManager::closeAllDocuments(bool closeURL)
{
  bool res = true;

  QList<KTextEditor::Document*> docs = m_docList;

  for (int i=0; i < KateApp::self()->mainWindows (); i++ )
  {
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(true);
  }

  while (!docs.isEmpty() && res)
    if (! closeDocument(docs.at(0),closeURL) )
      res = false;
    else
      docs.removeFirst();

  for (int i=0; i < KateApp::self()->mainWindows (); i++ )
  {
    KateApp::self()->mainWindow(i)->viewManager()->setViewActivationBlocked(false);

    for (uint s=0; s < KateApp::self()->mainWindow(i)->viewManager()->containers()->count(); s++)
      KateApp::self()->mainWindow(i)->viewManager()->containers()->at(s)->activateView (m_docList.at(0));
  }

  return res;
}

QList<KTextEditor::Document*> KateDocManager::modifiedDocumentList() {
  QList<KTextEditor::Document*> modified;
  foreach (KTextEditor::Document* doc,m_docList) {
    if (doc->isModified()) {
      modified.append(doc);
    }
  }
  return modified;
}


bool KateDocManager::queryCloseDocuments(KateMainWindow *w)
{
  int docCount = m_docList.count();
  foreach (KTextEditor::Document *doc,m_docList)
  {
    if (doc->url().isEmpty() && doc->isModified())
    {
      int msgres=KMessageBox::warningYesNoCancel( w,
                  i18n("<p>The document '%1' has been modified, but not saved."
                       "<p>Do you want to save your changes or discard them?").arg( doc->documentName() ),
                    i18n("Close Document"), KStdGuiItem::save(), KStdGuiItem::discard() );

      if (msgres==KMessageBox::Cancel)
        return false;

      if (msgres==KMessageBox::Yes)
      {
        KEncodingFileDialog::Result r=KEncodingFileDialog::getSaveURLAndEncoding( doc->encoding(),QString::null,QString::null,w,i18n("Save As"));

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

  // document count changed while queryClose, abort and notify user
  if (m_docList.count() > docCount)
  {
    KMessageBox::information (w,
                          i18n ("New file opened while trying to close Kate, closing aborted."),
                          i18n ("Closing Aborted"));
    return false;
  }

  return true;
}


void KateDocManager::saveAll()
{
  foreach ( KTextEditor::Document *doc, m_docList )
    if ( doc->isModified() )
      doc->documentSave();
}

void KateDocManager::saveDocumentList (KConfig* config)
{
  QString prevGrp=config->group();
  config->setGroup ("Open Documents");
  QString grp = config->group();

  config->writeEntry ("Count", m_docList.count());

  int i=0;
  foreach ( KTextEditor::Document *doc,m_docList)
  {
    config->setGroup(QString("Document %1").arg(i));

    if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
      iface->writeSessionConfig(config);
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

  unsigned int count = config->readUnsignedNumEntry("Count", 0);

  if (count == 0)
  {
    config->setGroup(prevGrp);
    return;
  }

  Q3ProgressDialog *pd=new Q3ProgressDialog(
        i18n("Reopening files from the last session..."),
        QString::null,
        count,
        0,
        "openprog");

  pd->setCaption (KateApp::self()->makeStdCaption(i18n("Starting Up")));

  bool first = true;
  for (unsigned int i=0; i < count; i++)
  {
    config->setGroup(QString("Document %1").arg(i));
    KTextEditor::Document *doc = 0;

    if (first)
    {
      first = false;
      doc = document (0);
    }
    else
      doc = createDoc ();

    if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
      iface->readSessionConfig(config);
    config->setGroup (grp);

    pd->setProgress(pd->progress()+1);
    KateApp::self()->processEvents();
  }

  delete pd;

  config->setGroup(prevGrp);
}

void KateDocManager::slotModifiedOnDisc (KTextEditor::Document *doc, bool b, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
  if (m_docInfos[qobject_cast<KTextEditor::Document*>(doc)])
  {
    m_docInfos[qobject_cast<KTextEditor::Document*>(doc)]->modifiedOnDisc = b;
    m_docInfos[qobject_cast<KTextEditor::Document*>(doc)]->modifiedOnDiscReason = reason;
  }
}

void KateDocManager::slotModChanged(KTextEditor::Document *doc)
{
  saveMetaInfos(doc);
}

/**
 * Load file and file' meta-informations iif the MD5 didn't change since last time.
 */
bool KateDocManager::loadMetaInfos(KTextEditor::Document *doc, const KURL &url)
{
  if (!m_saveMetaInfos)
    return false;

  if (!m_metaInfos->hasGroup(url.prettyURL()))
    return false;

  QByteArray md5;
  bool ok = true;

  if (computeUrlMD5(url, md5))
  {
    m_metaInfos->setGroup(url.prettyURL());
    QString old_md5 = m_metaInfos->readEntry("MD5");

    if ((const char *)md5 == old_md5)
    {
      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
        iface->readSessionConfig(m_metaInfos);
    }
    else
    {
      m_metaInfos->deleteGroup(url.prettyURL());
      ok = false;
    }

    m_metaInfos->sync();
  }

  return ok && doc->url() == url;
}

/**
 * Save file' meta-informations iif doc is in 'unmodified' state
 */
void KateDocManager::saveMetaInfos(KTextEditor::Document *doc)
{
  QByteArray md5;

  if (!m_saveMetaInfos)
    return;

  if (doc->isModified())
  {
//     kdDebug (13020) << "DOC MODIFIED: no meta data saved" << endl;
    return;
  }

  if (computeUrlMD5(doc->url(), md5))
  {
    m_metaInfos->setGroup(doc->url().prettyURL());

    if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(doc))
      iface->writeSessionConfig(m_metaInfos);

    m_metaInfos->writeEntry("MD5", (const char *)md5);
    m_metaInfos->writeEntry("Time", QDateTime::currentDateTime());
    m_metaInfos->sync();
  }
}

bool KateDocManager::computeUrlMD5(const KURL &url, QByteArray &result)
{
  QFile f(url.path());

  if (f.open(QIODevice::ReadOnly))
  {
    KMD5 md5;

    if (!md5.update(f))
      return false;

    md5.hexDigest(result);
    f.close();
  }
  else
    return false;

  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
