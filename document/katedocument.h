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

/*
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

#ifndef kate_document_h
#define kate_document_h

#include "../main/katemain.h"

#include <qobject.h>
#include <qptrlist.h>
#include <qcolor.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qdatetime.h>

#include "../view/kateview.h"
#include "katehighlight.h"
#include "katebuffer.h"
#include "katetextline.h"


#include <qptrdict.h>

class KateCmd;

#include "../interfaces/document.h"
#include "./katedocumentIface.h"

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

/**
  The text document. It contains the textlines, controls the
  document changing operations and does undo/redo. WARNING: do not change
  the text contents directly in methods where this is not explicitly
  permitted. All changes have to be made with some basic operations,
  which are recorded by the undo/redo system.
  @see TextLine
  @author Jochen Wilhelmy
*/
class KateDocument : public Kate::Document, public KateDocumentDCOPIface
{
    Q_OBJECT
    friend class KateViewInternal;
    friend class KateView;
    friend class KateIconBorder;

  public:
    KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, QWidget *parentWidget = 0, const char *widgetName = 0, QObject * = 0, const char * = 0);
    ~KateDocument ();

    // KTextEditor::Document stuff
    virtual KTextEditor::View *createView( QWidget *parent, const char *name );
    QPtrList<KTextEditor::View> views () const { return _views; };

    // KTextEditor::EditInterface stuff
    virtual QString text ( int line, int col, int len ) const;
    virtual QString textLine ( int line ) const;

    virtual bool insertText ( int line, int col, const QString &s );
    virtual bool removeText ( int line, int col, int len );

    virtual bool insertLine ( int line, const QString &s );
    virtual bool removeLine ( int line );

    virtual int length () const;
    virtual int lineLength ( int line ) const;

    // KTextEditor::CursorInterface stuff
    virtual KTextEditor::Cursor *createCursor ();
    virtual QPtrList<KTextEditor::Cursor> cursors () const;

    // internal edit stuff (mostly for view)
    bool insertChars ( int line, int col, const QString &chars, KateView *view );

  protected:
    QFont myFont, myFontBold, myFontItalic, myFontBI;
    CachedFontMetrics myFontMetrics, myFontMetricsBold, myFontMetricsItalic, myFontMetricsBI;

  public:
    void setFont (QFont font);
    QFont getFont () { return myFont; };
    CachedFontMetrics getFontMetrics () { return myFontMetrics; };

    virtual bool openFile();
    virtual bool saveFile();

    void insert_Line(const QString& s,int line=-1, bool update=true);
    void remove_Line(int line,bool update=true);
    void replaceLine(const QString& s,int line=-1);

    virtual void setSelection( int row_from, int col_from, int row_to, int col_t );
    virtual bool hasSelection() const;
    virtual QString selection() const;

    // only to make part work, don't change it !
    bool m_bSingleViewMode;

    QPtrList<KTextEditor::Cursor> myCursors;

// public interface
    /**
     *  gets the number of lines
     */
    virtual int numLines() const;

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
    void setReadOnly(bool);
    bool isReadOnly() const;
    void setNewDoc( bool );
    bool isNewDoc() const;
    virtual void setReadWrite( bool );
    virtual bool isReadWrite() const;
    virtual void setModified(bool);
    virtual bool isModified() const;
    void setSingleSelection(bool ss) {m_singleSelection = ss;}
    bool singleSelection() {return m_singleSelection;}

    void readConfig();
    void writeConfig();
    void readSessionConfig(KConfig *);
    void writeSessionConfig(KConfig *);

    bool hasBrowserExtension() const { return m_bBrowserView; }

  protected:
    bool m_bBrowserView;

  signals:
    void selectionChanged();
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
    /** Returns the first view in the documents list of views.
    *   see @ref QList
    */
    KateView* getFirstView();
    /** Returns the next view in the documents list of views.
    *   see @ref QList
    */
    KateView* getNextView();
    /** Returns the last view in the documents list of views.
    *   see @ref QList
    */
    KateView* getLastView();
    /** Returns the previous view in the documents list of views.
    *   see @ref QList
    */
    KateView* getPrevView();
    /** Returns the current view in the documents list of views.
    *   see @ref QList
    */
    KateView* getCurrentView();
    /** Returns the number of views for the document.
    *   see @ref QList
    */
    int getViewCount();

    void addView(KTextEditor::View *);
    void removeView(KTextEditor::View *);

    void addCursor(KTextEditor::Cursor *);
    void removeCursor(KTextEditor::Cursor *);

    bool ownedView(KateView *);
    bool isLastView(int numViews);

    int getTextLineCount() {return numLines();}

    int textWidth(const TextLine::Ptr &, int cursorX);
    int textWidth(PointStruc &cursor);
    int textWidth(bool wrapCursor, PointStruc &cursor, int xPos);
    int textPos(const TextLine::Ptr &, int xPos);
    int textWidth();
    int textHeight();

    int currentColumn(PointStruc &cursor);
    void newLine(VConfig &);
    void killLine(VConfig &);
    void backspace(VConfig &);
    void del(VConfig &);
    void clear();
    void cut(VConfig &);
    void copy(int flags);
    void paste(VConfig &);

    void toggleRect(int, int, int, int);
    void selectTo(VConfig &c, PointStruc &cursor, int cXPos);
    void selectAll();
    void deselectAll();
    void invertSelection();
    void selectWord(PointStruc &cursor, int flags);
    void selectLength(PointStruc &cursor, int length, int flags);

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
    QString getWord(PointStruc &cursor);

  public slots:
    virtual void setText(const QString &);

  public:
    long needPreHighlight(long till);
    bool hasMarkedText() {return (selectEnd >= selectStart);}
    QString markedText(int flags);
    void delMarkedText(VConfig &/*, bool undo = true*/);

    void tagLineRange(int line, int x1, int x2);
    void tagLines(int start, int end);
    void tagAll();
    void updateLines(int startLine = 0, int endLine = 0xffffff, int flags = 0, int cursorY = -1);
    void updateMaxLength(TextLine::Ptr &);
    void updateViews(KateView *exclude = 0L);

    QColor &cursorCol(int x, int y);
    void paintTextLine(QPainter &, int line, int xStart, int xEnd, bool showTabs);
    void paintTextLine(QPainter &, int line, int y, int xStart, int xEnd, bool showTabs);

    bool doSearch(SConfig &s, const QString &searchFor);

// internal
    void tagLine(int line);
    void insLine(int line);
    void delLine(int line);
    void optimizeSelection();

    void newUndo();

    int nextUndoType();
    int nextRedoType();
    void undoTypeList(QValueList<int> &lst);
    void redoTypeList(QValueList<int> &lst);
    void undo(VConfig &, int count = 1);
    void redo(VConfig &, int count = 1);
    void clearRedo();
    void setUndoSteps(int steps);

    void setPseudoModal(QWidget *);

    void newBracketMark(PointStruc &, BracketMark &);

  protected:
    virtual void guiActivateEvent( KParts::GUIActivateEvent *ev );
    // Comment, uncomment methods
    bool removeStringFromBegining(VConfig &c, QString &str);
    bool removeStringFromEnd(VConfig &c, QString &str);
    void addStartLineCommentToSingleLine(VConfig &c);
    bool removeStartLineCommentFromSingleLine(VConfig &c);
    void addStartStopCommentToSingleLine(VConfig &c);
    bool removeStartStopCommentFromSingleLine(VConfig &c);
    void addStartStopCommentToSelection(VConfig &c);
    void addStartLineCommentToSelection(VConfig &c);
    bool removeStartStopCommentFromSelection(VConfig &c);
    bool removeStartLineCommentFromSelection(VConfig &c);

  protected slots:
    void clipboardChanged();
    void slotBufferChanged();
    void slotBufferHighlight(long,long);
    void doPreHighlight();

  private slots:
    void slotViewDestroyed();

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

    PointStruc select;
    PointStruc anchor;
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

    int currentUndo;
    int undoState;
    int undoSteps;
    int tagStart;
    int tagEnd;
    int undoCount;          //counts merged undo steps

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
    QFileInfo* fileInfo;
    QDateTime mTime;
    QString myDocName;

  private:
    KateCmd *myCmd;

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
    void textChanged ();

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

  protected:
    uint configFlags;
    uint searchFlags;

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
};

#endif


