/* This file is part of the KDE libraries
   Copyright (C) 2002,2003 Hamish Rodda <meddie@yoyo.its.monash.edu.au>

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

#ifndef KATELINERANGE_H
#define KATELINERANGE_H

class LineRange
{
  public:
    LineRange();

    void clear();

    int line;
    int virtualLine;
    int startCol;
    int endCol;
    int startX;
    int endX;
    bool dirty;
    int viewLine;
    bool wrap;
    bool startsInvisibleBlock;
};

#endif
