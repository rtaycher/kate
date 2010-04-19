/* This file is part of the KDE project
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  Based on code of the SmartCursor/Range by:
 *  Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>
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

#include "movingcursor.h"

using namespace KTextEditor;

MovingCursor::MovingCursor ()
{
}

MovingCursor::~MovingCursor ()
{
}

void MovingCursor::setPosition(int line, int column)
{
  // just use setPosition
  setPosition(Cursor(line, column));
}

void MovingCursor::setLine (int line)
{
  // just use setPosition
  setPosition (line, column());
}

void MovingCursor::setColumn (int column)
{
  // just use setPosition
  setPosition (line(), column);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
