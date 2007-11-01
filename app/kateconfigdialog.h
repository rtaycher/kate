/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __kate_configdialog_h__
#define __kate_configdialog_h__

#include "katemain.h"

#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>

#include <KTextEditor/Document>
#include <KTextEditor/EditorChooser>

#include <KPageDialog>
#include <QList>
#include <QRadioButton>

class QCheckBox;
class QSpinBox;
class Q3ButtonGroup;
class KateMainWindow;

struct PluginPageListItem
{
  Kate::Plugin *plugin;
  Kate::PluginConfigPage *page;
  KPageWidgetItem *pageWidgetItem;
};

class KateConfigDialog : public KPageDialog
{
    Q_OBJECT

  public:
    KateConfigDialog (KateMainWindow *parent, KTextEditor::View *view);
    ~KateConfigDialog ();

  public:
    void addPluginPage (Kate::Plugin *plugin);
    void removePluginPage (Kate::Plugin *plugin);

  protected Q_SLOTS:
    void slotOk();
    void slotApply();
    void slotChanged();

  private:
    KateMainWindow *m_mainWindow;

    KTextEditor::View* m_view;
    bool m_dataChanged;

    QCheckBox *m_modNotifications;
    QCheckBox *m_saveMetaInfos;
    QSpinBox *m_daysMetaInfos;
    QCheckBox *m_restoreVC;

    // sessions start group:
    QRadioButton *m_startNewSessionRadioButton;
    QRadioButton *m_loadLastUserSessionRadioButton;
    QRadioButton *m_manuallyChooseSessionRadioButton;

    // sessions exit group:
    QRadioButton *m_doNotSaveSessionRadioButton;
    QRadioButton *m_saveSessionRadioButton;
    QRadioButton *m_askUserRadioButton;
    
    QList<PluginPageListItem*> m_pluginPages;
    QList<KTextEditor::ConfigPage*> m_editorPages;
    KTextEditor::EditorChooser *m_editorChooser;
    KPageWidgetItem *m_pluginPage;

    class KateFileListConfigPage *filelistConfigPage;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

