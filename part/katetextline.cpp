/***************************************************************************
                          katetextline.cpp  -  description      
                             -------------------      
    begin                : Mon Feb 5 2001      
    copyright            : (C) 2001 by Christoph Cullmann      
    email                : cullmann@kde.org      
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
  : text(0L), attributes(0L), textLen(0), ctx(0L), ctxLen(0), attr(0), myMark (0)     
{     
}      
      
TextLine::~TextLine()      
{      
  delete [] text;     
  delete [] attributes;     
  delete [] ctx;     
}     
     
void TextLine::replace(uint pos, uint delLen, const QChar *insText, uint insLen, uchar *insAttribs)      
{      
  int newLen, i, z;  
  uchar newAttr; 
  
  //find new length  
  newLen = textLen - delLen;  
    
  if (newLen < pos) newLen = pos;  
  newLen += insLen; 
  newAttr = (pos < textLen) ? attributes[pos] : attr;  
  
  if (newLen > textLen) 
  {  
    if (text == 0L) 
      text = (QChar *) malloc (sizeof (QChar)*newLen); 
    else 
      text = (QChar *) realloc(text, sizeof (QChar)*newLen); 
 
    if (attributes == 0L) 
      attributes = (uchar *) malloc (sizeof (uchar)*newLen); 
    else 
      attributes = (uchar *) realloc(attributes, sizeof (uchar)*newLen); 
  } 
  
  //fill up with spaces and attribute  
  for (z = textLen; z < pos; z++) {  
    text[z] = QChar(' ');  
    attributes[z] = attr;  
  }  
  
  i = (insLen - delLen);  
  
  // realloc has gone - no idea why it was here  
  if (i != 0)  
  {  
    if (i <= 0)  
    {  
      //text to replace longer than new text  
      for (z = pos + delLen; z < textLen; z++) {  
        if ((z+i) >= newLen) break;  
        text[z + i] = text[z];  
        attributes[z + i] = attributes[z];  
      }  
  
  
    } else {  
      //text to replace shorter than new text  
      for (z = textLen-1; z >= pos + delLen; z--) {  
        if (z < 0) break;  
        text[z + i] = text[z];  
        attributes[z + i] = attributes[z];  
      }  
    }  
  }  
   
  if (insAttribs == 0L) {  
    for (z = 0; z < insLen; z++) {  
      text[pos + z] = insText[z];  
      attributes[pos + z] = newAttr;  
    }  
  } else {  
    for (z = 0; z < insLen; z++) {  
      text[pos + z] = insText[z];  
      attributes[pos + z] = insAttribs[z];  
    }  
  }  
 
  if (newLen < textLen) 
  {  
    text = (QChar *) realloc(text, sizeof (QChar)*newLen);
    attributes = (uchar *) realloc(attributes, sizeof (uchar)*newLen);
  } 
 
  textLen = newLen;  
}      
      
void TextLine::append(const QChar *s, uint l)      
{      
  replace(textLen, 0, s, l);       
}      
      
void TextLine::truncate(uint newLen)      
{
  if (newLen < textLen)
  {
    textLen = newLen;
    text = (QChar *) realloc(text, sizeof (QChar)*newLen);
    attributes = (uchar *) realloc(attributes, sizeof (uchar)*newLen);
  }
}

void TextLine::wrap(TextLine::Ptr nextLine, uint pos)
{
  int l = textLen - pos;

  if (l > 0)
  {
    nextLine->replace(0, 0, &text[pos], l, &attributes[pos]);
    attr = attributes[pos];
    truncate(pos);
  }
}

void TextLine::unWrap(uint pos, TextLine::Ptr nextLine, uint len) {

  replace(pos, 0, nextLine->text, len, nextLine->attributes);
  attr = nextLine->getRawAttr(len);
  nextLine->replace(0, len, 0L, 0);
}

int TextLine::firstChar() const {
  uint z = 0;

  while (z < textLen && text[z].isSpace()) z++;

  if (z < textLen)
    return z;
  else
    return -1;
}

int TextLine::lastChar() const {
  uint z = textLen;

  while (z > 0 && text[z - 1].isSpace()) z--;
  return z;
}

void TextLine::removeSpaces()
{
  while (textLen > 0 && text[textLen - 1].isSpace()) truncate (textLen-1);
}

QChar TextLine::getChar(uint pos) const
{
  if (pos < textLen)
	  return text[pos];

  return QChar(' ');
}

const QChar *TextLine::firstNonSpace()
{
  int first=firstChar();
  return (first > -1) ? &text[first] : text;
}

bool TextLine::startingWith(const QString& match) const
{
  if (match.length() > textLen)
	  return false;

	for (int z=0; z<match.length(); z++)
	  if (match[z] != text[z])
		  return false;

  return true;
}

bool TextLine::endingWith(const QString& match) const
{
  if (match.length() > textLen)
	  return false;

	for (int z=textLen; z>textLen-match.length(); z--)
	  if (match[z] != text[z])
		  return false;

	return true;
}

int TextLine::cursorX(uint pos, uint tabChars) const
{
  int l, x, z;

  l = (pos < textLen) ? pos : textLen;
  x = 0;
  for (z = 0; z < l; z++) {
    if (text[z] == QChar('\t')) x += tabChars - (x % tabChars); else x++;
  }
  x += pos - l;
  return x;
}

void TextLine::setAttribs(uchar attribute, uint start, uint end) {
  uint z;

  if (end > textLen) end = textLen;
  for (z = start; z < end; z++) attributes[z] = (attributes[z] & taSelected) | attribute;
}

void TextLine::setAttr(uchar attribute) {
  attr = (attr & taSelected) | attribute;
}

uchar TextLine::getAttr(uint pos) const {
  if (pos < textLen) return attributes[pos] & taAttrMask;
  return attr & taAttrMask;
}

uchar TextLine::getAttr() const {
  return attr & taAttrMask;
}

uchar TextLine::getRawAttr(uint pos) const {
  if (pos < textLen) return attributes[pos];
  return (attr & taSelected) ? attr : attr | 256;
}

uchar TextLine::getRawAttr() const {      
  return attr;      
}      
      
void TextLine::setContext(signed char *newctx, uint len)      
{      
  ctxLen = len;      
	     
	if (ctx == 0L)     
    ctx = (signed char*)malloc (len);     
  else     
    ctx = (signed char*)realloc (ctx, len);     
     
  for (uint z=0; z < len; z++) ctx[z] = newctx[z];     
}      
      
void TextLine::select(bool sel, uint start, uint end) {      
  uint z;      
      
  if (end > textLen) end = textLen;        
  if (sel) {      
    for (z = start; z < end; z++) attributes[z] |= taSelected;      
  } else {      
    for (z = start; z < end; z++) attributes[z] &= ~taSelected;      
  }      
}      
      
void TextLine::selectEol(bool sel, uint pos) {      
  uint z;      
      
  if (sel) {      
    for (z = pos; z < textLen; z++) attributes[z] |= taSelected;       
    attr |= taSelected;      
  } else {      
    for (z = pos; z < textLen; z++) attributes[z] &= ~taSelected;       
    attr &= ~taSelected;      
  }      
}      
      
      
void TextLine::toggleSelect(uint start, uint end) {      
  uint z;      
      
  if (end > textLen) end = textLen;        
  for (z = start; z < end; z++) attributes[z] = attributes[z] ^ taSelected;      
}      
      
      
void TextLine::toggleSelectEol(uint pos) {      
  uint z;      
      
  for (z = pos; z < textLen; z++) attributes[z] = attributes[z] ^ taSelected;       
  attr = attr ^ taSelected;      
}      
      
      
int TextLine::numSelected() const {      
  uint z, n;      
      
  n = 0;      
  for (z = 0; z < textLen; z++) if (attributes[z] & taSelected) n++;       
  return n;      
}      
      
bool TextLine::isSelected(uint pos) const {      
  if (pos < textLen) return (attributes[pos] & taSelected);       
  return (attr & taSelected);      
}      
      
bool TextLine::isSelected() const {      
  return (attr & taSelected);      
}      
      
int TextLine::findSelected(uint pos) const {      
  while (pos < textLen && attributes[pos] & taSelected) pos++;       
  return pos;      
}      
      
int TextLine::findUnselected(uint pos) const {      
  while (pos < textLen && !(attributes[pos] & taSelected)) pos++;       
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
      
      
     
 
