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

// $Id$

#include "kateprojectviews.h"
#include "kateprojectviews.moc"

#include "kateprojectmanager.h"
#include "katemainwindow.h"
#include "kactionselector.h"
#include "kateprojecttreeview.h"

#include <qapplication.h>
#include <qlayout.h>
#include <qstringlist.h>

#include <klocale.h>


KateProjectViews::KateProjectViews (KateProjectManager *_projectManager, KateMainWindow *_mainWindow, QWidget * parent, const char * name ):  QWidget (parent, name)
{                              
  setFocusPolicy ((QWidget::FocusPolicy)0);

  QVBoxLayout* lo = new QVBoxLayout(this);
  
  m_projectManager = _projectManager;
  m_mainWindow = _mainWindow;
 
  m_stack = new QWidgetStack (this);
  lo->addWidget(m_stack);
  lo->setStretchFactor(m_stack, 2);
  
  // init of the combo box
  for (uint i = 0; i < m_projectManager->projects(); i++)
    projectCreated (m_projectManager->project(i));
    
  projectChanged ();
    
  // connecting
  connect(m_projectManager->projectManager(),SIGNAL(projectCreated(Kate::Project *)),this,SLOT(projectCreated(Kate::Project *)));
  connect(m_projectManager->projectManager(),SIGNAL(projectDeleted(uint)),this,SLOT(projectDeleted(uint)));
  connect(m_mainWindow->mainWindow(),SIGNAL(projectChanged()),this,SLOT(projectChanged()));
}

KateProjectViews::~KateProjectViews ()
{
}

void KateProjectViews::projectChanged ()
{
  Kate::Project *p = 0;

  if (!(p = m_mainWindow->mainWindow()->activeProject()))
    return;

  int n = m_numList.findIndex (p->projectNumber());
  
  if (n >= 0)
  {
    m_stack->raiseWidget (n);
  }
}

void KateProjectViews::projectCreated (Kate::Project *project)
{
  if (!project)
    return;

  m_numList.append (project->projectNumber());
  
  KateProjectTreeView *tree = new KateProjectTreeView (project, this);
  m_stack->addWidget (tree);
  m_stack->raiseWidget (tree);
}

void KateProjectViews::projectDeleted (uint projectNumber)
{
  int n = m_numList.findIndex (projectNumber);
  
  if (n >= 0)
  {
    m_numList.remove (projectNumber);
    
    QWidget *w = m_stack->widget (n);
    m_stack->removeWidget (w);
    delete w;
  }
}
