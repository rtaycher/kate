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

#include <qapplication.h>
#include <qpainter.h>

#include <kiconloader.h>
#include <klocale.h>

KateProjectList::KateProjectList (KateProjectManager *_projectManager, KateMainWindow *_mainWindow, QWidget * parent, const char * name ):  QWidget (parent, name)
{                              
  setFocusPolicy ((QWidget::FocusPolicy)0);       

  m_projectManager = _projectManager;
  m_mainWindow = _mainWindow;
}

KateProjectList::~KateProjectList ()
{
}
