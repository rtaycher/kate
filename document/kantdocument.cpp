/***************************************************************************
                          kantdocument.cpp  -  description
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

#include "kantdocument.h"
#include "kantdocument.moc"

#include "../kwrite/kwattribute.h"
#include "../kwrite/kwdialog.h"
#include "../kwrite/highlight.h"
#include "../kwrite/kwrite_factory.h"

#include <qfileinfo.h>
#include <qdatetime.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>

KantDocument::KantDocument (long docID, QFileInfo* fi)
   : KWriteDoc (HlManager::self (), QString::null)
{
  myDocID = docID;
  fileinfo = fi;
  setMTime();
}

KantDocument::~KantDocument ()
{
}

long KantDocument::docID ()
{
  return myDocID;
}

bool KantDocument::saveFile()
{
  if (KWriteDoc::saveFile()) {
    setMTime();
    return true;
  }
  return false;
}

void KantDocument::setMTime()
{
    if (fileinfo && !fileinfo->fileName().isEmpty()) {
      fileinfo->refresh();
      mTime = fileinfo->lastModified();
    }
}

void KantDocument::isModOnHD(bool forceReload)
{
  if (fileinfo && !fileinfo->fileName().isEmpty()) {
    fileinfo->refresh();
    if (fileinfo->lastModified() > mTime) {
      if ( forceReload ||
           (KMessageBox::warningContinueCancel(0,
               (i18n("The file %1 has changed on disk.\nDo you want to reload it?\n\nIf you cancel you will lose theese changes next time you save this file")).arg(url().filename()),
               i18n("File has changed on Disk"),
               i18n("Yes") ) == KMessageBox::Continue)
          )
        reloadFile();
      else
        setMTime();
    }
  }
}

void KantDocument::reloadFile()
{
  if (fileinfo && !fileinfo->fileName().isEmpty()) {
    KWriteDoc::openFile();
    setMTime();
  }
}
