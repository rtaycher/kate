/***************************************************************************
                          kantsplitter.cpp  -  description
                             -------------------
    begin                : Fri Mar 02 2001
    copyright            : (C) 2001 by Anders Lund, anders@alweb.dk
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "kantsplitter.h"
#include "kantsplitter.moc"

KantSplitter::KantSplitter(Orientation o, QWidget* parent, const char* name)
  : QSplitter(o, parent, name)
{
}

KantSplitter::KantSplitter(QWidget* parent, const char* name)
  : QSplitter(parent, name)
{
}

KantSplitter::~KantSplitter()
{
}

bool KantSplitter::isLastChild(QWidget* w)
{
  return ( idAfter( w ) == 0 );
}
