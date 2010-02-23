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
}

void TextBuffer::clear ()
{
  // kill all buffer blocks
  m_blocks.clear ();

  // create one block with one empty line
  m_blocks.append (TextBlock (this, 0));
  TextBlock &block = m_blocks.first ();

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
}

int TextBuffer::blockForLine (int line) const
{
  // only allow valid lines
  Q_ASSERT (line >= 0);
  Q_ASSERT (line < lines());
  
  // search block
  for (int index = 0; index < m_blocks.size(); ++index) {
      if (line >= m_blocks[index].startLine()
	&& line < m_blocks[index].startLine() + m_blocks[index].lines ())
	return index;
  }
  
  // we should always find a block
  Q_ASSERT (false);
  return -1;
}

void TextBuffer::fixStartLine (int startBlock)
{
  // only allow valid start block
  Q_ASSERT (startBlock >= 0);
  Q_ASSERT (startBlock < m_blocks.size());
  
  // new start line for next block
  int newStartLine = m_blocks[startBlock].startLine () + m_blocks[startBlock].lines ();
  
  // fixup block
  for (int index = startBlock + 1; index < m_blocks.size(); ++index) {
    // set new start line
    m_blocks[index].setStartLine (newStartLine);
    
    // calculate next start line
    newStartLine += m_blocks[index].lines ();
  }
}


}
