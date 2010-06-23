/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
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

#include "kateswapfile.h"
#include "katerecover.h"

#include <QtCore/QDataStream>
#include <QFile>

namespace Kate {

const static char EA_StartEditing  = 0x02; // 0000 0010
const static char EA_FinishEditing = 0x03; // 0000 0011
const static char EA_WrapLine      = 0x04; // 0000 0100
const static char EA_UnwrapLine    = 0x05; // 0000 0101
const static char EA_InsertText    = 0x08; // 0000 1000
const static char EA_RemoveText    = 0x09; // 0000 1001



SwapFile::SwapFile(KateDocument *document)
  : QObject(document)
  , m_document(document)
  , m_trackingEnabled(false)
  , swapfile(NULL)
{
	m_stream = new QDataStream();
	connect(&m_document->buffer(), SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
	connect(&m_document->buffer(), SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));
	kDebug( 13020 ) << "Swap file initializer";
}

SwapFile::~SwapFile()
{
  if (swapfile)
    swapfile->remove();
}

void SwapFile::setTrackingEnabled(bool enable)
{
	kDebug( 13020 ) << "Enabling tracking";
  if (m_trackingEnabled == enable) {
      return;
  }

  m_trackingEnabled = enable;

  //KateBuffer *buffer = m_document->buffer();
  TextBuffer &buffer = m_document->buffer();
 
  if (m_trackingEnabled) {
    //connect(&buffer, SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
    //connect(&buffer, SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));

    connect(&buffer, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    connect(&buffer, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    connect(&buffer, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    connect(&buffer, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    connect(&buffer, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    connect(&buffer, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  } else {
    disconnect(&buffer, SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
    disconnect(&buffer, SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));

    disconnect(&buffer, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    disconnect(&buffer, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    disconnect(&buffer, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    disconnect(&buffer, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    disconnect(&buffer, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    disconnect(&buffer, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  }
}

bool SwapFile::isTrackingEnabled() const
{
  return m_trackingEnabled;
}

// vim's sucking implementation is in function ml_recover in the file
// https://vim.svn.sourceforge.net/svnroot/vim/branches/vim7.2/src/memline.c
void SwapFile::fileLoaded(const QString&)
{  
  // 1. look for swap file
  if (!swapfile)
  {
    KUrl url = m_document->url();
    if (!url.isLocalFile())
      return;
    
    setTrackingEnabled(true);
      
    QString path = url.toLocalFile();
    int poz = path.lastIndexOf('/');
    path.insert(poz+1, ".swp.");

    swapfile = new QFile(path);
  }
  
  bool exists;

  if (!swapfile->exists())
  {
    kDebug (13020) << "No swap file";
    return;
  }
	
  if (!swapfile->open(QIODevice::ReadOnly))
  {
    kWarning() << "Can't open swap file";
    return;
  }
  
  m_stream->setDevice(swapfile);
	

  // 2. if exists, ask user whether to recover data
  // TODO: implement
  // NOTE: modified date of swap file should be newer than original file
  MyWidget recover(m_document->activeView());
/*  
  do
 {
  }
  while (recover.getOption() == NOTDEF);
  
  exists = recover.getOption() == RECOVER ? true : false;
*/

  // 3. if requested, replay swap file
  // FIXME: make sure start/finishEditing is blanced!

  exists = true;
  while(1) {};
  if (exists) { // replay
    KateBuffer &buffer = m_document->buffer();
    while (!m_stream->atEnd()) {
      qint8 type;
      *m_stream >> type;
      switch (type) {
        case EA_StartEditing: {
          buffer.editStart();
          break;
        }
        case EA_FinishEditing: {
          buffer.editEnd();
          break;
        }
        case EA_WrapLine: {
          int line, column;
          *m_stream >> line >> column;
          buffer.wrapLine(KTextEditor::Cursor(line, column));
          break;
        }
        case EA_UnwrapLine: {
          int line;
          *m_stream >> line;
          buffer.unwrapLine(line);
          break;
        }
        case EA_InsertText: {
          int line, column;
          QString text;
          *m_stream >> line >> column >> text;
          buffer.insertText(KTextEditor::Cursor(line, column), text);
          break;
        }
        case EA_RemoveText: {
          int startLine, startColumn, endLine, endColumn;
          *m_stream >> startLine >> startColumn >> endLine >> endColumn;
          buffer.removeText(KTextEditor::Range(KTextEditor::Cursor(startLine, startColumn),
                                                KTextEditor::Cursor(endLine, endColumn)));
        }
        default: {
          kWarning() << "Unknown type:" << type;
        }
      }
    }
  }
}

void SwapFile::fileSaved(const QString&)
{
  // TODO:
  // 1. purge existing/old swap file
  // 2. remember file name and create a swap file on the first editing action

  // !! nu merge sa scriu in m_stream
  
  if (!swapfile)
  {
    KUrl url = m_document->url();
    if (!url.isLocalFile())
      return;
    
    setTrackingEnabled(true);
      
    QString path = url.toLocalFile();
    int poz = path.lastIndexOf('/');
    path.insert(poz+1, ".swp.");

    swapfile = new QFile(path);
  }
  
  if (swapfile->exists())
    swapfile->remove();
  
  swapfile->open(QIODevice::WriteOnly);
  m_stream->setDevice(swapfile);
}

void SwapFile::startEditing ()
{
  // format: char  
  *m_stream << EA_StartEditing;
}

void SwapFile::finishEditing ()
{
  // format: char
  *m_stream << EA_FinishEditing;
  swapfile->flush();
}

void SwapFile::wrapLine (const KTextEditor::Cursor &position)
{
  // format: char, int, int
  *m_stream << EA_WrapLine << position.line() << position.column();
}

void SwapFile::unwrapLine (int line)
{
  // format: char, int
  *m_stream << EA_UnwrapLine << line;
}

void SwapFile::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // format: char, int, int, string
  *m_stream << EA_InsertText << position.line() << position.column() << text;
}

void SwapFile::removeText (const KTextEditor::Range &range)
{
  // format: char, int, int, int, int
  *m_stream << EA_RemoveText
            << range.start().line() << range.start().column()
            << range.end().line() << range.end().column();
}
}
