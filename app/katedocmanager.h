/***************************************************************************
                          katedocmanager.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_DOCMANAGER_H__
#define __KATE_DOCMANAGER_H__

#include "katemain.h"
#include "../interfaces/docmanager.h"
#include "../interfaces/document.h"

#include <qptrlist.h>
#include <qobject.h>

class KateDocManager : public Kate::DocManager
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateMainWindow;

  public:
    KateDocManager ();
    ~KateDocManager ();

    KateDocument *createDoc ();
    void deleteDoc (KateDocument *doc);

    KateDocument *nthDoc (uint n);
    KateDocument *currentDoc ();
    KateDocument *firstDoc ();
    KateDocument *nextDoc ();

    KateDocument *docWithID (uint id);

    int findDoc (KateDocument *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    int findDoc (KURL url);
    bool isOpen(KURL url);

    uint docCount ();

  public slots:
    void checkAllModOnHD(bool forceReload=false);

  protected:
    QPtrList<KateDocument> docList;

  signals:
    void documentCreated (KateDocument *doc);
    void documentDeleted (uint documentNumber);

  public:
    Kate::Document *getNthDoc (uint n) { return (Kate::Document *)nthDoc (n); };
    Kate::Document *getCurrentDoc () { return (Kate::Document *)currentDoc (); };
    Kate::Document *getFirstDoc () { return (Kate::Document *)firstDoc(); };
    Kate::Document *getNextDoc () { return (Kate::Document *)nextDoc(); };

    Kate::Document *getDocWithID (uint id) { return (Kate::Document *)docWithID (id); };
};

#endif
