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
#include <QMap>
#include <QPair>
#include <QDateTime>

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

    const KateDocumentInfo *documentInfo (KTextEditor::Document *doc);

    int findDocument (KTextEditor::Document *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    KTextEditor::Document *findDocument (KUrl url);

    bool isOpen(KUrl url);

    uint documents ();

    QList<KTextEditor::Document*> &documentList () { return m_docList; };

    KTextEditor::Document *openURL(const KUrl&,const QString &encoding=QString(),bool isTempFile=false);

    bool closeDocument(class KTextEditor::Document *,bool closeURL=true);
    bool closeDocument(uint);
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

  public Q_SLOTS:
    /**
     * saves all documents that has at least one view.
     * documents with no views are ignored :P
     */
    void saveAll();

  Q_SIGNALS:
    void documentCreated (KTextEditor::Document *doc);
    void documentDeleted (KTextEditor::Document *doc);
    void documentChanged ();
    void initialDocumentReplaced ();

  private Q_SLOTS:
    void slotModifiedOnDisc (KTextEditor::Document *doc, bool b, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);
    void slotModChanged(KTextEditor::Document *doc);

  private:
    bool loadMetaInfos(KTextEditor::Document *doc, const KUrl &url);
    void saveMetaInfos(KTextEditor::Document *doc);
    bool computeUrlMD5(const KUrl &url, QByteArray &result);

    Kate::DocumentManager *m_documentManager;
    QList<KTextEditor::Document*> m_docList;
    QHash<KTextEditor::Document*,KateDocumentInfo*> m_docInfos;

    QPointer<KTextEditor::Document> m_currentDoc;
    KConfig *m_metaInfos;
    bool m_saveMetaInfos;
    int m_daysMetaInfos;

    DCOPObject *m_dcop;

    //KParts::Factory *m_factory;
    KTextEditor::Editor *m_editor;

    typedef QPair<KURL,QDateTime> TPair;
    QMap<KTextEditor::Document *,TPair> m_tempFiles;   
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
