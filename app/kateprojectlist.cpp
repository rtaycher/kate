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

#include "kateprojectlist.h"
#include "kateprojectlist.moc"

#include "kateprojectmanager.h"
#include "katemainwindow.h"
#include "kactionselector.h"
#include "kateprojecttreeview.h"

#include <qapplication.h>
#include <qlayout.h>
#include <qstringlist.h>

#include <kiconloader.h>
#include <klocale.h>
#include <ktoolbarbutton.h>
#include <qtoolbar.h>

// from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
extern static void silenceQToolBar(QtMsgType, const char *){}

KateProjectList::KateProjectList (KateProjectManager *_projectManager, KateMainWindow *_mainWindow, QWidget * parent, const char * name ):  QWidget (parent, name)
{                              
  setFocusPolicy ((QWidget::FocusPolicy)0);

  QVBoxLayout* lo = new QVBoxLayout(this);
  
  mActionCollection = _mainWindow->actionCollection();
  
  m_projectManager = _projectManager;
  m_mainWindow = _mainWindow;
  
  QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );
  
  KateProjectListToolBarParent *tbp=new KateProjectListToolBarParent(this);
  toolbar = new KateProjectListToolBar(tbp);
  tbp->setToolBar(toolbar);
  lo->addWidget(tbp);
  toolbar->setMovingEnabled(false);
  toolbar->setFlat(true);
  qInstallMsgHandler( oldHandler );
  toolbar->setIconText( KToolBar::IconOnly );
  toolbar->setIconSize( 16 );
  toolbar->setEnableContextMenu( false );
  
  m_projectCombo = new KComboBox (this);
  m_projectCombo->hide ();
  lo->addWidget(m_projectCombo);
  
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
  connect(m_projectCombo,SIGNAL(activated(int)),this,SLOT(projectActivated(int)));
}

KateProjectList::~KateProjectList ()
{
}

void KateProjectList::setupActions ()
{
  toolbar->clear();
  
  QStringList tbactions;
   tbactions << "project_new" << "project_open" << "project_save" << "project_close";

  KAction *ac;
  for ( QStringList::Iterator it=tbactions.begin(); it != tbactions.end(); ++it ) {
    ac = mActionCollection->action( (*it).latin1() );
    if ( ac )
      ac->plug( toolbar );
  }
}

void KateProjectList::projectActivated (int num)
{
  if (num >= m_numList.count())
    return;
  
  uint id = m_numList[num];
  
  for (uint i = 0; i < m_projectManager->projects(); i++)
    if (m_projectManager->project(i)->projectNumber() == id)
    {
      m_mainWindow->activateProject (m_projectManager->project(i));
      return;
    }
}

void KateProjectList::projectChanged ()
{
  Kate::Project *p = 0;

  if (!(p = m_mainWindow->mainWindow()->activeProject()))
    return;

  int n = m_numList.findIndex (p->projectNumber());
  
  if (n >= 0)
  {
    m_projectCombo->setCurrentItem (n);
    m_stack->raiseWidget (n);
  }
}

void KateProjectList::projectCreated (Kate::Project *project)
{
  if (!project)
    return;

  m_numList.append (project->projectNumber());
  m_projectCombo->insertItem (project->name());
  
  KateProjectTreeView *tree = new KateProjectTreeView (project, this);
  m_stack->addWidget (tree);
  m_stack->raiseWidget (tree);
  
  if (m_projectCombo->isHidden())
    m_projectCombo->show ();
}

void KateProjectList::projectDeleted (uint projectNumber)
{
  int n = m_numList.findIndex (projectNumber);
  
  if (n >= 0)
  {
    m_numList.remove (projectNumber);
    m_projectCombo->removeItem (n);
    
    QWidget *w = m_stack->widget (n);
    m_stack->removeWidget (w);
    delete w;
    
    if (m_numList.isEmpty())
    {
      if (!m_projectCombo->isHidden())
        m_projectCombo->hide ();
    }
  }
}

//
// STUFF FOR THE TOOLBAR
//

KateProjectListToolBar::KateProjectListToolBar(QWidget *parent):KToolBar( parent, "Kate ProjectList Toolbar", true )
{
	setMinimumWidth(10);
}

KateProjectListToolBar::~KateProjectListToolBar(){}

void KateProjectListToolBar::setMovingEnabled( bool)
{
	//kdDebug()<<"JoWenn's setMovingEnabled called ******************************"<<endl;
	KToolBar::setMovingEnabled(false);
}


KateProjectListToolBarParent::KateProjectListToolBarParent(QWidget *parent)
	:QFrame(parent),m_tb(0){}
KateProjectListToolBarParent::~KateProjectListToolBarParent(){}
void KateProjectListToolBarParent::setToolBar(KateProjectListToolBar *tb)
{
	m_tb=tb;
}

void KateProjectListToolBarParent::resizeEvent ( QResizeEvent * )
{
	if (m_tb)
	{
		setMinimumHeight(m_tb->sizeHint().height());
		m_tb->resize(width(),height());
	}
}
