/***************************************************************************
                          kantdocument.h  -  description
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

#ifndef kant_document_h
#define kant_document_h

#include "../kantmain.h"

#include "../kwrite/ktextprint.h"
#include "../kwrite/kwattribute.h"
#include "../kwrite/kwdoc.h"
#include "../kwrite/kwdialog.h"
#include "../kwrite/highlight.h"
#include "../kwrite/kwrite_factory.h"

class QFileInfo;
class QDateTime;
class KantDocument : public KWriteDoc
{
  Q_OBJECT

  public:
    KantDocument(long docID, QFileInfo* fi);
    ~KantDocument();

    long docID ();
    /** anders: reimplemented from kwdoc to update mTime */
    virtual bool saveFile();
    /** Tjecks if the file on disk is newer than document contents.
      If forceReload is true, the document is reloaded without asking the user,
      otherwise [default] the user is asked what to do. */
    void isModOnHD(bool forceReload=false);

  public slots:
    /** Reloads the current document from disk if possible */
    void reloadFile();

  private:
    /** updates mTime to reflect file on fs.
     called from constructor and from saveFile. */
    void setMTime();
    long myDocID;
    QFileInfo* fileinfo;
    QDateTime mTime;
};

#endif
