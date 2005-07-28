/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __kate_configdialog_h__
#define __kate_configdialog_h__

#include "katemain.h"

#include "../interfaces/plugin.h"
#include "../interfaces/pluginconfiginterface.h"
#include "../interfaces/pluginconfiginterfaceextension.h"

#include <ktexteditor/document.h>
#include <ktexteditor/editorchooser.h>

#include <kdialogbase.h>
//Added by qt3to4:
#include <Q3PtrList>

class QCheckBox;
class QSpinBox;
class Q3ButtonGroup;

struct PluginPageListItem
{
  Kate::Plugin *plugin;
  Kate::PluginConfigPage *page;
};

class KateConfigDialog : public KDialogBase
{
  Q_OBJECT

  public:
    KateConfigDialog (KateMainWindow *parent, KTextEditor::View *view);
    ~KateConfigDialog ();

  public:
    void addPluginPage (Kate::Plugin *plugin);
    void removePluginPage (Kate::Plugin *plugin);

  protected slots:
    void slotOk();
    void slotApply();
    void slotChanged();

  private:
    KateMainWindow *mainWindow;

    KTextEditor::View* v;
    bool dataChanged;

    QCheckBox *cb_fullPath;
    QCheckBox *cb_syncKonsole;
    QCheckBox *cb_modNotifications;
    QCheckBox *cb_saveMetaInfos;
    QSpinBox *sb_daysMetaInfos;
    QCheckBox* cb_restoreVC;
    Q3ButtonGroup *sessions_start;
    Q3ButtonGroup *sessions_exit;
    KTextEditor::ConfigPage *fileSelConfigPage;
    KTextEditor::ConfigPage *filelistConfigPage;
    KTextEditor::ConfigPage *configExternalToolsPage;
    Q3PtrList<PluginPageListItem> pluginPages;
    Q3PtrList<KTextEditor::ConfigPage> editorPages;
    KTextEditor::EditorChooser *m_editorChooser;
};

#endif
