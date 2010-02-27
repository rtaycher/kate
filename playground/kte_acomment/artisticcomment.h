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
#include <QStringList>
#include <cstddef>
using std::size_t;

/**
 * Class to add artistic decorations to comments/license headers.
 * An instance of ArtistComment represents a style.
 * The static methods are used to manage those styles.
 */
struct ArtisticComment
{
    QString begin, end;
    QString lineBegin, lineEnd;
    QString textBegin, textEnd;
    QChar lfill, rfill;
    size_t minfill;
    size_t realWidth;
    bool truncate;
    /**
     * The alignment enum.
     */
    enum type_t { Left,         /** Fillers on the right side **/
                  Center,       /** Fillers on both sides **/
                  Right,        /** Fillers on the left side **/
                  LeftNoFill    /** No Fillers **/
    } type;
    /**
     * @warning No initialization!
     */
    ArtisticComment() {}
    /**
     * Constructor
     * @param begin The first line of the comment
     * @param end The last line of the comment
     * @param lineBegin Inserted at the beginning of each line
     * @param lineEnd Inserted at the end of each line
     * @param textBegin Inserted immediately before the text in each line
     * @param textEnd Inserted immediately after the text in each line
     * @param lfill A filling symbol to be used before the text
     * @param rfill A filling symbol to be used after the text
     * @param minfill The minimal number of fillers to be used
     * @param realWidth The length of each line including all decorations
     * @param truncate Indicates if there should be more fillers on the left or on the right side, if there is an odd number of fillers
     * @param type The alignment
     */
    ArtisticComment(const QString& begin, const QString& end,
                    const QString& lineBegin, const QString& lineEnd,
                    const QString& textBegin, const QString& textEnd,
                    QChar lfill, QChar rfill,
                    size_t minfill, size_t realWidth,
                    bool truncate, type_t type);
    /**
     * Get a decorated string.
     */
    QString apply(const QString& text);
    
    /**
     * Read the styles from disk.
     */
    static void readConfig();
    /**
     * Save the styles.
     */
    static void writeConfig();
    /**
     * A list of all available styles.
     */
    static QStringList styles();
    /**
     * A reference to the style with the given name.
     */
    static ArtisticComment& style(const QString& name);
    /**
     * Store the style.
     */
    static void setStyle(const QString& name, const ArtisticComment& style);
    /**
     * Like "style(name).apply(text)".
     */
    static QString decorate(const QString& name, const QString& text);
};

#endif

