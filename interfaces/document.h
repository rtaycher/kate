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

class Mark
{
  public:
    uint line;
    uint type;
};

class Document : public KTextEditor::Document
{
  Q_OBJECT

  public:
    Document ();
    virtual ~Document ();

  public:
    // read/save config of the document
    virtual void readConfig () { ; };
    virtual void writeConfig () { ; };

    // read/save sessionconfig of the document
    virtual void readSessionConfig (KConfig *) { ; };
    virtual void writeSessionConfig (KConfig *) { ; };

    // docID
    virtual uint docID () { return 0L; };

    // marks
    enum marks
    {
    Bookmark = 1,
    Breakpoint = 2
    };

    virtual QList<Mark> marks () { QList<Mark> l; return l; };

  public slots:
    // clear buffer/filename - update the views  
    virtual void flush () { ; };
};

};

#endif
