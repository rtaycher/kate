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

TextLine TextBlock::line (int line) const
{
  // calc internal line
  line = line - startLine ();

  // get text line
  return m_lines[line];
}

void TextBlock::text (QString &text) const
{
  // combine all lines
  for (int i = 0; i < m_lines.size(); ++i) {
      // not first line, insert \n
      if (i > 0 || startLine() > 0)
        text.append ('\n');

      text.append (m_lines[i]->text ());
  }
}

void TextBlock::wrapLine (const KTextEditor::Cursor &position)
{
  // calc internal line
  int line = position.line () - startLine ();

  // get text
  QString &text = m_lines[line]->textReadWrite ();

  // check if valid column
  Q_ASSERT (position.column() >= 0);
  Q_ASSERT (position.column() <= text.size());

  // create new line and insert it
  m_lines.insert (m_lines.begin() + line + 1, TextLine (new TextLineData()));

  // perhaps remove some text from previous line and append it
  if (position.column() < text.size ()) {
    // text from old line moved first to new one
    m_lines[line+1]->textReadWrite() = text.right (text.size() - position.column());

    // now remove wrapped text from old line
    text.chop (text.size() - position.column());
  }
}

void TextBlock::unwrapLine (int line, TextBlock *previousBlock)
{
  // calc internal line
  line = line - startLine ();

  // two possiblities: either first line of this block or later line
  if (line == 0) {
      // we need previous block with at least one line
      Q_ASSERT (previousBlock);
      Q_ASSERT (previousBlock->lines () > 0);

      // move last line of previous block to this one, might result in empty block
      m_lines.first()->textReadWrite().prepend (previousBlock->m_lines.last()->text());
      previousBlock->m_lines.erase (previousBlock->m_lines.begin() + (previousBlock->lines () - 1));

      // patch startLine of this block
      --m_startLine;
  } else {
      // easy: just move text to previous line and remove current one
      m_lines[line-1]->textReadWrite().append (m_lines[line]->text());
      m_lines.erase (m_lines.begin () + line);
  }
}

void TextBlock::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // calc internal line
  int line = position.line () - startLine ();

  // get text
  QString &textOfLine = m_lines[line]->textReadWrite ();

  // check if valid column
  Q_ASSERT (position.column() >= 0);
  Q_ASSERT (position.column() <= textOfLine.size());

  // insert text
  textOfLine.insert (position.column(), text);
}

void TextBlock::removeText (const KTextEditor::Range &range, QString &removedText)
{
  // calc internal line
  int line = range.start().line () - startLine ();

  // get text
  QString &textOfLine = m_lines[line]->textReadWrite ();

  // check if valid column
  Q_ASSERT (range.start().column() >= 0);
  Q_ASSERT (range.start().column() <= textOfLine.size());
  Q_ASSERT (range.end().column() >= 0);
  Q_ASSERT (range.end().column() <= textOfLine.size());

  // get text which will be removed
  removedText = textOfLine.mid (range.start().column(), range.end().column() - range.start().column());

  // remove text
  textOfLine.remove (range.start().column(), range.end().column() - range.start().column());
}

void TextBlock::debugPrint (int blockIndex) const
{
  // print all blocks
  for (int i = 0; i < m_lines.size(); ++i)
    printf ("%4d - %4d : %4d : '%s'\n", blockIndex, startLine() + i
      , m_lines[i]->text().size(), qPrintable (m_lines[i]->text()));
}

TextBlock *TextBlock::splitBlock (int fromLine)
{
  // half the block
  int linesOfNewBlock = lines () - fromLine;

  // create and insert new block
  TextBlock *newBlock = new TextBlock (m_buffer, startLine() + fromLine);

  // move lines
  newBlock->m_lines.reserve (linesOfNewBlock);
  for (int i = fromLine; i < m_lines.size(); ++i)
    newBlock->m_lines.append (m_lines[i]);
  m_lines.resize (fromLine);

  // move cursors
  QSet<TextCursor*> oldBlockSet;
  foreach (TextCursor *cursor, m_cursors) {
      if (cursor->lineInBlock() >= fromLine) {
        cursor->m_line = cursor->lineInBlock() - fromLine;
        cursor->m_block = newBlock;
        newBlock->m_cursors.insert (cursor);
      }
      else
        oldBlockSet.insert (cursor);
  }
  m_cursors = oldBlockSet;

  // return the new generated block
  return newBlock;
}

void TextBlock::mergeBlock (TextBlock *targetBlock)
{
   // move cursors, do this first, now still lines() count is correct for target
  foreach (TextCursor *cursor, m_cursors) {
    cursor->m_line = cursor->lineInBlock() + targetBlock->lines ();
    cursor->m_block = targetBlock;
    targetBlock->m_cursors.insert (cursor);
  }
  m_cursors.clear ();

  // move lines
  targetBlock->m_lines.reserve (targetBlock->lines() + lines ());
  for (int i = 0; i < m_lines.size(); ++i)
    targetBlock->m_lines.append (m_lines[i]);
}

}
