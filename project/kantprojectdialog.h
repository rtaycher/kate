/***************************************************************************
                          kantprojectmanager.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
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

#ifndef kant_projectdialog_h
#define kant_projectdialog_h

#include "../kantmain.h"
#include "../document/kantdocmanager.h"
#include "../view/kantviewmanager.h"

#include <kdialog.h>

class KantProjectDialog : public KDialog
{
  Q_OBJECT

  public:
    KantProjectDialog(QWidget* parent = 0, const char* name = 0);
    ~KantProjectDialog();

  private:
    KLineEdit* e_name;
    KLineEdit* e_workdir;
    KLineEdit* e_compile;
    KLineEdit* e_run;

    KPushButton* b_ok;
    KPushButton* b_cancel;
};

#endif
