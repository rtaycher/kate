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

#include "katetextline.h"

namespace Kate {

TextLineData::TextLineData ()
{
}

TextLineData::~TextLineData ()
{
}

int TextLineData::firstChar() const
{
  return nextNonSpaceChar(0);
}

int TextLineData::lastChar() const
{
  return previousNonSpaceChar(m_text.length() - 1);
}

int TextLineData::nextNonSpaceChar (int pos) const
{
  Q_ASSERT (pos >= 0);

  for(int i = pos; i < m_text.length(); i++)
    if (!m_text[i].isSpace())
      return i;

  return -1;
}

int TextLineData::previousNonSpaceChar (int pos) const
{
  Q_ASSERT (pos >= 0);

  if (pos >= m_text.length())
    pos = m_text.length() - 1;

  for(int i = pos; i >= 0; i--)
    if (!m_text[i].isSpace())
      return i;

  return -1;
}

}
