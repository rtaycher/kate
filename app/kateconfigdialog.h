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
#include "../interfaces/document.h"
#include <kdialogbase.h>

struct PluginPageListItem
{
  Kate::Plugin *plugin;
  Kate::PluginConfigPage *page;
};

class KateConfigDialog : public KDialogBase
{
  Q_OBJECT

  friend class KateMainWindow;
  friend class KateConfigPluginPage;

  public:
    KateConfigDialog (KateMainWindow *parent, const char * = 0);
    ~KateConfigDialog ();

    int exec ();

  private:
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
    class QCheckBox *cb_sdi;
    class QCheckBox *cb_syncKonsole;

    Kate::ConfigPage *colorConfigPage;
    Kate::ConfigPage *fontConfigPage;
    Kate::ConfigPage *indentConfigPage;
    Kate::ConfigPage *selectConfigPage;
    Kate::ConfigPage *editConfigPage;
    Kate::ConfigPage *keysConfigPage;
    Kate::ConfigPage *kSpellConfigPage;
    Kate::ConfigPage *hlConfigPage;

    QPtrList<PluginPageListItem> pluginPages;
};

#endif
