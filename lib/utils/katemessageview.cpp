/* This file is part of the KDE project
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

// $Id$

#include "katemessageview.h"
#include "katemessageview.moc"

#include <qtextbrowser.h>

KateMessageView::KateMessageView( QWidget *parent, const char *name )
  : KateDockViewBase( parent, name )
{
  m_view = new QTextBrowser( this );
  // m_view->setFormat( Qt::richText ); // should be!!
  connect( m_view, SIGNAL( linkClicked( const QString & ) ), 
           SIGNAL( linkClicked( const QString & ) ) );
}

KateMessageView::~KateMessageView()
{
}

void KateMessageView::addMessage( const QString &msg )
{
  m_view->append( msg );
  m_view->scrollToBottom();
}

void KateMessageView::clear()
{
  m_view->clear();
}
