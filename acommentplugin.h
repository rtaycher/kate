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

#ifndef ACOMMENTPLUGIN_H
#define ACOMMENTPLUGIN_H

#include <KTextEditor/Plugin>

namespace KTextEditor
{
class View;
}

#include "artisticcomment.h"

#include <QMap>

#include <KSharedConfig>

class ACommentView;

class ACommentPlugin
            : public KTextEditor::Plugin
{
public:
    // Constructor
    explicit ACommentPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    // Destructor
    virtual ~ACommentPlugin();

    void addView(KTextEditor::View *view);
    void removeView(KTextEditor::View *view);

    void readConfig();
    void writeConfig();

//     void readConfig (KConfig *);
//     void writeConfig (KConfig *);

private:
    KSharedConfigPtr m_config;
    QMap<QString, ArtisticComment> m_styles;
    QList<class ACommentView*> m_views;
    static ACommentPlugin *instance;
    friend class ACommentView;
};

#endif
