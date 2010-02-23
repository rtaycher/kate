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

int main (int argc, char *argv[])
{
  // no warnings
  Q_UNUSED (argc);
  Q_UNUSED (argv);

  // construct an empty text buffer
  Kate::TextBuffer buffer;

  // one line per default
  Q_ASSERT (buffer.lines() == 1);
  Q_ASSERT (buffer.text () == "");

  // start editing
  buffer.startEditing ();

  // end editing
  buffer.finishEditing ();

  // print debug
  buffer.debugPrint ("Empty Buffer");
  Q_ASSERT (buffer.text () == "");

  // wrap first line
  buffer.startEditing ();
  buffer.wrapLine (KTextEditor::Cursor (0, 0));
  buffer.finishEditing ();
  buffer.debugPrint ("Two empty lines");
  Q_ASSERT (buffer.text () == "\n");

  // unwrap second line
  buffer.startEditing ();
  buffer.unwrapLine (1);
  buffer.finishEditing ();
  buffer.debugPrint ("One empty line");
  Q_ASSERT (buffer.text () == "");

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


  return 0;
}
