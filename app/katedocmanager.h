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

#include <kate/document.h>

#include <qptrlist.h>
#include <qobject.h>

class KateDocManager : public Kate::DocManager
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateMainWindow;
  friend class KateViewManager;

  public:
    KateDocManager ();
    ~KateDocManager ();

    Kate::Document *createDoc ();
    void deleteDoc (Kate::Document *doc);

    Kate::Document *nthDoc (uint n);

    Kate::Document *currentDoc ();
    void setCurrentDoc (Kate::Document *doc);

    Kate::Document *firstDoc ();
    Kate::Document *nextDoc ();

    Kate::Document *docWithID (uint id);

    int findDoc (Kate::Document *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    int findDoc (KURL url);
    // Anders: The above is not currently stable ??!
    Kate::Document *findDocByUrl( KURL url );
    bool isOpen(KURL url);

    uint docCount ();

  public slots:
    void checkAllModOnHD(bool forceReload=false);

  private:
    QPtrList<Kate::Document> docList;
    Kate::Document *myCurrentDoc;
    class KLibrary *partLib;
    bool myfirstDoc;

  signals:
    void documentCreated (Kate::Document *doc);
    void documentDeleted (uint documentNumber);

  public:
    Kate::Document *getNthDoc (uint n) { return (Kate::Document *)nthDoc (n); };
    Kate::Document *getCurrentDoc () { return (Kate::Document *)currentDoc (); };
    Kate::Document *getFirstDoc () { return (Kate::Document *)firstDoc(); };
    Kate::Document *getNextDoc () { return (Kate::Document *)nextDoc(); };

    Kate::Document *getDocWithID (uint id) { return (Kate::Document *)docWithID (id); };
};

#endif
