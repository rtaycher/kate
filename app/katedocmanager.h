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

#ifndef __KATE_DOCMANAGER_H__
#define __KATE_DOCMANAGER_H__

#include "katemain.h"
#include "../interfaces/documentmanager.h"

#include <kate/document.h>

#include <qptrlist.h>
#include <qobject.h>
#include <qptrdict.h>
#include <qintdict.h>

class KConfig;
class DCOPObject;

class KateDocumentInfo
{
  public:
    KateDocumentInfo ()
     : modifiedOnDisc (false),
       modifiedOnDiscReason (0)
    {
    }

    bool modifiedOnDisc;
    unsigned char modifiedOnDiscReason;
};

class KateDocManager : public QObject
{
  Q_OBJECT

  public:
    KateDocManager (QObject *parent);
    ~KateDocManager ();

    Kate::DocumentManager *documentManager () { return m_documentManager; };

    Kate::Document *createDoc ();
    void deleteDoc (Kate::Document *doc);

    Kate::Document *document (uint n);

    Kate::Document *activeDocument ();
    void setActiveDocument (Kate::Document *doc);

    Kate::Document *firstDocument ();
    Kate::Document *nextDocument ();

    Kate::Document *documentWithID (uint id);

    uint documentID(Kate::Document *);

    const KateDocumentInfo *documentInfo (Kate::Document *doc);

    int findDocument (Kate::Document *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    int findDocument (KURL url);
    // Anders: The above is not currently stable ?
    Kate::Document *findDocumentByUrl( KURL url );

    bool isOpen(KURL url);

    uint documents ();

    QPtrList<Kate::Document> &documentList () { return m_docList; };

    virtual class Kate::Document *openURL(const KURL&,const QString &encoding=QString::null,uint *id =0);
    virtual bool closeDocument(class Kate::Document *);
    virtual bool closeDocument(uint);
    virtual bool closeDocumentWithID(uint);
    virtual bool closeAllDocuments();
    bool queryCloseDocuments(KateMainWindow *w);

    void saveDocumentList (class KConfig *config);
    void restoreDocumentList (class KConfig *config);

    DCOPObject *dcopObject () { return m_dcop; };

  signals:
    void documentCreated (Kate::Document *doc);
    void documentDeleted (uint documentNumber);
    void documentChanged ();

  private slots:
    void slotModifiedOnDisc (Kate::Document *doc, bool b, unsigned char reason);

  private:
    Kate::DocumentManager *m_documentManager;
    QPtrList<Kate::Document> m_docList;
    QIntDict<Kate::Document> m_docDict;
    QPtrDict<KateDocumentInfo> m_docInfos;
    Kate::Document *m_currentDoc;

    DCOPObject *m_dcop;
};

#endif
