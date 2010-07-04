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

#include <QFileInfo>
#include <QDir>

namespace Kate {

const static qint8 EA_StartEditing  = 0x02; // 0000 0010
const static qint8 EA_FinishEditing = 0x03; // 0000 0011
const static qint8 EA_WrapLine      = 0x04; // 0000 0100
const static qint8 EA_UnwrapLine    = 0x05; // 0000 0101
const static qint8 EA_InsertText    = 0x08; // 0000 1000
const static qint8 EA_RemoveText    = 0x09; // 0000 1001



SwapFile::SwapFile(KateDocument *document)
  : QObject(document)
  , m_document(document)
  , m_trackingEnabled(false)
{
  connect(&m_document->buffer(), SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
  connect(&m_document->buffer(), SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));
  
  setTrackingEnabled(true);
}

SwapFile::~SwapFile()
{
  m_stream.setDevice(0);
  if (m_swapfile.exists())
    m_swapfile.remove();
}
void SwapFile::setTrackingEnabled(bool enable)
{
  if (m_trackingEnabled == enable) {
      return;
  }

  m_trackingEnabled = enable;

  //KateBuffer *buffer = m_document->buffer();
  TextBuffer &buffer = m_document->buffer();
 
  if (m_trackingEnabled) {
    connect(&buffer, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    connect(&buffer, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    connect(&buffer, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    connect(&buffer, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    connect(&buffer, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    connect(&buffer, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  } else {
    disconnect(&buffer, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    disconnect(&buffer, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    disconnect(&buffer, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    disconnect(&buffer, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    disconnect(&buffer, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    disconnect(&buffer, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  }
}

void SwapFile::fileLoaded(const QString&)
{
  // TODO FIXME: remove old swap file if there exists one
  
  // look for swap file
  KUrl url = m_document->url();
  if (!url.isLocalFile())
    return;
      
  QString path = url.toLocalFile();
  int poz = path.lastIndexOf(QDir::separator());
  path.insert(poz+1, ".swp.");

  m_swapfile.setFileName(path);

  if (!m_swapfile.exists())
  {
    kDebug (13020) << "No swap file";
    return;
  }

  if (!QFileInfo(m_swapfile).isReadable())
  {
    kWarning( 13020 ) << "Can't open swap file (missing permissions)";
    return;
  }
  
  emit swapFileFound();
  // TODO set file as read-only
}

void SwapFile::recover()
{
  setTrackingEnabled(false);

  m_swapfile.open(QIODevice::ReadOnly);
  m_stream.setDevice(&m_swapfile);
  KateBuffer &buffer = m_document->buffer();
  bool editStarted = false;
  while (!m_stream.atEnd()) {
    qint8 type;
    m_stream >> type;
    switch (type) {
      case EA_StartEditing: {
        buffer.editStart();
        editStarted = true;
        break;
      }
      case EA_FinishEditing: {
        buffer.editEnd();
        editStarted = false;
        break;
      }
      case EA_WrapLine: {
        int line, column;
        m_stream >> line >> column;
        buffer.wrapLine(KTextEditor::Cursor(line, column));
        break;
      }
      case EA_UnwrapLine: {
        int line;
        m_stream >> line;
        buffer.unwrapLine(line);
        break;
      }
      case EA_InsertText: {
        int line, column;
        QString text;
        m_stream >> line >> column >> text;
        buffer.insertText(KTextEditor::Cursor(line, column), text);
        break;
      }
      case EA_RemoveText: {
        int startLine, startColumn, endLine, endColumn;
        m_stream >> startLine >> startColumn >> endLine >> endColumn;
        buffer.removeText(KTextEditor::Range(KTextEditor::Cursor(startLine, startColumn),
                                              KTextEditor::Cursor(endLine, endColumn)));
      }
      default: {
        kWarning( 13020 ) << "Unknown type:" << type;
      }
    }
  }
  
  if (editStarted) {
    kWarning ( 13020 ) << "Some data might be lost";
    buffer.editEnd();
  }
  
  m_stream.setDevice(0);
  m_swapfile.close();

  setTrackingEnabled(true);
}

void SwapFile::fileSaved(const QString&)
{
  // remove old swap file (e.g. if a file A was "saved as" B)
  if (m_swapfile.exists()) {
    m_stream.setDevice(0);
    m_swapfile.close();
    m_swapfile.remove();
  }
  
  KUrl url = m_document->url();
  if (!url.isLocalFile())
    return;

  QString path = url.toLocalFile();
  int poz = path.lastIndexOf(QDir::separator());
  path.insert(poz+1, ".swp.");

  m_swapfile.setFileName(path);
}

void SwapFile::startEditing ()
{
  if (!m_swapfile.exists()) {
    m_swapfile.open(QIODevice::WriteOnly);
    m_stream.setDevice(&m_swapfile);
  } else if (m_stream.device() == 0) {
    m_swapfile.open(QIODevice::Append);
    m_stream.setDevice(&m_swapfile);
  }
  
  // format: char  
  m_stream << EA_StartEditing;
}

void SwapFile::finishEditing ()
{
  // format: char
  m_stream << EA_FinishEditing;
  m_swapfile.flush();
}

void SwapFile::wrapLine (const KTextEditor::Cursor &position)
{
  // format: char, int, int
  m_stream << EA_WrapLine << position.line() << position.column();
}

void SwapFile::unwrapLine (int line)
{
  // format: char, int
  m_stream << EA_UnwrapLine << line;
}

void SwapFile::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // format: char, int, int, string
  m_stream << EA_InsertText << position.line() << position.column() << text;
}

void SwapFile::removeText (const KTextEditor::Range &range)
{
  // format: char, int, int, int, int
  m_stream << EA_RemoveText
            << range.start().line() << range.start().column()
            << range.end().line() << range.end().column();
}
}

// kate: space-indent on; indent-width 2; replace-tabs on;
