/*
  $Id$

   Copyright (C) 1998, 1999 Jochen Wilhelmy
                            digisnap@cs.tu-berlin.de

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
*/

#ifndef __KWVIEV_H__
#define __KWVIEV_H__

#define i18nop // a no-operation i18n()

#include <qstring.h>
#include <qdialog.h>
#include <qkeycode.h>
#include <qintdict.h>
#include <qiodevice.h>
#include <qdropsite.h>
#include <qscrollbar.h>
#include <qpopupmenu.h>
#include <qdragobject.h>

#include <kurl.h>
#include <kconfig.h>
#include <kparts/part.h>


class KWriteDoc;
class KWCommandDispatcher;
class KWKeyData;
class KTextPrint;
class KSpell;
class KSpellConfig;

//search flags
const int sfCaseSensitive     = 1;
const int sfWholeWords        = 2;
const int sfFromCursor        = 4;
const int sfBackward          = 8;
const int sfSelected          = 16;
const int sfPrompt            = 32;
const int sfReplace           = 64;
const int sfAgain             = 128;
const int sfWrapped           = 256;
const int sfFinished          = 512;

//dialog results
const int srYes               = QDialog::Accepted;
const int srNo                = 10;
const int srAll               = 11;
const int srCancel            = QDialog::Rejected;

// config flags
enum KWConfigFlags {
  // indent
  cfAutoIndent        = 0x1,
  cfSpaceIndent       = 0x2,
  cfBackspaceIndents  = 0x4,
  cfTabIndents        = 0x8,
  cfKeepIndentProfile = 0x10,
  cfKeepExtraSpaces   = 0x20,

  // select
  cfPersistent        = 0x100,
  cfDelOnInput        = 0x200,
  cfMouseAutoCopy     = 0x400,
  cfSingleSelection   = 0x800,
  cfVerticalSelect    = 0x1000,
  cfXorSelect         = 0x2000,

  // edit
  cfWordWrap          = 0x10000,
  cfReplaceTabs       = 0x20000,
  cfRemoveSpaces      = 0x40000,
  cfAutoBrackets      = 0x80000,
  cfGroupUndo         = 0x100000,
  cfShowTabs          = 0x200000,
  cfSmartHome         = 0x400000,
  cfPageUDMovesCursor = 0x800000,
  cfWrapCursor        = 0x1000000,

  // other
  cfKeepSelection     = 0x10000000,
  cfOvr               = 0x20000000,
  cfMark              = 0x40000000
};

//update flags
const int ufDocGeometry       = 1;
const int ufUpdateOnScroll    = 2;
const int ufPos               = 4;

//load flags
const int lfInsert            = 1;
const int lfNewFile           = 2;
const int lfNoAutoHl          = 4;

//end of line settings
const int eolUnix             = 0;
const int eolMacintosh        = 1;
const int eolDos              = 2;

//command categories
const int ctCursorCommands    = 0;
const int ctEditCommands      = 1;
const int ctFindCommands      = 2;
const int ctBookmarkCommands  = 3;
const int ctStateCommands     = 4;

// cursor movement commands
enum KWCursorCommands {
  kSelectFlag         = 0x100000,
  kMultiSelectFlag    = 0x200000,
  cmLeft              = 1,
  cmRight             = 2,
  cmWordLeft          = 3,
  cmWordRight         = 4,
  cmHome              = 5,
  cmEnd               = 6,
  cmUp                = 7,
  cmDown              = 8,
  cmScrollUp          = 9,
  cmScrollDown        = 10,
  cmTopOfView         = 11,
  cmBottomOfView      = 12,
  cmPageUp            = 13,
  cmPageDown          = 14,
  cmCursorPageUp      = 15,
  cmCursorPageDown    = 16,
  cmTop               = 17,
  cmBottom            = 18,
  cmSelectLeft        = cmLeft | kSelectFlag,
  cmSelectRight       = cmRight | kSelectFlag,
  cmSelectUp          = cmUp | kSelectFlag,
  cmSelectDown        = cmDown | kSelectFlag
};  

// edit commands
enum KWEditCommands {
  cmReturn            = 1,
  cmBackspace         = 2,
  cmBackspaceWord     = 3,
  cmDelete            = 4,
  cmDeleteWord        = 5,
  cmKillLine          = 6,
  cmUndo              = 7,
  cmRedo              = 8,
  cmCut               = 9,
  cmCopy              = 10,
  cmPaste             = 11,
  cmIndent            = 12,
  cmUnindent          = 13,
  cmCleanIndent       = 14,
  cmSelectAll         = 15,
  cmDeselectAll       = 16,
  cmInvertSelection   = 17
};

// search commands
enum KWSearchCommands {
  cmFind              = 1,
  cmReplace           = 2,
  cmFindAgain         = 3,
  cmGotoLine          = 4
};

// bookmark commands
enum KWBookmarkCommands {
  cmSetBookmark       = 1,
  cmAddBookmark       = 2,
  cmClearBookmarks    = 3,
  cmSetBookmarks      = 10,
  cmGotoBookmarks     = 20
};

//state commands
const int cmToggleInsert      = 1;
const int cmToggleVertical    = 2;

class KWCursor {
  public:
    KWCursor() : m_x(0), m_y(0) {}
    KWCursor(int x, int y) : m_x(x), m_y(y) {}
    KWCursor(const KWCursor &c) : m_x(c.m_x), m_y(c.m_y) {}

    KWCursor &operator=(const KWCursor &c) {m_x = c.m_x; m_y = c.m_y; return *this;}
    KWCursor &operator+=(const KWCursor &c) {m_x += c.m_x; m_y += c.m_y; return *this;}
    KWCursor &operator-=(const KWCursor &c) {m_x -= c.m_x; m_y -= c.m_y; return *this;}
    bool operator==(const KWCursor &c) {return m_x == c.m_x && m_y == c.m_y;}
    bool operator!=(const KWCursor &c) {return m_x != c.m_x || m_y != c.m_y;}
    bool operator>(const KWCursor &c) {return m_y > c.m_y || (m_y == c.m_y && m_x > c.m_x);}

    void set(int x, int y) {m_x = x; m_y = y;}
    void setX(int x) {m_x = x;}
    void setY(int y) {m_y = y;}
    int x() const {return m_x;}
    int y() const {return m_y;}
    void incX() {m_x++;}
    void incY() {m_y++;}
    void decX() {m_x--;}
    void decY() {m_y--;}
    void move(int dx, int dy) {m_x += dx; m_y += dy;}
    void moveX(int dx) {m_x += dx;}
    void moveY(int dy) {m_y += dy;}

  protected:
    int m_x;
    int m_y;
};

class KWrite;
class KWriteView;

struct VConfig {
  KWriteView *view;
  KWCursor cursor;
  int cXPos;
  int flags;
  int wrapAt;
};

struct SConfig {
  KWCursor cursor;
  KWCursor startCursor;
  int flags;
};

struct LineRange {
  int start;
  int end;
};

struct BracketMark {
  KWCursor cursor;
  int sXPos;
  int eXPos;
};

class KWBookmark {
  public:

    KWBookmark();
    int xPos;
    int yPos;
    KWCursor cursor;
    QString Name;
};

/**
  The KWrite text editor widget. It has many options, document/view
  architecture and syntax highlight.
  @author Jochen Wilhelmy
*/
class KWrite : public QWidget {
    Q_OBJECT

    friend KWriteView;
    friend KWriteDoc;

  public:

    /**
      The document can be used by more than one KWrite objects.
      HandleOwnURIDrops should be set to false for a container that can handle URI drops
      better than KWriteView does.
    */
    KWrite(KWriteDoc *, QWidget *parent = 0L, const QString &name = QString::null, 
      bool HandleOwnURIDrops = true);

    /**
      The destructor does not delete the document
    */
    ~KWrite();


//status and config functions

    /**
      Returns the current line number, that is the line the cursor is on.
      For the first line it returns 0. Signal newCurPos() is emitted on
      cursor position changes.
    */
    int currentLine();

    /**
      Returns the current column number. It handles tab's correctly.
      For the first column it returns 0.
    */
    int currentColumn();

    /**
      Returns the number of the character, that the cursor is on (cursor x)
    */
    int currentCharNum();

    /**
      Sets the current cursor position
    */
    void setCursorPosition(int line, int col);

    /**
      Returns the config flags. See the cfXXX constants in the .h file.
    */
    int config();// {return m_configFlags;}

    /**
      Sets the config flags
    */
    void setConfig(int);

    int wordWrapAt() {return m_wrapAt;}
    void setWordWrapAt(int at) {m_wrapAt = at;}
    int tabWidth();
    void setTabWidth(int);
    int undoSteps();
    void setUndoSteps(int);

    void setColors(QColor *colors);
    void getColors(QColor *colors);

  //    bool isOverwriteMode();

    /**
      Returns true if the document is in read only mode.
    */
    bool isReadOnly();

    /**
      Returns true if the document has been modified.
    */
    bool isModified();

    /**
      Sets the read-only flag of the document
    */
    void setReadOnly(bool);

    /**
      Sets the modification status of the document
    */
    void setModified(bool m = true);

    void findHightlighting(const QString &filename);

    /**
      Returns true if this editor is the only owner of its document
    */
    bool isLastView();

    /**
      Returns the document object
    */
    KWriteDoc *doc();

    /**
      Returns the view object
    */
    KWriteView *view();

    /**
      Bit 0 : undo possible, Bit 1 : redo possible.
      Used to enable/disable undo/redo menu items and toolbar buttons
    */
    int undoState();

    /**
      Returns the type of the next undo group.
    */
    int nextUndoType();

    /**
      Returns the type of the next redo group.
    */
    int nextRedoType();

    /**
      Returns a list of all available undo types, in undo order.
    */
    void undoTypeList(QValueList<int>& lst);

    /**
      Returns a list of all available redo types, in redo order.
    */
    void redoTypeList(QValueList<int>& lst);

    /**
      Returns a short text description of the given undo type,
      which is obtained with nextUndoType(), nextRedoType(), undoTypeList(), and redoTypeList(),
      suitable for display in a menu entry.  It is not translated;
      use i18n() before displaying this string.
    */
    QString undoTypeName(int undoType);

    void copySettings(KWrite *);

  public slots:

    /**
      Presents a options dialog to the user
    */
//    void optDlg();

    /**
      Presents a color dialog to the user
    */
    void colDlg();

    /**
      Executes state command cmdNum
    */
    void doStateCommand(int cmdNum);

    /**
      Toggles Insert mode
    */
    void toggleInsert();

    /**
      Toggles "Vertical Selections" option
    */
    void toggleVertical();

  signals:

    /**
      The cursor position has changed. Get the values with currentLine()
      and currentColumn()
    */
    void newCurPos();

    /**
      Modified flag or config flags have changed
    */
    void newStatus();

    /**
      The undo/redo enable status has changed
    */
    void newUndo();

    /**
      The marked text state has changed. This can be used to enable/disable
      cut and copy
    */
    void newMarkStatus();

    /**
      The file name has changed. The main window can use this to change
      its caption
    */
    void fileChanged();

    /**
      Emits messages for the status line
    */
    void statusMsg(const QString &);

  protected:

    int m_configFlags;
    int m_wrapAt;

    /*
     * The source, the destination of the copy, and the flags
     * for each job being run(job id is the dict key).
     */
    QIntDict <QString> m_sNet;
    QIntDict <QString> m_sLocal;
    QIntDict <int> m_flags;

//text access

  public:

     /**
       Gets the number of text lines;
     */
     int numLines();

     /**
       Gets the complete document content as string
     */
     QString text();

     /**
       Gets the text line where the cursor is on
     */
     QString currentTextLine();

     /**
       Gets a text line
     */
     QString textLine(int num);

     /**
       Gets the word where the cursor is on
     */
     QString currentWord();

     /**
       Gets the word at position x, y. Can be used to find
       the word under the mouse cursor
     */
     QString word(int x, int y);

     /**
       Discard old text without warning and set new text
     */
     void setText(const QString &);

     /**
       Insert text at the current cursor position. If length is a positive
       number, it restricts the number of inserted characters
     */
     void insertText(const QString &);

     /**
       Queries if there is marked text
     */
     bool hasMarkedText();

     /**
       Gets the marked text as string
     */
     QString markedText();

//url aware file functions

  public:

//    enum fileAction{GET, PUT}; //tells us what kind of job kwrite is waiting for
    enum fileResult {OK, CANCEL, RETRY, ERROR};

    /**
      Loads a file from the given QIODevice. For insert = false the old
      contents will be lost.
    */
    void loadFile(QIODevice &, bool insert = false);

    /**
      Writes the document into the given QIODevice
    */
    void writeFile(QIODevice &);

    /**
      Loads the file given in name into the editor
    */
    bool loadFile(const QString &name, int flags = 0);

    /**
      Saves the file as given in name
    */
    bool writeFile(const QString &name);

    /**
      Loads the file given in url into the editor.
      See the lfXXX constants in the .h file.
    */
    void loadURL(const KURL &url, int flags = 0);

    /**
      Saves the file as given in url
    */
    void writeURL(const KURL &url, int flags = 0);

  protected slots:

    /**
      Gets signals from iojob
    */
    void slotGETFinished(int id);
    void slotPUTFinished(int id);
    void slotIOJobError(int, const char *);

  public:

    /**
      Returns true if the document has a filename(not counting the path).
    */
    bool hasFileName();

    /**
      Returns the URL of the currnet file
    */
    const QString fileName();

    /**
      Set the file name. This starts the automatic highlight selection.
    */
    void setFileName(const QString &);

    /**
      Mainly for internal use. Returns true if the current document can be
      discarded. If the document is modified, the user is asked if he wants
      to save it. On "cancel" the function returns false.
    */
    bool canDiscard();

  public slots:

    /**
      Opens a new untitled document in the text widget. The user is given
      a chance to save the current document if the current document has
      been modified.
    */
    void newDoc();

    /**
      This will present an open file dialog and open the file specified by
      the user, if possible. The user will be given a chance to save the
      current file if it has been modified. This starts the automatic
      highlight selection.
    */
    void open();

    /**
      Calling this method will let the user insert a file at the current
      cursor position.
    */
    void insertFile();

    /**
      Saves the file if necessary under the current file name. If the current file
      name is Untitled, as it is after a call to newFile(), this routing will
      call saveAs().
    */
    fileResult save();

    /**
      Allows the user to save the file under a new name. This starts the
      automatic highlight selection.
    */
    fileResult saveAs();

  protected:

//    KFM *kfm;
//    QString kfmURL;
//    QString kfmFile;
//    fileAction kfmAction;
//    int kfmFlags;

//command processors

  public slots:

    /**
      Does Cursor Command cmdNum
    */
    void doCursorCommand(int cmdNum);

    /**
      Does Edit Command cmdNum
    */
    void doEditCommand(int cmdNum);

//    void configureKWriteKeys();
  public:
    void getKeyData(KWKeyData &);
    void setKeyData(const KWKeyData &);
  protected:

    KWCommandDispatcher *m_dispatcher;
    bool m_persistent; // to make multiple selections always persistent
    
//edit functions

  public:

    /**
      Clears the document without any warnings or requesters.
    */
    void clear();

  public slots:

    /**
      Moves the marked text into the clipboard
    */
    void cut() {doEditCommand(cmCut);}

    /**
      Copies the marked text into the clipboard
    */
    void copy() {doEditCommand(cmCopy);}

    /**
      Inserts text from the clipboard at the actual cursor position
    */
    void paste() {doEditCommand(cmPaste);}

    /**
      Undoes the last operation. The number of undo steps is configurable
    */
    void undo() {doEditCommand(cmUndo);}

    /**
      Repeats an operation which has been undone before.
    */
    void redo() {doEditCommand(cmRedo);}

    /**
      Undoes <count> operations.
      Called by slot undo().
    */
    void undoMultiple(int count);

    /**
      Repeats <count> operation which have been undone before.
      Called by slot redo().
    */
    void redoMultiple(int count);

    /**
      Displays the undo history dialog
    */
    void undoHistory();

    /**
      Moves the current line or the selection one position to the right
    */
    void indent();

    /**
      Moves the current line or the selection one position to the left
    */
    void unIndent();

    /**
      Optimizes the selected indentation, replacing tabs and spaces as needed
    */
    void cleanIndent();

    /**
      Selects all text
    */
    void selectAll() {doEditCommand(cmSelectAll);}

    /**
      Deselects all text
    */
    void deselectAll() {doEditCommand(cmDeselectAll);}

    /**
      Inverts the current selection
    */
    void invertSelection();

//search/replace functions

  public slots:

    /**
      Presents a search dialog to the user
    */
    void find();

    /**
      Presents a replace dialog to the user
    */
    void replace();

    /**
      Repeasts the last search or replace operation. On replace, the
      user is prompted even if the "Prompt On Replace" option was off.
    */
    void findAgain();

    /**
      Presents a "Goto Line" dialog to the user
    */
    void gotoLine();

  protected:

    void initSearch(SConfig &, int flags);
    void continueSearch(SConfig &);
    void searchAgain(SConfig &);
    void replaceAgain();
    void doReplaceAction(int result, bool found = false);
    void exposeFound(KWCursor &cursor, int slen, int flags, bool replace);
    void deleteReplacePrompt();
    bool askReplaceEnd();

  protected slots:

    void replaceSlot();

  protected:

    QStringList  m_searchForList;
    QStringList  m_replaceWithList;
    int          m_searchFlags;
    int          m_replaces;
    SConfig      s;
    QDialog     *m_replacePrompt;

//right mouse button popup menu

  public:

    /**
      Install a Popup Menu. The Popup Menu will be activated on
      a right mouse button press event.
    */
    void installRBPopup(QPopupMenu *);

  protected:

    QPopupMenu *popup;

//bookmarks

  public:

    /**
      Install a Bookmark Menu. The bookmark items will be added to the
      end of the menu
    */
    //void installBMPopup(KGuiCmdPopup *);
    /**
      Sets the actual edit position as bookmark number n
    */
    void setBookmark();
    void addBookmark();
    void clearBookmarks();
    void setBookmark(int n);
    void gotoBookmark(int n);

  public slots:

    void doBookmarkCommand(int cmdNum);

    /**
      Shows a popup that lets the user choose the bookmark number
    */
//    void setBookmark();

    /**
      Adds the actual edit position to the end of the bookmark list
    */
//    void addBookmark();

    /**
      Clears all bookmarks
    */
//    void clearBookmarks();

    /**
      Sets the cursor to the bookmark n
    */
//    void gotoBookmark(int n);

  protected slots:

    /**
      Updates the bookmark popup menu when it emit aboutToShow()
    */
    void updateBMPopup();

  protected:

    QList<KWBookmark> bookmarks;
//    int bmEntries;

//config file / session management functions

  public:

    /**
      Reads config entries out of the KConfig object
    */
    void readConfig(KConfig *);

    /**
      Writes config entries into the KConfig object
    */
    void writeConfig(KConfig *);

    /**
      Reads session config out of the KConfig object. This also includes
      the actual cursor position and the bookmarks.
    */
    void readSessionConfig(KConfig *);

    /**
      Writes session config into the KConfig object
    */
    void writeSessionConfig(KConfig *);

//syntax highlight

  public slots:

    /**
      Presents the highlight defaults dialog to the user
    */
    void hlDef();

    /**
      Presents the highlight setup dialog to the user
    */
    void hlDlg();

    /**
      Gets the highlight number
    */
    int getHl();

    /**
      Sets the highlight number n
    */
    void setHl(int n);

    /**
      Get the end of line mode(Unix, Macintosh or Dos)
    */
    int getEol();

    /**
      Set the end of line mode(Unix, Macintosh or Dos)
    */
    void setEol(int);

//internal

  protected:

    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);

    KWriteDoc *m_doc;
    KWriteView *m_view;

//spell checker

  public:

    /**
     * Returns the KSpellConfig object
     */
    KSpellConfig *ksConfig();

    /**
     * Sets the KSpellConfig object.  (The object is
     *  copied internally.)
     */
    void setKSConfig(const KSpellConfig);

  public slots:    //please keep prototypes and implementations in same order

    void spellcheck();
    void spellcheck2(KSpell *);
    void misspelling(QString word, QStringList *, unsigned pos);
    void corrected(QString originalword, QString newword, unsigned pos);
    void spellResult(const char *newtext);
    void spellCleanDone();
  signals:

    /** This says spellchecking is <i>percent</i> done.
      */
    void  spellcheck_progress(unsigned int percent);

    /** Emitted when spellcheck is complete.
     */
    void spellcheck_done();

  protected:

    // all spell checker data stored in here
    struct _kspell {
      KSpell *kspell;
      KSpellConfig *ksc;
      QString spell_tmptext;
      bool kspellon;              // are we doing a spell check?
      int kspellMispellCount;     // how many words suggested for replacement so far
      int kspellReplaceCount;     // how many words actually replaced so far
      bool kspellPristine;        // doing spell check on a clean document?
    } kspell;
};

#endif // __KWVIEW_H__
