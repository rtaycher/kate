/***************************************************************************
                          katecmd.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
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

#include "katecmd.h"
#include "katecmd.moc"

#include "katedocument.h"
#include <qregexp.h>
#include <qdatetime.h>

KateCmd::KateCmd (KateDocument *doc) : QObject (doc)
{
  myDoc = doc;

  myParser.append (new KateInsertTimeParser (myDoc));
}

KateCmd::~KateCmd ()
{
}

void KateCmd::execCmd (QString cmd, int line, int col, KateView *view)
{
  for (int i=0; i<myParser.count(); i++)
  {
    if (myParser.at(i)->execCmd (cmd, line, col, view))
      break;
  }
}

KateCmdParser::KateCmdParser (KateDocument *doc)
{
  myDoc = doc;
}

KateCmdParser::~KateCmdParser ()
{
}

bool KateInsertTimeParser::execCmd (QString cmd, int line, int col, KateView *view)
{
  if (cmd.left( 5 ) == "time:")
  {
     view->insertText (QTime::currentTime().toString());
     return true;
  }

  return false;
}
