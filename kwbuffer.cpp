/*
   $Id$
   This file is part of KWrite
   Copyright (c) 2000 Waldo Bastian <bastian@kde.org>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/


#include "kwbuffer.h"

// Includes for reading file
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <qfile.h>
#include <qtimer.h>

#include <assert.h>

#define LOADED_BLOCKS_MAX	25
#define AVG_BLOCK_SIZE		8192

/**
 * Create an empty buffer.
 */
KWBuffer::KWBuffer()
{
   m_totalLines = 0;
   m_fdSwap = -1;
   m_blocks.setAutoDelete(true);
   m_loader.setAutoDelete(true);
   connect( &m_loadTimer, SIGNAL(timeout()), this, SLOT(slotLoadFile()));
}

/**
 * Reads @p size bytes from file-descriptor @p fd.
 */
static QByteArray readBlock(int fd, int size)
{
   QByteArray result(size);
   int bytesToRead = size;
   int bytesRead = 0;
   while(bytesToRead)
   {
      int n = ::read(fd, result.data()+bytesRead, bytesToRead);
      if (n == 0) break; // Done
      if ((n == -1) && (errno == EAGAIN))
         continue;
      if (n == -1)
      {
         // TODO: Do some error handling.
         break;
      }
      bytesRead += n;
      bytesToRead -= n;
   }
qWarning("Read = %d", bytesRead);
   result.truncate(bytesRead);
   return result;
}

/**
 * Writes the bytes buf[begin] to buf[end] to @p fd.
 */
static void writeBlock(int fd, const QByteArray &buf, int begin, int end)
{
   while(begin != end)
   {
      int n = ::write(fd, buf.data()+begin, end - begin);
      if ((n == -1) && (errno == EAGAIN))
         continue;
      if (n == -1)
         return; // TODO: Do some error handling.
      begin += n;
   }
}


/**
 * Insert a file at line @p line in the buffer.
 */
void
KWBuffer::insertFile(int line, const QString &file, QTextCodec *codec)
{
  assert(line == 0); // Inserting at other places not yet handled.

  int fd = open(QFile::encodeName(file), O_RDONLY);
  if (fd < 0)
     return; // Do some error propagation here.

  KWBufFileLoader *loader = new KWBufFileLoader;
  loader->fd = fd;
  loader->dataStart = 0;
  loader->blockNr = 0;
  loader->codec = codec;
  m_loader.append(loader);

  loadFilePart();
}

void
KWBuffer::loadFilePart()
{
  const int blockSize = AVG_BLOCK_SIZE;
  const int blockRead = 5; // Read 5 blocks in a row

  KWBufFileLoader *loader = m_loader.first();

  KWBufState state;
  if (loader->blockNr > 0)
  {
     KWBufBlock *block = m_blocks.at(loader->blockNr-1);
     state = block->m_endState;
  }
  else
  {
     // Initial state.
     state.lineNr = 0;
  }
  int startLine = state.lineNr;
  bool eof = false;
 
 
  for(int i = 0; i < blockRead; i++)
  {
     QByteArray currentBlock = readBlock(loader->fd, blockSize);
     eof = ((int) currentBlock.size()) != blockSize;
     KWBufBlock *block = new KWBufBlock(state);
     m_blocks.insert(loader->blockNr++, block);
     m_loadedBlocks.append(block);
     block->m_codec = loader->codec;
     loader->dataStart = block->blockFill(loader->dataStart, 
                                  loader->lastBlock, currentBlock, eof);
     state = block->m_endState;
qWarning("current->ref = %d last->ref = %d", currentBlock.nrefs(), loader->lastBlock.nrefs());
     loader->lastBlock = currentBlock;
     if (eof) break;
  }
  if (eof)
  {
qWarning("Loading finished.");
     m_loader.removeRef(loader);
  }
  if (m_loader.count())
  {
qWarning("Starting timer...");
     m_loadTimer.start(0, true);
  }

  m_totalLines += state.lineNr - startLine;
}

void
KWBuffer::slotLoadFile()
{
  loadFilePart();
  emit linesChanged(m_totalLines);
}
   
/**
 * Return the total number of lines in the buffer.
 */
int 
KWBuffer::count()
{
   return m_totalLines;
}

KWBufBlock *
KWBuffer::findBlock(int i)
{
   if ((i < 0) || (i >= m_totalLines))
      return 0;

   int lastLine = 0;
   // This needs a bit of optimisation/caching so that we don't walk 
   // through the list every time.
   KWBufBlock *buf;
   for(buf = m_blocks.first(); buf; buf = m_blocks.next())
   {
      // Adjust line numbering....
      if (buf->m_beginState.lineNr != lastLine)
      {
         int offset = lastLine - buf->m_beginState.lineNr;
         buf->m_beginState.lineNr += offset;
         buf->m_endState.lineNr += offset;
      }
      lastLine = buf->m_endState.lineNr;
      if ((i >= buf->m_beginState.lineNr) && (i < lastLine))
      {
         // We found the block.
         break;
      }
   }   

   if (!buf)
   {
      // Huh? Strange, m_totalLines must have been out of sync?
      assert(lastLine == m_totalLines);
      assert(false);
      return 0;
   }
   return buf;
}
   
/**
 * Return line @p i
 */
TextLine::Ptr
KWBuffer::line(int i)
{
   KWBufBlock *buf = findBlock(i);
   if (!buf)
      return 0;

   if (!buf->b_stringListValid) 
   {
      parseBlock(buf);
   }
   return buf->line(i - buf->m_beginState.lineNr);
}

void
KWBuffer::insertLine(int i, TextLine::Ptr line)
{
   KWBufBlock *buf;
   if (i == m_totalLines)
      buf = findBlock(i-1);
   else
      buf = findBlock(i);

   if (!buf)
   {
      KWBufState state;
      // Initial state.
      state.lineNr = 0;
      buf = new KWBufBlock(state);
      m_blocks.insert(0, buf);
   }

   if (!buf->b_stringListValid) 
   {
      parseBlock(buf);
   }
   if (buf->b_rawDataValid)
   {
      dirtyBlock(buf);
   }
   buf->insertLine(i -  buf->m_beginState.lineNr, line);
   m_totalLines++;
}

void
KWBuffer::removeLine(int i)
{
   KWBufBlock *buf = findBlock(i);
   assert(buf);
   if (!buf->b_stringListValid) 
   {
      parseBlock(buf);
   }
   if (buf->b_rawDataValid)
   {
      dirtyBlock(buf);
   }
   buf->removeLine(i -  buf->m_beginState.lineNr);
   m_totalLines--;
}

void
KWBuffer::changeLine(int i)
{
qWarning("changeLine(%d)", i);
   KWBufBlock *buf = findBlock(i);
   assert(buf);
   assert(buf->b_stringListValid); 
   if (buf->b_rawDataValid)
   {
      dirtyBlock(buf);
   }
}

void
KWBuffer::parseBlock(KWBufBlock *buf)
{
qWarning("parseBlock(%p)", buf);
   if (!buf->b_rawDataValid)
      loadBlock(buf);
   if (m_parsedBlocksClean.count() > 5)
   {
      KWBufBlock *buf2 = m_parsedBlocksClean.take(2);
      buf2->disposeStringList();
   }
   buf->buildStringList();
   m_parsedBlocksClean.append(buf);
}

void
KWBuffer::loadBlock(KWBufBlock *buf)
{
qWarning("loadBlock(%p)", buf);
   if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)
   {
      KWBufBlock *buf2 = m_loadedBlocks.take(2);
//      buf2->swapOut(m_fdSwap, some_offset);
   }

//   buf->swapIn(m_fdSwap); TODO: Open m_fdSwap
   m_parsedBlocksClean.append(buf);
   m_loadedBlocks.append(buf);
}

void
KWBuffer::dirtyBlock(KWBufBlock *buf)
{
qWarning("dirtyBlock(%p)", buf);
   m_loadedBlocks.removeRef(buf);
   buf->b_rawDataValid = false;
   m_parsedBlocksClean.removeRef(buf);
}

//-----------------------------------------------------------------

/**
 * The KWBufBlock class contains an amount of data representing 
 * a certain number of lines.
 */

/*
 * Create an empty block.
 */
KWBufBlock::KWBufBlock(const KWBufState &beginState)
 : m_beginState(beginState), m_endState(beginState)
{
   m_rawData1Start = 0;
   m_rawData2End = 0;  
   m_rawSize = 0;
   m_vmDataOffset = 0;
   b_stringListValid = false;
   b_rawDataValid = false;
   b_vmDataValid = false;
   b_appendEOL = false;
}

/**
 * Fill block with lines from @p data1 and @p data2.
 * The first line starts at @p data1[@p dataStart].
 * If @p last is true, all bytes from @p data2 are stored.
 * @return The number of bytes stored form @p data2
 */
int 
KWBufBlock::blockFill(int dataStart, QByteArray data1, QByteArray data2, bool last)
{
   m_rawData1 = data1;
   m_rawData1Start = dataStart;
   m_rawData2 = data2;
  
   int lineNr = m_beginState.lineNr;

   const char *p;
   const char *e;
   QString lastLine;
   if (!m_rawData1.isEmpty())
   {
      p = m_rawData1.data() + m_rawData1Start;
      e = m_rawData1.data() + m_rawData1.count();
      while(p < e)
      {
         if (*p == '\n')
         {
            lineNr++;
         }
         p++;
      }
   }

   p = m_rawData2.data();
   e = m_rawData2.data() + m_rawData2.count();
   const char *l = 0;
   while(p < e)
   {
      if (*p == '\n')
      {
         lineNr++;
         l = p+1;
      }
      p++;
   }

   // If this is the end of the data OR 
   // if the data did not contain any linebreaks up to now
   // create a line break at the end of the block.
   if ((last && (e != l)) || 
       (l == 0))
   {
      b_appendEOL = true;
      lineNr++;
      l = e;
   }

   m_rawData2End = l - m_rawData2.data();
   m_endState.lineNr = lineNr;
   m_rawSize = m_rawData1.count() - m_rawData1Start + m_rawData2End;
   b_rawDataValid = true;
   return m_rawData2End;
}

/**
 * Swaps raw data to secondary storage.
 * Uses the filedescriptor @p swap_fd and the file-offset @p swap_offset
 * to store m_rawSize bytes.
 */
void 
KWBufBlock::swapOut(int swap_fd, long swap_offset)
{
   assert(b_rawDataValid);
   // TODO: Error checking and reporting (?)
   if (!b_vmDataValid)
   {
      m_vmDataOffset = swap_offset;
      lseek(swap_fd, m_vmDataOffset, SEEK_SET);
      if (!m_rawData1.isEmpty())
      {
         writeBlock(swap_fd, m_rawData1, m_rawData1Start, m_rawData1.count());
      }
      if (!m_rawData2.isEmpty())
      {
         writeBlock(swap_fd, m_rawData2, 0, m_rawData2End);
      }
   }
   assert(m_vmDataOffset == swap_offset); // Shouldn't change!
   m_rawData1 = QByteArray();
   m_rawData1Start = 0;
   m_rawData2 = QByteArray();
   m_rawData2End = 0;
   b_rawDataValid = false;
   b_vmDataValid = true;
}

/**
 * Swaps m_rawSize bytes in from offset m_vmDataOffset in the file
 * with file-descirptor swap_fd.
 */
void 
KWBufBlock::swapIn(int swap_fd)
{
   assert(b_vmDataValid);
   assert(!b_rawDataValid);
   lseek(swap_fd, m_vmDataOffset, SEEK_SET);
   m_rawData2 = readBlock(swap_fd, m_rawSize);
   assert(m_rawData2 == m_rawSize); // TODO: Error checking / reporting.
   m_rawData2End = m_rawSize;
}

/**
 * Create a valid stringList.
 */
void 
KWBufBlock::buildStringList()
{
   assert(m_stringList.count() == 0);
   const char *p;
   const char *e;
   const char *l = 0; // Pointer to start of last line.
   QString lastLine;
   if (!m_rawData1.isEmpty())
   {
      p = m_rawData1.data() + m_rawData1Start;
      e = m_rawData1.data() + m_rawData1.count();
      l = p;
      while(p < e)
      {
         if (*p == '\n')
         {
            // TODO: Use codec
            QString line = QString::fromLatin1(l, (p-l-1)+1);
            TextLine::Ptr textLine = new TextLine();
            textLine->append(line.unicode(), line.length());
            m_stringList.append(textLine);
            l = p+1;
         }
         p++;
      }
      if (l < e)
         lastLine = QString::fromLatin1(l, (e-l)+1);
   }

   if (!m_rawData2.isEmpty())
   {
      p = m_rawData2.data();
      e = m_rawData2.data() + m_rawData2End;
      l = p;
      while(p < e)
      {
         if (*p == '\n')
         {
            QString line = QString::fromLatin1(l, (p-l-1)+1);
            if (!lastLine.isEmpty())
            {
               line = lastLine + line;
               lastLine.truncate(0);
            }
            TextLine::Ptr textLine = new TextLine();
            textLine->append(line.unicode(), line.length());
            m_stringList.append(textLine);
            l = p+1;
         }
         p++;
      }

      // If this is the end of the data OR 
      // if the data did not contain any linebreaks up to now
      // create a line break at the end of the block.
      if (b_appendEOL)
      {
         QString line = QString::fromLatin1(l, (e-l)+1);
         if (!lastLine.isEmpty())
         {
            line = lastLine + line;
            lastLine.truncate(0);
         }
         TextLine::Ptr textLine = new TextLine();
         textLine->append(line.unicode(), line.length());
         m_stringList.append(textLine);
      }
   }
   assert(m_stringList.count() == (m_endState.lineNr - m_beginState.lineNr));
   b_stringListValid = true;
}

/**
 * Dispose of a stringList.
 */
void
KWBufBlock::disposeStringList()
{
   assert(b_rawDataValid);
   m_stringList.clear();
   b_stringListValid = false;      
}


/**
 * Return line @p i
 * The first line of this block is line 0.
 */
TextLine::Ptr
KWBufBlock::line(int i)
{
   assert(b_stringListValid);
   assert(i < m_stringList.count());
   return m_stringList[i];
}

void
KWBufBlock::insertLine(int i, TextLine::Ptr line)
{
   assert(b_stringListValid);
   assert(i <= m_stringList.count());
   m_stringList.insert(m_stringList.at(i), line);  
   m_endState.lineNr++;
}

void
KWBufBlock::removeLine(int i)
{
   assert(b_stringListValid);
   assert(i < m_stringList.count());
   m_stringList.remove(m_stringList.at(i));  
   m_endState.lineNr--;
}

#include <kwbuffer.moc>
