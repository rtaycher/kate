/* 
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

#ifndef _KWBUFFER_H_
#define _KWBUFFER_H_

#include <qstring.h>
#include <qstringlist.h>
#include <qlist.h>
#include <qobject.h>
#include <qtimer.h>

class KWBufBlock;
class KWBufFileLoader;
class KWBufState;
class QTextCodec;

/**
 * The KWBuffer class maintains a collections of lines.
 * It allows to maintain state information in a lazy way. 
 * It handles swapping out of data using secondary storage.
 *
 * It is designed to handle large amounts of text-data efficiently
 * with respect to CPU and memory usage.
 *
 * @author Waldo Bastian <bastian@kde.org>
 */
class KWBuffer : public QObject
{
   Q_OBJECT
public:
   /**
    * Create an empty buffer.
    */
   KWBuffer();
   
   /**
    * Insert a file at line @p line in the buffer. 
    */
   void insertFile(int line, const QString &file);
   
   /**
    * Set the codec to decode the buffer with.
    */
   void setCodec(QTextCodec *codec);
     
   /**
    * Return the total number of lines in the buffer.
    */
   int count();
   
   /**
    * Return line @p i
    */
   const QString &line(int i);

signals:
   void linesChanged(int lines);

protected:
   /**
    * Make sure @p buf gets loaded.
    */
   void loadBlock(KWBufBlock *buf);

   /**
    * Make sure @p buf gets parsed.
    */
   void parseBlock(KWBufBlock *buf);
   
protected slots:
   void slotLoadFile();
   
protected:
   int m_totalLines;
   int m_fdSwap;
   QTextCodec *m_codec;
   QList<KWBufBlock> m_blocks;
   QList<KWBufFileLoader> m_loader;
   QTimer m_loadTimer;
   
   // List of parsed blocks that can be disposed.
   QList<KWBufBlock> m_parsedBlocksClean; 
   // List of blocks that can be swapped out.
   QList<KWBufBlock> m_loadedBlocks; 
};

class KWBufFileLoader
{
public:
  int fd;
  QByteArray lastBlock;
  int dataStart;
  int blockNr;  
};



class KWBufState
{
public:
   long lineNr;
};


/**
 * The KWBufBlock class contains an amount of data representing 
 * a certain number of lines.
 */
class KWBufBlock
{
   friend class KWBuffer;
public:
   /*
    * Create an empty block.
    */
   KWBufBlock(const KWBufState &beginState);

   /**
    * Fill block with lines from @p data1 and @p data2.
    * The first line starts at @p data1[@p dataStart].
    * If @p last is true, all bytes from @p data2 are stored.
    * @return The number of bytes stored form @p data2
    */
   int blockFill(int dataStart, QByteArray data1, QByteArray data2, bool last);

   /**
    * Create a valid stringList.
    * Post Condition: b_stringListValid is true.
    */
   void buildStringList();

   /**
    * Dispose of a stringList.
    * Post Condition: b_stringListValid is false.
    */
   void disposeStringList();

   /**
    * Swaps raw data to secondary storage.
    * Uses the filedescriptor @p swap_fd and the file-offset @p swap_offset
    * to store m_rawSize bytes.
    * Post Condition: b_vmDataValid is true, b_rawDataValid is false
    */
   void swapOut(int swap_fd, long swap_offset);

   /**
    * Swaps m_rawSize bytes in from offset m_vmDataOffset in the file
    * with file-descirptor swap_fd.
    * Post Condition: b_rawDataValid is true.
    */
   void swapIn(int swap_fd);
    
   /**
    * Return line @p i
    * The first line of this block is line 0.
    */
   const QString &line(int i);
  
protected:
   QStringList *m_stringList;
   QByteArray m_rawData1;   
   int m_rawData1Start;
   QByteArray m_rawData2;   
   int m_rawData2End;
   long m_rawSize;
   long m_vmDataOffset;
   bool b_stringListValid;
   bool b_rawDataValid;
   bool b_vmDataValid;
   bool b_appendEOL; // Buffer is not terminated with '\n'.
   KWBufState m_beginState;
   KWBufState m_endState;
};

#endif
