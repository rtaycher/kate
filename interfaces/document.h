/***************************************************************************
                          document.h -  description
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

#ifndef _KATE_DOCUMENT_INCLUDE_
#define _KATE_DOCUMENT_INCLUDE_

#include <ktexteditor.h>

class KConfig;

namespace Kate
{

class Document : public KTextEditor::Document
{
  Q_OBJECT

  public:
    Document ( ) : KTextEditor::Document (0L, 0L) {;};
    virtual ~Document () {;};

  public:
    virtual void readConfig () { ; };
    virtual void writeConfig () { ; };

    virtual void readSessionConfig(KConfig *) { ; };
    virtual void writeSessionConfig(KConfig *) { ; };
};

};

#endif
