/***************************************************************************
                          katemainwindow.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
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

#include "../main/katemain.h"

#include "../document/katehighlight.h"
#include <kdialogbase.h>

class KateConfigDialog : public KDialogBase
{
  Q_OBJECT

  friend class KateMainWindow;

  public:
    KateConfigDialog (KateMainWindow *parent, const char * = 0);
    ~KateConfigDialog ();

    int exec ();

  protected slots:
    virtual void slotApply();

  private:
    class KConfig *config;
    KateDocManager *docManager;
    KateViewManager *viewManager;
    KatePluginManager *pluginManager;
    KateMainWindow *mainWindow;

    KateView* v;

    class QCheckBox* cb_opaqueResize;
    class QCheckBox* cb_reopenFiles;
    class QCheckBox* cb_restoreVC;
    class QCheckBox *cb_singleInstance;
    class QCheckBox *cb_fileSidebarStyle;
    class QCheckBox *cb_syncKonsole;

    class ColorConfig *colorConfig;
    class KSpellConfig * ksc;
    class IndentConfigTab * indentConfig;
    class SelectConfigTab * selectConfig;
    class EditConfigTab * editConfig;
    class QColor* colors;

    class HighlightDialogPage *hlPage;
    class HlManager *hlManager;
    HlDataList hlDataList;
    ItemStyleList defaultStyleList;
    ItemFont defaultFont;
};

#endif
