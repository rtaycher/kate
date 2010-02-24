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
#include <QtCore/QList>
#include <QtCore/QSet>

#include "katetextblock.h"
#include "katetextcursor.h"
#include "katetextrange.h"

namespace Kate {

/**
 * Class representing a text buffer.
 * The interface is line based, internally the text will be stored in blocks of text lines.
 */
class TextBuffer : public QObject {
  Q_OBJECT

  public:
    /**
     * Construct an empty text buffer.
     * Empty means one empty line in one block.
     * @param parent parent qobject
     */
    TextBuffer (QObject *parent = 0);

    /**
     * Destruct the text buffer
     */
    ~TextBuffer ();

    /**
     * Clears the buffer, reverts to initial empty state.
     * Empty means one empty line in one block.
     */
    void clear ();

    /**
     * Load the given file. This will first clear the buffer and then load the file.
     * Even on error during loading the buffer will still be cleared.
     * @param filename file to open
     * @return success
     */
    bool load (const QString &filename);

    /**
     * Save the current buffer content to the given file.
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
     */
    void startEditing ();

    /**
     * Finish an editing transaction. Only allowed to be called if editing transaction is started.
     */
    void finishEditing ();

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
     * Insert text at given cursor position.
     * @param position position where to insert text
     * @param text text to insert
     */
    void insertText (const KTextEditor::Cursor &position, const QString &text);

    /**
     * Remove text at given range.
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

  public:
    /**
     * Debug output, print whole buffer content with line numbers and line length
     * @param title title for this output
     */
    void debugPrint (const QString &title) const;

  private:
    /**
     * List of blocks which contain the lines of this buffer
     */
    QList<TextBlock *> m_blocks;

    /**
     * Number of lines in buffer
     */
    int m_lines;

    /**
     * Current number of running edit transactions
     */
    int m_editingTransactions;

    /**
     * Set of invalid cursors for this whole buffer.
     * Valid cursors are inside the block the belong to.
     */
    QSet<TextCursor *> m_invalidCursors;

    /**
     * Set of ranges of this whole buffer.
     */
    QSet<TextRange *> m_ranges;
};

}

#endif
