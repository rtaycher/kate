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

#include "katetextblock.h"
#include "katetextbuffer.h"

namespace Kate {

TextBlock::TextBlock (TextBuffer *buffer, int startLine)
  : m_buffer (buffer)
  , m_startLine (startLine)
{
}

TextBlock::~TextBlock ()
{
}

void TextBlock::setStartLine (int startLine)
{
  // allow only valid lines
  Q_ASSERT (startLine >= 0);
  Q_ASSERT (startLine < m_buffer->lines ());

  m_startLine = startLine;
}

void TextBlock::wrapLine (const KTextEditor::Cursor &position)
{
  // calc internal line
  int line = position.line () - startLine ();

  // get text
  QString &text = m_lines[line]->text ();

  // check if valid column
  Q_ASSERT (position.column() >= 0);
  Q_ASSERT (position.column() <= text.size());

  // create new line and insert it
  m_lines.insert (m_lines.begin() + line + 1, TextLine (new TextLineData()));

  // perhaps remove some text from previous line and append it
  if (position.column() < text.size ()) {
    // append text from old line first to new one
    m_lines[line+1]->text().append (text.right (text.size() - position.column()));

    // now remove it
    text.truncate (text.size() - position.column());
  }
}

void TextBlock::unwrapLine (int line, TextBlock *previousBlock)
{
}

void TextBlock::insertText (const KTextEditor::Cursor &position, const QString &text)
{
}

void TextBlock::removeText (const KTextEditor::Range &range, QString &removedText)
{
}

void TextBlock::debugPrint () const
{
  // print all blocks
  for (int i = 0; i < m_lines.size(); ++i)
    printf ("%4d : %4d : '%s'\n", startLine() + i
      , m_lines[i]->text().size(), qPrintable (m_lines[i]->text()));
}


}
