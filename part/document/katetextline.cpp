/* This file is part of the KDE libraries
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   Based on:
     KateTextLine : Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katetextline.h"
#include "katerenderer.h"

#include <kglobal.h>
#include <kdebug.h>

#include <QtCore/QRegExp>

KateTextLine::KateTextLine ()
  : m_flags(0)
{
}

KateTextLine::KateTextLine (const QChar *data, int length)
  : m_text (data, length), m_flags(0)
{
}

KateTextLine::~KateTextLine()
{
}

void KateTextLine::insertText (int pos, const QString& insText)
{
  // nothing to do
  if (insText.length() == 0)
    return;

  m_text.insert (pos, insText);
}

void KateTextLine::removeText (uint pos, uint delLen)
{
  // nothing to do
  if (delLen == 0)
    return;

  uint textLen = m_text.length();

  if (textLen == 0)
    return; // uh, again nothing real to do ;)

  if (pos >= textLen)
    return;

  if ((pos + delLen) > textLen)
    delLen = textLen - pos;

  m_text.remove (pos, delLen);
}

void KateTextLine::truncate(int newLen)
{
  if (newLen < 0)
    newLen = 0;

  if (newLen < m_text.length())
    m_text.truncate (newLen);
}

int KateTextLine::nextNonSpaceChar(uint pos) const
{
  const int len = m_text.length();
  const QChar *unicode = m_text.unicode();

  for(int i = pos; i < len; i++)
  {
    if(!unicode[i].isSpace())
      return i;
  }

  return -1;
}

int KateTextLine::previousNonSpaceChar(int pos) const
{
  const int len = m_text.length();
  const QChar *unicode = m_text.unicode();

  if (pos < 0)
    pos = 0;

  if (pos >= len)
    pos = len - 1;

  for(int i = pos; i >= 0; i--)
  {
    if(!unicode[i].isSpace())
      return i;
  }

  return -1;
}

int KateTextLine::firstChar() const
{
  return nextNonSpaceChar(0);
}

int KateTextLine::lastChar() const
{
  return previousNonSpaceChar(m_text.length() - 1);
}

QString KateTextLine::leadingWhitespace() const
{
  if (firstChar() < 0)
    return string(0, length());

  return string(0, firstChar());
}

int KateTextLine::indentDepth (int tabWidth) const
{
  int d = 0;
  const int len = m_text.length();
  const QChar *unicode = m_text.unicode();

  for(int i = 0; i < len; ++i)
  {
    if(unicode[i].isSpace())
    {
      if (unicode[i] == QLatin1Char('\t'))
        d += tabWidth - (d % tabWidth);
      else
        d++;
    }
    else
      return d;
  }

  return d;
}

bool KateTextLine::matchesAt(int column, const QString& match) const
{
  if (column < 0)
    return false;

  const int len = m_text.length();
  const int matchlen = match.length();

  if ((column + matchlen) > len)
    return false;

  const QChar *unicode = m_text.unicode();
  const QChar *matchUnicode = match.unicode();

  for (int i=0; i < matchlen; ++i)
    if (unicode[i+column] != matchUnicode[i])
      return false;

  return true;
}

int KateTextLine::toVirtualColumn (int column, int tabWidth) const
{
  if (column < 0)
    return 0;

  int x = 0;
  const int zmax = qMin(column, m_text.length());
  const QChar *unicode = m_text.unicode();

  for ( int z = 0; z < zmax; ++z)
  {
    if (unicode[z] == QLatin1Char('\t'))
      x += tabWidth - (x % tabWidth);
    else
      x++;
  }

  return x + column - zmax;
}

int KateTextLine::fromVirtualColumn (int column, int tabWidth) const
{
  if (column < 0)
    return 0;

  const int zmax = qMin(m_text.length(), column);
  const QChar *unicode = m_text.unicode();

  int x = 0;
  int z = 0;
  for (; z < zmax; ++z)
  {
    int diff = 1;
    if (unicode[z] == QLatin1Char('\t'))
      diff = tabWidth - (x % tabWidth);

    if (x + diff > column)
      break;
    x += diff;
  }

  return z;
}

int KateTextLine::virtualLength (int tabWidth) const
{
  int x = 0;
  const int len = m_text.length();
  const QChar *unicode = m_text.unicode();

  for ( int z = 0; z < len; ++z)
  {
    if (unicode[z] == QLatin1Char('\t'))
      x += tabWidth - (x % tabWidth);
    else
      x++;
  }

  return x;
}

void KateTextLine::addAttribute (int start, int length, int attribute)
{
//  kDebug( 13020 ) << "addAttribute: " << start << " " << length << " " << attribute;

  // try to append to previous range
  if ((m_attributesList.size() > 2) && (m_attributesList[m_attributesList.size()-1] == attribute)
      && (m_attributesList[m_attributesList.size()-3]+m_attributesList[m_attributesList.size()-2]
         == start))
  {
    m_attributesList[m_attributesList.size()-2] += length;
    return;
  }

  m_attributesList.resize (m_attributesList.size()+3);
  m_attributesList[m_attributesList.size()-3] = start;
  m_attributesList[m_attributesList.size()-2] = length;
  m_attributesList[m_attributesList.size()-1] = attribute;
}

// kate: space-indent on; indent-width 2; replace-tabs on;