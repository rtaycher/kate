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

#include <QtCore/QObject>

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
     * @param parent parent text buffer
     */
    TextBlock (TextBuffer &parent);

    /**
     * Destruct the text block
     */
    ~TextBlock ();

  private:
    /**
     * parent text block
     * is a reference, and no pointer, as this must always exist and can't change
     */
    TextBuffer &m_parent;
};

}
