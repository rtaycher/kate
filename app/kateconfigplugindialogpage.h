/***************************************************************************
                          kateconfigplugindialogpage.h  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_CONFIGPLUGINDIALOGPAGE_H__
#define __KATE_CONFIGPLUGINDIALOGPAGE_H__

#include "katemain.h"

#include <qvbox.h>

class QListBoxItem;

class KateConfigPluginPage: public QVBox
{
  Q_OBJECT

  public:
    KateConfigPluginPage(QWidget *parent, class KateConfigDialog *dialog);
    ~KateConfigPluginPage(){;};

  private:
    KatePluginManager *myPluginMan;
   class KateConfigDialog *myDialog;

    KListBox *availableBox;
    KListBox *loadedBox;
    class QLabel *label;

    class QPushButton *unloadButton;
    class QPushButton *loadButton;

  private slots:
    void slotUpdate ();
    void slotActivatePluginItem (QListBoxItem *item);

    void loadPlugin ();
    void unloadPlugin ();
};

#endif
