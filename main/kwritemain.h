/*
  $Id$

    Copyright (C) 1998, 1999 Jochen Wilhelmy
                             digisnap@cs.tu-berlin.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _TOPLEVEL_H_
#define _TOPLEVEL_H_

#include <kparts/mainwindow.h>
#include "../view/kantview.h"
#include "../document/kantdocument.h"
#include "../factory/kantfactory.h"

class KAction;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;

class TopLevel : public KParts::MainWindow
{
  Q_OBJECT

  public:
    TopLevel(KantDocument * = 0L);
    ~TopLevel();

    void init(); //initialize caption, status and show

    void loadURL(const KURL &url, int flags = 0);

  protected:
    virtual bool queryClose();
    virtual bool queryExit();

    void setupEditWidget(KantDocument *);
    void setupActions();
    void setupStatusBar();

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

    KantView *kWrite;

    KToggleAction * m_paShowPath;
    KToggleAction * m_paShowMenuBar;
    KToggleAction * m_paShowToolBar;
    KToggleAction * m_paShowStatusBar;

    QTimer *statusbarTimer;

  public slots:
    void newWindow();
    void newView();
    void configure();
    void toggleMenuBar();
    void toggleToolBar();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();

  public slots:
    void printNow();
    void printDlg();

    void newCurPos();
    void newStatus();
    void statusMsg(const QString &);
    void timeout();
    void newCaption();

    void slotDropEvent(QDropEvent *);

    void slotEnableActions( bool enable );

  //config file functions
  public:
    //common config
    void readConfig(KConfig *);
    void writeConfig(KConfig *);
    //config file
    void readConfig();

  public slots:
    void writeConfig();

  //session management
  public:
    void restore(KConfig *,int);

  protected:
    virtual void readProperties(KConfig *);
    virtual void saveProperties(KConfig *);
    virtual void saveGlobalProperties(KConfig *);
};

#endif
