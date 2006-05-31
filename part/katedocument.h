/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#ifndef _KATE_DOCUMENT_H_
#define _KATE_DOCUMENT_H_

#include "katetextline.h"

#include <ktexteditor/document.h>
#include <ktexteditor/sessionconfiginterface.h>
#include <ktexteditor/searchinterface.h>
#include <ktexteditor/highlightinginterface.h>
#include <ktexteditor/markinterface.h>
#include <ktexteditor/variableinterface.h>
#include <ktexteditor/modificationinterface.h>
#include <ktexteditor/smartinterface.h>
#include <ktexteditor/rangefeedback.h>

#include <kmimetype.h>
#include <klocale.h>
#include <kshortcut.h>

#include <qlinkedlist.h>
#include <qmap.h>
#include <qdatetime.h>
#include <QClipboard>

namespace KTextEditor { class Plugin; }

namespace KIO { class TransferJob; }

class KateUndoGroup;
class KateCmd;
class KTextEditor::Attribute;
class KateAutoIndent;
class KateCodeFoldingTree;
class KateBuffer;
class KateView;
class KateSmartRange;
class KateLineInfo;
class KateBrowserExtension;
class KateDocumentConfig;
class KateHighlighting;
class KatePartPluginItem;
class KatePartPluginInfo;
class KateSmartManager;

class KTempFile;

class QTimer;

class KateKeyInterceptorFunctor;

//
// Kate KTextEditor::Document class (and even KTextEditor::Editor ;)
//
class KateDocument : public KTextEditor::Document,
                     public KTextEditor::SessionConfigInterface,
                     public KTextEditor::SearchInterface,
                     public KTextEditor::HighlightingInterface,
                     public KTextEditor::MarkInterface,
                     public KTextEditor::VariableInterface,
                     public KTextEditor::ModificationInterface,
                     public KTextEditor::SmartInterface,
                     private KTextEditor::SmartRangeWatcher
{
  Q_OBJECT
  Q_INTERFACES(KTextEditor::SessionConfigInterface)
  Q_INTERFACES(KTextEditor::SearchInterface)
  Q_INTERFACES(KTextEditor::HighlightingInterface)
  Q_INTERFACES(KTextEditor::MarkInterface)
  Q_INTERFACES(KTextEditor::VariableInterface)
  Q_INTERFACES(KTextEditor::ModificationInterface)
  Q_INTERFACES(KTextEditor::SmartInterface)

  public:
    KateDocument (bool bSingleViewMode=false, bool bBrowserView=false, bool bReadOnly=false,
                  QWidget *parentWidget = 0, QObject * = 0);
    ~KateDocument ();

    bool closeURL();

    KTextEditor::Editor *editor ();

  //
  // Plugins section
  //
  public:
    void unloadAllPlugins ();

    void enableAllPluginsGUI (KateView *view);
    void disableAllPluginsGUI (KateView *view);

    void loadPlugin (uint pluginIndex);
    void unloadPlugin (uint pluginIndex);

    void enablePluginGUI (KTextEditor::Plugin *plugin, KateView *view);
    void enablePluginGUI (KTextEditor::Plugin *plugin);

    void disablePluginGUI (KTextEditor::Plugin *plugin, KateView *view);
    void disablePluginGUI (KTextEditor::Plugin *plugin);

  private:
     QVector<KTextEditor::Plugin *> m_plugins;

  public:
    bool readOnly () const { return m_bReadOnly; }
    bool browserView () const { return m_bBrowserView; }
    bool singleViewMode () const { return m_bSingleViewMode; }
    KateBrowserExtension *browserExtension () { return m_extension; }

  private:
    // only to make part work, don't change it !
    bool m_bSingleViewMode;
    bool m_bBrowserView;
    bool m_bReadOnly;
    KateBrowserExtension *m_extension;

  //
  // KTextEditor::Document stuff
  //
  public:
    KDocument::View *createView( QWidget *parent );
    const QList<KDocument::View*> &views () const;

    virtual KTextEditor::View* activeView() const { return m_activeView; }
    // Invalid covariant returns my a$$... for some reason gcc won't let me return a KateView above!
    KateView* activeKateView() const;

  Q_SIGNALS:
    void activeViewSelectionChanged(KTextEditor::View* view);

  private:
    QLinkedList<KateView*> m_views;
    QList<KDocument::View*> m_textEditViews;
    KTextEditor::View *m_activeView;

  //
  // KTextEditor::EditInterface stuff
  //
  public Q_SLOTS:
    virtual QString text() const;

    virtual QString text ( const KTextEditor::Range &range, bool blockwise = false ) const;
    virtual QStringList textLines ( const KTextEditor::Range& range, bool block = false ) const;

    virtual QString line ( int line ) const;

    virtual QChar character( const KTextEditor::Cursor& position ) const;

    virtual bool setText(const QString &);
    virtual bool setText(const QStringList& text);
    virtual bool clear ();

    virtual bool insertText ( const KTextEditor::Cursor &position, const QString &s, bool block = false );
    virtual bool insertText ( const KTextEditor::Cursor &position, const QStringList &text, bool block = false );

    virtual bool insertLine ( int line, const QString &s );
    virtual bool insertLines ( int line, const QStringList &s );

    bool removeText ( const KTextEditor::Range &range, bool block = false );
    bool removeLine ( int line );

    bool replaceText ( const KTextEditor::Range &range, const QString &s, bool block = false );

    int lines() const;
    virtual KTextEditor::Cursor documentEnd() const;
    int numVisLines() const;
    int totalCharacters() const;
    int lineLength ( int line ) const;

  Q_SIGNALS:
    void charactersSemiInteractivelyInserted(int ,int ,const QString&);

  public:
//BEGIN editStart/editEnd (start, end, undo, cursor update, view update)
    /**
     * Enclose editor actions with @p editStart() and @p editEnd() to group
     * them.
     * @param withUndo if true, add undo history
     */
    void editStart (bool withUndo = true, KTextEditor::View *view = 0);
    /** Same as editStart() with undo */
    void editBegin (KTextEditor::View *view = 0) { editStart(true, view); }
    /**
     * End a editor operation.
     * @see editStart()
     */
    void editEnd ();

    bool startEditing (KTextEditor::View *view = 0) { editStart (true, view); return true; }
    bool endEditing () { editEnd (); return true; }

//END editStart/editEnd

//BEGIN LINE BASED INSERT/REMOVE STUFF (editStart() and editEnd() included)
    /**
     * Add a string in the given line/column
     * @param line line number
     * @param col column
     * @param s string to be inserted
     * @return true on success
     */
    bool editInsertText ( int line, int col, const QString &s );
    /**
     * Remove a string in the given line/column
     * @param line line number
     * @param col column
     * @param len length of text to be removed
     * @return true on success
     */
    bool editRemoveText ( int line, int col, int len );

    /**
     * Mark @p line as @p autowrapped. This is necessary if static word warp is
     * enabled, because we have to know whether to insert a new line or add the
     * wrapped words to the followin line.
     * @param line line number
     * @param autowrapped autowrapped?
     * @return true on success
     */
    bool editMarkLineAutoWrapped ( int line, bool autowrapped );

    /**
     * Wrap @p line. If @p newLine is true, ignore the textline's flag
     * KateTextLine::flagAutoWrapped and force a new line. Whether a new line
     * was needed/added you can grab with @p newLineAdded.
     * @param line line number
     * @param col column
     * @param newLine if true, force a new line
     * @param newLineAdded return value is true, if new line was added (may be 0)
     * @return true on success
     */
    bool editWrapLine ( int line, int col, bool newLine = true, bool *newLineAdded = 0 );
    /**
     * Unwrap @p line. If @p removeLine is true, we force to join the lines. If
     * @p removeLine is true, @p length is ignored (eg not needed).
     * @param line line number
     * @param removeLine if true, force to remove the next line
     * @return true on success
     */
    bool editUnWrapLine ( int line, bool removeLine = true, int length = 0 );

    /**
     * Insert a string at the given line.
     * @param line line number
     * @param s string to insert
     * @return true on success
     */
    bool editInsertLine ( int line, const QString &s );
    /**
     * Remove a line
     * @param line line number
     * @return true on success
     */
    bool editRemoveLine ( int line );

    /**
     * Remove a line
     * @param startLine line to begin wrapping
     * @param endLine line to stop wrapping
     * @return true on success
     */
    bool wrapText (int startLine, int endLine);
//END LINE BASED INSERT/REMOVE STUFF

  Q_SIGNALS:
    /**
     * Emmitted when text from @p line was wrapped at position pos onto line @p nextLine.
     */
    void editLineWrapped ( int line, int col, int len );

    /**
     * Emitted each time text from @p nextLine was upwrapped onto @p line.
     */
    void editLineUnWrapped ( int line, int col );

  private:
    void undoStart();
    void undoEnd();

  public:
    void undoSafePoint();

    bool undoDontMerge() const;
    void setUndoDontMerge(bool dontMerge);

    bool undoDontMergeComplex() const;
    void setUndoDontMergeComplex(bool dontMerge);

    bool isEditRunning() const;

  private Q_SLOTS:
    void undoCancel();

  private:
    void editAddUndo (int type, uint line, uint col, uint len, const QString &text);

    uint editSessionNumber;
    bool editIsRunning;
    bool editWithUndo;
    KateView *editView;
    bool m_undoComplexMerge;
    KateUndoGroup* m_editCurrentUndo;

  //
  // KTextEditor::UndoInterface stuff
  //
  public Q_SLOTS:
    void undo ();
    void redo ();
    void clearUndo ();
    void clearRedo ();

    uint undoCount () const;
    uint redoCount () const;

    uint undoSteps () const;
    void setUndoSteps ( uint steps );

  public:
    class KateEditHistory* history() const { return m_editHistory; }

  private:
    KateEditHistory* m_editHistory;

    //
    // some internals for undo/redo
    //
    QList<KateUndoGroup*> undoItems;
    QList<KateUndoGroup*> redoItems;
    bool m_undoDontMerge; //create a setter later on and remove the friend declaration
    bool m_undoIgnoreCancel;
    QTimer* m_undoMergeTimer;
    // these two variables are for resetting the document to
    // non-modified if all changes have been undone...
    KateUndoGroup* lastUndoGroupWhenSaved;
    bool docWasSavedWhenUndoWasEmpty;

    // this sets
    void updateModified();

  Q_SIGNALS:
    void undoChanged ();
    void textInserted(int line,int column);

  //
  // KTextEditor::SearchInterface stuff
  //
  public Q_SLOTS:
    KTextEditor::Range searchText (const KTextEditor::Cursor& startPosition,
        const QString &text, bool casesensitive = true, bool backwards = false);
    KTextEditor::Range searchText (const KTextEditor::Cursor& startPosition,
        const QRegExp &regexp, bool backwards = false);

  //
  // KTextEditor::HighlightingInterface stuff
  //
  public Q_SLOTS:
    uint hlMode ();
    bool setHlMode (uint mode);
    uint hlModeCount ();
    QString hlModeName (uint mode);
    QString hlModeSectionName (uint mode);

  public:
    void bufferHlChanged ();

  private:
    void setDontChangeHlOnSave();

  Q_SIGNALS:
    void hlChanged ();

  //
  // KTextEditor::ConfigInterface stuff
  //
  public:
    void readSessionConfig (KConfig *);
    void writeSessionConfig (KConfig *);

  //
  // KTextEditor::MarkInterface
  //
  public Q_SLOTS:
    uint mark( int line );

    void setMark( int line, uint markType );
    void clearMark( int line );

    void addMark( int line, uint markType );
    void removeMark( int line, uint markType );

    const QHash<int, KTextEditor::Mark*> &marks ();
    void clearMarks();

    void setMarkPixmap( MarkInterface::MarkTypes, const QPixmap& );
    QPixmap markPixmap( MarkInterface::MarkTypes ) const;

    void setMarkDescription( MarkInterface::MarkTypes, const QString& );
    QString markDescription( MarkInterface::MarkTypes ) const;
    QColor markColor( MarkInterface::MarkTypes ) const;

    void setEditableMarks( uint markMask );
    uint editableMarks() const;

  Q_SIGNALS:
    void marksChanged( KTextEditor::Document* );
    void markChanged( KTextEditor::Document*, KTextEditor::Mark, KTextEditor::MarkInterface::MarkChangeAction );

  private:
    QHash<int, KTextEditor::Mark*> m_marks;
    QHash<int,QPixmap>           m_markPixmaps;
    QHash<int,QString>           m_markDescriptions;
    uint                        m_editableMarks;

  //
  // KTextEditor::PrintInterface
  //
  public Q_SLOTS:
    bool printDialog ();
    bool print ();

  //
  // KTextEditor::DocumentInfoInterface ( ### unfinished )
  //
  public:
    /**
     * @return the name of the mimetype for the document.
     *
     * This method is using KMimeType::findByURL, and if the pointer
     * is then still the default MimeType for a nonlocal or unsaved file,
     * uses mimeTypeForContent().
     */
    QString mimeType();

    /**
     * @return a pointer to the KMimeType for this document, found by analyzing the
     * actual content.
     *
     * Note that this method is *not* part of the DocumentInfoInterface.
     */
    KMimeType::Ptr mimeTypeForContent();

  //
  // KTextEditor::VariableInterface
  //
  public:
    QString variable( const QString &name ) const;

  Q_SIGNALS:
    void variableChanged( KTextEditor::Document*, const QString &, const QString & );

  private:
    QMap<QString, QString> m_storedVariables;

  //
  // KTextEditor::SmartInterface
  //
  public:
    virtual void clearSmartInterface();

    virtual KTextEditor::SmartCursor* newSmartCursor(const KTextEditor::Cursor& position, KTextEditor::SmartCursor::InsertBehaviour insertBehaviour = KTextEditor::SmartCursor::MoveOnInsert);
    virtual void deleteCursors();

    virtual KTextEditor::SmartRange* newSmartRange(const KTextEditor::Range& range, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::DoNotExpand);
    virtual KTextEditor::SmartRange* newSmartRange(KTextEditor::SmartCursor* start, KTextEditor::SmartCursor* end, KTextEditor::SmartRange* parent = 0L, KTextEditor::SmartRange::InsertBehaviors insertBehavior = KTextEditor::SmartRange::DoNotExpand);
    virtual void unbindSmartRange(KTextEditor::SmartRange* range);
    virtual void deleteRanges();

    // Syntax highlighting extension
    virtual void addHighlightToDocument(KTextEditor::SmartRange* topRange, bool supportDynamic);
    virtual const QList<KTextEditor::SmartRange*> documentHighlights() const;
    virtual void clearDocumentHighlights();

    virtual void addHighlightToView(KTextEditor::View* view, KTextEditor::SmartRange* topRange, bool supportDynamic);
    virtual void removeHighlightFromView(KTextEditor::View* view, KTextEditor::SmartRange* topRange);
    virtual const QList<KTextEditor::SmartRange*> viewHighlights(KTextEditor::View* view) const;
    virtual void clearViewHighlights(KTextEditor::View* view);

    // Action association extension
    virtual void addActionsToDocument(KTextEditor::SmartRange* topRange);
    virtual const QList<KTextEditor::SmartRange*> documentActions() const;
    virtual void clearDocumentActions();

    virtual void addActionsToView(KTextEditor::View* view, KTextEditor::SmartRange* topRange);
    virtual void removeActionsFromView(KTextEditor::View* view, KTextEditor::SmartRange* topRange);
    virtual const QList<KTextEditor::SmartRange*> viewActions(KTextEditor::View* view) const;
    virtual void clearViewActions(KTextEditor::View* view);

    KateSmartManager* smartManager() const { return m_smartManager; }

  Q_SIGNALS:
    void dynamicHighlightAdded(KateSmartRange* range);
    void dynamicHighlightRemoved(KateSmartRange* range);

  public Q_SLOTS:
    virtual void removeHighlightFromDocument(KTextEditor::SmartRange* topRange);
    virtual void removeActionsFromDocument(KTextEditor::SmartRange* topRange);

  protected:
    virtual void attributeDynamic(KTextEditor::Attribute::Ptr a);
    virtual void attributeNotDynamic(KTextEditor::Attribute::Ptr a);

  private:
    // Smart range watcher overrides
    virtual void rangeDeleted(KTextEditor::SmartRange* range);

    KateSmartManager* m_smartManager;
    QList<KTextEditor::SmartRange*> m_documentHighlights;
    QList<KTextEditor::SmartRange*> m_documentDynamicHighlights;
    QList<KTextEditor::SmartRange*> m_documentActions;

  //
  // KParts::ReadWrite stuff
  //
  public:
    bool openURL( const KUrl &url );

    /* Anders:
      I reimplemented this, since i need to check if backup succeeded
      if requested */
    bool save();

    /* Anders: Reimplemented to do kate specific stuff */
    bool saveAs( const KUrl &url );

    bool openFile (KIO::Job * job);
    bool openFile ();

    bool saveFile ();

    void setReadWrite ( bool rw = true );

    void setModified( bool m );

  private Q_SLOTS:
    void slotDataKate ( KIO::Job* kio_job, const QByteArray &data );
    void slotFinishedKate ( KJob * job );

  private:
    void abortLoadKate();

    void activateDirWatch ();
    void deactivateDirWatch ();

    QString m_dirWatchFile;

  public:
    /**
     * Type chars in a view
     */
    bool typeChars ( KateView *type, const QString &chars );

    /**
     * gets the last line number (numLines() -1)
     */
    inline int lastLine() const { return lines()-1; }

    // Repaint all of all of the views
    void repaintViews(bool paintOnlyDirty = true);

    KateHighlighting *highlight () const;

  public Q_SLOTS:    //please keep prototypes and implementations in same order
    void tagLines(int start, int end);
    void tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end);

  //export feature, obsolute
  public Q_SLOTS:
     void exportAs(const QString&) { };

  Q_SIGNALS:
    void preHighlightChanged(uint);

  private Q_SLOTS:
    void internalHlChanged();

  public:
    void addView(KTextEditor::View *);
    void removeView(KTextEditor::View *);
    void setActiveView(KTextEditor::View*);

    bool ownedView(KateView *);

    uint currentColumn( const KTextEditor::Cursor& );
    void newLine(             KTextEditor::Cursor&, KateView* ); // Changes input
    void backspace(     KateView *view, const KTextEditor::Cursor& );
    void del(           KateView *view, const KTextEditor::Cursor& );
    void transpose(     const KTextEditor::Cursor& );

    void paste ( KateView* view, QClipboard::Mode = QClipboard::Clipboard );

  public:
    void indent ( KateView *view, uint line, int change );
    void comment ( KateView *view, uint line, uint column, int change );
    void align ( KateView *view, uint line );

    enum TextTransform { Uppercase, Lowercase, Capitalize };

    /**
      Handling uppercase, lowercase and capitalize for the view.

      If there is a selection, that is transformed, otherwise for uppercase or
      lowercase the character right of the cursor is transformed, for capitalize
      the word under the cursor is transformed.
    */
    void transform ( KateView *view, const KTextEditor::Cursor &, TextTransform );
    /**
      Unwrap a range of lines.
    */
    void joinLines( uint first, uint last );

  private:
    bool removeStringFromBegining(int line, QString &str);
    bool removeStringFromEnd(int line, QString &str);

    /**
      Find the position (line and col) of the next char
      that is not a space. If found line and col point to the found character.
      Otherwise they have both the value -1.
      @param line Line of the character which is examined first.
      @param col Column of the character which is examined first.
      @return True if the specified or a following character is not a space
               Otherwise false.
    */
    bool nextNonSpaceCharPos(int &line, int &col);

    /**
      Find the position (line and col) of the previous char
      that is not a space. If found line and col point to the found character.
      Otherwise they have both the value -1.
      @return True if the specified or a preceding character is not a space.
               Otherwise false.
    */
    bool previousNonSpaceCharPos(int &line, int &col);

    /**
    * Sets a comment marker as defined by the language providing the attribute
    * @p attrib on the line @p line
    */
    void addStartLineCommentToSingleLine(int line, int attrib=0);
    /**
    * Removes a comment marker as defined by the language providing the attribute
    * @p attrib on the line @p line
    */
    bool removeStartLineCommentFromSingleLine(int line, int attrib=0);

    /**
    * @see addStartLineCommentToSingleLine.
    */
    void addStartStopCommentToSingleLine(int line, int attrib=0);
    /**
    *@see removeStartLineCommentFromSingleLine.
    */
    bool removeStartStopCommentFromSingleLine(int line, int attrib=0);
    /**
    *@see removeStartLineCommentFromSingleLine.
    */
    bool removeStartStopCommentFromRegion(const KTextEditor::Cursor &start, const KTextEditor::Cursor &end, int attrib=0);

    /**
     * Add a comment marker as defined by the language providing the attribute
     * @p attrib to each line in the selection.
     */
           void addStartStopCommentToSelection( KateView *view, int attrib=0 );
    /**
    * @see addStartStopCommentToSelection.
    */
          void addStartLineCommentToSelection( KateView *view, int attrib=0 );

    /**
    * Removes comment markers relevant to the language providing
    * the attribuge @p attrib from each line in the selection.
    *
    * @return whether the operation succeded.
    */
    bool removeStartStopCommentFromSelection( KateView *view, int attrib=0 );
    /**
    * @see removeStartStopCommentFromSelection.
    */
    bool removeStartLineCommentFromSelection( KateView *view, int attrib=0 );

  public:
    QString getWord( const KTextEditor::Cursor& cursor );

  public:
    void tagAll();

    void newBracketMark( const KTextEditor::Cursor& start, KTextEditor::Range& bm, int maxLines = -1 );
    bool findMatchingBracket( KTextEditor::Range& range, int maxLines = -1 );

  private:
    void guiActivateEvent( KParts::GUIActivateEvent *ev );

  public:
    const QString &documentName () const { return m_docName; }

    void setDocName (QString docName);

    void lineInfo (KateLineInfo *info, unsigned int line);

    KateCodeFoldingTree *foldingTree ();

  public:
    /**
     * @return wheather the document is modified on disc since last saved.
     */
    bool isModifiedOnDisc() { return m_modOnHd; };

    void setModifiedOnDisk( ModifiedOnDiskReason reason );

    void setModifiedOnDiskWarning ( bool on );

  public Q_SLOTS:
    /**
     * Ask the user what to do, if the file has been modified on disc.
     * Reimplemented from KTextEditor::Document.
     */
    void slotModifiedOnDisk( KTextEditor::View *v = 0 );

    /**
     * Reloads the current document from disc if possible
     */
    bool documentReload ();

    bool documentSave ();
    bool documentSaveAs ();

  Q_SIGNALS:
    /**
     * Indicate this file is modified on disk
     * @param doc the KTextEditor::Document object that represents the file on disk
     * @param isModified indicates the file was modified rather than created or deleted
     * @param reason the reason we are emitting the signal.
     */
    void modifiedOnDisk (KTextEditor::Document *doc, bool isModified, KTextEditor::ModificationInterface::ModifiedOnDiskReason reason);

  public:
    void ignoreModifiedOnDiskOnce();

  private:
    int m_isasking; // don't reenter slotModifiedOnDisk when this is true
                    // -1: ignore once, 0: false, 1: true

  public:
    bool setEncoding (const QString &e);
    const QString &encoding() const;

  public Q_SLOTS:
    void setWordWrap (bool on);
    bool wordWrap ();

    void setWordWrapAt (uint col);
    uint wordWrapAt ();

  public Q_SLOTS:
    void setPageUpDownMovesCursor(bool on);
    bool pageUpDownMovesCursor();

   // code folding
  public:
    uint getRealLine(unsigned int virtualLine);
    uint getVirtualLine(unsigned int realLine);
    uint visibleLines ();
    KateTextLine::Ptr kateTextLine(uint i);
    KateTextLine::Ptr plainKateTextLine(uint i);

  Q_SIGNALS:
    void codeFoldingUpdated();
    void aboutToRemoveText(const KTextEditor::Range&);
    void textRemoved();

  private Q_SLOTS:
    void slotModOnHdDirty (const QString &path);
    void slotModOnHdCreated (const QString &path);
    void slotModOnHdDeleted (const QString &path);

  private:
    /**
     * create a MD5 digest of the file, if it is a local file,
     * and fill it into the string @p result.
     * This is using KMD5::hexDigest().
     *
     * @return wheather the operation was attempted and succeded.
     */
    bool createDigest ( QByteArray &result );

    /**
     * create a string for the modonhd warnings, giving the reason.
     */
    QString reasonedMOHString() const;

    /**
     * Removes all trailing whitespace form @p line, if
     * the cfRemoveTrailingDyn confg flag is set,
     * and the active view cursor is not on line and behind
     * the last nonspace character.
     */
    void removeTrailingSpace( int line );

  public:
    void updateFileType (int newType, bool user = false);

    int fileType () const { return m_fileType; };

  //
  // REALLY internal data ;)
  //
  private:
    // text buffer
    KateBuffer *m_buffer;

    KateAutoIndent *m_indenter;

    bool hlSetByUser;

    bool m_modOnHd;
    ModifiedOnDiskReason m_modOnHdReason;
    QByteArray m_digest; // MD5 digest, updated on load/save

    QString m_docName;
    int m_docNameNumber;

    // file type !!!
    int m_fileType;
    bool m_fileTypeSetByUser;

    /**
     * document is still reloading a file
     */
    bool m_reloading;

  public Q_SLOTS:
    void slotQueryClose_save(bool *handled, bool* abortClosing);

  public:
    void makeAttribs (bool needInvalidate = true);

    static bool checkOverwrite( KUrl u );

  /**
   * Configuration
   */
  public:
    inline KateDocumentConfig *config () { return m_config; };

    void updateConfig ();

  private:
    KateDocumentConfig *m_config;

  /**
   * Variable Reader
   * TODO add register functionality/ktexteditor interface
   */
  private:
    /**
     * read dir config file
     */
    void readDirConfig ();

    /**
      Reads all the variables in the document.
      Called when opening/saving a document
    */
    void readVariables(bool onlyViewAndRenderer = false);

    /**
      Reads and applies the variables in a single line
      TODO registered variables gets saved in a [map]
    */
    void readVariableLine( QString t, bool onlyViewAndRenderer = false );
    /**
      Sets a view variable in all the views.
    */
    void setViewVariable( QString var, QString val );
    /**
      @return weather a string value could be converted
      to a bool value as supported.
      The value is put in *result.
    */
    static bool checkBoolValue( QString value, bool *result );
    /**
      @return weather a string value could be converted
      to a integer value.
      The value is put in *result.
    */
    static bool checkIntValue( QString value, int *result );
    /**
      Feeds value into @p col using QColor::setNamedColor() and returns
      wheather the color is valid
    */
    static bool checkColorValue( QString value, QColor &col );

    static QRegExp kvLine;
    static QRegExp kvVar;

    KIO::TransferJob *m_job;
    KTempFile *m_tempFile;

    bool s_fileChangedDialogsActivated;

  // TemplateInterface
  public:
      bool setTabInterceptor(KateKeyInterceptorFunctor *interceptor); /* perhaps make it moregeneral like an eventfilter*/
      bool removeTabInterceptor(KateKeyInterceptorFunctor *interceptor);
      bool invokeTabInterceptor(int key);
      bool insertTemplateTextImplementation ( const KTextEditor::Cursor &c, const QString &templateString, const QMap<QString,QString> &initialValues, QWidget *); //PORT ME

  protected:
      KateKeyInterceptorFunctor *m_tabInterceptor;

  protected Q_SLOTS:
      //void testTemplateCode();
      void dumpRegionTree();
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

