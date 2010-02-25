/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#include "katetextbuffertest.h"
#include "katetextbuffer.h"
#include "katetextcursor.h"

QTEST_MAIN(KateTextBufferTest)

KateTextBufferTest::KateTextBufferTest()
  : QObject()
{
}

KateTextBufferTest::~KateTextBufferTest()
{
}

void KateTextBufferTest::basicBufferTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer;

  // one line per default
  Q_ASSERT (buffer.lines() == 1);
  Q_ASSERT (buffer.text () == "");

  //FIXME: use QTestLib macros for checking the correct state
  // start editing
  buffer.startEditing ();

  // end editing
  buffer.finishEditing ();
}

void KateTextBufferTest::wrapLineTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer;

  // wrap first empty line -> we should have two empty lines
  buffer.startEditing ();
  buffer.wrapLine(KTextEditor::Cursor(0, 0));
  buffer.finishEditing ();
  buffer.debugPrint ("Two empty lines");
  Q_ASSERT (buffer.text () == "\n");

  // unwrap again -> only one empty line
  buffer.startEditing ();
  buffer.unwrapLine(1);
  buffer.finishEditing ();

  // print debug
  buffer.debugPrint ("Empty Buffer");
  Q_ASSERT (buffer.text () == "");
}

void KateTextBufferTest::insertRemoveTextTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer (0, 1);

  // wrap first line
  buffer.startEditing ();
  buffer.wrapLine (KTextEditor::Cursor (0, 0));
  buffer.finishEditing ();
  buffer.debugPrint ("Two empty lines");
  Q_ASSERT (buffer.text () == "\n");

  // remember second line
  Kate::TextLine second = buffer.line (1);

  // unwrap second line
  buffer.startEditing ();
  buffer.unwrapLine (1);
  buffer.finishEditing ();
  buffer.debugPrint ("One empty line");
  Q_ASSERT (buffer.text () == "");

  // second text line should be still there
  //const QString &secondText = second->text ();
  //Q_ASSERT (secondText == "")

  // insert text
  buffer.startEditing ();
  buffer.insertText (KTextEditor::Cursor (0, 0), "testremovetext");
  buffer.finishEditing ();
  buffer.debugPrint ("One line");
  Q_ASSERT (buffer.text () == "testremovetext");

  // remove text
  buffer.startEditing ();
  buffer.removeText (KTextEditor::Range (KTextEditor::Cursor (0, 4), KTextEditor::Cursor (0, 10)));
  buffer.finishEditing ();
  buffer.debugPrint ("One line");
  Q_ASSERT (buffer.text () == "testtext");

  // wrap text
  buffer.startEditing ();
  buffer.wrapLine (KTextEditor::Cursor (0, 2));
  buffer.finishEditing ();
  buffer.debugPrint ("Two line");
  Q_ASSERT (buffer.text () == "te\nsttext");

  // unwrap text
  buffer.startEditing ();
  buffer.unwrapLine (1);
  buffer.finishEditing ();
  buffer.debugPrint ("One line");
  Q_ASSERT (buffer.text () == "testtext");
}

void KateTextBufferTest::cursorTest()
{
  // construct an empty text buffer
  Kate::TextBuffer buffer;

  // wrap first line
  buffer.startEditing ();
  buffer.insertText (KTextEditor::Cursor (0, 0), "sfdfjdsklfjlsdfjlsdkfjskldfjklsdfjklsdjkfl");
  buffer.wrapLine (KTextEditor::Cursor (0, 8));
  buffer.wrapLine (KTextEditor::Cursor (1, 8));
  buffer.wrapLine (KTextEditor::Cursor (2, 8));
  buffer.finishEditing ();
  buffer.debugPrint ("Cursor buffer");

  // construct cursor
  Kate::TextCursor *cursor = new Kate::TextCursor (buffer, KTextEditor::Cursor (0, 0), Kate::TextCursor::MoveOnInsert);
  printf ("cursor %d, %d\n", cursor->line(), cursor->column());

  cursor = new Kate::TextCursor (buffer, KTextEditor::Cursor (1, 8), Kate::TextCursor::MoveOnInsert);
  printf ("cursor %d, %d\n", cursor->line(), cursor->column());

  cursor = new Kate::TextCursor (buffer, KTextEditor::Cursor (0, 123), Kate::TextCursor::MoveOnInsert);
  printf ("cursor %d, %d\n", cursor->line(), cursor->column());

  cursor = new Kate::TextCursor (buffer, KTextEditor::Cursor (1323, 1), Kate::TextCursor::MoveOnInsert);
  printf ("cursor %d, %d\n", cursor->line(), cursor->column());
}
