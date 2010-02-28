/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_TEXTBUFFER_H
#define KATE_TEXTBUFFER_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QSet>
#include <QtCore/QTextCodec>

#include "katetextblock.h"
#include "katetextcursor.h"
#include "katetextrange.h"

namespace Kate {

/**
 * Class representing a text buffer.
 * The interface is line based, internally the text will be stored in blocks of text lines.
 */
class TextBuffer : public QObject {
  friend class TextCursor;
  friend class TextRange;

  Q_OBJECT

  public:
    /**
     * Construct an empty text buffer.
     * Empty means one empty line in one block.
     * @param parent parent qobject
     * @param blockSize block size in lines the buffer should try to hold, default 256 lines
     */
    TextBuffer (QObject *parent = 0, int blockSize = 256);

    /**
     * Destruct the text buffer
     */
    virtual ~TextBuffer ();

    /**
     * Clears the buffer, reverts to initial empty state.
     * Empty means one empty line in one block.
     */
    void clear ();

    /**
     * Set codec for this buffer to use for load/save.
     * Loading might overwrite this, if it encounters problems and finds a better codec.
     * @param codec QTextCodec to use for encoding
     */
    void setTextCodec (QTextCodec *codec) { m_textCodec = codec; }

    /**
     * Get codec for this buffer
     * @return currently in use codec of this buffer
     */
    QTextCodec *textCodec () const { return m_textCodec; }

    /**
     * Load the given file. This will first clear the buffer and then load the file.
     * Even on error during loading the buffer will still be cleared.
     * Before calling this, setTextCodec must have been used to set codec!
     * @param filename file to open
     * @return success
     */
    bool load (const QString &filename);

    /**
     * Save the current buffer content to the given file.
     * Before calling this, setTextCodec must have been used to set codec!
     * @param filename file to save
     * @return success
     */
    bool save (const QString &filename);

    /**
     * Lines currently stored in this buffer.
     * This is never 0, even clear will let one empty line remain.
     */
    int lines () const { Q_ASSERT (m_lines > 0); return m_lines; }

    /**
     * Revision of this buffer. Is set to 0 on construction, clear(), load and successfull save.
     * Is incremented on each change to the buffer.
     * @return current revision
     */
    qint64 revision () const { return m_revision; }

    /**
     * Retrieve a text line.
     * @param line wanted line number
     * @return text line
     */
    TextLine line (int line) const;

    /**
     * Retrieve text of complete buffer.
     * @return text for this buffer, lines separated by '\n'
     */
    QString text () const;

    /**
     * Start an editing transaction, the wrapLine/unwrapLine/insertText and removeText functions
     * are only to be allowed to be called inside a editing transaction.
     * Editing transactions can stack. The number startEdit and endEdit calls must match.
     * @return returns true, if no transaction was already running
     */
    bool startEditing ();

    /**
     * Finish an editing transaction. Only allowed to be called if editing transaction is started.
     * @return returns true, if this finished last running transaction
     */
    bool finishEditing ();

    /**
     * Query the number of editing transactions running atm.
     * @return number of running transactions
     */
    int editingTransactions () const { return m_editingTransactions; }

    /**
     * Query information from the last editing transaction: was the content of the buffer changed?
     * @return content of buffer was changed in last transaction?
     */
    bool editingChangedBuffer () const { return m_editingChangedBuffer; }

    /**
     * Wrap line at given cursor position.
     * @param position line/column as cursor where to wrap
     */
    void wrapLine (const KTextEditor::Cursor &position);

    /**
     * Unwrap given line.
     * @param line line to unwrap
     */
    void unwrapLine (int line);

    /**
     * Insert text at given cursor position. Does nothing if text is empty, beside some consistency checks.
     * @param position position where to insert text
     * @param text text to insert
     */
    void insertText (const KTextEditor::Cursor &position, const QString &text);

    /**
     * Remove text at given range. Does nothing if range is empty, beside some consistency checks.
     * @param range range of text to remove, must be on one line only.
     */
    void removeText (const KTextEditor::Range &range);

  Q_SIGNALS:
    /**
     * Buffer got cleared. This is emited when constructor or load have called clear() internally,
     * or when the user of the buffer has called clear() itself.
     * @param buffer buffer which got cleared
     */
    void cleared (TextBuffer *buffer);

    /**
     * Editing transaction has started.
     * @param buffer buffer for transaction
     */
    void editingStarted (TextBuffer *buffer);

    /**
     * Editing transaction has finished.
     * @param buffer buffer for transaction
     */
    void editingFinished (TextBuffer *buffer);

    /**
     * A line got wrapped.
     * @param buffer buffer which contains the line
     * @param position position where the wrap occured
     */
    void lineWrapped (TextBuffer *buffer, const KTextEditor::Cursor &position);

    /**
     * A line got unwrapped.
     * @param buffer buffer which contains the line
     * @param line line where the unwrap occured
     */
    void lineUnwrapped (TextBuffer *buffer, int line);

    /**
     * Text got inserted.
     * @param buffer buffer which contains the line
     * @param position position where the insertion occured
     * @param text inserted text
     */
    void textInserted (TextBuffer *buffer, const KTextEditor::Cursor &position, const QString &text);

    /**
     * Text got removed.
     * @param buffer buffer which contains the line
     * @param range range where the removal occured
     * @param text removed text
     */
    void textRemoved (TextBuffer *buffer, const KTextEditor::Range &range, const QString &text);

  private:
    /**
     * Find block containing given line.
     * @param line we want to find block for this line
     * @return index of found block
     */
    int blockForLine (int line) const;

    /**
     * Fix start lines of all blocks after the given one
     * @param startBlock index of block from which we start to fix
     */
    void fixStartLines (int startBlock);

    /**
     * Balance the given block. Look if it is too small or too large.
     * @param index block to balance
     */
    void balanceBlock (int index);

    /**
     * Block for given index in block list.
     * @param index block index
     * @return block matching this index
     */
    TextBlock *blockForIndex (int index) { return m_blocks[index]; }

  public:
    /**
     * Debug output, print whole buffer content with line numbers and line length
     * @param title title for this output
     */
    void debugPrint (const QString &title) const;

  private:
    /**
     * block size in lines the buffer will try to hold
     */
    const int m_blockSize;

    /**
     * List of blocks which contain the lines of this buffer
     */
    QVector<TextBlock *> m_blocks;

    /**
     * Number of lines in buffer
     */
    int m_lines;

    /**
     * Revision of the buffer.
     */
    qint64 m_revision;

    /**
     * Current number of running edit transactions
     */
    int m_editingTransactions;

    /**
     * Did the last running transaction change the buffer?
     */
    bool m_editingChangedBuffer;

    /**
     * Set of invalid cursors for this whole buffer.
     * Valid cursors are inside the block the belong to.
     */
    QSet<TextCursor *> m_invalidCursors;

    /**
     * Set of ranges of this whole buffer.
     */
    QSet<TextRange *> m_ranges;

    /**
     * Text codec to use
     */
    QTextCodec *m_textCodec;
};

}

#endif
