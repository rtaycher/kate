/***************************************************************************
                          kantconfigplugindialogpage.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _kant_config_Plugin_Dialog_Page_h_
#define _kant_config_Plugin_Dialog_Page_h_

#include "../kantmain.h"

#include <qvbox.h>

class QListBoxItem;

class KantConfigPluginPage: public QVBox
{
  Q_OBJECT

  public:
    KantConfigPluginPage(QWidget *parent);
    ~KantConfigPluginPage(){;};

  private:
    KantPluginManager *myPluginMan;

    KListBox *availableBox;
    KListBox *loadedBox;
    class QLabel *label;

    class QPushButton *unloadButton;
    class QPushButton *loadButton;

  private slots:
    void slotUpdate ();
    void slotActivatePluginItem (QListBoxItem *item);
};

#endif
