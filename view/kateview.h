/***************************************************************************
                          kateview.h  -  description
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

#ifndef kate_view_h
#define kate_view_h

#include "../main/katemain.h"

#include "../interfaces/view.h"
#include "../interfaces/document.h"

#include <kparts/browserextension.h>
#include <qptrlist.h>
#include <qregexp.h>
#include <qstring.h>
#include <qdialog.h>
#include <kspell.h>

#include "kateviewhighlightaction.h"

class KToggleAction;
class KActionMenu;
class KAction;
class KRecentFilesAction;
class KSelectAction;
class QTextDrag;
class KPrinter;
class KateDocument;
class Highlight;

/*
//dialog results
const int srYes               = QDialog::Accepted;
const int srNo                = 10;
const int srAll               = 11;
const int srCancel            = QDialog::Rejected;
*/
// --- config flags ---
// indent

enum Select_flags {
  selectFlag          = 0x100000,
  multiSelectFlag     = 0x200000
};
//state commands
enum State_commands {
  cmToggleInsert      = 1,
  cmToggleVertical    = 2
};

class KateViewInternal;
class KateView;

struct PointStruc {
  int x;
  int y;
};

struct VConfig {
  KateView *view;
  PointStruc cursor;
  int cXPos;
  int flags;
};

struct SConfig {
  PointStruc cursor;
  PointStruc startCursor;
  int flags;

  // Set the pattern to be used for searching.
  void setPattern(QString &newPattern);

  // Search the given string.
  int search(QString &text, int index);

  // The length of the last match found using pattern or regExp.
  int matchedLength;

private:
  QString m_pattern;

  // The regular expression corresponding to pattern. Only guaranteed valid if
  // flags has sfRegularExpression set.
  QRegExp m_regExp;
};

struct LineRange {
  int start;
  int end;
};

struct BracketMark {
  PointStruc cursor;
  int sXPos;
  int eXPos;
};


class KateIconBorder : public QWidget
{
public:
    KateIconBorder(KateView *view, class KateViewInternal *internalView);
    ~KateIconBorder();

    void paintLine(int i);

protected:
    void paintEvent(QPaintEvent* e);
    void mousePressEvent(QMouseEvent* e);

private:

    KateView *myView;
    class KateViewInternal *myInternalView;
    bool lmbSetsBreakpoints;
};

class KateViewInternal : public QWidget {
    Q_OBJECT
    friend class KateDocument;
    friend class KateView;
    friend class KateIconBorder;

  private:
    long waitForPreHighlight;
    int iconBorderWidth;
    int iconBorderHeight;

  protected slots:
    void slotPreHighlightUpdate(long line);

  public:
    KateViewInternal(KateView *view, KateDocument *doc);
    ~KateViewInternal();

    virtual void doCursorCommand(VConfig &, int cmdNum);
    virtual void doEditCommand(VConfig &, int cmdNum);

    void cursorLeft(VConfig &);
    void cursorRight(VConfig &);
    void wordLeft(VConfig &);
    void wordRight(VConfig &);
    void home(VConfig &);
    void end(VConfig &);
    void cursorUp(VConfig &);
    void cursorDown(VConfig &);
    void scrollUp(VConfig &);
    void scrollDown(VConfig &);
    void topOfView(VConfig &);
    void bottomOfView(VConfig &);
    void pageUp(VConfig &);
    void pageDown(VConfig &);
    void cursorPageUp(VConfig &);
    void cursorPageDown(VConfig &);
    void top(VConfig &);
    void bottom(VConfig &);
    void top_home(VConfig &c);
    void bottom_end(VConfig &c);

  protected slots:
    void changeXPos(int);
    void changeYPos(int);

  protected:
    void getVConfig(VConfig &);
    void changeState(VConfig &);
    void insLine(int line);
    void delLine(int line);
    void updateCursor();
    void updateCursor(PointStruc &newCursor);
    void updateCursor(PointStruc &newCursor, int flags);
    void clearDirtyCache(int height);
    void tagLines(int start, int end, int x1, int x2);
    void tagAll();
    void setPos(int x, int y);
    void center();

    void updateView(int flags);

    void paintTextLines(int xPos, int yPos);
    void paintCursor();
    void paintBracketMark();

    void placeCursor(int x, int y, int flags = 0);
    bool isTargetSelected(int x, int y);

    void doDrag();

    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void wheelEvent( QWheelEvent *e );
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void timerEvent(QTimerEvent *);

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

    KateView *myView;
    KateDocument *myDoc;
    QScrollBar *xScroll;
    QScrollBar *yScroll;
    KateIconBorder *leftBorder;

    int xPos;
    int yPos;

    int mouseX;
    int mouseY;
    int scrollX;
    int scrollY;
    int scrollTimer;

    PointStruc cursor;
    bool cursorOn;
    int cursorTimer;
    int cXPos;
    int cOldXPos;

    int startLine;
    int endLine;

    bool exposeCursor;
    int updateState;
    int numLines;
    LineRange *lineRanges;
    int newXPos;
    int newYPos;

    QPixmap *drawBuffer;

    BracketMark bm;

    enum DragState { diNone, diPending, diDragging };

    struct _dragInfo {
      DragState       state;
      PointStruc      start;
      QTextDrag       *dragObject;
    } dragInfo;

  signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);
};

/**
  The KateView text editor widget. It has many options, document/view
  architecture and syntax highlighting.
  @author Jochen Wilhelmy
*/

class KateView : public Kate::View
{
    Q_OBJECT
    friend class KateViewInternal;
    friend class KateDocument;
    friend class KateIconBorder;

  public:
    KateView(KateDocument *doc=0L, QWidget *parent = 0L, const char * name = 0);
    ~KateView();

    virtual void setCursorPosition( int line, int col, bool mark = false );
    virtual void getCursorPosition( int *line, int *col );

    virtual bool isOverwriteMode() const;
    virtual void setOverwriteMode( bool b );

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
    void setCursorPositionInternal(int line, int col);

    int tabWidth();
    void setTabWidth(int);
    void setEncoding (QString e);

    /**
      Returns true if this editor is the only owner of its document
    */
    bool isLastView();
    /**
      Returns the document object
    */
    KateDocument *doc();

    QColor* getColors();
    void applyColors();

    void setupActions();

    KAction *editUndo, *editRedo, *bookmarkToggle, *bookmarkClear;

    KActionMenu *bookmarkMenu;
    KToggleAction *setVerticalSelection, *viewBorder;
    KRecentFilesAction *fileRecent;
    KSelectAction *setEndOfLine;
    KateViewHighlightAction *setHighlight;

  protected slots:
    void slotDropEventPass( QDropEvent * ev );

  public slots:
    void slotUpdate();
    void slotFileStatusChanged();
    void slotHighlightChanged();

  public slots:
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
      The marked text state has changed. This can be used to enable/disable
      cut and copy
    */
    void newMarkStatus();

    // emitted when saving a remote URL with KIO::NetAccess. In that case we have to disable the UI.
    void enableUI( bool enable );

  protected:
    virtual void keyPressEvent( QKeyEvent *ev );
    virtual void customEvent( QCustomEvent *ev );

    /*
     * Check if the given URL already exists. Currently used by both save() and saveAs()
     *
     * Asks the user for permission and returns the message box result and defaults to
     * KMessageBox::Yes in case of doubt
     */
    int checkOverwrite( KURL u );

//text access
  public:
     /**
       Gets the text line where the cursor is on
     */
     QString currentTextLine();
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
       Insert text at the current cursor position.
       The parameter @param mark is unused.
     */
     virtual void insertText(const QString &, bool = false );

  public:

    /**
      Returns true if the current document can be
      discarded. If the document is modified, the user is asked if he wants
      to save it. On "cancel" the function returns false.
    */
    bool canDiscard();

  public slots:
    /**
      Flushes the document of the text widget. The user is given
      a chance to save the current document if the current document has
      been modified.
    */
    void flush ();
    /**
      Saves the file if necessary under the current file name. If the current file
      name is Untitled, as it is after a call to newFile(), this routine will
      call saveAs().
    */
    saveResult save();
    /**
      Allows the user to save the file under a new name. This starts the
      automatic highlight selection.
    */
    saveResult saveAs();
    /**
      Moves the marked text into the clipboard
    */
    void cut() {doEditCommand(KateView::cmCut);}
    /**
      Copies the marked text into the clipboard
    */
    void copy() {doEditCommand(KateView::cmCopy);}
    /**
      Inserts text from the clipboard at the actual cursor position
    */
    void paste() {doEditCommand(KateView::cmPaste);}
    /**
      Moves the current line or the selection one position to the right
    */
    void indent() {doEditCommand(KateView::cmIndent);};
    /**
      Moves the current line or the selection one position to the left
    */
    void unIndent() {doEditCommand(KateView::cmUnindent);};
    /**
      Optimizes the selected indentation, replacing tabs and spaces as needed
    */
    void cleanIndent() {doEditCommand(KateView::cmCleanIndent);};
    /**
      Selects all text
    */
    void selectAll() {doEditCommand(KateView::cmSelectAll);};
    /**
      Inverts the current selection
    */
    void invertSelection() {doEditCommand(KateView::cmInvertSelection);};
    /**
      comments out current line
    */
    void comment() {doEditCommand(KateView::cmComment);};
    /**
      removes comment signs in the current line
    */
    void uncomment() {doEditCommand(KateView::cmUncomment);};

    void keyReturn() {doEditCommand(KateView::cmReturn);};
    void keyDelete() {doEditCommand(KateView::cmDelete);};
    void backspace() {doEditCommand(KateView::cmBackspace);};
    void killLine() {doEditCommand(KateView::cmKillLine);};

// cursor commands...

    void cursorLeft() {doCursorCommand(KateView::cmLeft);};
    void shiftCursorLeft() {doCursorCommand(KateView::cmLeft | selectFlag);};
    void cursorRight() {doCursorCommand(KateView::cmRight);}
    void shiftCursorRight() {doCursorCommand(KateView::cmRight | selectFlag);}
    void wordLeft() {doCursorCommand(KateView::cmWordLeft);};
    void shiftWordLeft() {doCursorCommand(KateView::cmWordLeft | selectFlag);};
    void wordRight() {doCursorCommand(KateView::cmWordRight);};
    void shiftWordRight() {doCursorCommand(KateView::cmWordRight | selectFlag);};
    void home() {doCursorCommand(KateView::cmHome);};
    void shiftHome() {doCursorCommand(KateView::cmHome | selectFlag);};
    void end() {doCursorCommand(KateView::cmEnd);};
    void shiftEnd() {doCursorCommand(KateView::cmEnd | selectFlag);};
    void up() {doCursorCommand(KateView::cmUp);};
    void shiftUp() {doCursorCommand(KateView::cmUp | selectFlag);};
    void down() {doCursorCommand(KateView::cmDown);};
    void shiftDown() {doCursorCommand(KateView::cmDown | selectFlag);};
    void scrollUp() {doCursorCommand(KateView::cmScrollUp);};
    void scrollDown() {doCursorCommand(KateView::cmScrollDown);};
    void topOfView() {doCursorCommand(KateView::cmTopOfView);};
    void bottomOfView() {doCursorCommand(KateView::cmBottomOfView);};
    void pageUp() {doCursorCommand(KateView::cmPageUp);};
    void shiftPageUp() {doCursorCommand(KateView::cmPageUp | selectFlag);};
    void pageDown() {doCursorCommand(KateView::cmPageDown);};
    void shiftPageDown() {doCursorCommand(KateView::cmPageDown | selectFlag);};
    void top() {doCursorCommand(KateView::cmTop);};
    void shiftTop() {doCursorCommand(KateView::cmTop | selectFlag);};
    void bottom() {doCursorCommand(KateView::cmBottom);};
    void shiftBottom() {doCursorCommand(KateView::cmBottom | selectFlag);};

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
      Presents a "Goto Line" dialog to the user
    */
    void gotoLine();
  protected:
    void initSearch(SConfig &, int flags);
    void continueSearch(SConfig &);
    void findAgain(SConfig &);
    void replaceAgain();
    void doReplaceAction(int result, bool found = false);
    void exposeFound(PointStruc &cursor, int slen, int flags, bool replace);
    void deleteReplacePrompt();
    bool askReplaceEnd();
  protected slots:
    void replaceSlot();
  protected:
    uint searchFlags;
    int replaces;
    SConfig s;
    QDialog *replacePrompt;

//right mouse button popup menu & bookmark menu
  public:
    /**
      Install a Popup Menu. The Popup Menu will be activated on
      a right mouse button press event.
    */
    void installPopup(QPopupMenu *rmb_Menu);

  protected:
    QPopupMenu *rmbMenu;

  signals:
    void bookAddChanged(bool enabled);
    void bookClearChanged(bool enabled);

//config file / session management functions
  public:
    /**
      Reads session config out of the KConfig object. This also includes
      the actual cursor position and the bookmarks.
    */
    void readSessionConfig(KConfig *);
    /**
      Writes session config into the KConfig object
    */
    void writeSessionConfig(KConfig *);


  public:
	void setDontChangeHlOnSave();
  // printing
  public slots:
    void printDlg ();

  protected:
    KPrinter *printer;

  // syntax highlight
  public slots:
    /**
      Presents the setup dialog to the user
    */
    void configDialog ();
    /**
      Gets the highlight number
    */
    int getHl();
    /**
      Sets the highlight number n
    */
    void setHl(int n);
    /**
      Get the end of line mode (Unix, Macintosh or Dos)
    */
    int getEol();
    /**
      Set the end of line mode (Unix, Macintosh or Dos)
    */
    void setEol(int);

//internal
  protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);

    void doCursorCommand(int cmdNum);
    void doEditCommand(int cmdNum);

    KateViewInternal *myViewInternal;
    KateDocument *myDoc;

//spell checker
  public:
    /**
     * Returns the KSpellConfig object
     */
    KSpellConfig *ksConfig(void) {return kspell.ksc;}
    /**
     * Sets the KSpellConfig object.  (The object is
     *  copied internally.)
     */
    void setKSConfig (const KSpellConfig _ksc) {*kspell.ksc=_ksc;}

  public slots:    //please keep prototypes and implementations in same order
    void spellcheck();
    void spellcheck2(KSpell*);
    void misspelling (QString word, QStringList *, unsigned pos);
    void corrected (QString originalword, QString newword, unsigned pos);
    void spellResult (const QString &newtext);
    void spellCleanDone();
  signals:
    /** This says spellchecking is <i>percent</i> done.
      */
    void  spellcheck_progress (unsigned int percent);
    /** Emitted when spellcheck is complete.
     */
    void spellcheck_done ();

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

    // some kwriteview stuff
  protected:
    void insLine(int line) { myViewInternal->insLine(line); };
    void delLine(int line) { myViewInternal->delLine(line); };
    void updateCursor() { myViewInternal->updateCursor(); };
    void updateCursor(PointStruc &newCursor) { myViewInternal->updateCursor(newCursor); };
    void updateCursor(PointStruc &newCursor, int flags) { myViewInternal->updateCursor(newCursor, flags); };

    void clearDirtyCache(int height) { myViewInternal->clearDirtyCache(height); };
    void tagLines(int start, int end, int x1, int x2) { myViewInternal->tagLines(start, end, x1, x2); };
    void tagAll() { myViewInternal->tagAll(); };
    void setPos(int x, int y) { myViewInternal->setPos(x, y); };
    void center() { myViewInternal->center(); };

    void updateView(int flags) { myViewInternal->updateView(flags); };

  protected slots:
    // to send dropEventPass
    void dropEventPassEmited (QDropEvent* e);


   signals:
    // emitted when KateViewInternal is not handling its own URI drops
    void dropEventPass(QDropEvent*);
  // end of kwriteview stuff


  public:
    enum Dialog_results {
      srYes=QDialog::Accepted,
      srNo=10,
      srAll,
      srCancel=QDialog::Rejected};

//search flags
    enum Search_flags {
     sfCaseSensitive=1,
     sfWholeWords=2,
     sfFromBeginning=4,
     sfBackward=8,
     sfSelected=16,
     sfPrompt=32,
     sfReplace=64,
     sfAgain=128,
     sfWrapped=256,
     sfFinished=512,
     sfRegularExpression=1024};

//update flags
    enum Update_flags {
     ufDocGeometry=1,
     ufUpdateOnScroll=2,
     ufPos=4};

//load flags
    enum Load_flags {
     lfInsert=1,
     lfNewFile=2,
     lfNoAutoHl=4};

//cursor movement commands
    enum Cursor_commands
	   { cmLeft,cmRight,cmWordLeft,cmWordRight,
       cmHome,cmEnd,cmUp,cmDown,
       cmScrollUp,cmScrollDown,cmTopOfView,cmBottomOfView,
       cmPageUp,cmPageDown,cmCursorPageUp,cmCursorPageDown,
       cmTop,cmBottom};
//edit commands
    enum Edit_commands {
		    cmReturn=1,cmDelete,cmBackspace,cmKillLine,
        cmCut,cmCopy,cmPaste,cmIndent,cmUnindent,cmCleanIndent,
        cmSelectAll,cmDeselectAll,cmInvertSelection,cmComment,
        cmUncomment};
//find commands
    enum Find_commands { cmFind=1,cmReplace,cmFindAgain,cmGotoLine};

  public:
    void setActive (bool b);
    bool isActive ();

  private:
    bool active;
    bool myIconBorder;
    QPtrList<Kate::Mark> list;

  public slots:
    virtual void setFocus ();
    void findAgain(bool back=false);
//    void findAgain () { findAgain(false); };
    void findPrev () { findAgain(true); };

  protected:
    bool eventFilter(QObject* o, QEvent* e);

  signals:
    void gotFocus (KateView *);

  public slots:
    void slotEditCommand ();
    void setIconBorder (bool enable);
    void toggleIconBorder ();
    void gotoMark (Kate::Mark *mark);
    void toggleBookmark ();
    void clearBookmarks ();

  public:
    bool iconBorder() { return myIconBorder; } ;

  private slots:
    void bookmarkMenuAboutToShow();
    void gotoBookmark (int n);

  public:
    Kate::Document *getDoc ()
      { return (Kate::Document*) myDoc; };

  public slots:
    int getHlCount ();
    QString getHlName (int);
    QString getHlSection (int);

    void slotIncFontSizes ();
    void slotDecFontSizes ();

  protected:
    uint myViewID;
    static uint uniqueID;

  public:
    KTextEditor::Document *document () const { return (KTextEditor::Document *)myDoc; };
};

class KateBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

  public:
    KateBrowserExtension( KateDocument *doc, KateView *view );

  public slots:
    void copy();
    void slotSelectionChanged();
    void print();

  private:
    KateDocument *m_doc;
    KateView *m_view;
};

#endif






