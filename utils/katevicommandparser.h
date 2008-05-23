/* This file is part of the KDE libraries
   Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) version 3.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_VI_COMMAND_PARSER
#define KATE_VI_COMMAND_PARSER

#include "kateview.h"

#include <QVector>
#include <QKeyEvent>

/**
 * A finite state machine for Kate's vi mode. This class will receive key
 * presses and when an accept state is reached it will call methods in
 * KateView to do the work.
 */

class KateViCommandParser
{
  public:
  KateViCommandParser( KateView * view );
  ~KateViCommandParser( );

  bool eatKey( QKeyEvent * e );

  private:
  void reset( );

  QVector<int> m_keys;
  KateView *m_view;
  unsigned int m_count;
  bool m_gettingCount;
};

#endif
