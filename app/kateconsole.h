/***************************************************************************
                          kateconsole.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Anders Lund
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
#ifndef __KATE_CONSOLE_H__
#define __KATE_CONSOLE_H__

#include "katemain.h"
#include "../interfaces/viewmanager.h"

#include <qwidget.h>
#include <kparts/part.h>

class KateConsole : public QWidget
{
  Q_OBJECT

  public:
    KateConsole (QWidget* parent, const char* name, Kate::ViewManager *);
    ~KateConsole ();

    void cd (KURL url=0L);

  protected:
    void focusInEvent( QFocusEvent * ) { if (part) part->widget()->setFocus(); };
    virtual void showEvent(QShowEvent *);

  private:
    KParts::ReadOnlyPart *part;
    QVBoxLayout* lo;
    Kate::ViewManager *m_kvm;

  public slots:
    void loadConsoleIfNeeded();


  // Only needed for Konsole
  private slots:
    void notifySize (int,int) {};
    void changeColumns (int) {};
    void changeTitle(int,const QString&) {};

    void slotDestroyed ();
};

#endif
