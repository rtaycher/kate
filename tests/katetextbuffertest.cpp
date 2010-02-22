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
  QCOMPARE(buffer.lines(), 1);

  //FIXME: use QTestLib macros for checking the correct state
  // start editing
  buffer.startEditing ();

  // end editing
  buffer.finishEditing ();
}
