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
{
	setTrackingEnabled(true);
}

void SwapFile::setTrackingEnabled(bool enable)
{
  if (m_trackingEnabled == enable) {
      return;
  }

  m_trackingEnabled = enable;

  //KateBuffer *buffer = m_document->buffer();
  if (m_trackingEnabled) {
    connect(m_document, SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
    connect(m_document, SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));

    connect(m_document, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    connect(m_document, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    connect(m_document, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    connect(m_document, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    connect(m_document, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    connect(m_document, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  } else {
    disconnect(m_document, SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
    disconnect(m_document, SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));

    disconnect(m_document, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    disconnect(m_document, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    disconnect(m_document, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    disconnect(m_document, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    disconnect(m_document, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    disconnect(m_document, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  }
}

bool SwapFile::isTrackingEnabled() const
{
  return m_trackingEnabled;
}

// vim's sucking implementation is in function ml_recover in the file
// https://vim.svn.sourceforge.net/svnroot/vim/branches/vim7.2/src/memline.c
void SwapFile::fileLoaded(const QString& filename)
{
  // 1. look for swap file
  // TODO: implement
  QFile *swapf = new QFile (".swp." + filename);
  bool exists;

  if (!swapf->exists())
    return;
	
  if (!swapf->open(QIODevice::ReadOnly))
  {
    kWarning() << "Can't open swap file";
    return;
  }
  m_stream->setDevice(swapf);
	

  // 2. if exists, ask user whether to recover data
  // TODO: implement
  // NOTE: modified date of swap file should be newer than original file
  MyWidget recover(m_document->activeView());
  
  do
 {
  }
  while (recover.getOption() == NOTDEF);
  
  exists = recover.getOption() == RECOVER ? true : false;

  // 3. if requested, replay swap file
  // FIXME: make sure start/finishEditing is blanced!
  if (exists) { // replay
    KateBuffer &buffer = m_document->buffer();
    while (!m_stream->atEnd()) {
      char *type;
      *m_stream >> type;
      switch (*type) {
        case EA_StartEditing: {
          buffer.startEditing();
          break;
        }
        case EA_FinishEditing: {
          buffer.startEditing();
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

void SwapFile::fileSaved(const QString &filename)
{
  // TODO:
  // 1. purge existing/old swap file
  // 2. remember file name and create a swap file on the first editing action
  	
  setTrackingEnabled(true);
  
  QFile *swapf = new QFile(".swp." + filename);
  
  if (swapf->exists())
    swapf->remove();
  
  swapf->open(QIODevice::WriteOnly);
  m_stream->setDevice(swapf);
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
