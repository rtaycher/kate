/***************************************************************************
                          document.h -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
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

#include <qptrlist.h>
#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/editinterface.h>
#include <ktexteditor/undointerface.h>
#include <ktexteditor/cursorinterface.h>
#include <ktexteditor/selectioninterface.h>
#include <ktexteditor/blockselectioninterface.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/highlightinginterface.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/printinterface.h>

namespace Kate
{

class View;

class Cursor : public KTextEditor::Cursor
{
  public:
    Cursor () { ; };
    virtual ~Cursor () { ; };
};

/** This interface provides access to the Kate Document class.
*/
class Document : public KTextEditor::Document, public KTextEditor::EditInterface,
                     public KTextEditor::UndoInterface, public KTextEditor::CursorInterface,
                     public KTextEditor::SelectionInterface, public KTextEditor::SearchInterface,
                     public KTextEditor::HighlightingInterface, public KTextEditor::BlockSelectionInterface,
                     public KTextEditor::ConfigInterface, public KTextEditor::MarkInterface,
                     public KTextEditor::PrintInterface
{
  Q_OBJECT

  public:
    Document ();
    virtual ~Document ();

  public:
    /** Checks if the file on disk is newer than document contents.
      If forceReload is true, the document is reloaded without asking the user,
      otherwise [default] the user is asked what to do. */
    virtual void isModOnHD(bool ) { ; };

    /** Returns the document name.
    */
    virtual QString docName () { return 0L; };

    /** Sets the document name.
    */
    virtual void setDocName (QString ) { ; };

  public slots:
    // clear buffer/filename - update the views
    virtual void flush () { ; };
};

};

#endif
