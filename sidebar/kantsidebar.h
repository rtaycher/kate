/***************************************************************************
                          kantsidebar.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Anders Lund, anders@alweb.dk
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __kant_sidebar_h__
#define __kant_sidebar_h__

#include "../kantmain.h"

#include <qwidget.h>

class KantSidebar : public QWidget
{
  Q_OBJECT
  public:
    KantSidebar(QWidget* parent=0, const char* name=0);
    ~KantSidebar();

    void addWidget(QWidget* widget, const QString & label);
    void removeWidget(QWidget* widget);
    void focusNextWidget();
    void readConfig(KConfig* config, const char* group="Sidebar");
    void saveConfig(KConfig* config, const char* group="Sidebar");

  private:
    QWidgetStack* stack;
    QComboBox* cmb;
};

#endif
