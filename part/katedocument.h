/***************************************************************************
                          katedocument.h  -  description
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

#ifndef kate_document_h
#define kate_document_h

#include <config.h>

#include <qobject.h>
#include <qptrlist.h>
#include <qcolor.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qdatetime.h>

#include "kateview.h"
#include "katehighlight.h"
#include "katebuffer.h"
#include "katetextline.h"

class KateCmd;

#include "../interfaces/document.h"

class CachedFontMetrics : public QFontMetrics {
private:
    short *warray[256];
public:
    CachedFontMetrics(const QFont& f) : QFontMetrics(f) {
        for (int i=0; i<256; i++) warray[i]=0;
    }
    ~CachedFontMetrics() {
        for (int i=0; i<256; i++)
                if (warray[i]) delete[] warray[i];
    }
    int width(QChar c) {
        uchar cell=c.cell();
        uchar row=c.row();
        short *wa=warray[row];
        if (!wa) {
                // qDebug("create row: %d",row);
                wa=warray[row]=new short[256];
                for (int i=0; i<256; i++) wa[i]=-1;
        }
        if (wa[cell]<0) wa[cell]=(short) QFontMetrics::width(c);
        return (int)wa[cell];
    }
    int width(QString s) { return QFontMetrics::width(s); }
};

class Attribute {
  public:
    Attribute() { ; };

    QColor col;
    QColor selCol;
    bool bold;
    bool italic;
};

class KateCursor : public Kate::Cursor
{
  public:
    KateCursor (KateDocument *doc);
    ~KateCursor ();

    virtual void position ( int *line, int *col ) const;

    virtual bool setPosition ( int line, int col );

    virtual bool insertText ( const QString& text );

    virtual bool removeText ( int numberOfCharacters );

    virtual QChar currentChar () const;

  private:
    class KateDocument *myDoc;
};

class KateUndo;
class KateUndoGroup;

/**
  The text document. It contains the textlines, controls the
  document changing operations and does undo/redo. WARNING: do not change
  the text contents directly in methods where this is not explicitly
  permitted. All changes have to be made with some basic operations,
  which are recorded by the undo/redo system.
  @see TextLine
  @author Jochen Wilhelmy
*/
class KateDocument : public Kate::Document
{
    Q_OBJECT
    friend class KateViewInternal;
    friend class KateView;
    friend class KateIconBorder;
		friend class KateUndoGroup;
		friend class KateUndo;

  public:
    KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, QWidget *parentWidget = 0, const char *widgetName = 0, QObject * = 0, const char * = 0);
    ~KateDocument ();

  //
  // KTextEditor::Document stuff
  //
  public:
    KTextEditor::View *createView( QWidget *parent, const char *name );
    QPtrList<KTextEditor::View> views () const;

  //
  // KTextEditor::EditInterface stuff
  //
	public slots:
    QString text ( uint startLine, uint startCol, uint endLine, uint endCol ) const;
    QString textLine ( uint line ) const;

    bool setText(const QString &);
    bool clear ();

		bool insertText ( uint line, uint col, const QString &s );
    bool removeText ( uint startLine, uint startCol, uint endLine, uint endCol );

    bool insertLine ( uint line, const QString &s );
    bool removeLine ( uint line );

		uint numLines() const;
    uint length () const;
    int lineLength ( uint line ) const;

	signals:
		void textChanged ();

  protected:
	  //
		// 6 internal functions (mostly to enable undo/redo atomic )
		//
	  bool internalInsertText ( uint line, uint col, const QString &s );
    bool internalRemoveText ( uint line, uint col, uint len );

		bool internalWrapLine ( uint line, uint col );
    bool internalUnWrapLine ( uint line, uint col);

		bool internalInsertLine ( uint line, const QString &s );
    bool internalRemoveLine ( uint line );

	//
	// KTextEditor::SelectionInterface stuff
  //
  public slots:
    bool setSelection ( uint startLine, uint startCol, uint endLine, uint endCol );
    bool clearSelection ();

    bool hasSelection () const;
    QString selection () const ;

    bool removeSelectedText ();

		bool selectAll();
    bool invertSelection();

	signals:
		void selectionChanged ();

	//
  // KTextEditor::UndoInterface stuff
  //
	public slots:
    void undo ();
    void redo ();
    void clearUndo ();
		void clearRedo ();

		uint undoCount () const;
		uint redoCount () const;

		uint undoSteps () const;
    void setUndoSteps ( uint steps );

	signals:
		void undoChanged ();

  //
  // KTextEditor::CursorInterface stuff
  //
  public:
    KTextEditor::Cursor *createCursor ();
    QPtrList<KTextEditor::Cursor> cursors () const;

	//
  // KParts::ReadWrite stuff
  //
  public:
	  bool openFile ();
    bool saveFile ();

    void setReadWrite( bool );
    bool isReadWrite() const;

    void setModified(bool);
    bool isModified() const;

  protected:
    //
    // internal edit stuff (mostly for view)
    //
    bool insertChars ( int line, int col, const QString &chars, KateView *view );

	private:
	  bool _autoUpdate;

  protected:
    QFont myFont, myFontBold, myFontItalic, myFontBI;
    CachedFontMetrics myFontMetrics, myFontMetricsBold, myFontMetricsItalic, myFontMetricsBI;

  public:
    void setFont (QFont font);
    QFont getFont () { return myFont; };
    CachedFontMetrics getFontMetrics () { return myFontMetrics; };

    // only to make part work, don't change it !
    bool m_bSingleViewMode;

    QPtrList<KTextEditor::Cursor> myCursors;

    /**
     * gets the last line number (numLines() -1)
     */
    int lastLine() const {return numLines()-1;}

    /**
      gets the given line
      @return  the TextLine object at the given line
      @see     TextLine
    */
    TextLine::Ptr getTextLine(int line) const;

    /**
      get the length in pixels of the given line
    */
    int textLength(int line);

    void setTabWidth(int);
    int tabWidth() {return tabChars;}
    void setNewDoc( bool );
    bool isNewDoc() const;

    void readConfig();
    void writeConfig();
    void readSessionConfig(KConfig *);
    void writeSessionConfig(KConfig *);

    bool hasBrowserExtension() const { return m_bBrowserView; }

  protected:
    bool m_bBrowserView;

  signals:
    void highlightChanged();
    void modifiedChanged ();
    void preHighlightChanged(long);

  // search stuff
  protected:
    static QStringList searchForList;
    static QStringList replaceWithList;
    static uint uniqueID;

  // highlight stuff
  public:
    Highlight *highlight() {return m_highlight;}
    int highlightNum() {return hlManager->findHl(m_highlight);}
    int numAttribs() {return m_numAttribs;}
    Attribute *attribs() {return m_attribs;}
    void setDontChangeHlOnSave();

  protected:
    void setHighlight(int n);
    void makeAttribs();
    void updateFontData();

  protected slots:
    void hlChanged();

// view interaction
  public:
    void addView(KTextEditor::View *);
    void removeView(KTextEditor::View *);

    void addCursor(KTextEditor::Cursor *);
    void removeCursor(KTextEditor::Cursor *);

    bool ownedView(KateView *);
    bool isLastView(int numViews);

    int getTextLineCount() {return numLines();}

    int textWidth(const TextLine::Ptr &, int cursorX);
    int textWidth(KateViewCursor &cursor);
    int textWidth(bool wrapCursor, KateViewCursor &cursor, int xPos);
    int textPos(const TextLine::Ptr &, int xPos);
    int textWidth();
    int textHeight();

    int currentColumn(KateViewCursor &cursor);
    void newLine(VConfig &);
    void killLine(VConfig &);
    void backspace(VConfig &);
    void del(VConfig &);
    void cut(VConfig &);
    void copy(int flags);
    void paste(VConfig &);

    void toggleRect(int, int, int, int);
    void selectTo(VConfig &c, KateViewCursor &cursor, int cXPos);
    void selectWord(KateViewCursor &cursor, int flags);
    void selectLength(KateViewCursor &cursor, int length, int flags);

    void indent(VConfig &c) {doIndent(c, 1);}
    void unIndent(VConfig &c) {doIndent(c, -1);}
    void cleanIndent(VConfig &c) {doIndent(c, 0);}
    // called by indent/unIndent/cleanIndent
    // just does some setup and then calls optimizeLeadingSpace()
    void doIndent(VConfig &, int change);
    // optimize leading whitespace on a single line - see kwdoc.cpp for full description
    void optimizeLeadingSpace(int line, int flags, int change);

    void comment(VConfig &c) {doComment(c, 1);}
    void unComment(VConfig &c) {doComment(c, -1);}
    void doComment(VConfig &, int change);

    virtual QString text() const;
    QString getWord(KateViewCursor &cursor);

  public:
    long needPreHighlight(long till);

    void tagLineRange(int line, int x1, int x2);
    void tagLines(int start, int end);
    void tagAll();
    void updateLines(int startLine = 0, int endLine = 0xffffff);
    void updateMaxLength(TextLine::Ptr &);
    void updateViews(KateView *exclude = 0L);

    QColor &cursorCol(int x, int y);
    void paintTextLine(QPainter &, int line, int xStart, int xEnd, bool showTabs);
    void paintTextLine(QPainter &, int line, int y, int xStart, int xEnd, bool showTabs);

    bool doSearch(SConfig &s, const QString &searchFor);

// internal
    void tagLine(int line);
    void optimizeSelection();

	public:

    void setPseudoModal(QWidget *);

    void newBracketMark(KateViewCursor &, BracketMark &);

  protected:
    virtual void guiActivateEvent( KParts::GUIActivateEvent *ev );

  protected:
    //
    // Comment, uncomment methods
    //
    bool removeStringFromBegining(int line, QString &str);
    bool removeStringFromEnd(int line, QString &str);

    void addStartLineCommentToSingleLine(int line);
    bool removeStartLineCommentFromSingleLine(int line);

    void addStartStopCommentToSingleLine(int line);
    bool removeStartStopCommentFromSingleLine(int line);

    void addStartStopCommentToSelection();
    void addStartLineCommentToSelection();

    bool removeStartStopCommentFromSelection();
    bool removeStartLineCommentFromSelection();

  protected slots:
    void clipboardChanged();
    void slotBufferChanged();
    void slotBufferHighlight(long,long);
    void doPreHighlight();


// member variables
  protected:
    long PreHighlightedTill;
    long RequestPreHighlightTill;
    KWBuffer *buffer;
    QColor colors[2];
    HlManager *hlManager;
    Highlight *m_highlight;
    int m_numAttribs;
    static const int maxAttribs;
    Attribute *m_attribs;

    int eolMode;

    int tabChars;
    int m_tabWidth;
    int fontHeight;
    int fontAscent;

    QPtrList<KateView> myViews;
    QPtrList<KTextEditor::View> _views;

    bool newDocGeometry;

    TextLine::Ptr longestLine;
    float maxLength;

    KateViewCursor select;
    KateViewCursor anchor;
    int aXPos;
    int selectStart;
    int selectEnd;
    bool oldMarkState;
    bool m_singleSelection; // false: windows-like, true: X11-like

    bool readOnly;
    bool newDoc;          // True if the file is a new document (used to determine whether
                          // to check for overwriting files on save)
    bool modified;

    bool myWordWrap;
    uint myWordWrapAt;

    int tagStart;
    int tagEnd;

    QWidget *pseudoModal;   //the replace prompt is pseudo modal

    public:
    /** Checks if the file on disk is newer than document contents.
      If forceReload is true, the document is reloaded without asking the user,
      otherwise [default] the user is asked what to do. */
    void isModOnHD(bool forceReload=false);

    uint docID () {return myDocID;};
    QString docName () {return myDocName;};

    void setDocName (QString docName);

  public slots:
    /** Reloads the current document from disk if possible */
    void reloadFile();

  private slots:
    void slotModChanged ();

  private:
    /** updates mTime to reflect file on fs.
     called from constructor and from saveFile. */
    void setMTime();
    uint myDocID;
    class QFileInfo* fileInfo;
    class QDateTime mTime;
    QString myDocName;

  private:
    class KateCmd *myCmd;

  public:
    KateCmd *cmd () { return myCmd; };

  private:
    QString myEncoding;

  public:
    void setEncoding (QString e) { myEncoding = e; };
    QString encoding() { return myEncoding; };

    void setWordWrap (bool on);
    bool wordWrap () { return myWordWrap; };

    void setWordWrapAt (uint col);
    uint wordWrapAt () { return myWordWrapAt; };

  signals:
    void modStateChanged (KateDocument *doc);
    void nameChanged (KateDocument *doc);

  public:
    QPtrList<Kate::Mark> marks ();

  public slots:
    // clear buffer/filename - update the views
    void flush ();

  signals:
    /**
      The file has been saved (perhaps the name has changed). The main window
      can use this to change its caption
    */
    void fileNameChanged ();

  public:
    //end of line settings
    enum Eol_settings {eolUnix=0,eolDos=1,eolMacintosh=2};

  // for the DCOP interface
  public:
    void open (const QString &name=0);

  public:
    // wrap the text of the document at the column col
    void wrapText (uint col);

  public slots:
     void applyWordWrap ();

  private:
    bool hlSetByUser;

	public:
	  uint configFlags ();
		void setConfigFlags (uint flags);

  protected:
    uint _configFlags;
    uint _searchFlags;

  public:
    enum Config_flags {
      cfAutoIndent= 0x1,
      cfBackspaceIndents= 0x2,
      cfWordWrap= 0x4,
      cfReplaceTabs= 0x8,
      cfRemoveSpaces = 0x10,
      cfWrapCursor= 0x20,
      cfAutoBrackets= 0x40,
      cfPersistent= 0x80,
      cfKeepSelection= 0x100,
      cfVerticalSelect= 0x200,
      cfDelOnInput= 0x400,
      cfXorSelect= 0x800,
      cfOvr= 0x1000,
      cfMark= 0x2000,
			cfGroupUndo= 0x4000,
      cfKeepIndentProfile= 0x8000,
      cfKeepExtraSpaces= 0x10000,
      cfMouseAutoCopy= 0x20000,
      cfSingleSelection= 0x40000,
      cfTabIndents= 0x80000,
      cfPageUDMovesCursor= 0x100000,
      cfShowTabs= 0x200000,
      cfSpaceIndent= 0x400000,
      cfSmartHome = 0x800000};

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

	private:
	  QPtrList<KateUndoGroup> undoItems;
		QPtrList<KateUndoGroup> redoItems;
		uint myUndoSteps;
		KateUndoGroup *currentUndo;
};

#endif


