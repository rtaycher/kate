/***************************************************************************
                          katedocmanager.h  -  description
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

#ifndef kate_docmanager_h
#define kate_docmanager_h

#include "../main/katemain.h"
#include "../interfaces/katedocmanagerIface.h"

#include <qlist.h>
#include <qobject.h>

class KateDocManager : public KateDocManagerIface
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateMainWindow;

  public:
    KateDocManager ();
    ~KateDocManager ();

    KateDocument *createDoc (QFileInfo* fi=0L);
    void deleteDoc (KateDocument *doc);

    KateDocument *nthDoc (uint n);
    KateDocument *currentDoc ();
    KateDocument *firstDoc ();
    KateDocument *nextDoc ();

    KateDocument *docWithID (uint id);

    int findDoc (KateDocument *doc);
    /** Returns the docID of the doc with url URL or -1 if no such doc is found */
    int findDoc (KURL url);
    bool isOpen(KURL url);

    uint docCount ();

  public slots:
    void checkAllModOnHD(bool forceReload=false);

  protected:
    QList<KateDocument> docList;

  private:
    static uint myDocID;

  signals:
    void documentCreated (KateDocument *doc);
    void documentDeleted (uint docID);

  public:
    KateDocumentIface *getNthDoc (uint n) { return (KateDocumentIface *)nthDoc (n); };
    KateDocumentIface *getCurrentDoc () { return (KateDocumentIface *)currentDoc (); };
    KateDocumentIface *getFirstDoc () { return (KateDocumentIface *)firstDoc(); };
    KateDocumentIface *getNextDoc () { return (KateDocumentIface *)nextDoc(); };

    KateDocumentIface *getDocWithID (uint id) { return (KateDocumentIface *)docWithID (id); };
};

#endif
