/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_PROJECTLIST_H__
#define __KATE_PROJECTLIST_H__

#include "katemain.h"

#include "../interfaces/project.h"

#include <ktoolbar.h>
#include <kcombobox.h>

#include <qwidget.h>
#include <qvaluelist.h>
#include <qframe.h>

class KateMainWindow;
class KActionCollection;
class KActionSelector;

/* I think this fix for not moving toolbars is better */
class KateProjectListToolBar: public KToolBar
{
  Q_OBJECT
public:
  KateProjectListToolBar(QWidget *parent);
  ~KateProjectListToolBar();

   void setMovingEnabled( bool b );
};

class KateProjectListToolBarParent: public QFrame
{
  Q_OBJECT
public:
  KateProjectListToolBarParent(QWidget *parent);
  ~KateProjectListToolBarParent();
  void setToolBar(KateProjectListToolBar *tb);
private:
  KateProjectListToolBar *m_tb;
protected:
  void resizeEvent ( QResizeEvent * );
};

class KateProjectList : public QWidget
{
  Q_OBJECT

  public:
    KateProjectList (class KateMainWindow *_mainWindow, QWidget * parent = 0, const char * name = 0 );
    ~KateProjectList ();

    void setupActions();

  private slots:
    void projectChanged ();
    void projectCreated (Kate::Project *project);
    void projectDeleted (uint projectNumber);
    void slotActivated ( int index );

  private:
    KComboBox *m_projectList;
    class KateMainWindow *m_mainWindow;
    KateProjectListToolBar *toolbar;
    KActionCollection *mActionCollection;
    QValueList<unsigned int> m_projects;
};

#endif
