/* This file is part of the KDE project
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_SPLITTER_H__
#define __KATE_SPLITTER_H__

#include <qsplitter.h>

/** This class is needed because QSplitter cant return an index for a widget. */
class KateSplitter : public QSplitter
{
  Q_OBJECT

  public:
    KateSplitter(QWidget* parent=0, const char* name=0);
    KateSplitter(Orientation o, QWidget* parent=0, const char* name=0);
    ~KateSplitter();

    /** Since there is supposed to be only 2 childs of a katesplitter,
     * any child other than the last is the first.
     * This method uses QSplitter::idAfter(widget) which
     * returns 0 if there is no widget after this one.
     * This results in an error if widget is not a child
     * in this splitter */
    bool isLastChild(QWidget* w);
};

#endif
