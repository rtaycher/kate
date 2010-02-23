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

#ifndef KATE_TEXTBLOCK_H
#define KATE_TEXTBLOCK_H

#include <QtCore/QList>

#include "katetextrange.h"
#include "katetextcursor.h"
#include "katetextline.h"

namespace Kate {

class TextBuffer;

/**
 * Class representing a text block.
 * This is used to build up a Kate::TextBuffer.
 */
class TextBlock {
  public:
    /**
     * Construct an empty text block.
     * @param buffer parent text buffer
     * @param startLine start line of this block
     */
    TextBlock (TextBuffer *buffer, int startLine);

    /**
     * Destruct the text block
     */
    ~TextBlock ();

    /**
     * Start line of this block.
     * @return start line of this block
     */
    int startLine () const { return m_startLine; }
    
    /**
     * Set start line of this block.
     * @param startLine new start line of this block
     */
    void setStartLine (int startLine);
    
    /**
     * Number of lines in this block.
     * @return number of lines
     */
    int lines () const { return m_lines.size(); }
    
    /**
     * Wrap line at given cursor position.
     * @param position line/column as cursor where to wrap
     * @param nextBlock next block, if any
     */
    void wrapLine (const KTextEditor::Cursor &position);

    /**
     * Unwrap given line.
     * @param line line to unwrap
     * @param previousBlock previous block, if any, if we unwrap first line in block, we need to have this
     */
    void unwrapLine (int line, TextBlock *previousBlock);

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

  private:
    /**
     * parent text buffer
     */
    TextBuffer *m_buffer;

    /**
     * Lines contained in this buffer. These are shard pointers.
     */
    QList<Kate::TextLine> m_lines;
    
    /**
     * Startline of this block
     */
    int m_startLine;
};

}

#endif
