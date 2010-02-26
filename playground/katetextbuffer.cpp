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

#include "katetextbuffer.h"

namespace Kate {

TextBuffer::TextBuffer (QObject *parent, int blockSize)
  : QObject (parent)
  , m_blockSize (blockSize)
  , m_lines (0)
  , m_editingTransactions (0)
{
  // minimal block size must be > 0
  Q_ASSERT (m_blockSize > 0);

  // create initial state
  clear ();
}

TextBuffer::~TextBuffer ()
{
  // not allowed during editing
  Q_ASSERT (m_editingTransactions == 0);

  // clean out all cursors and lines, only cursors belonging to range will survive
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->deleteBlockContent ();

  // kill all ranges
  qDeleteAll (m_ranges);
  m_ranges.clear ();

  // delete all blocks, now that all cursors are really deleted
  // else asserts in destructor of blocks will fail!
  qDeleteAll (m_blocks);
  m_blocks.clear ();

  // kill all invalid cursors, do this after block deletion, to uncover if they might be still linked in blocks
  qDeleteAll (m_invalidCursors);
  m_invalidCursors.clear ();
}

void TextBuffer::clear ()
{
  // not allowed during editing
  Q_ASSERT (m_editingTransactions == 0);

  // new block for empty buffer
  TextBlock *newBlock = new TextBlock (this, 0);
  newBlock->appendLine (TextLine (new TextLineData()));

  // clean out all cursors and lines, either move them to newBlock or invalidate them, if belonging to a range
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->clearBlockContent (newBlock);

  // kill all buffer blocks
  qDeleteAll (m_blocks);
  m_blocks.clear ();

  // insert one block with one empty line
  m_blocks.append (newBlock);

  // reset lines
  m_lines = 1;

  // we got cleared
  emit cleared (this);
}

TextLine TextBuffer::line (int line) const
{
  // get block, this will assert on invalid line
  int blockIndex = blockForLine (line);

  // get line
  return m_blocks[blockIndex]->line (line);
}

QString TextBuffer::text () const
{
  QString text;

  // combine all blocks
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->text (text);

  // return generated string
  return text;
}

void TextBuffer::startEditing ()
{
  // increment transaction counter
  ++m_editingTransactions;

  // if not first running transaction, do nothing
  if (m_editingTransactions > 1)
    return;

  // transaction has started
  emit editingStarted (this);
}

void TextBuffer::finishEditing ()
{
  // only allowed if still transactions running
  Q_ASSERT (m_editingTransactions > 0);

  // decrement counter
  --m_editingTransactions;

  // if not last running transaction, do nothing
  if (m_editingTransactions > 0)
    return;

  // transaction has finished
  emit editingFinished (this);
}

void TextBuffer::wrapLine (const KTextEditor::Cursor &position)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (position.line());

  // let the block handle the wrapLine
  // this can only lead to one more line in this block
  // no other blocks will change
  m_blocks[blockIndex]->wrapLine (position);
  ++m_lines;

  // fixup all following blocks
  fixStartLines (blockIndex);

  // balance the changed block if needed
  balanceBlock (blockIndex);

  // emit signal about done change
  emit lineWrapped (this, position);
}

void TextBuffer::unwrapLine (int line)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // line 0 can't be unwrapped
  Q_ASSERT (line > 0);

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (line);

  // is this the first line in the block?
  bool firstLineInBlock = (line == m_blocks[blockIndex]->startLine());

  // let the block handle the unwrapLine
  // this can either lead to one line less in this block or the previous one
  // the previous one could even end up with zero lines
  m_blocks[blockIndex]->unwrapLine (line, (blockIndex > 0) ? m_blocks[blockIndex-1] : 0);
  --m_lines;

  // decrement index for later fixup, if we modified the block in front of the found one
  if (firstLineInBlock)
    --blockIndex;

  // fixup all following blocks
  fixStartLines (blockIndex);

  // balance the changed block if needed
  balanceBlock (blockIndex);

  // emit signal about done change
  emit lineUnwrapped (this, line);
}

void TextBuffer::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (position.line());

  // let the block handle the insertText
  m_blocks[blockIndex]->insertText (position, text);

  // emit signal about done change
  emit textInserted (this, position, text);
}

void TextBuffer::removeText (const KTextEditor::Range &range)
{
  // only allowed if editing transaction running
  Q_ASSERT (m_editingTransactions > 0);

  // only ranges on one line are supported
  Q_ASSERT (range.start().line() == range.end().line());

  // start colum <= end column and >= 0
  Q_ASSERT (range.start().column() <= range.end().column());
  Q_ASSERT (range.start().column() >= 0);

  // get block, this will assert on invalid line
  int blockIndex = blockForLine (range.start().line());

  // let the block handle the removeText, retrieve removed text
  QString text;
  m_blocks[blockIndex]->removeText (range, text);

  // emit signal about done change
  emit textRemoved (this, range, text);
}

int TextBuffer::blockForLine (int line) const
{
  // only allow valid lines
  Q_ASSERT (line >= 0);
  Q_ASSERT (line < lines());

  // search block
  for (int index = 0; index < m_blocks.size(); ++index) {
      if (line >= m_blocks[index]->startLine()
          && line < m_blocks[index]->startLine() + m_blocks[index]->lines ())
        return index;
  }

  // we should always find a block
  Q_ASSERT (false);
  return -1;
}

void TextBuffer::fixStartLines (int startBlock)
{
  // only allow valid start block
  Q_ASSERT (startBlock >= 0);
  Q_ASSERT (startBlock < m_blocks.size());

  // new start line for next block
  int newStartLine = m_blocks[startBlock]->startLine () + m_blocks[startBlock]->lines ();

  // fixup block
  for (int index = startBlock + 1; index < m_blocks.size(); ++index) {
    // set new start line
    m_blocks[index]->setStartLine (newStartLine);

    // calculate next start line
    newStartLine += m_blocks[index]->lines ();
  }
}

void TextBuffer::balanceBlock (int index)
{
  /**
   * two cases, too big or too small block
   */
  TextBlock *blockToBalance = m_blocks[index];

  // first case, too big one, split it
  if (blockToBalance->lines () >= 2 * m_blockSize) {
    // half the block
    int halfSize = blockToBalance->lines () / 2;

    // create and insert new block behind current one, already set right start line
    TextBlock *newBlock = blockToBalance->splitBlock (halfSize);
    Q_ASSERT (newBlock);
    m_blocks.insert (m_blocks.begin() + index + 1, newBlock);

    // split is done
    return;
  }

  // second case: possibly too small block

  // if only one block, no chance to unite
  // same if this is first block, we always append to previous one
  if (index == 0)
    return;

  // block still large enough, do nothing
  if (2 * blockToBalance->lines () > m_blockSize)
    return;

  // unite small block with predecessor
  TextBlock *targetBlock = m_blocks[index-1];

  // merge block
  blockToBalance->mergeBlock (targetBlock);

  // delete old block
  delete blockToBalance;
  m_blocks.erase (m_blocks.begin() + index);
}

void TextBuffer::debugPrint (const QString &title) const
{
  // print header with title
  printf ("%s\n", qPrintable (title));

  // print all blocks
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->debugPrint (i);
}

}
