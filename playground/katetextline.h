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

    /**
     * Returns the position of the first non-whitespace character
     * @return position of first non-whitespace char or -1 if there is none
     */
    int firstChar() const;

    /**
     * Returns the position of the last non-whitespace character
     * @return position of last non-whitespace char or -1 if there is none
     */
    int lastChar() const;

    /**
     * Find the position of the next char that is not a space.
     * @param pos Column of the character which is examined first.
     * @return True if the specified or a following character is not a space
     *          Otherwise false.
     */
    int nextNonSpaceChar(int pos) const;

    /**
     * Find the position of the previous char that is not a space.
     * @param pos Column of the character which is examined first.
     * @return The position of the first non-whitespace character preceding pos,
     *   or -1 if none is found.
     */
    int previousNonSpaceChar(int pos) const;

    /**
     * Returns the character at the given \e column. If \e column is out of
     * range, the return value is QChar().
     * @param column column you want char for
     * @return char at given column or QChar()
     */
    inline QChar at (int column) const
    {
      if (column >= 0 && column < m_text.length())
        return m_text[column];

      return QChar();
    }

    /**
     * Same as at().
     * @param column column you want char for
     * @return char at given column or QChar()
     */
    inline QChar operator[](int column) const
    {
      if (column >= 0 && column < m_text.length())
        return m_text[column];

      return QChar();
    }

  private:
    /**
     * Accessor to the text contained in this line.
     * This accessor is private, only the friend class text buffer is allowed to access the text read/write.
     * @return text of this line
     */
    QString &textReadWrite () { return m_text; }

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
