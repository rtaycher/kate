/* This file is part of the KDE libraries
   Copyright (C) 2001-2003 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   Based on:
     TextLine : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KATE_TEXTLINE_H_
#define _KATE_TEXTLINE_H_

#include <qmemarray.h>
#include <qstring.h>
#include <ksharedptr.h>
#include <qvaluevector.h>

class KateBuffer;

/**
  The TextLine represents a line of text. A text line that contains the
  text, an attribute for each character, an attribute for the free space
  behind the last character and a context number for the syntax highlight.
  The attribute stores the index to a table that contains fonts and colors
  and also if a character is selected.
*/

class TextLine : public KShared
{
  public:
    typedef KSharedPtr<TextLine> Ptr;
    typedef QValueVector<Ptr> List;

  public:
    /**
      Creates an empty text line with given attribute and syntax highlight
      context
    */
    TextLine(KateBuffer* buf);
    ~TextLine();

  /**
    Methods to get data
  */
  public:
    /**
     * Returns the parent buffer.
     */
    KateBuffer* buffer() const { return m_buf; }

    /**
      Returns the length
    */
    inline uint length() const { return m_text.length(); }

    /**
      Returns the visibility flag
    */
    inline bool isVisible() const { return m_flags & TextLine::flagVisible; }

    inline bool isFoldingListValid() const { return m_flags & TextLine::flagFoldingListValid; }

    /**
      Returns the position of the first character which is not a white space
    */
    int firstChar() const;

    /**
      Returns the position of the last character which is not a white space
    */
    int lastChar() const;

    /**
      Returns the position of the first non-space char after a given position
    */
    int nextNonSpaceChar(uint pos) const;

    /**
      Returns the position of the last non-space char before a given position
    */
    int previousNonSpaceChar(uint pos) const;

    /**
      Gets the char at the given position
    */
    inline QChar getChar (uint pos) const
    {
      return m_text[pos];
    }

    /**
      Gets the text.
    */
    inline const QChar *text() const { return m_text.unicode(); };

    inline uchar *attributes () const { return m_attributes.data(); }

    /**
      Gets a QString
    */
    inline const QString& string() const { return m_text; };

    /**
      Gets a QString.
    */
    inline QString string(uint startCol, uint length) const { return m_text.mid(startCol, length); };
    inline QConstString constString(uint startCol, uint length) const { return QConstString(m_text.unicode() + startCol, length); };

    /*
      Gets a null terminated pointer to first non space char
    */
    const QChar *firstNonSpace() const;

    /**
      Returns the x position of the cursor at the given position, which
      depends on the number of tab characters
    */
    int cursorX(uint pos, uint tabChars) const;

    /**
      Can we find the given string at the given position
    */
    bool stringAtPos(uint pos, const QString& match) const;

    /**
      Is the line starting with the given string
    */
    bool startingWith(const QString& match) const;

    /**
      Is the line ending with the given string
    */
    bool endingWith(const QString& match) const;

    /**
      Gets the syntax highlight context number
    */
    inline uint *ctx () const { return m_ctx.data (); };

    /**
      Gets size of the ctxArray
    */
    inline bool ctxSize () const { return m_ctx.size (); };

    /**
      Empty ctx stack ?
    */
    inline bool ctxEmpty () const { return m_ctx.isEmpty (); };

    bool searchText (uint startCol, const QString &text, uint *foundAtCol, uint *matchLen, bool casesensitive = true, bool backwards = false);
    bool searchText (uint startCol, const QRegExp &regexp, uint *foundAtCol, uint *matchLen, bool backwards = false);

     /**
      Gets the attribute at the given position
    */
    inline uchar attribute (uint pos) const
    {
      if (pos < m_text.length()) return m_attributes[pos];
      return 0;
    }

    inline bool hlLineContinue () const
    {
      return m_flags & TextLine::flagHlContinue;
    }

    /**
      Raw access on the memarray's, for example the katebuffer class
    */
    inline const QString &textArray () const { return m_text; };
    inline const QMemArray<uchar> &attributesArray () const { return m_attributes; };
    inline const QMemArray<uint> &ctxArray () const { return m_ctx; };
    inline const QMemArray<signed char> &foldingListArray () const { return m_foldingList; };

  /**
    Methodes to manipulate data
  */
  public:
     /**
      Universal text manipulation methoda. They can be used to insert or delete text
    */
    void insertText (uint pos, uint insLen, const QChar *insText, uchar *insAttribs = 0);
    void removeText (uint pos, uint delLen);

    /**
      Appends a string of length l to the textline
    */
    void append(const QChar *s, uint l);

    /**
      Wraps the text from the given position to the end to the next line
    */
    void wrap(TextLine::Ptr nextLine, uint pos);

    /**
      Wraps the text of given length from the beginning of the next line to
      this line at the given position
    */
    void unWrap(uint pos, TextLine::Ptr nextLine, uint len);

    /**
      Truncates the textline to the new length
    */
    void truncate(uint newLen);

    /**
      Removes trailing spaces
    */
    QString withoutTrailingSpaces();

    /**
      Sets the visibility flag
    */
    inline void setVisible(bool val)
    {
      if (val) m_flags = m_flags | TextLine::flagVisible;
      else m_flags = m_flags & ~ TextLine::flagVisible;
    }

    /**
      Sets the attributes from start to end -1
    */
    void setAttribs(uchar attribute, uint start, uint end);

    /**
      Sets the syntax highlight context number
    */
    inline void setContext(uint *newctx, uint len)
    {
      m_ctx.duplicate (newctx, len);
    }

    inline void setHlLineContinue (bool cont)
    {
      if (cont) m_flags = m_flags | TextLine::flagHlContinue;
      else m_flags = m_flags & ~ TextLine::flagHlContinue;
    }

    inline void setFoldingList (QMemArray<signed char> &val)
    {
      m_foldingList=val;
      m_foldingList.detach();
      m_flags = m_flags | TextLine::flagFoldingListValid;
    }

  /**
    Methodes for dump/restore of the line in the buffer
  */
  public:
    /**
      Dumpsize in bytes
    */
    uint dumpSize () const;

    /**
      Dumps the line to *buf and counts buff dumpSize bytes up
      as return value
    */
    char *dump (char *buf) const;

    /**
      Restores the line from *buf and counts buff dumpSize bytes up
      as return value
    */
    char *restore (char *buf);

    enum Flags
    {
      flagHlContinue = 0x1,
      flagVisible = 0x2,
      flagFoldingListValid = 0x4,
      flagNoOtherData = 0x8 // ONLY INTERNAL USE, NEVER EVER SET THAT !!!!
    };

  /**
   REALLY PRIVATE ;) please no new friend classes
   */
  private:
    /**
      The text & attributes
    */
    QString m_text;
    QMemArray<uchar> m_attributes;

    /**
     Data for context + folding
     */
    QMemArray<uint> m_ctx;
    QMemArray<signed char> m_foldingList;

    /**
     Some bools packed
    */
    uchar m_flags;

    /**
     * The parent buffer.
     */
    KateBuffer* m_buf;
    static bool m_noSignal;
};

#endif
