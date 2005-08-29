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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_DOCMANAGER_H__
#define __KATE_DOCMANAGER_H__

#include "katemain.h"
#include "../interfaces/documentmanager.h"

#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/modificationinterface.h>

#include <QPointer>
#include <QList>
#include <QObject>
#include <QByteArray>
#include <QHash>

namespace KParts { class Factory; }

class KConfig;
class DCOPObject;

class KateDocumentInfo
{
  public:
    KateDocumentInfo ()
     : modifiedOnDisc (false),
       modifiedOnDiscReason (KTextEditor::ModificationInterface::OnDiskUnmodified)
    {
    }

    bool modifiedOnDisc;
    KTextEditor::ModificationInterface::ModifiedOnDiskReason modifiedOnDiscReason;
};

class KateDocManager : public QObject
{
  Q_OBJECT

  public:
    KateDocManager (QObject *parent);
    ~KateDocManager ();

    static KateDocManager *self ();

    Kate::DocumentManager *documentManager () { return m_documentManager; };
    KTextEditor::Editor *editor() {return m_editor;}

    KTextEditor::Document *createDoc ();
    void deleteDoc (KTextEditor::Document *doc);

    KTextEditor::Document *document (uint n);

    KTextEditor::Document *activeDocument ();
    void setActiveDocument (KTextEditor::Document *doc);

    // search document with right documentNumber()
    KTextEditor::Document *documentWithID (uint id);

    const KateDocumentInfo *documentInfo (KTextEditor::Document *doc);

    int findDocument (KTextEditor::Document *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    int findDocument (KURL url);
    // Anders: The above is not currently stable ?
    KTextEditor::Document *findDocumentByUrl( KURL url );

    bool isOpen(KURL url);

    uint documents ();

    QList<KTextEditor::Document*> &documentList () { return m_docList; };

    KTextEditor::Document *openURL(const KURL&,const QString &encoding=QString::null,uint *id =0);

    bool closeDocument(class KTextEditor::Document *,bool closeURL=true);
    bool closeDocument(uint);
    bool closeDocumentWithID(uint);
    bool closeAllDocuments(bool closeURL=true);

    QList<KTextEditor::Document*> modifiedDocumentList();
    bool queryCloseDocuments(KateMainWindow *w);

    void saveDocumentList (class KConfig *config);
    void restoreDocumentList (class KConfig *config);

    DCOPObject *dcopObject () { return m_dcop; };

    inline bool getSaveMetaInfos() { return m_saveMetaInfos; };
    inline void setSaveMetaInfos(bool b) { m_saveMetaInfos = b; };

    inline int getDaysMetaInfos() { return m_daysMetaInfos; };
    inline void setDaysMetaInfos(int i) { m_daysMetaInfos = i; };

  public slots:
    /**
     * saves all documents that has at least one view.
     * documents with no views are ignored :P
     */
    void saveAll();

  signals:
    void documentCreated (KTextEditor::Document *doc);
    void documentDeleted (uint documentNumber);
    void documentChanged ();
    void initialDocumentReplaced ();

  private slots:
    void slotModifiedOnDisc (KTextEditor::Document *doc, bool b, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);
    void slotModChanged(KTextEditor::Document *doc);

  private:
    bool loadMetaInfos(KTextEditor::Document *doc, const KURL &url);
    void saveMetaInfos(KTextEditor::Document *doc);
    bool computeUrlMD5(const KURL &url, QByteArray &result);

    Kate::DocumentManager *m_documentManager;
    QList<KTextEditor::Document*> m_docList;
    QHash<int,KTextEditor::Document*> m_docDict;
    QHash<KTextEditor::Document*,KateDocumentInfo*> m_docInfos;

    QPointer<KTextEditor::Document> m_currentDoc;
    KConfig *m_metaInfos;
    bool m_saveMetaInfos;
    int m_daysMetaInfos;

    DCOPObject *m_dcop;

    //KParts::Factory *m_factory;
    KTextEditor::Editor *m_editor;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
