/***************************************************************************
                          kantdocmanager.h  -  description
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

#ifndef kant_docmanager_h
#define kant_docmanager_h

#include "../main/kantmain.h"
#include "../interfaces/kantdocmanagerIface.h"

#include <qlist.h>
#include <qobject.h>

class KantDocManager : public KantDocManagerIface
{
  Q_OBJECT

  friend class KantMainWindow;

  public:
    KantDocManager ();
    ~KantDocManager ();

    KantDocument *createDoc (QFileInfo* fi=0L);
    void deleteDoc (KantDocument *doc);

    KantDocument *nthDoc (long n);
    KantDocument *currentDoc ();
    KantDocument *firstDoc ();
    KantDocument *nextDoc ();

    KantDocument *docWithID (long id);

    long findDoc (KantDocument *doc);
    /** Returns the docID of the doc with url URL or -1 if no such doc is found */
    long findDoc (KURL url);
    bool isOpen(KURL url);

    long docCount ();

  public slots:
    void checkAllModOnHD(bool forceReload=false);

  protected:
    QList<KantDocument> docList;

  private:
    long myDocID;

  signals:
    void documentCreated (KantDocument *doc);
    void documentDeleted (long docID);
};

#endif
