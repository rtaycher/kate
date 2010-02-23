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

TextBuffer::TextBuffer (QObject *parent)
  : QObject (parent)
  , m_lines (0)
  , m_editingTransactions (0)
{
  // create initial state
  clear ();
}

TextBuffer::~TextBuffer ()
{
  // delete all blocks
  qDeleteAll (m_blocks);
}

void TextBuffer::clear ()
{
  // not allowed during editing
  Q_ASSERT (m_editingTransactions == 0);

  // kill all buffer blocks
  qDeleteAll (m_blocks);
  m_blocks.clear ();

  // create one block with one empty line
  m_blocks.append (new TextBlock (this, 0));
  TextBlock *block = m_blocks.first ();
  block->appendLine (TextLine (new TextLineData()));

  // reset lines
  m_lines = 1;

  // we got cleared
  emit cleared (this);
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

  // fixup all following blocks
  fixStartLines (blockIndex);

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

  // let the block handle the unwrapLine
  // this can either lead to one line less in this block or the previous one
  // the previous one could even end up with zero lines
  m_blocks[blockIndex]->unwrapLine (line, (blockIndex > 0) ? m_blocks[blockIndex-1] : 0);

  // handle the case that previous block gets empty
  if ((blockIndex > 0) && (m_blocks[blockIndex-1]->lines () == 0)) {
      // decrement index for later fixup
      --blockIndex;

      // delete empty block
      delete m_blocks[blockIndex];

      // and remove it
      m_blocks.erase (m_blocks.begin() + blockIndex);
  }

  // fixup all following blocks
  fixStartLines (blockIndex);

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

void TextBuffer::debugPrint (const QString &title) const
{
  // print header with title
  printf ("%s\n", qPrintable (title));

  // print all blocks
  for (int i = 0; i < m_blocks.size(); ++i)
    m_blocks[i]->debugPrint ();
}

}
