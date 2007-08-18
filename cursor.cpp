/* This file is part of the KDE project
   Copyright (C) 2003-2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "cursor.h"

#include "range.h"

#include <kdebug.h>

using namespace KTextEditor;

SecuredInteger::SecuredInteger() {
  init();
}

 SecuredInteger::SecuredInteger(int init) : m_int(init) {
   this->init();
 }

SecuredInteger& SecuredInteger::operator=( int rhs ) {
  verify();
  //Break here, it is called from everywhere!
  m_int = rhs;
  return *this;
}


Cursor::Cursor( )
  : m_line(0)
  , m_column(0)
  , m_range(0L)
{
}

Cursor::Cursor( int _line, int _column )
  : m_line(_line)
  , m_column(_column)
  , m_range(0L)
{
}

Cursor::Cursor(const Cursor& copy)
  : m_line(copy.line())
  , m_column(copy.column())
  , m_range(0L)
{
}

bool Cursor::isValid() const
{
  return m_line >= 0 && m_column >= 0;
}

Cursor Cursor::invalid( )
{
  return Cursor (-1,-1);
}

Cursor Cursor::start()
{
  return Cursor (0, 0);
}

int Cursor::line() const
{
  return m_line;
}

void Cursor::setLine( int line )
{
  if (line == this->line())
    return;
  //kDebug() << this << "(" << m_range <<  ")Cursor changing line from " << this->line() << " to " << line << endl;

  Cursor old = *this;

  m_line = line;

  cursorChangedDirectly(old);
}

int Cursor::column() const
{
    return m_column;
}

void Cursor::setColumn( int column )
{
  if (column == this->column())
    return;
  //kDebug() << this << "(" << m_range <<  ")Cursor changing column from " << this->column() << " to " << column << endl;

  Cursor old = *this;

  m_column = column;

  cursorChangedDirectly(old);
}

void Cursor::setPosition( const Cursor & pos )
{
  if (pos == *this)
    return;
  //kDebug() << this << "(" << m_range <<  ")Cursor changing position from " << *this << " to " << pos << endl;

  Cursor old = *this;

  m_line = pos.line();
  m_column = pos.column();

  cursorChangedDirectly(old);
}

bool Cursor::isSmartCursor( ) const
{
  return false;
}

void Cursor::setPosition(int line, int column)
{
    setPosition(Cursor(line, column));
}

void Cursor::position (int &_line, int &_column) const
{
  _line = line(); _column = column();
}

Range* Cursor::range() const
{
    return m_range;
}

Cursor::~ Cursor( )
{
}

void Cursor::setRange( Range * range )
{
  m_range = range;
}

void KTextEditor::Cursor::cursorChangedDirectly(const Cursor& from)
{
  if (m_range) {
    if (this == &m_range->start())
      m_range->rangeChanged(this, Range(from, m_range->end()));
    else
      m_range->rangeChanged(this, Range(m_range->start(), from));
  }
}

bool KTextEditor::Cursor::atStartOfLine( ) const
{
  return m_column == 0;
}

bool KTextEditor::Cursor::atStartOfDocument( ) const
{
  return m_line == 0 && m_column == 0;
}

SmartCursor * KTextEditor::Cursor::toSmartCursor( ) const
{
  return 0L;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
