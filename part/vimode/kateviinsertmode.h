/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
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

#ifndef KATE_VI_INSERT_MODE_INCLUDED
#define KATE_VI_INSERT_MODE_INCLUDED

#include <QKeyEvent>
#include "katevimodebase.h"

class KateViMotion;
class KateView;
class KateViewInternal;

/**
 * Commands for the vi insert mode
 */

class KateViInsertMode : public KateViModeBase
{
  public:
    KateViInsertMode( KateViInputModeManager *viInputModeManager, KateView * view, KateViewInternal * viewInternal );
    ~KateViInsertMode();

    bool handleKeypress( const QKeyEvent *e );

    bool commandInsertFromAbove();
    bool commandInsertFromBelow();

    bool commandDeleteWord();

    bool commandIndent();
    bool commandUnindent();

    bool commandToFirstCharacterInFile();
    bool commandToLastCharacterInFile();

    bool commandMoveOneWordLeft();
    bool commandMoveOneWordRight();

    bool commandCompleteNext();
    bool commandCompletePrevious();

    // mappings not supported in insert mode yet
    void addMapping( const QString &from, const QString &to ) { Q_UNUSED(from) Q_UNUSED(to) }
    const QString getMapping( const QString &from ) const { Q_UNUSED(from) return QString(); }
    const QStringList getMappings() const { return QStringList(); }
};

#endif
