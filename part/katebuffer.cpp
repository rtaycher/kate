/*
   This file is part of Kate     
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
     
     
#include "katebuffer.h"     
     
// Includes for reading file     
#include <sys/types.h>     
#include <sys/stat.h>     
#include <fcntl.h>     
#include <errno.h>     
#include <unistd.h>     
     
#include <qfile.h>     
#include <qtimer.h>     
#include <qtextcodec.h>     
     
#include "katevmallocator.h"     
     
//#include <katebuffer.moc>     
     
#include <assert.h>     
#include <kdebug.h>     
#define LOADED_BLOCKS_MAX	10     
#define DIRTY_BLOCKS_MAX        1     
#define AVG_BLOCK_SIZE		8192     
     
/**     
 * Create an empty buffer.     
 */     
KateBuffer::KateBuffer()     
{     
   m_blocks.setAutoDelete(true);     
   m_loader.setAutoDelete(true);     
   connect( &m_loadTimer, SIGNAL(timeout()), this, SLOT(slotLoadFile()));     
   m_vm = 0;     
   clear();     
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
   //kdDebug(13020)<<"Read = "<< bytesRead<<endl;     
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
     
void     
KateBuffer::clear()     
{     
   m_parsedBlocksClean.clear();     
   m_parsedBlocksDirty.clear();     
   m_loadedBlocks.clear();     
   m_loader.clear();     
   m_blocks.clear();     
   delete m_vm;     
   m_vm = new KVMAllocator;     
   KateBufState state;     
   // Initial state.     
   state.lineNr = 0;     
   KateBufBlock *block = new KateBufBlock(state);     
   m_blocks.insert(0, block);     
   block->b_rawDataValid = true;     
   block->b_appendEOL = true;     
   block->b_emptyBlock = true;     
   block->m_endState.lineNr++;     
   m_loadedBlocks.append(block);
   m_totalLines = block->m_endState.lineNr;     
}     
     
/**     
 * Insert a file at line @p line in the buffer.     
 */     
void     
KateBuffer::insertFile(uint line, const QString &file, QTextCodec *codec)
{
  assert(line == 0); // Inserting at other places not yet handled.

  int fd = open(QFile::encodeName(file), O_RDONLY);
  if (fd < 0)
  {
      //kdDebug(13020)<<"Error loading file.\n";
     return; // Do some error propagation here.
  }

  KateBufFileLoader *loader = new KateBufFileLoader;
  loader->fd = fd;
  loader->dataStart = 0;
  loader->blockNr = 0;
  loader->codec = codec;
  m_loader.append(loader);

  loadFilePart();
}

void
KateBuffer::loadFilePart()
{
  const int blockSize = AVG_BLOCK_SIZE;
  const int blockRead = 3; // Read 5 blocks in a row

  KateBufFileLoader *loader = m_loader.first();

  KateBufState state;
  if (loader->blockNr > 0)
  {
     KateBufBlock *block = m_blocks.at(loader->blockNr-1);
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
     KateBufBlock *block = new KateBufBlock(state);
     m_blocks.insert(loader->blockNr++, block);
     m_loadedBlocks.append(block);
     if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)
     {
        KateBufBlock *buf2 = m_loadedBlocks.take(2);
        //kdDebug(13020)<<"swapOut "<<buf2<<endl;     
        buf2->swapOut(m_vm);     
        assert(m_parsedBlocksClean.find(buf2) == -1);
        assert(m_parsedBlocksDirty.find(buf2) == -1);
     }     
     block->m_codec = loader->codec;     
     loader->dataStart = block->blockFill(loader->dataStart,     
                                  loader->lastBlock, currentBlock, eof);     
     state = block->m_endState;     
     //kdDebug(13020)<<"current->ref ="<<currentBlock.nrefs()<<" last->ref ="<<loader->lastBlock.nrefs()<<endl;     
     loader->lastBlock = currentBlock;     
     if (eof) break;     
  }     
  if (eof)     
  {     
     //kdDebug(13020)<<"Loading finished.\n";     
     close( loader->fd );
     m_loader.removeRef(loader);     
//     emit needHighlight(startLine,state.lineNr);     
  }     
  if (m_loader.count())     
  {     
      //kdDebug(13020)<<"Starting timer...\n";     
     m_loadTimer.start(0, true);     
 //JW 0     
  }     
     
  m_totalLines += state.lineNr - startLine;     
//    emit needHighlight(startLine,state.lineNr);     
}


void
KateBuffer::insertData(uint line, const QByteArray &data, QTextCodec *codec)
{
   assert(line == m_totalLines);
   KateBufBlock *prev_block;

   // Remove all preceding empty blocks.
   while(true)
   {
      prev_block = m_blocks.last();
      if (!prev_block || !prev_block->b_emptyBlock)
         break;

      m_totalLines -= prev_block->m_endState.lineNr - prev_block->m_beginState.lineNr;
kdDebug(13020)<< "Removing empty block "<< prev_block << endl;
      m_blocks.removeRef(prev_block);
      m_parsedBlocksClean.removeRef(prev_block);
      m_parsedBlocksDirty.removeRef(prev_block);
      m_loadedBlocks.removeRef(prev_block);
   }

   KateBufState state;
   if (prev_block)
   {
       state = prev_block->m_endState;
   }
   else
   {
        // Initial state.
       state.lineNr = 0;
   }

  int startLine = state.lineNr;
  KateBufBlock *block = new KateBufBlock(state);
  m_blocks.append(block);
  m_loadedBlocks.append(block);
  block->m_codec = codec;

  // TODO: We always create a new block.
  // It would be more correct to collect the data in larger blocks.
  // We should do that without unnecasserily copying the data though.
  QByteArray lastData;
  int lastLine = 0;
  if (prev_block && prev_block->b_appendEOL && (prev_block->m_codec == codec))
  {
     // Take the last line of the previous block and add it to the
     // the new block.
     prev_block->truncateEOL(lastLine, lastData);
     m_totalLines--;
  }
  block->blockFill(lastLine, lastData, data, true);
  state = block->m_endState;
  m_totalLines += state.lineNr - startLine;
}

void
KateBuffer::slotLoadFile()
{
  loadFilePart();
  emit linesChanged(m_totalLines);
}

/**
 * Return the total number of lines in the buffer.
 */
uint
KateBuffer::count()
{
   return m_totalLines;
}

KateBufBlock *
KateBuffer::findBlock(uint i)
{
   if ((i >= m_totalLines))
      return 0;

   uint lastLine = 0;
   // This needs a bit of optimisation/caching so that we don't walk
   // through the list every time.
   KateBufBlock *buf;
   for(buf = m_blocks.current(); buf; )
   {
      lastLine = buf->m_endState.lineNr;
      if (i < buf->m_beginState.lineNr)
      {
         // Search backwards
         buf = m_blocks.prev();
      }
      else if ((i >= buf->m_beginState.lineNr) && (i < lastLine))
      {
         // We found the block.
         break;     
      }     
      else     
      {     
         // Search forwards     
         buf = m_blocks.next();     
         // Adjust line numbering....     
         if (buf->m_beginState.lineNr != lastLine)     
         {     
            int offset = lastLine - buf->m_beginState.lineNr;     
            buf->m_beginState.lineNr += offset;     
            buf->m_endState.lineNr += offset;     
         }     
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
KateBuffer::line(uint i)
{     
   KateBufBlock *buf = findBlock(i);     
   if (!buf)
      return 0;     
     
   if (!buf->b_stringListValid)     
   {     
      parseBlock(buf);     
   }     
   return buf->line(i - buf->m_beginState.lineNr);     
}     
     
void     
KateBuffer::insertLine(uint i, TextLine::Ptr line)
{     
   KateBufBlock *buf;     
   if (i == m_totalLines)     
      buf = findBlock(i-1);     
   else     
      buf = findBlock(i);     
     
   if (!buf)     
   {     
      KateBufState state;     
      // Initial state.     
      state.lineNr = 0;     
      buf = new KateBufBlock(state);     
      m_blocks.insert(0, buf);     
      buf->b_rawDataValid = true;
      m_loadedBlocks.append(buf);
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
KateBuffer::removeLine(uint i)
{
   KateBufBlock *buf = findBlock(i);     
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
   if (buf->m_beginState.lineNr == buf->m_endState.lineNr)
   {
kdDebug(13020)<< "Removing empty block "<< buf << endl;
      if (buf->b_vmDataValid)     
      {
kdDebug(13020)<< "Empty block still has VM."<< buf << endl;
         assert(false);
         buf->disposeSwap(m_vm);
      }
      m_blocks.removeRef(buf);     
      m_parsedBlocksClean.removeRef(buf);     
      m_parsedBlocksDirty.removeRef(buf);     
      m_loadedBlocks.removeRef(buf);
   }
}     
     
void     
KateBuffer::changeLine(uint i)
{     
    ////kdDebug(13020)<<"changeLine "<< i<<endl;     
   KateBufBlock *buf = findBlock(i);     
   assert(buf);     
   assert(buf->b_stringListValid);
   if (buf->b_rawDataValid)     
   {     
      dirtyBlock(buf);     
   }     
   emit textChanged();     
}     
     
void     
KateBuffer::parseBlock(KateBufBlock *buf)
{     
   //kdDebug(13020)<<"parseBlock "<< buf<<endl;     
   if (!buf->b_rawDataValid)     
      loadBlock(buf);     
   if (m_parsedBlocksClean.count() > 5)     
   {     
      KateBufBlock *buf2 = m_parsedBlocksClean.take(2);     
      buf2->disposeStringList();     
      m_loadedBlocks.append(buf2);
      assert(m_parsedBlocksDirty.find(buf2) == -1);
   }     
   buf->buildStringList();     
   assert(m_parsedBlocksClean.find(buf) == -1);     
   m_parsedBlocksClean.append(buf);
   assert(m_loadedBlocks.find(buf) != -1);     
   m_loadedBlocks.removeRef(buf);
     
   // From now on store the raw block in unicode.     
   // As a side-effect this will also store the highlighting info, which     
   // is the real reason that we do this.     
   if (buf->m_codec)     
      dirtyBlock(buf);     
}     
     
void     
KateBuffer::loadBlock(KateBufBlock *buf)     
{     
    //kdDebug(13020)<<"loadBlock "<<buf<<endl;     
   if (m_loadedBlocks.count() > LOADED_BLOCKS_MAX)     
   {     
      KateBufBlock *buf2 = m_loadedBlocks.take(2);     
      //kdDebug(13020)<<"swapOut "<<buf2<<endl;     
      buf2->swapOut(m_vm);     
      assert(m_parsedBlocksClean.find(buf2) == -1);     
      assert(m_parsedBlocksDirty.find(buf2) == -1);
   }     
     
   //kdDebug(13020)<<"swapIn "<<buf<<endl;     
   buf->swapIn(m_vm);     
   m_loadedBlocks.append(buf);     
   assert(m_parsedBlocksClean.find(buf) == -1);     
   assert(m_parsedBlocksDirty.find(buf) == -1);     
}     

void     
KateBuffer::dirtyBlock(KateBufBlock *buf)     
{     
   kdDebug(13020)<<"dirtyBlock "<<buf<<endl;     
   buf->b_emptyBlock = false;     
   if (m_parsedBlocksDirty.count() > DIRTY_BLOCKS_MAX)     
   {     
      KateBufBlock *buf2 = m_parsedBlocksDirty.take(0);     
      buf2->flushStringList(); // Copy stringlist to raw     
      buf2->disposeStringList(); // dispose stringlist.     
      m_loadedBlocks.append(buf2);     
      assert(m_parsedBlocksClean.find(buf2) == -1);
   }     
   assert(m_loadedBlocks.find(buf) == -1);
   m_parsedBlocksClean.removeRef(buf);     
   m_parsedBlocksDirty.append(buf);     
   buf->disposeRawData();     
   if (buf->b_vmDataValid)     
      buf->disposeSwap(m_vm);     
}     
     
//-----------------------------------------------------------------     
     
/**     
 * The KateBufBlock class contains an amount of data representing     
 * a certain number of lines.     
 */     
     
/*     
 * Create an empty block.     
 */     
KateBufBlock::KateBufBlock(const KateBufState &beginState)     
 : m_beginState(beginState), m_endState(beginState)     
{     
   m_rawData1Start = 0;
   m_rawData2End = 0;     
   m_rawSize = 0;     
   m_vmblock = 0;     
   b_stringListValid = false;     
   b_rawDataValid = false;     
   b_vmDataValid = false;     
   b_appendEOL = false;     
   b_emptyBlock = false;     
   m_lastLine = 0;
}     

/**     
 * Remove the last line of this block.     
 */     
void     
KateBufBlock::truncateEOL( int &lastLine, QByteArray &data1 )     
{     
   assert(b_appendEOL);     
   assert(b_rawDataValid);     
     
   data1 = m_rawData2;     
   lastLine = m_lastLine;     
   b_appendEOL = false;     
   m_rawData2End = m_lastLine;     
   m_rawSize = m_rawData1.count() - m_rawData1Start + m_rawData2End;     
     
   m_endState.lineNr--;     
   if (b_stringListValid)     
      m_stringList.pop_back();     
}     
     
/**     
 * Fill block with lines from @p data1 and @p data2.     
 * The first line starts at @p data1[@p dataStart].     
 * If @p last is true, all bytes from @p data2 are stored.     
 * @return The number of bytes stored form @p data2     
 */     
int     
KateBufBlock::blockFill(int dataStart, QByteArray data1, QByteArray data2, bool last)     
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
      if (!m_rawData1.isEmpty() || !m_rawData2.isEmpty())     
      {     
         b_appendEOL = true;     
         if (l)     
            m_lastLine = l - m_rawData2.data();     
         else
            m_lastLine = 0;     
         lineNr++;     
      }     
      l = e;     
   }     
     
   m_rawData2End = l - m_rawData2.data();     
   m_endState.lineNr = lineNr;     
   //kdDebug(13020)<<"blockFill "<<this<<" beginState ="<<m_beginState.lineNr<<"%ld endState ="<< m_endState.lineNr<<endl;
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
KateBufBlock::swapOut(KVMAllocator *vm)     
{     
   // kdDebug(13020)<<"KateBufBlock: swapout this ="<< this<<endl;     
   assert(b_rawDataValid);     
   // TODO: Error checking and reporting (?)     
   if (!b_vmDataValid)     
   {     
      m_vmblock = vm->allocate(m_rawSize);     
      off_t ofs = 0;     
      if (!m_rawData1.isEmpty())     
      {     
         ofs = m_rawData1.count() - m_rawData1Start;     
         vm->copy(m_vmblock, m_rawData1.data()+m_rawData1Start, 0, ofs);     
      }     
      if (!m_rawData2.isEmpty())     
      {     
         vm->copy(m_vmblock, m_rawData2.data(), ofs, m_rawData2End);     
      }     
      b_vmDataValid = true;     
   }     
   disposeRawData();     
}     
     
/**
 * Swaps m_rawSize bytes in from offset m_vmDataOffset in the file     
 * with file-descirptor swap_fd.     
 */     
void     
KateBufBlock::swapIn(KVMAllocator *vm)     
{     
   // kdDebug(13020)<<"KateBufBlock: swapin this ="<< this<<endl;     
   assert(b_vmDataValid);     
   assert(!b_rawDataValid);
   assert(m_vmblock);     
   m_rawData2.resize(m_rawSize);     
   vm->copy(m_rawData2.data(), m_vmblock, 0, m_rawSize);     
   m_rawData2End = m_rawSize;     
   b_rawDataValid = true;     
}     
     
     
/**     
 * Create a valid stringList.     
 */     
void     
KateBufBlock::buildStringList()     
{     
   //kdDebug(13020)<<"KateBufBlock: buildStringList this ="<< this<<endl;     
   assert(m_stringList.empty());     
   if (!m_codec && !m_rawData2.isEmpty())     
   {     
      buildStringListFast();     
      return;     
   }     
     
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
            QString line = m_codec->toUnicode(l, (p-l-1)+1);     
            TextLine::Ptr textLine = new TextLine();     
            textLine->append(line.unicode(), line.length());     
            m_stringList.push_back(textLine);     
            l = p+1;     
         }     
         p++;     
      }
      if (l < e)     
         lastLine = m_codec->toUnicode(l, (e-l-1)+1);     
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
            QString line = m_codec->toUnicode(l, (p-l-1)+1);     
            if (!lastLine.isEmpty())     
            {     
               line = lastLine + line;     
               lastLine.truncate(0);     
            }     
            TextLine::Ptr textLine = new TextLine();     
            textLine->append(line.unicode(), line.length());
            m_stringList.push_back(textLine);
            l = p+1;
         }
         p++;
      }

      // If this is the end of the data OR
      // if the data did not contain any linebreaks up to now
      // create a line break at the end of the block.
      if (b_appendEOL)
      {
          //kdDebug(13020)<<"KateBufBlock: buildStringList this ="<<this<<" l ="<< l<<" e ="<<e <<" (e-l)+1 ="<<(e-l)<<endl;
         QString line = m_codec->toUnicode(l, (e-l));
         //kdDebug(13020)<<"KateBufBlock: line ="<< line.latin1()<<endl;
         if (!lastLine.isEmpty())
         {
            line = lastLine + line;
            lastLine.truncate(0);
         }
         TextLine::Ptr textLine = new TextLine();
         textLine->append(line.unicode(), line.length());
         m_stringList.push_back (textLine);
      }
   }
   else
   {
      if (b_appendEOL)
      {
         TextLine::Ptr textLine = new TextLine();
         m_stringList.push_back (textLine);
      }
   }
   assert(m_stringList.size() == (m_endState.lineNr - m_beginState.lineNr));
   m_stringListIt = m_stringList.begin();
   m_stringListCurrent = 0;
   b_stringListValid = true;
}

/**
 * Flush string list
 * Copies a string list back to the raw buffer.
 */
void
KateBufBlock::flushStringList()
{
   kdDebug(13020)<<"KateBufBlock: flushStringList this ="<< this<<endl;
   assert(b_stringListValid);
   assert(!b_rawDataValid);

   // Stores the data as lines of <lenght><length characters>
   // both <length> as well as <character> have size of sizeof(QChar)

   // Calculate size.
   uint size = 0;
   for(TextLine::List::const_iterator it = m_stringList.begin();
       it != m_stringList.end(); ++it)
   {
      uint l = (*it)->textLen;
      uint lctx = (*it)->ctxLen;
      size += (2*sizeof(uint)) + (l*sizeof(QChar)) + l + 1 + sizeof(uint) + (lctx * sizeof(signed char));
   }
   //kdDebug(13020)<<"Size = "<< size<<endl;
   m_rawData2 = QByteArray(size);
   m_rawData2End = size;
   m_rawSize = size;
   char *buf = m_rawData2.data();
   // Copy data
   for(TextLine::List::iterator it = m_stringList.begin();
       it != m_stringList.end(); ++it)
   {
      TextLine *tl = (*it).data();
      uint l = tl->textLen;
      uint lctx = tl->ctxLen;

      memcpy(buf, &l, sizeof(uint));
      buf += sizeof(uint);

      memcpy(buf, &lctx, sizeof(uint));
      buf += sizeof(uint);

      memcpy(buf, (char *) tl->text, sizeof(QChar)*l);
      buf += sizeof(QChar)*l;

      memcpy(buf, (char *) tl->attributes, l);
      buf += l;

      memcpy(buf, (char *)&tl->attr, 1);
      buf += 1;

      memcpy(buf, &tl->myMark, sizeof(uint));
      buf += sizeof(uint);

      memcpy(buf, (signed char *)tl->ctx, lctx);
      buf += sizeof (signed char) * lctx;
   }
   assert(buf-m_rawData2.data() == (int)size);
   m_codec = 0; // No codec
   b_rawDataValid = true;
   kdDebug()<<"KateBuffer::FlushStringList"<<endl;

}

/**
 * Create a valid stringList from raw data in our own format.
 */
void
KateBufBlock::buildStringListFast()
{ 
   // kdDebug(13020)<<"KateBufBlock: buildStringListFast this = "<< this<<endl; 
   char *buf = m_rawData2.data(); 
   char *end = buf + m_rawSize; 
   while(buf < end) 
   { 
      uint l = 0; 
      uint lctx = 0; 
 
      memcpy((char *) &l, buf, sizeof(uint)); 
      buf += sizeof(uint); 
      memcpy((char *) &lctx, buf, sizeof(uint)); 
      buf += sizeof(uint); 
 
      TextLine::Ptr textLine = new TextLine(); 
 
      if (l > 0) 
      { 
        textLine->replace(0, 0, (QChar *) buf, l, (uchar *) buf+(sizeof(QChar)*l)); 
        buf += (sizeof(QChar)*l); 
        buf += l; 
      } 
 
      uchar a = 0; 
      memcpy((char *)&a, buf, sizeof(uchar)); 
      buf += sizeof(uchar); 
      textLine->attr = a; 
 
      uint mark = 0; 
      memcpy((char *)&mark, buf, sizeof(uint)); 
      buf += sizeof(uint); 
      textLine->myMark = mark; 
 
      if (lctx > 0)
      {
			  textLine->ctx = (signed char *)malloc (lctx);
				memcpy((signed char *)textLine->ctx, buf, lctx);
        buf += lctx;
      }
			textLine->ctxLen = lctx;

      m_stringList.push_back (textLine);
   }
   //kdDebug(13020)<<"stringList.count = "<< m_stringList.size()<<" should be "<< (m_endState.lineNr - m_beginState.lineNr) <<endl;
   assert(m_stringList.size() == (m_endState.lineNr - m_beginState.lineNr));
   m_stringListIt = m_stringList.begin(); 
   m_stringListCurrent = 0; 
   b_stringListValid = true; 
} 
 
/** 
 * Dispose of a stringList. 
 */ 
void 
KateBufBlock::disposeStringList() 
{ 
//   kdDebug(13020)<<"KateBufBlock: disposeStringList this = "<< this<<endl; 
   assert(b_rawDataValid || b_vmDataValid); 
   m_stringList.clear(); 
   b_stringListValid = false;     
}     
     
/**     
 * Dispose of raw data.     
 */     
void     
KateBufBlock::disposeRawData()     
{     
//   kdDebug(13020)<< "KateBufBlock: disposeRawData this = "<< this<<endl;     
   assert(b_stringListValid || b_vmDataValid);     
   b_rawDataValid = false;     
   m_rawData1 = QByteArray();     
   m_rawData1Start = 0;     
   m_rawData2 = QByteArray();     
   m_rawData2End = 0;     
}     
     
/**     
 * Dispose of data in vm
 */     
void     
KateBufBlock::disposeSwap(KVMAllocator *vm)     
{
   kdDebug(13020)<<"KateBufBlock: disposeSwap this = "<< this<<endl;     
   assert(b_stringListValid || b_rawDataValid);     
   vm->free(m_vmblock);     
   m_vmblock = 0;     
   b_vmDataValid = false;     
}     

/**
 * Make line @p i the current line
 */
void KateBufBlock::seek(uint i)
{
   if (m_stringListCurrent == (int)i)
      return;
   while(m_stringListCurrent < (int)i)
   {
      ++m_stringListCurrent;
      ++m_stringListIt;
   }
   while(m_stringListCurrent > (int)i)
   {
      --m_stringListCurrent;
      --m_stringListIt;
   }
}

/**
 * Return line @p i
 * The first line of this block is line 0.
 */
TextLine::Ptr     
KateBufBlock::line(uint i)
{     
   assert(b_stringListValid);     
   assert(i < m_stringList.size());
   seek(i);
   return *m_stringListIt;
}

void
KateBufBlock::insertLine(uint i, TextLine::Ptr line)
{
   assert(b_stringListValid);
   assert(i <= m_stringList.size());
   seek(i);
   m_stringListIt = m_stringList.insert(m_stringListIt, line);
   m_stringListCurrent = i;
   m_endState.lineNr++;
}

void
KateBufBlock::removeLine(uint i)
{
   assert(b_stringListValid);
   assert(i < m_stringList.size());
   seek(i);
   m_stringListIt = m_stringList.erase(m_stringListIt);
   m_stringListCurrent = i;
   m_endState.lineNr--;
}

#include "katebuffer.moc"


