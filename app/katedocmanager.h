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
#include "../interfaces/documentmanager.h"

#include <kate/document.h>

#include <qptrlist.h>
#include <qobject.h>

class KateDocManager : public Kate::DocumentManager
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

    Kate::Document *document (uint n);

    Kate::Document *activeDocument ();
    void setActiveDocument (Kate::Document *doc);

    Kate::Document *firstDocument ();
    Kate::Document *nextDocument ();

    Kate::Document *documentWithID (uint id);

    int findDocument (Kate::Document *doc);
    /** Returns the documentNumber of the doc with url URL or -1 if no such doc is found */
    int findDocument (KURL url);
    // Anders: The above is not currently stable ?
    Kate::Document *findDocumentByUrl( KURL url );
    bool isOpen(KURL url);

    uint documents ();

  public slots:
    void checkAllModOnHD(bool forceReload=false);

  private:
    QPtrList<Kate::Document> docList;
    Kate::Document *myCurrentDoc;
    bool myfirstDoc;

  signals:
    void documentCreated (Kate::Document *doc);
    void documentDeleted (uint documentNumber);
};

#endif
