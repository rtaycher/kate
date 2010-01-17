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

#ifndef ACOMMENTVIEW_H
#define ACOMMENTVIEW_H

#include <QObject>
#include <KXMLGUIClient>

class QAction;
class KMenu;
class KDialog;

#include "ui_styledialog.h"

class ACommentView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit ACommentView(KTextEditor::View *view = 0);
    ~ACommentView();
private slots:
    void insertAComment(QAction *action);
    void disableOptions(int type);
    void loadStyle(QString name);
    void changeEntry();
private:
    KTextEditor::View *m_view;
    KMenu *m_menu;
    KDialog *m_dialog;
    Ui::StyleDialog m_ui;
};

#endif
