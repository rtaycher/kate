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

KateProjectList::KateProjectList (KateProjectManager *_projectManager, KateMainWindow *_mainWindow, QWidget * parent, const char * name ):  QWidget (parent, name)
{                              
  setFocusPolicy ((QWidget::FocusPolicy)0);

  QVBoxLayout* lo = new QVBoxLayout(this);
  
  mActionCollection = _mainWindow->actionCollection();
  
  m_projectManager = _projectManager;
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
    
  m_projectList = new KListBox (this);
  lo->addWidget(m_projectList);
  lo->setStretchFactor(m_projectList, 2);
  
  // init of the combo box
  for (uint i = 0; i < m_projectManager->projects(); i++)
    projectCreated (m_projectManager->project(i));
    
  projectChanged ();
    
  // connecting
  connect(m_projectManager->projectManager(),SIGNAL(projectCreated(Kate::Project *)),this,SLOT(projectCreated(Kate::Project *)));
  connect(m_projectManager->projectManager(),SIGNAL(projectDeleted(uint)),this,SLOT(projectDeleted(uint)));
  connect(m_mainWindow->mainWindow(),SIGNAL(projectChanged()),this,SLOT(projectChanged()));
  connect(m_projectList,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivateView(QListBoxItem *)));
  connect(m_projectList,SIGNAL(selected(QListBoxItem *)), this,SLOT(slotActivateView(QListBoxItem *)));
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

void KateProjectList::slotActivateView( QListBoxItem *item )
{
  uint id = ((KateProjectListItem *)item)->projectNumber();
  
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

  for (uint i = 0; i < m_projectList->count(); i++)
  {
    if (((KateProjectListItem *) m_projectList->item (i)) ->projectNumber() == p->projectNumber())
    {
      m_projectList->setCurrentItem (i);
      if ( !m_projectList->isSelected( m_projectList->item(i) ) )
        m_projectList->setSelected( i, true );
      break;
    }
  }
}

void KateProjectList::projectCreated (Kate::Project *project)
{
  if (!project)
    return;

  m_projectList->insertItem (new KateProjectListItem (project->projectNumber(), SmallIcon("null"), project->name()) );
}

void KateProjectList::projectDeleted (uint projectNumber)
{
  for (uint i = 0; i < m_projectList->count(); i++)
  {
    if (((KateProjectListItem *) m_projectList->item (i)) ->projectNumber() == projectNumber)
    {
      if (m_projectList->count() > 1)
        m_projectList->removeItem( i );
      else
        m_projectList->clear();
    }
  }
}

KateProjectListItem::KateProjectListItem( uint projectNumber, const QPixmap &pix, const QString& text): QListBoxItem()
{
  _bold=false;
  myProjectID = projectNumber;
  setPixmap(pix);
  setText( text );
}

KateProjectListItem::~KateProjectListItem()
{
}

uint KateProjectListItem::projectNumber ()
{
  return myProjectID;
}


void KateProjectListItem::setText(const QString &text)
{
  QListBoxItem::setText(text);
}

void KateProjectListItem::setPixmap(const QPixmap &pixmap)
{
  pm=pixmap;
}

void KateProjectListItem::setBold(bool bold)
{
  bold=bold;
}

int KateProjectListItem::height( const QListBox* lb ) const
{
  int h;

  if ( text().isEmpty() )
    h = pm.height();
  else
    h = QMAX( pm.height(), lb->fontMetrics().lineSpacing() + 1 );

  return QMAX( h, QApplication::globalStrut().height() );
}

int KateProjectListItem::width( const QListBox* lb ) const
{
  if ( text().isEmpty() )
    return QMAX( pm.width() + 6, QApplication::globalStrut().width() );

  return QMAX( pm.width() + lb->fontMetrics().width( text() ) + 6, QApplication::globalStrut().width() );
}

void KateProjectListItem::paint( QPainter *painter )
{
  painter->drawPixmap( 3, 0, pm );
  QFont f=painter->font();
  f.setBold(_bold);
  painter->setFont(f);

  if ( !text().isEmpty() )
  {
    QFontMetrics fm = painter->fontMetrics();
    int yPos;                       // vertical text position

    if ( pm.height() < fm.height() )
      yPos = fm.ascent() + fm.leading()/2;
    else
      yPos = pm.height()/2 - fm.height()/2 + fm.ascent();

    painter->drawText( pm.width() + 5, yPos, text() );
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
