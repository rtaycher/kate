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

#include "kateprojectlist.h"
#include "kateprojectlist.moc"

#include "kateprojectmanager.h"
#include "katemainwindow.h"
#include "kactionselector.h"

#include <qapplication.h>
#include <qlayout.h>
#include <qstringlist.h>
#include <qpainter.h>

#include <kiconloader.h>
#include <klocale.h>
#include <ktoolbarbutton.h>
#include <qtoolbar.h>

// from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar2 (QtMsgType, const char *) {}

KateProjectList::KateProjectList (KateMainWindow *_mainWindow, QWidget * parent, const char * name ):  QWidget (parent, name)
{
  setFocusPolicy ((QWidget::FocusPolicy)0);

  QVBoxLayout* lo = new QVBoxLayout(this);

  mActionCollection = _mainWindow->actionCollection();

  m_mainWindow = _mainWindow;

  QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar2 );

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

  m_projectList = new KComboBox (this);
  lo->addWidget(m_projectList);
  lo->setStretchFactor(m_projectList, 2);

  // init of the combo box
  for (uint i = 0; i < KateProjectManager::self()->projects(); i++)
    projectCreated (KateProjectManager::self()->project(i));

  projectChanged ();

  // connecting
  connect(KateProjectManager::self()->projectManager(),SIGNAL(projectCreated(Kate::Project *)),this,SLOT(projectCreated(Kate::Project *)));
  connect(KateProjectManager::self()->projectManager(),SIGNAL(projectDeleted(uint)),this,SLOT(projectDeleted(uint)));
  connect(m_mainWindow->mainWindow(),SIGNAL(projectChanged()),this,SLOT(projectChanged()));
  connect(m_projectList,SIGNAL(activated(int)),this,SLOT(slotActivated(int)));
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

void KateProjectList::slotActivated ( int index )
{
  if ((uint)index >= m_projects.size())
    return;

  for (uint i = 0; i < KateProjectManager::self()->projects(); i++)
    if (KateProjectManager::self()->project(i)->projectNumber() == m_projects[index])
    {
      m_mainWindow->activateProject (KateProjectManager::self()->project(i));
      return;
    }
}

void KateProjectList::projectChanged ()
{
  Kate::Project *p = 0;

  if (!(p = m_mainWindow->mainWindow()->activeProject()))
    return;

  for (uint i = 0; i < m_projects.size(); i++)
  {
    if (m_projects[i] == p->projectNumber())
    {
      m_projectList->setCurrentItem (i);
      return;
    }
  }
}

void KateProjectList::projectCreated (Kate::Project *project)
{
  if (!project)
    return;

  m_projects.append (project->projectNumber());
  m_projectList->insertItem (project->name());
}

void KateProjectList::projectDeleted (uint projectNumber)
{
  for (uint i = 0; i < m_projects.size(); i++)
  {
    if (m_projects[i] == projectNumber)
    {
      m_projectList->removeItem (i);
      m_projects.remove (projectNumber);
      return;
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
	//kdDebug(13001)<<"JoWenn's setMovingEnabled called ******************************"<<endl;
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
