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

#ifndef KATE_TEXTLINE_H
#define KATE_TEXTLINE_H

#include <QtCore/QString>
#include <QtCore/QSharedPointer>

namespace Kate {

/**
 * Class representing a single text line.
 * For efficience reasons, not only pure text is stored here, but also additional data.
 * Will be only accessed over shared pointers.
 */
class TextLineData {
  /**
   * TextBuffer/Block are friend classes, only ones allowed to touch the text content.
   */
  friend class TextBuffer;
  friend class TextBlock;

  public:
    /**
     * Construct an empty text line.
     */
    TextLineData ();

    /**
     * Destruct the text line
     */
    ~TextLineData ();

    /**
     * Accessor to the text contained in this line.
     * @return text of this line as constant reference
     */
    const QString &text () const { return m_text; }

  private:
    /**
     * Accessor to the text contained in this line.
     * This accessor is private, only the friend class text buffer is allowed to access the text read/write.
     * @return text of this line
     */
    QString &text () { return m_text; }

  private:
    /**
     * text of this line
     */
    QString m_text;
};

/**
 * The normal world only accesses the text lines with shared pointers.
 */
typedef QSharedPointer<TextLineData> TextLine;

}

#endif
