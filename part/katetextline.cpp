/***************************************************************************
                          katetextline.cpp  -  description 
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
    
#include "katetextline.h"    
#include <kdebug.h>    
    
TextLine::TextLine()
  : text(0L), attributes(0L), ctx(0L), ctxLen(0), attr(0), myMark (0) 
{ 
} 
 
TextLine::~TextLine() 
{ 
  delete [] attributes; 
  delete [] ctx; 
} 
 
void TextLine::replace(uint pos, uint delLen, const QChar *insText, uint insLen, uchar *insAttribs) 
{ 
  uint oldLen = text.length(); 
 
  text.remove (pos, delLen); 
  text.insert (pos, insText, insLen); 
 
  if (oldLen<text.length()) attributes = (uchar *) realloc (attributes, text.length()); 
 
  if (text.length() == 0) 
  { 
    attributes = (uchar *) realloc (attributes, 0); 
    return; 
  } 
 
  if (pos >= oldLen) 
  { 
    for (uint t=oldLen; t < text.length(); t++) 
    { 
      attributes[t]=0; 
    } 
  } 
 
  int newAtStuff = insLen-delLen; 
  for (uint m=pos+delLen; m < text.length(); m++) 
  { 
    if (m+newAtStuff >= text.length()) break; 
    if (m >= text.length()) break; 
 
    attributes[m+newAtStuff]=attributes[m]; 
  } 
 
  if (insAttribs == 0L) 
  { 
    for (uint m3=pos; m3 < pos+insLen; m3++) 
    { 
      if (m3 < text.length()) attributes[m3]=0; 
    } 
  } 
  else 
  { 
    for (uint m2=pos; m2 < pos+insLen; m2++) 
    { 
      if (m2 < text.length()) attributes[m2]=insAttribs[m2-pos]; 
    } 
  } 
 
  if (oldLen>text.length()) attributes = (uchar *) realloc (attributes, text.length()); 
} 
 
void TextLine::append(const QChar *s, uint l) 
{ 
  replace(text.length(), 0, s, l); 
} 
 
void TextLine::truncate(uint newLen) 
{ 
  text.truncate(newLen); 
	attributes = (uchar *) realloc (attributes, text.length()); 
} 
 
void TextLine::wrap(TextLine::Ptr nextLine, uint pos) 
{ 
  int l = text.length() - pos; 
 
  if (l > 0) 
  { 
    nextLine->replace(0, 0, &text.unicode()[pos], l, &attributes[pos]); 
    attr = attributes[pos]; 
    truncate(pos); 
  } 
} 
 
void TextLine::unWrap(uint pos, TextLine::Ptr nextLine, uint len) { 
 
  replace(pos, 0, nextLine->text.unicode(), len, nextLine->attributes); 
  attr = nextLine->getRawAttr(len); 
  nextLine->replace(0, len, 0L, 0); 
} 
 
int TextLine::firstChar() const { 
  uint z = 0; 
 
  while (z < text.length() && text[z].isSpace()) z++; 
 
  if (z < text.length()) 
    return z; 
  else 
    return -1; 
} 
 
int TextLine::lastChar() const { 
  uint z = text.length(); 
 
  while (z > 0 && text[z - 1].isSpace()) z--; 
  return z; 
} 
 
void TextLine::removeSpaces() { 
 
  while (text.length() > 0 && text[text.length() - 1].isSpace()) text.truncate (text.length()-1); 
} 
 
QChar TextLine::getChar(uint pos) const { 
  if (pos < text.length()) return text.constref(pos); 
  return ' '; 
} 
const QChar *TextLine::firstNonSpace() 
{ 
  const QChar *ptr=getText(); 
  int first=firstChar(); 
  return (first > -1) ? ptr+first : ptr; 
} 
 
bool TextLine::startingWith(QString& match) 
{ 
  return text.startsWith (match); 
} 
 
bool TextLine::endingWith(QString& match) { 
 
  int matchLen = match.length(); 
 
  // Get the last chars of the textline 
  QString lastChars = text.right(matchLen); 
 
  return (lastChars == match); 
} 
 
int TextLine::cursorX(uint pos, uint tabChars) const { 
  int l, x, z; 
 
  l = (pos < text.length()) ? pos : text.length(); 
  x = 0; 
  for (z = 0; z < l; z++) { 
    if (text[z] == '\t') x += tabChars - (x % tabChars); else x++; 
  } 
  x += pos - l; 
  return x; 
} 
 
void TextLine::setAttribs(uchar attribute, uint start, uint end) { 
  uint z; 
 
  if (end > text.length()) end = text.length(); 
  for (z = start; z < end; z++) attributes[z] = (attributes[z] & taSelected) | attribute; 
} 
 
void TextLine::setAttr(uchar attribute) { 
  attr = (attr & taSelected) | attribute; 
} 
 
uchar TextLine::getAttr(uint pos) const { 
  if (pos < text.length()) return attributes[pos] & taAttrMask; 
  return attr & taAttrMask; 
} 
 
uchar TextLine::getAttr() const { 
  return attr & taAttrMask; 
} 
 
uchar TextLine::getRawAttr(uint pos) const { 
  if (pos < text.length()) return attributes[pos]; 
  return (attr & taSelected) ? attr : attr | 256; 
} 
 
uchar TextLine::getRawAttr() const { 
  return attr; 
} 
 
void TextLine::setContext(signed char *newctx, uint len) 
{ 
  ctxLen = len; 
  ctx = (signed char*)realloc (ctx, len); 
  //delete [] ctx; 
	//ctx = newctx; 
 
  for (uint z=0; z < len; z++) ctx[z] = newctx[z]; 
} 
 
void TextLine::select(bool sel, uint start, uint end) { 
  uint z; 
 
  if (end > text.length()) end = text.length(); 
  if (sel) { 
    for (z = start; z < end; z++) attributes[z] |= taSelected; 
  } else { 
    for (z = start; z < end; z++) attributes[z] &= ~taSelected; 
  } 
} 
 
void TextLine::selectEol(bool sel, uint pos) { 
  uint z; 
 
  if (sel) { 
    for (z = pos; z < text.length(); z++) attributes[z] |= taSelected; 
    attr |= taSelected; 
  } else { 
    for (z = pos; z < text.length(); z++) attributes[z] &= ~taSelected; 
    attr &= ~taSelected; 
  } 
} 
 
 
void TextLine::toggleSelect(uint start, uint end) { 
  uint z; 
 
  if (end > text.length()) end = text.length(); 
  for (z = start; z < end; z++) attributes[z] = attributes[z] ^ taSelected; 
} 
 
 
void TextLine::toggleSelectEol(uint pos) { 
  uint z; 
 
  for (z = pos; z < text.length(); z++) attributes[z] = attributes[z] ^ taSelected; 
  attr = attr ^ taSelected; 
} 
 
 
int TextLine::numSelected() const { 
  uint z, n; 
 
  n = 0; 
  for (z = 0; z < text.length(); z++) if (attributes[z] & taSelected) n++; 
  return n; 
} 
 
bool TextLine::isSelected(uint pos) const { 
  if (pos < text.length()) return (attributes[pos] & taSelected); 
  return (attr & taSelected); 
} 
 
bool TextLine::isSelected() const { 
  return (attr & taSelected); 
} 
 
int TextLine::findSelected(uint pos) const { 
  while (pos < text.length() && attributes[pos] & taSelected) pos++; 
  return pos; 
} 
 
int TextLine::findUnselected(uint pos) const { 
  while (pos < text.length() && !(attributes[pos] & taSelected)) pos++; 
  return pos; 
} 
 
int TextLine::findRevSelected(uint pos) const { 
  while (pos > 0 && attributes[pos - 1] & taSelected) pos--; 
  return pos; 
} 
 
int TextLine::findRevUnselected(uint pos) const { 
  while (pos > 0 && !(attributes[pos - 1] & taSelected)) pos--; 
  return pos; 
} 
 
void TextLine::addMark (uint m) 
{ 
  myMark = myMark | m; 
} 
 
void TextLine::delMark (uint m) 
{ 
  myMark = myMark & ~m; 
} 
 
 
