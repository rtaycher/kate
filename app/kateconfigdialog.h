/***************************************************************************
                          katemainwindow.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
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
    KateConfigDialog (KateMainWindow *parent, const char * = 0);
    ~KateConfigDialog ();

    int exec ();

  public:
    void addPluginPage (Kate::Plugin *plugin);
    void removePluginPage (Kate::Plugin *plugin);

  protected slots:
    virtual void slotApply();

  private:
    class KConfig *config;
    KateDocManager *docManager;
    KateViewManager *viewManager;
    KatePluginManager *pluginManager;
    KateMainWindow *mainWindow;

    Kate::View* v;

    class QCheckBox* cb_opaqueResize;
    class QCheckBox* cb_reopenFiles;
    class QCheckBox* cb_restoreVC;
    class QCheckBox *cb_singleInstance;
    class QCheckBox *cb_fullPath;
    class QCheckBox *cb_syncKonsole;
    class QSpinBox *sb_numRecentFiles;
    class QComboBox *cb_mode;
    Kate::ConfigPage *fileSelConfigPage;

    QPtrList<PluginPageListItem> pluginPages;
    QPtrList<KTextEditor::ConfigPage> editorPages;
};

#endif
