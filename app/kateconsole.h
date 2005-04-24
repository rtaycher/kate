/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_CONSOLE_H__
#define __KATE_CONSOLE_H__

#include "katemain.h"
#include "katemdi.h"
#include "katemainwindow.h"
#include "../interfaces/viewmanager.h"

#include <qwidget.h>
#include <kparts/part.h>

class KateConsole : public QWidget
{
  Q_OBJECT

  public:
    KateConsole (KateMainWindow *mw, KateMDI::ToolView* parent, const char* name, Kate::ViewManager *);
    ~KateConsole ();

    void cd (KURL url=KURL());

    void sendInput( const QString& text );

  protected:
    void focusInEvent( QFocusEvent * ) { if (part) part->widget()->setFocus(); };
    virtual void showEvent(QShowEvent *);


  private:
    KParts::ReadOnlyPart *part;
    QVBoxLayout* lo;
    Kate::ViewManager *m_kvm;
    KateMainWindow *m_mw;
  KateMDI::ToolView *m_toolView;

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
