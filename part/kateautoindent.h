/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>
   Copyright (C) 2004 >Anders Lund <anders@alweb.dk> (KateVarIndent class)
   Copyright (C) 2005 Dominik Haumann <dhdev@gmx.de> (basic support for config page)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_AUTO_INDENT_H__
#define __KATE_AUTO_INDENT_H__

#include "katecursor.h"
#include "kateconfig.h"
#include "katejscript.h"

#include <kactionmenu.h>

class KateDocument;
class KateIndentJScript;

/**
 * This widget will be embedded into a modal dialog when clicking
 * the "Configure..." button in the indentation config page.
 * To add a config page for an indenter there are several todos:
 * - Derive a class from this class. This widget will be embedded into
 *   the config dialog.
 * - Override the slot @p apply(), which is called when the configuration
 *   needs to be saved.
 * - Override @p KateAutoIndent::configPage() to return an instance of
 *   this dialog.
 * - Return @p true in @p KateAutoIndent::hasConfigPage() for the
 *   corresponding indenter id.
 */
class IndenterConfigPage : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Standard constructor
     * @param parent parent widget
     */
    IndenterConfigPage ( QWidget *parent=0 ) : QWidget(parent) {}
    virtual ~IndenterConfigPage () {}

  public Q_SLOTS:
    /**
     * Apply the changes. Save options here, use @p kapp->config() and
     * group [Kate Indenter MyIndenter].
     */
    virtual void apply () = 0;
};

/**
 * Provides Auto-Indent functionality for katepart.
 * This baseclass is a real dummy, does nothing beside remembering the document it belongs too,
 * only to have the object around
 */
class KateAutoIndent : public QObject
{
  Q_OBJECT

  /**
   * Static methods to create and list indention modes
   */
  public:
    /**
     * Create an indenter
     * @param doc document for the indenter
     * @param name indention mode wanted
     * @return created autoindention object
     */
    static KateAutoIndent *createIndenter (KateDocument *doc, const QString &name);

    /**
     * List all possible modes by name
     * @return list of modes
     */
    static QStringList listModes ();

    /**
     * Return the mode name given the mode
     * @param mode mode index
     * @return name for this mode index
     */
    static QString modeName (int mode);

    /**
     * Return the mode description
     * @param mode mode index
     * @return mode index
     */
    static QString modeDescription (int mode);

    /**
     * Maps name -> index
     * @param name mode name
     * @return mode index
     */
    static uint modeNumber (const QString &name);

    /**
     * count of modes
     * @return number of existing modes
     */
    static int modeCount ();

    /**
     * Config page support
     * @param mode mode index
     * @return true, if the indenter @p mode has a configuration page
     */
    static bool hasConfigPage (int mode);

    /**
     * Support for a config page.
     * @return config page or 0 if not available.
     */
    static IndenterConfigPage* configPage(QWidget *parent, int mode);

  public:
    /**
     * Constructor
     * @param doc parent document
     */
    KateAutoIndent (KateDocument *doc);

    /**
     * Virtual Destructor for the baseclass
     */
    virtual ~KateAutoIndent ();

  public Q_SLOTS:
    /**
     * Update indenter's configuration (indention width, attributes etc.)
     */
    virtual void updateConfig () {}

  public:
    /**
     * does this indenter support processNewLine
     * @return can you do it?
     */
    virtual bool canProcessNewLine () const { return false; }

    /**
     * Called every time a newline character is inserted in the document.
     *
     * @param view the current active view
     * @param cur The position to start processing. Contains the new cursor position after the indention.
     * @param needContinue Used to determine whether to calculate a continue indent or not.
     */
    virtual void processNewline (KateView *view, KateDocCursor &cur, bool needContinue)
    { Q_UNUSED(cur); Q_UNUSED(view); Q_UNUSED(needContinue); }

    /**
     * @param view the current active view
     * Called every time a character is inserted into the document.
     * @param c character inserted
     */
    virtual void processChar (KateView *view, QChar c)
    { Q_UNUSED(view); Q_UNUSED(c); }

    /**
     * @param view the current active view
     * Aligns/indents the given line to the proper indent position.
     */
    virtual void processLine (KateView *view, KateDocCursor &line)
    { Q_UNUSED(view); Q_UNUSED(line); }

    /**
     * @param view the current active view
     * Processes a section of text, indenting each line in between.
     */
    virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end)
    { Q_UNUSED(view); Q_UNUSED(begin); Q_UNUSED(end); }

    /**
     * Set to true if an actual implementation of 'processLine' is present.
     * This is used to prevent a needless Undo action from being created.
     */
    virtual bool canProcessLine() const { return false; }

    /**
     * @param view the current active view
     * Indents the specified line by the number of levels
     * specified by change.
     */
    virtual void indent ( KateView *view, uint line, int change );

    /**
     * mode name
     */
    virtual QString modeName () { return QString (""); }

  protected:
    KateDocument *doc;
};

/**
 * This action provides a list of available indenters and gets plugged
 * into the KateView's KActionCollection.
 */
class KateViewIndentationAction : public KActionMenu
{
  Q_OBJECT

  public:
    KateViewIndentationAction(KateDocument *_doc, const QString& text, KActionCollection* parent = 0, const char* name = 0);

    ~KateViewIndentationAction(){;};

  private:
    KateDocument* doc;

  public  Q_SLOTS:
    void slotAboutToShow();

  private Q_SLOTS:
    void setMode (QAction*);
};

/**
 * Provides Auto-Indent functionality for katepart.
 */
class KateNormalIndent : public KateAutoIndent
{
  Q_OBJECT

public:
    /**
     * Constructor
     * @param doc parent document
     */
  KateNormalIndent (KateDocument *doc);

    /**
     * Virtual Destructor for the baseclass
     */
  virtual ~KateNormalIndent ();

public Q_SLOTS:
    /**
     * Update indenter's configuration (indention width, attributes etc.)
     */
  virtual void updateConfig ();

public:
    /**
     * does this indenter support processNewLine
     * @return can you do it?
     */
  virtual bool canProcessNewLine () const { return true; }

    /**
     * Called every time a newline character is inserted in the document.
     *
     * @param begin The position to start processing. Contains the new cursor position after the indention.
     * @param needContinue Used to determine whether to calculate a continue indent or not.
     */
  virtual void processNewline (KateView *view, KateDocCursor &begin, bool needContinue);

    /**
     * Called every time a character is inserted into the document.
     * @param c character inserted
     */
  virtual void processChar (KateView *view, QChar c)
  { Q_UNUSED(view); Q_UNUSED(c); }

    /**
     * Aligns/indents the given line to the proper indent position.
     */
  virtual void processLine (KateView *view, KateDocCursor &line)
  { Q_UNUSED(view); Q_UNUSED(line); }

    /**
     * Processes a section of text, indenting each line in between.
     */
  virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end)
  { Q_UNUSED(view); Q_UNUSED(begin); Q_UNUSED(end); }

    /**
     * Set to true if an actual implementation of 'processLine' is present.
     * This is used to prevent a needless Undo action from being created.
     */
  virtual bool canProcessLine() const { return false; }

    /**
     * Indents the specified line by the number of levels
     * specified by change.
     */
  virtual void indent ( KateView *view, uint line, int change );

    /**
     * mode name
     */
    virtual QString modeName () { return QString ("normal"); }

protected:

    /**
     * Determines if the characters open and close are balanced between @p begin and @p end
     * Fills in @p pos with the column position of first opened character if found.
     *
     * @param begin Beginning cursor position.
     * @param end Ending cursor position where the processing will stop.
     * @param open The open character.
     * @param close The closing character which should be matched against @p open.
     * @param pos Contains the position of the first @p open character in the line.
     * @return True if @p open and @p close have an equal number of occurrences between @p begin and @p end. False otherwise.
     */
  bool isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close, uint &pos) const;

    /**
     * Skip all whitespace starting at @p cur and ending at @p max. Spans lines if @p newline is set.
     * @p cur is set to the current position afterwards.
     *
     * @param cur The current cursor position to start from.
     * @param max The furthest cursor position that will be used for processing
     * @param newline Whether we are allowed to span multiple lines when skipping blanks
     * @return True if @p cur < @p max after processing.  False otherwise.
     */
  bool skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const;

    /**
     * Measures the indention of the current textline marked by cur
     * @param cur The cursor position to measure the indent to.
     * @return The length of the indention in characters.
     */
  uint measureIndent (KateDocCursor &cur) const;

    /**
     * Produces a string with the proper indentation characters for its length.
     *
     * @param length The length of the indention in characters.
     * @return A QString representing @p length characters (factoring in tabs and spaces)
     */
  QString tabString(uint length) const;

  void optimizeLeadingSpace( uint line, int change );

  uint  tabWidth;     //!< The number of characters simulated for a tab
  uint  indentWidth;  //!< The number of characters used when tabs are replaced by spaces

    // Attributes that we should skip over or otherwise know about
  uchar commentAttrib;
  uchar doxyCommentAttrib;
  uchar regionAttrib;
  uchar symbolAttrib;
  uchar alertAttrib;
  uchar tagAttrib;
  uchar wordAttrib;
  uchar keywordAttrib;
  uchar normalAttrib;
  uchar extensionAttrib;

  bool  useSpaces;    //!< Should we use spaces or tabs to indent
  bool  keepProfile;  //!< Always try to honor the leading whitespace of lines already in the file
  bool  keepExtra;    //!< Keep indentation that is not on indentation boundaries
};

class KateCSmartIndent : public KateNormalIndent
{
  Q_OBJECT

  public:
    KateCSmartIndent (KateDocument *doc);
    ~KateCSmartIndent ();

    virtual void processNewline (KateView *view, KateDocCursor &begin, bool needContinue);
    virtual void processChar (KateView *view, QChar c);

    virtual void processLine (KateView *view, KateDocCursor &line);
    virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end);

    virtual bool canProcessLine() const { return true; }

    /**
     * mode name
     */
    virtual QString modeName () { return QString ("cstyle"); }

  private:
    uint calcIndent (KateDocCursor &begin, bool needContinue);
    uint calcContinue (KateDocCursor &begin, KateDocCursor &end);
    uint findOpeningBrace (KateDocCursor &start);
    uint findOpeningParen (KateDocCursor &start);
    uint findOpeningComment (KateDocCursor &start);
    bool firstOpeningBrace (KateDocCursor &start);
    bool handleDoxygen (KateDocCursor &begin);

    bool allowSemi;
    bool processingBlock;
};

class KatePythonIndent : public KateNormalIndent
{
  Q_OBJECT

  public:
    KatePythonIndent (KateDocument *doc);
    ~KatePythonIndent ();

    virtual void processNewline (KateView *view, KateDocCursor &begin, bool needContinue);

    /**
     * mode name
     */
    virtual QString modeName () { return QString ("python"); }

  private:
    int calcExtra (int &prevBlock, int &pos, KateDocCursor &end);

    static QRegExp endWithColon;
    static QRegExp stopStmt;
    static QRegExp blockBegin;
};

class KateXmlIndent : public KateNormalIndent
{
  Q_OBJECT

  public:
    KateXmlIndent (KateDocument *doc);
    ~KateXmlIndent ();

    virtual void processNewline (KateView *view, KateDocCursor &begin, bool needContinue);
    virtual void processChar (KateView *view, QChar c);
    virtual void processLine (KateView *view, KateDocCursor &line);
    virtual bool canProcessLine() const { return true; }
    virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end);

    /**
     * mode name
     */
    virtual QString modeName () { return QString ("xml"); }

  private:
    // sets the indentation of a single line based on previous line
    //  (returns indentation width)
    uint processLine (uint line);

    // gets information about a line
    void getLineInfo (uint line, uint &prevIndent, int &numTags,
      uint &attrCol, bool &unclosedTag);

    // useful regular expressions
    static const QRegExp startsWithCloseTag;
    static const QRegExp unclosedDoctype;
};

class KateCSAndSIndent : public KateNormalIndent
{
  Q_OBJECT

  public:
    KateCSAndSIndent (KateDocument *doc);
    ~KateCSAndSIndent ();

    virtual void processNewline (KateView *view, KateDocCursor &begin, bool needContinue);
    virtual void processChar (KateView *view, QChar c);

    virtual void processLine (KateView *view, KateDocCursor &line);
    virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end);

    virtual bool canProcessLine() const { return true; }

    /**
     * mode name
     */
    virtual QString modeName () { return QString ("csands"); }

  private:
    void updateIndentString();

    bool inForStatement( int line );
    int lastNonCommentChar( const KateDocCursor &line );
    bool startsWithLabel( int line );
    bool inStatement( const KateDocCursor &begin );
    QString continuationIndent( const KateDocCursor &begin );

    QString calcIndent (const KateDocCursor &begin);
    QString calcIndentAfterKeyword(const KateDocCursor &indentCursor, const KateDocCursor &keywordCursor, int keywordPos, bool blockKeyword);
    QString calcIndentInBracket(const KateDocCursor &indentCursor, const KateDocCursor &bracketCursor, int bracketPos);
    QString calcIndentInBrace(const KateDocCursor &indentCursor, const KateDocCursor &braceCursor, int bracePos);

    bool handleDoxygen (KateDocCursor &begin);
    QString findOpeningCommentIndentation (const KateDocCursor &start);

    QString indentString;
};

/**
 * This indenter uses document variables to determine when to add/remove indents.
 *
 * It attempts to get the following variables from the document:
 * - var-indent-indent-after: A rerular expression which will cause a line to
 *   be indented by one unit, if the first non-whitespace-only line above matches.
 * - var-indent-indent: A regular expression, which will cause a matching line
 *   to be indented by one unit.
 * - var-indent-unindent: A regular expression which will cause the line to be
 *   unindented by one unit if matching.
 * - var-indent-triggerchars: a list of characters that should cause the
 *   indentiou to be recalculated immediately when typed.
 * - var-indent-handle-couples: a list of paren sets to handle. Any combination
 *   of 'parens' 'braces' and 'brackets'. Each set type is handled
 *   the following way: If there are unmatched opening instances on the above line,
 *   one indent unit is added, if there are unmatched closing instances on the
 *   current line, one indent unit is removed.
 * - var-indent-couple-attribute: When looking for unmatched couple openings/closings,
 *   only characters with this attribute is considered. The value must be the
 *   attribute name from the syntax xml file, for example "Symbol". If it's not
 *   specified, attribute 0 is used (usually 'Normal Text').
 *
 * The idea is to provide a somewhat intelligent indentation for perl, php,
 * bash, scheme and in general formats with humble indentation needs.
 */
class KateVarIndent : public KateNormalIndent
{
  Q_OBJECT

  public:
    /**
     * Purely for readability, couples we know and love
     */
    enum pairs {
      Parens=1,
      Braces=2,
      Brackets=4,
      AngleBrackets=8
    };

    KateVarIndent( KateDocument *doc );
    virtual ~KateVarIndent();

    virtual void processNewline (KateView *view, KateDocCursor &cur, bool needContinue);
    virtual void processChar (KateView *view, QChar c);

    virtual void processLine (KateView *view, KateDocCursor &line);
    virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end);

    virtual bool canProcessLine() const { return true; }

    /**
     * mode name
     */
    virtual QString modeName () { return QString ("varindent"); }

  private Q_SLOTS:
    void slotVariableChanged( KTextEditor::Document*, const QString&, const QString&);

  private:
    /**
     * Check if coupled characters are in balance within one line.
     * @param line the line to check
     * @param open the opening character
     * @param close the closing character
     * @param attrib the attribute the characters must have, defaults to
     *               KateAutoIndent::symbolAttrib
     */
    int coupleBalance( int line, const QChar &open, const QChar &close ) const;

    /**
     * @return true if there is a matching opening with the correct attribute
     * @param end a cursor pointing to the closing character
     */
    bool hasRelevantOpening( const KateDocCursor &end ) const;

    class KateVarIndentPrivate* const d;
};

class KateScriptIndent : public KateNormalIndent
{
  Q_OBJECT

  public:
    KateScriptIndent( KateIndentJScript *script, KateDocument *doc );
    ~KateScriptIndent();

    virtual void processChar( KateView *view, QChar c );

    virtual bool canProcessNewLine() const;
    virtual void processNewline( KateView *view, KateDocCursor &begin, bool needContinue );

    virtual bool canProcessLine() const;
    virtual void processLine (KateView *view, KateDocCursor &line);
    virtual void processSection (KateView *view, const KateDocCursor &begin, const KateDocCursor &end);

    virtual void indent( KateView *view, uint line, int levels );

    // TODO: return sth. like m_script->internalName(); (which is the filename)
    virtual QString modeName () { return QString ("scriptindent"); }

  protected:
    bool canProcessIndent() const;

  private:
    KateIndentJScript *m_script;
    mutable bool m_canProcessNewLineSet : 1;
    mutable bool m_canProcessNewLine : 1;
    mutable bool m_canProcessLineSet : 1;
    mutable bool m_canProcessLine : 1;
    mutable bool m_canProcessIndentSet : 1;
    mutable bool m_canProcessIndent : 1;
};

class ScriptIndentConfigPage : public IndenterConfigPage
{
  Q_OBJECT

  public:
    ScriptIndentConfigPage ( QWidget *parent=0 );
    virtual ~ScriptIndentConfigPage ();

  public Q_SLOTS:
    /**
     * Apply changes.
     */
    virtual void apply ();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
