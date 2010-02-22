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

#include "katetextblock.h"

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
     * Lines currently stored in this buffer.
     * This is never 0.
     */
    int lines () const { return m_lines; }

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

  Q_SIGNALS:
    /**
     * Buffer got cleared. This is emited when constructor or load have called clear() internally,
     * or when the user of the buffer has called clear() itself.
     * @param buffer buffer which got cleared
     */
    void cleared (TextBuffer *buffer);

  private:
    /**
     * List of blocks which contain the lines of this buffer
     */
    QList<TextBlock> m_blocks;

    /**
     * Number of lines in buffer
     */
    int m_lines;
};

}

#endif
