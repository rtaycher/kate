/***************************************************************************
                          view.h -  description
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

#ifndef _KATE_VIEW_INCLUDE_
#define _KATE_VIEW_INCLUDE_

#include <ktexteditor.h>

class KConfig;

namespace Kate
{

class Document;
class Mark;


/**
  The Kate::View text editor interface.
  @author Jochen Wilhelmy, modified by rokrau (6/21/01)
*/
class View : public KTextEditor::View
{
  Q_OBJECT

  public:
    /**
     Return values for "save" related commands.
    */
    enum saveResult { SAVE_OK, SAVE_CANCEL, SAVE_RETRY, SAVE_ERROR };
    /**
     Constructor (should much rather take a reference to the document).
    */
    View ( KTextEditor::Document *doc, QWidget *parent, const char *name = 0 );
    /**
     Destructor, you need a destructor if Scott Meyers says so.
    */
    virtual ~View ();
    /**
     Set cursor position
    */
    virtual void setCursorPosition( int , int , bool = false ) { ; };
    /**
     Get cursor position
    */
    virtual void getCursorPosition( int * , int * ) { ; };
    /**
     Set editor mode
    */
    virtual bool isOverwriteMode() const  { return false; };
    /**
     Get editor mode
    */
    virtual void setOverwriteMode( bool ) { ; };
    /**
      Returns the current line number, that is the line the cursor is on.
      For the first line it returns 0. Signal newCurPos() is emitted on
      cursor position changes.
    */
    virtual int currentLine() { return 0L; };
    /** Returns the current column number. It handles tab's correctly.
      For the first column it returns 0.
    */
    virtual int currentColumn() { return 0L; };
    /**
      Returns the number of the character, that the cursor is on (cursor x)
    */
    virtual int currentCharNum() { return 0L; };
    /**
      Returns true if the document is in read only mode.
    */
    virtual bool isReadOnly() { return false; };
    /**
      Returns true if the document has been modified.
    */
    virtual bool isModified() { return false; };
    /**
      Sets the read-only flag of the document
    */
    virtual void setReadOnly(bool) { ; };
    /**
      Sets the modification status of the document
    */
    virtual void setModified(bool ) { ; };
    /**
      Bit 0 : undo possible, Bit 1 : redo possible.
      Used to enable/disable undo/redo menu items and toolbar buttons
    */
    virtual int undoState() { return 0L; };
    /**
      Returns the number of lines
    */
    virtual int numLines() { return 0L; };
    /**
      Gets the complete document content as string
    */
    virtual QString text() { return 0L; };
    /**
      Gets the text line where the cursor is on
    */
    virtual QString currentTextLine() { return 0L; };
    /**
      Gets a text line
    */
    virtual QString textLine(int ) { return 0L; };
    /**
      Gets the word where the cursor is on
    */
    virtual QString currentWord() { return 0L; };
    /**
      Gets the word at position x, y. Can be used to find
      the word under the mouse cursor
    */
    virtual QString word(int , int ) { return 0L; };
    /**
      Discard old text without warning and set new text
    */
    virtual void setText(const QString &) { ; };
    /**
      Insert text at the current cursor position.
      The parameter @param mark is unused.
    */
    virtual void insertText(const QString &, bool mark = false ) { ; };
    /**
      Queries if there is marked text
    */
    virtual bool hasMarkedText() { return false; };
    /**
      Gets the marked text as string
    */
    virtual QString markedText() { return 0L; };
    /**
      Returns true if the current document can be
      discarded. If the document is modified, the user is asked if he wants
      to save it. On "cancel" the function returns false.
    */
    virtual bool canDiscard() { return false; };

  public slots:
    /**
     Flushes the document of the text widget. The user is given
     a chance to save the current document if the current document has
     been modified.
    */
    virtual void flush () { ; };
    /**
      Saves the file under the current file name. If the current file
      name is Untitled, as it is after a call to newFile(), this routine will
      call saveAs().
    */
    virtual saveResult save() { return SAVE_CANCEL; };
    /**
      Allows the user to save the file under a new name.
    */
    virtual saveResult saveAs() { return SAVE_CANCEL; };
    /**
      Moves the marked text into the clipboard.
    */
    virtual void cut() { ; };
    /**
     Copies the marked text into the clipboard.
    */
    virtual void copy() { ; };
    /**
     Inserts text from the clipboard at the actual cursor position.
    */
    virtual void paste() { ; };
    /**
      Undoes the last operation. The number of undo steps is configurable.
    */
    virtual void undo() { ; };
    /**
     Repeats an operation which has been undone before.
    */
    virtual void redo() { ; };
    /**
      Undoes <count> operations.
      Called by slot undo().
    */
    virtual void undoMultiple(int ) { ; };
    /**
      Repeats <count> operation which have been undone before.
      Called by slot redo().
    */
    virtual void redoMultiple(int ) { ; };
    /**
      Displays the undo history dialog.
    */
    virtual void undoHistory() { ; };
    /**
      Moves the current line or the selection one position to the right.
    */
    virtual void indent() { ; };
    /**
      Moves the current line or the selection one position to the left.
    */
    virtual void unIndent() { ; };
    /**
      Optimizes the selected indentation, replacing tabs and spaces as needed.
    */
    virtual void cleanIndent() { ; };
    /**
      Selects all text.
    */
    virtual void selectAll() { ; };
    /**
      Deselects all text.
    */
    virtual void deselectAll() { ; };
    /**
      Inverts the current selection.
    */
    virtual void invertSelection() { ; };
    /**
      Comments out current line.
    */
    virtual void comment() { ; };
    /**
      Removes comment signs in the current line.
    */
    virtual void uncomment() { ; };
    /**
      Some simply key commands.
    */
    virtual void keyReturn () { ; };
    virtual void keyDelete () { ; };
    virtual void backspace () { ; };
    virtual void killLine () { ; };
    /**
      Move cursor in the view
    */
    virtual void cursorLeft () { ; };
    virtual void shiftCursorLeft () { ; };
    virtual void cursorRight () { ; };
    virtual void shiftCursorRight () { ; };
    virtual void wordLeft () { ; };
    virtual void shiftWordLeft () { ; };
    virtual void wordRight () { ; };
    virtual void shiftWordRight () { ; };
    virtual void home () { ; };
    virtual void shiftHome () { ; };
    virtual void end () { ; };
    virtual void shiftEnd () { ; };
    virtual void up () { ; };
    virtual void shiftUp () { ; };
    virtual void down () { ; };
    virtual void shiftDown () { ; };
    virtual void scrollUp () { ; };
    virtual void scrollDown () { ; };
    virtual void topOfView () { ; };
    virtual void bottomOfView () { ; };
    virtual void pageUp () { ; };
    virtual void shiftPageUp () { ; };
    virtual void pageDown () { ; };
    virtual void shiftPageDown () { ; };
    virtual void top () { ; };
    virtual void shiftTop () { ; };
    virtual void bottom () { ; };
    virtual void shiftBottom () { ; };
    /**
      Presents a search dialog to the user.
    */
    virtual void find() { ; };
    /**
      Presents a replace dialog to the user.
    */
    virtual void replace() { ; };
    /**
      Presents a "Goto Line" dialog to the user.
    */
    virtual void gotoLine() { ; };

  public:
    /**
      Install a Popup Menu. The Popup Menu will be activated on
      a right mouse button press event.
    */
    virtual void installPopup(QPopupMenu *) { ; };
    /**
      Reads config entries.
    */
    virtual void readConfig() { ; };
    /**
      Writes config entries.
    */
    virtual void writeConfig() { ; };
    /**
      Reads session config out of the KConfig object. This also includes
      the actual cursor position and the bookmarks.
    */
    virtual void readSessionConfig(KConfig *) { ; };
    /**
      Writes session config into the KConfig object.
    */
    virtual void writeSessionConfig(KConfig *) { ; };

  public slots:
    /**
      Shows the print dialog.
    */
    virtual void printDlg () { ; };
    /**
      Presents the setup dialog to the user.
    */
    virtual void configDialog () { ; };
    /**
      Gets the highlight number.
    */
    virtual int getHl() { return 0L; };
    /**
      Sets the highlight number n.
    */
    virtual void setHl(int ) { ; };
    /**
      Get the end of line mode (Unix, Macintosh or Dos).
    */
    virtual int getEol() { return 0L; };
    /**
      Set the end of line mode (Unix, Macintosh or Dos).
    */
    virtual void setEol(int) { ; };
    /**
      Set focus to the current window.
    */
    virtual void setFocus () { ; };
    /**
      Searches for the last searched text forward from cursor position.
      @param bool forward determines the search direction.
    */
    virtual void findAgain(bool ) { ; };
    /**
      Searches for the last searched text forward from cursor position.
      Searches forward from current cursor position.
    */
    virtual void findAgain () { ; };
    /**
      Searches for the last searched text forward from cursor position.
      Searches backward from current cursor position.
    */
    virtual void findPrev () { ; };
    /**
      Presents an edit command popup window, where the user can
      apply a shell command to the contents of the current window.
    */
    virtual void slotEditCommand () { ; };
    /**
      Sets icon border on or off depending on
      @param bool enable.
    */
    virtual void setIconBorder (bool) { ; };
    /**
      Toggles icon border.
    */
    virtual void toggleIconBorder () { ; };
    /**
      Goto bookmark.
    */
    virtual void gotoMark (Mark *) { ; };
    /**
      Toggle current line bookmark.
    */
    virtual void toggleBookmark () { ; };
    /**
      Clear all bookmarks
    */
    virtual void clearBookmarks () { ; };

  public:
    /**
      Returns whether iconborder is visible.
    */
    virtual bool iconBorder() { return false; };
    /**
     Returns a pointer to the document of the view.
    */
    virtual Document *getDoc () { return 0L; };

  public slots:
    /**
      Returns highlight count.
    */
    virtual int getHlCount () { return 0; };
    /**
      Returns highlight name.
    */
    virtual QString getHlName (int) { return 0L; };
    /**
      Returns highlight selection.
    */
    virtual QString getHlSection (int) { return 0L; };
    /**
      Increase font size.
    */
    virtual void slotIncFontSizes () { ; };
    /**
      Decrease font size.
    */
    virtual void slotDecFontSizes () { ; };
};
};

#endif
