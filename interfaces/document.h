/***************************************************************************
                          document.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
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
