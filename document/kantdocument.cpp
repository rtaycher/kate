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

#include <qfileinfo.h>
#include <qdatetime.h>

#include <kmessagebox.h>
#include <klocale.h>

#include "kantdocument.h"
#include "kantdocument.moc"

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

void KantDocument::print (KTextPrint &printer)
{
  int z, numAttribs;
  Attribute *a;
  int line, attr, nextAttr, oldZ;
  TextLine *textLine;
  const QChar *s;

//  printer.setTitle(kWriteDoc->fileName());
  printer.setTabWidth(this->tabWidth());

  numAttribs = this->numAttribs();
  a = this->attribs();
  for (z = 0; z < numAttribs; z++) {
    printer.defineColor(z, a[z].col.red(), a[z].col.green(), a[z].col.blue());
  }

  printer.begin();

  line = 0;
  attr = -1;
  while (true) {
    textLine = this->getTextLine(line);
    s = textLine->getText();
//    printer.print(s, textLine->length());
    oldZ = 0;
    for (z = 0; z < textLine->length(); z++) {
      nextAttr = textLine->getAttr(z);
      if (nextAttr != attr) {
        attr = nextAttr;
        printer.print(&s[oldZ], z - oldZ);
        printer.setColor(attr);
        int fontStyle = 0;
        if (a[attr].font.bold()) fontStyle |= KTextPrint::Bold;
        if (a[attr].font.italic()) fontStyle |= KTextPrint::Italics;
        printer.setFontStyle(fontStyle);
        oldZ = z;
      }
    }
    printer.print(&s[oldZ], z - oldZ);

    line++;
    if (line == this->numLines()) break;
    printer.newLine();
  }

  printer.end();
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
