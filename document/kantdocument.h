/***************************************************************************
                          kantdocument.h  -  description
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

#ifndef kant_document_h
#define kant_document_h

#include "../main/kantmain.h"

#include <qobject.h>
#include <qlist.h>
#include <qcolor.h>
#include <qfont.h>
#include <qfontmetrics.h>

#include <kparts/browserextension.h>

#include "../view/kantview.h"
#include "kanthighlight.h"
#include <ktexteditor.h>
#include "kantbuffer.h"
#include "kanttextline.h"

class Attribute;

class KantAction {
  public:
    enum Action {replace, wordWrap, wordUnWrap, newLine, delLine,
      insLine, killLine};//, doubleLine, removeLine};

    KantAction(Action, PointStruc &cursor, int len = 0,
      const QString &text = QString::null);

    Action action;
    PointStruc cursor;
    int len;
    QString text;
    KantAction *next;
};

class KantActionGroup {
  public:
    // the undo group types
    enum {  ugNone,         //
            ugPaste,        // paste
            ugDelBlock,     // delete/replace selected text
            ugIndent,       // indent
            ugUnindent,     // unindent
            ugComment,      // comment
            ugUncomment,    // uncomment
            ugReplace,      // text search/replace
            ugSpell,        // spell check
            ugInsChar,      // char type/deleting
            ugDelChar,      // ''  ''
            ugInsLine,      // line insert/delete
            ugDelLine       // ''  ''
         };

    KantActionGroup(PointStruc &aStart, int type = ugNone);
    ~KantActionGroup();
    void insertAction(KantAction *);

    static const char * typeName(int type);

    PointStruc start;
    PointStruc end;
    KantAction *action;
    int undoType;
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
class KantDocument : public KTextEditor::Document
{
    Q_OBJECT
    friend class KantViewInternal;
    friend class KantView;

  public:
    KantDocument(long docID, QFileInfo* fi, bool bSingleViewMode=false, bool bBrowserView=false, QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0);
    ~KantDocument();

    virtual bool openFile();
    virtual bool saveFile();

    virtual KTextEditor::View *createView( QWidget *parent, const char *name );
    virtual QString textLine( int line ) const;

    virtual void insertLine( const QString &s, int line = -1 );
    virtual void insertAt( const QString &s, int line, int col, bool mark = FALSE );
    virtual void removeLine( int line );
    virtual int length() const;

    virtual void setSelection( int row_from, int col_from, int row_to, int col_t );
    virtual bool hasSelection() const;
    virtual QString selection() const;

    // only to make part work, don't change it !
    bool m_bSingleViewMode;

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

    void readConfig(KConfig *);
    void writeConfig(KConfig *);
    void readSessionConfig(KConfig *);
    void writeSessionConfig(KConfig *);

    bool hasBrowserExtension() const { return m_bBrowserView; }

  protected:
    bool m_bBrowserView;

  signals:
    void selectionChanged();
    void highlightChanged();
    void modifiedChanged ();

  // search stuff
  protected:
    QStringList searchForList;
    QStringList replaceWithList;

  // highlight stuff
  public:
    Highlight *highlight() {return m_highlight;}
    int highlightNum() {return hlManager->findHl(m_highlight);}
    int numAttribs() {return m_numAttribs;}
    Attribute *attribs() {return m_attribs;}
  protected:
    void setHighlight(int n);
    void makeAttribs();
    void updateFontData();
  protected slots:
    void hlChanged();

// view interaction
  public:
    virtual void addView(KTextEditor::View *);
    virtual void removeView(KTextEditor::View *);
    bool ownedView(KantView *);
    bool isLastView(int numViews);

    int getTextLineCount() { return contents.count(); }

    int textWidth(const TextLine::Ptr &, int cursorX);
    int textWidth(PointStruc &cursor);
    int textWidth(bool wrapCursor, PointStruc &cursor, int xPos);
    int textPos(const TextLine::Ptr &, int xPos);
//    int textPos(TextLine::Ptr &, int xPos, int &newXPos);
    int textWidth();
    int textHeight();

    void insert(VConfig &, const QString &);
    void insertFile(VConfig &, QIODevice &);
#ifdef NEW_CODE
    void loadFile(const QString &file, QTextCodec *codec);
    bool writeFile(const QString &file, QTextCodec *codec);
    void appendData(const QByteArray &data, QTextCodec *codec);
#else
    void loadFile(QIODevice &);
    void writeFile(QIODevice &);
#endif

    int currentColumn(PointStruc &cursor);
    bool insertChars(VConfig &, const QString &chars);
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
    void doCommentLine(PointStruc &cursor);
    void doUncommentLine(PointStruc &cursor);

    virtual QString text() const;
    QString getWord(PointStruc &cursor);

  public slots:
    virtual void setText(const QString &);

  public:
    bool hasMarkedText() {return (selectEnd >= selectStart);}
    QString markedText(int flags);
    void delMarkedText(VConfig &/*, bool undo = true*/);

    void tagLineRange(int line, int x1, int x2);
    void tagLines(int start, int end);
    void tagAll();
    void updateLines(int startLine = 0, int endLine = 0xffffff, int flags = 0,
      int cursorY = -1);
    void updateMaxLength(TextLine::Ptr &);
    void updateViews(KantView *exclude = 0L);

    QColor &cursorCol(int x, int y);
    QFont &getTextFont(int x, int y);
    void paintTextLine(QPainter &, int line, int xStart, int xEnd, bool showTabs);

    void setURL( const KURL &url, bool updateHighlight );
    void clearFileName();

    bool doSearch(SConfig &s, const QString &searchFor);
    void unmarkFound();
    void markFound(PointStruc &cursor, int len);

// internal
    void tagLine(int line);
    void insLine(int line);
    void delLine(int line);
    void optimizeSelection();

    void doAction(KantAction *);
    void doReplace(KantAction *);
    void doWordWrap(KantAction *);
    void doWordUnWrap(KantAction *);
    void doNewLine(KantAction *);
    void doDelLine(KantAction *);
    void doInsLine(KantAction *);
    void doKillLine(KantAction *);
    void newUndo();

    void recordStart(VConfig &, int newUndoType);
    void recordStart(KantView *, PointStruc &, int flags, int newUndoType,
      bool keepModal = false, bool mergeUndo = false);
    void recordAction(KantAction::Action, PointStruc &);
    void recordInsert(VConfig &, const QString &text);
    void recordReplace(VConfig &, int len, const QString &text);
    void recordInsert(PointStruc &, const QString &text);
    void recordDelete(PointStruc &, int len);
    void recordReplace(PointStruc &, int len, const QString &text);
    void recordEnd(VConfig &);
    void recordEnd(KantView *, PointStruc &, int flags);
//  void recordReset();
    void doActionGroup(KantActionGroup *, int flags, bool undo = false);

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

  protected slots:
    void clipboardChanged();
    void slotBufferChanged();

  private slots:
    void slotViewDestroyed();

// member variables
  protected:
    TextLine::List contents;
    KWBuffer *buffer;
    QColor colors[5];
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

    QList<KantView> views;
    bool newDocGeometry;

    TextLine::Ptr longestLine;
    int maxLength;

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

    int foundLine;

    QList<KantActionGroup> undoList;
    int currentUndo;
    int undoState;
    int undoSteps;
    int tagStart;
    int tagEnd;

//    int undoType;           // what kind of undo is active
    int undoCount;          //counts merged undo steps
//    bool undoReported;      // true if the current undo has been reported to the views
//    KantView* undoView;   // the KantView that owns the undo group

    QWidget *pseudoModal;   //the replace prompt is pseudo modal

    public:
    /** anders: reimplemented from kwdoc to update mTime */
   // virtual bool saveFile();
    /** Tjecks if the file on disk is newer than document contents.
      If forceReload is true, the document is reloaded without asking the user,
      otherwise [default] the user is asked what to do. */
    void isModOnHD(bool forceReload=false);

    long docID () {return myDocID;};
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
    long myDocID;
    QFileInfo* fileinfo;
    QDateTime mTime;
    QString myDocName;

  signals:
    void modStateChanged (KantDocument *doc);
    void nameChanged (KantDocument *doc);
};

class KantBrowserExtension : public KParts::BrowserExtension
{
  Q_OBJECT

  public:
    KantBrowserExtension( KantDocument *doc );

  private slots:
    void copy();
    void slotSelectionChanged();

  private:
    KantDocument *m_doc;
};

#endif
