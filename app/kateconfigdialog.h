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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __kate_configdialog_h__
#define __kate_configdialog_h__

#include "katemain.h"

#include "../interfaces/plugin.h"
#include "../interfaces/pluginconfiginterface.h"
#include "../interfaces/pluginconfiginterfaceextension.h"

#include <kate/document.h>
#include <ktexteditor/configinterfaceextension.h>

#include <kdialogbase.h>

struct PluginPageListItem
{
  Kate::Plugin *plugin;
  Kate::PluginConfigPage *page;
};

class KateConfigDialog : public KDialogBase
{
  Q_OBJECT

  public:
    KateConfigDialog (KateMainWindow *parent, Kate::View *view);
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

    Kate::View* v;
    bool dataChanged;

    class QCheckBox* cb_reopenProjects;
    class QCheckBox* cb_reopenFiles;
    class QCheckBox* cb_restoreVC;
    class QCheckBox *cb_singleInstance;
    class QCheckBox *cb_fullPath;
    class QCheckBox *cb_syncKonsole;
    class QSpinBox *sb_numRecentFiles;
    class QCheckBox *cb_modNotifications;
    class QCheckBox *cb_saveMetaInfos;
    class QSpinBox *sb_daysMetaInfos;
    Kate::ConfigPage *fileSelConfigPage;
    Kate::ConfigPage *configExternalToolsPage;
    QPtrList<PluginPageListItem> pluginPages;
    QPtrList<KTextEditor::ConfigPage> editorPages;
};

#endif
