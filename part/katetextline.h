/***************************************************************************
                          katetextline.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KWTEXTLINE_H_
#define _KWTEXTLINE_H_

#include <stdlib.h>

#include <qstring.h>
#include <qmemarray.h>
#include <qvaluelist.h>
#include <qvaluevector.h>
#include <ksharedptr.h>

/**
  The TextLine represents a line of text. A text line that contains the 
  text, an attribute for each character, an attribute for the free space 
  behind the last character and a context number for the syntax highlight. 
  The attribute stores the index to a table that contains fonts and colors 
  and also if a character is selected. 
*/ 
 
typedef QMemArray<signed char> TContexts;
 
class TextLine : public KShared 
{ 
  friend class KWBuffer; 
  friend class KWBufBlock; 
 
public: 
    typedef KSharedPtr<TextLine> Ptr; 
    typedef QValueVector<Ptr> List; 
 
public: 
    /** 
      Creates an empty text line with given attribute and syntax highlight 
      context 
    */ 
    TextLine(uchar attribute = 0);
    ~TextLine(); 
 
    /** 
      Returns the length 
    */ 
    uint length() const {return text.length();} 
    /** 
      Universal text manipulation method. It can be used to insert, delete 
      or replace text. 
    */ 
    void replace(uint pos, uint delLen, const QChar *insText, uint insLen, uchar *insAttribs = 0L); 
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
      Returns the position of the first character which is not a white space 
    */ 
    int firstChar() const; 
    /** 
      Returns the position of the last character which is not a white space 
    */ 
    int lastChar() const; 
    /** 
      Removes trailing spaces 
    */ 
    void removeSpaces(); 
    /** 
      Gets the char at the given position 
    */ 
    QChar getChar(uint pos) const; 
    /** 
      Gets the text. WARNING: it is not null terminated 
    */ 
    const QChar *getText() const {return text.unicode();}; 
    /** 
      Gets a C-like null terminated string 
    */ 
    const QString getString() { return text; }; 
 
    /* 
      Gets a null terminated pointer to first non space char 
    */ 
    const QChar *firstNonSpace(); 
    /** 
      Returns the x position of the cursor at the given position, which 
      depends on the number of tab characters 
    */ 
    int cursorX(uint pos, uint tabChars) const; 
    /** 
      Is the line starting with the given string 
    */ 
    bool startingWith(QString& match); 
    /** 
      Is the line ending with the given string 
    */ 
    bool endingWith(QString& match); 
 
    /** 
      Sets the attributes from start to end -1 
    */ 
    void setAttribs(uchar attribute, uint start, uint end);
    /** 
      Sets the attribute for the free space behind the last character 
    */ 
    void setAttr(uchar attribute);
    /** 
      Gets the attribute at the given position 
    */ 
    uchar getAttr(uint pos) const; 
    /** 
      Gets the attribute for the free space behind the last character 
    */ 
    uchar getAttr() const; 
    /** 
      Gets the attribute, including the select state, at the given position 
    */ 
    uchar getRawAttr(uint pos) const; 
    /** 
      Gets the attribute, including the select state, for the free space 
      behind the last character 
    */ 
    uchar getRawAttr() const; 
 
    /** 
      Sets the syntax highlight context number 
    */ 
    void setContext(signed char *newctx, uint len);
    /**
      Gets the syntax highlight context number
    */
    signed char *getContext() const { return ctx; };
		/**
      Gets the syntax highlight context number
    */
    uint getContextLength() const { return ctxLen; };
 
    /** 
      Sets the select state from start to end -1 
    */ 
    void select(bool sel, uint start, uint end); 
    /** 
      Sets the select state from the given position to the end, including 
      the free space behind the last character 
    */ 
    void selectEol(bool sel, uint pos); 
    /** 
      Toggles the select state from start to end -1 
    */ 
    void toggleSelect(uint start, uint end); 
    /** 
      Toggles the select state from the given position to the end, including 
      the free space behind the last character 
    */ 
    void toggleSelectEol(uint pos); 
    /** 
      Returns the number of selected characters 
    */ 
    int numSelected() const; 
    /** 
      Returns if the character at the given position is selected 
    */ 
    bool isSelected(uint pos) const; 
    /**
      Returns true if the free space behind the last character is selected
    */
    bool isSelected() const;
    /**
      Finds the next selected character, starting at the given position
    */
    int findSelected(uint pos) const;
    /**
      Finds the next unselected character, starting at the given position
    */
    int findUnselected(uint pos) const;
    /**
      Finds the previous selected character, starting at the given position
    */
    int findRevSelected(uint pos) const;
    /**
      Finds the previous unselected character, starting at the given position
    */
    int findRevUnselected(uint pos) const;

    void clearMark () { myMark = 0; };
    void addMark ( uint m );
    void delMark ( uint m );
    uint mark() { return myMark; };

    uchar *getAttribs() { return attributes; }

  protected:
    /**
      The text
    */
    QString text;
    /**
      The attributes
    */
    uchar *attributes;
    /**
      The attribute of the free space behind the end
    */
    uchar attr;
    /**
      The syntax highlight context
    */
    signed char *ctx;
		uint ctxLen;
    /**
      The marks of the current line 
    */ 
    uint myMark;
}; 
 
//text attribute constants 
const int taSelected = 0x40; 
const int taAttrMask = ~taSelected & 0xFF; 
 
#endif //KWTEXTLINE_H 
 
 
