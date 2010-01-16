/****************************************************************************
 **                - Artistic Comment KTextEditor-Plugin -                 **
 ** - Copyright (C) 2010 by Jonathan Schmidt-Dominé <devel@the-user.org> - **
 **                                  ----                                  **
 **   - This program is free software: you can redistribute it and/or -    **
 **   - modify it under the terms of the GNU General Public License as -   **
 ** - published by the Free Software Foundation, either version 2 of the - **
 **          - License, or (at your option) any later version. -           **
 **  - This program is distributed in the hope that it will be useful, -   **
 **   - but WITHOUT ANY WARRANTY; without even the implied warranty of -   **
 ** - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU -  **
 **              - General Public License for more details. -              **
 ** - You should have received a copy of the GNU General Public License -  **
 **   - along with this program. If not, see <http://gnu.org/licenses> -   **
 ***************************************************************************/

#ifndef ARTISTICCOMMENT_H
#define ARTISTICCOMMENT_H

#include <QString>

struct ArtisticComment
{
    QString begin, end;
    QString lineBegin, lineEnd;
    QString textBegin, textEnd;
    QChar lfill, rfill;
    size_t minfill;
    size_t realWidth;
    bool truncate;
    enum type_t { LeftNoFill, Left, Center, Right } type;
    ArtisticComment() {}
    ArtisticComment(QString begin, QString end,
                    QString lineBegin, QString lineEnd,
                    QString textBegin, QString textEnd,
                    QChar lfill, QChar rfill,
                    size_t minfill, size_t realWidth,
                    bool truncate, type_t);
    QString apply(const QString& text);
};

#endif

