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

#ifndef KATE_TEXTCURSOR_H
#define KATE_TEXTCURSOR_H

#include <ktexteditor/cursor.h>

namespace Kate {

class TextBuffer;
class TextRange;

/**
 * Class representing a text cursor.
 */
class TextCursor : public KTextEditor::Cursor {
  public:
    /**
     * Construct a text cursor.
     * @param parent parent text buffer
     * @param range range this cursor should belong to, if any
     */
    TextCursor (TextBuffer &parent, TextRange *range);

    /**
     * Destruct the text block
     */
    ~TextCursor ();

  private:
    /**
     * parent text buffer
     * is a reference, and no pointer, as this must always exist and can't change
     */
    TextBuffer &m_parent;

    /**
     * range this cursor belongs to
     * may be null, then no range owns this cursor
     */
    TextRange *m_range;
};

}

#endif
