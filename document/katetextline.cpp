/*
   Copyright (C) 1998, 1999 Jochen Wilhelmy
                            digisnap@cs.tu-berlin.de

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "katetextline.h"
#include <kdebug.h>

TextLine::TextLine(int attribute, int context)
  : size(0), text(0L), attributes(0L), attr(attribute), ctx(context), myMark (0)
{
}

TextLine::~TextLine()
{
}


void TextLine::replace(int pos, int delLen, const QChar *insText, int insLen,
  uchar *insAttribs)
{
  text.replace (pos, delLen, insText, insLen);

  attributes.resize (text.length());

  if (attributes.size() < 1) return;

  int newAtStuff = insLen-delLen;
  for (int m=pos; m < attributes.size()-1; m++)
  {
    if (m+newAtStuff < attributes.size()) attributes[m+newAtStuff]=attributes[m];
  }

  if (insAttribs != 0L)
  {
  for (int m2=pos; m2 < pos+insLen; m2++)
  {
    if (m2 < attributes.size()) attributes[m2]=insAttribs[m2-pos];
  }
  }
  else
  {
  for (int m3=pos; m3 < pos+insLen; m3++)
  {
    if (m3 < attributes.size()) attributes[m3]=0;
  }
  }
}

void TextLine::wrap(TextLine::Ptr nextLine, int pos) {
  int l;

  l = text.length() - pos;
  if (l > 0) {
    nextLine->replace(0, 0, &text.unicode()[pos], l, &attributes[pos]);
    attr = attributes[pos];
  }
}

void TextLine::unWrap(int pos, TextLine::Ptr nextLine, int len) {

  replace(pos, 0, nextLine->text.unicode(), len, nextLine->attributes.data());
  attr = nextLine->getRawAttr(len);
  nextLine->replace(0, len, 0L, 0);
}

int TextLine::firstChar() const {
  int z = 0;

  while (z < text.length() && text[z].isSpace()) z++;
  return (z < text.length()) ? z : -1;
}

int TextLine::lastChar() const {
  int z = text.length();

  while (z > 0 && text[z - 1].isSpace()) z--;
  return z;
}

void TextLine::removeSpaces() {

  while (text.length() > 0 && text[text.length() - 1].isSpace()) text.truncate (text.length()-1);
}

QChar TextLine::getChar(int pos) const {
  if (pos < text.length()) return text.constref(pos);
  return ' ';
}
const QChar *TextLine::firstNonSpace()
{
  const QChar *ptr=getText();
  int first=firstChar();
  return (first > -1) ? ptr+first : ptr;
}

bool TextLine::startingWith(QString& match) {

  int matchLen = match.length();

  // Get the first chars of the textline
  QString firstChars = text.left(matchLen);

  return (firstChars == match);
}

bool TextLine::endingWith(QString& match) {

  int matchLen = match.length();

  // Get the last chars of the textline
  QString lastChars = text.right(matchLen);

  return (lastChars == match);
}

int TextLine::cursorX(int pos, int tabChars) const {
  int l, x, z;

  l = (pos < text.length()) ? pos : text.length();
  x = 0;
  for (z = 0; z < l; z++) {
    if (text[z] == '\t') x += tabChars - (x % tabChars); else x++;
  }
  x += pos - l;
  return x;
}

void TextLine::setAttribs(int attribute, int start, int end) {
  int z;

  if (end > text.length()) end = text.length();
  for (z = start; z < end; z++) attributes[z] = (attributes[z] & taSelected) | attribute;
}

void TextLine::setAttr(int attribute) {
  attr = (attr & taSelected) | attribute;
}

int TextLine::getAttr(int pos) const {
  if (pos < text.length()) return attributes[pos] & taAttrMask;
  return attr & taAttrMask;
}

int TextLine::getAttr() const {
  return attr & taAttrMask;
}

int TextLine::getRawAttr(int pos) const {
  if (pos < text.length()) return attributes[pos];
  return (attr & taSelected) ? attr : attr | 256;
}

int TextLine::getRawAttr() const {
  return attr;
}

void TextLine::setContext(int context) {
  ctx = context;
}

int TextLine::getContext() const {
  return ctx;
}


void TextLine::select(bool sel, int start, int end) {
  int z;

  if (end > text.length()) end = text.length();
  if (sel) {
    for (z = start; z < end; z++) attributes[z] |= taSelected;
  } else {
    for (z = start; z < end; z++) attributes[z] &= ~taSelected;
  }
}

void TextLine::selectEol(bool sel, int pos) {
  int z;

  if (sel) {
    for (z = pos; z < text.length(); z++) attributes[z] |= taSelected;
    attr |= taSelected;
  } else {
    for (z = pos; z < text.length(); z++) attributes[z] &= ~taSelected;
    attr &= ~taSelected;
  }
}


void TextLine::toggleSelect(int start, int end) {
  int z;

  if (end > text.length()) end = text.length();
  for (z = start; z < end; z++) attributes[z] = attributes[z] ^ taSelected;
}


void TextLine::toggleSelectEol(int pos) {
  int z;

  for (z = pos; z < text.length(); z++) attributes[z] = attributes[z] ^ taSelected;
  attr = attr ^ taSelected;
}


int TextLine::numSelected() const {
  int z, n;

  n = 0;
  for (z = 0; z < text.length(); z++) if (attributes[z] & taSelected) n++;
  return n;
}

bool TextLine::isSelected(int pos) const {
  if (pos < text.length()) return (attributes[pos] & taSelected);
  return (attr & taSelected);
}

bool TextLine::isSelected() const {
  return (attr & taSelected);
}

int TextLine::findSelected(int pos) const {
  while (pos < text.length() && attributes[pos] & taSelected) pos++;
  return pos;
}

int TextLine::findUnselected(int pos) const {
  while (pos < text.length() && !(attributes[pos] & taSelected)) pos++;
  return pos;
}

int TextLine::findRevSelected(int pos) const {
  while (pos > 0 && attributes[pos - 1] & taSelected) pos--;
  return pos;
}

int TextLine::findRevUnselected(int pos) const {
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
