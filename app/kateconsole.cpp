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

// $Id$

#include "kateconsole.h"
#include "kateconsole.moc"

#include <kurl.h>
#include <qlayout.h>
#include <klibloader.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>

#include <kate/view.h>
#include <kate/document.h>

#include "katemainwindow.h"

KateConsole::KateConsole (QWidget* parent, const char* name, Kate::ViewManager *kvm) : QWidget (parent, name),part(0)
{

    lo = new QVBoxLayout(this);
    m_kvm=kvm;
}

KateConsole::~KateConsole ()
{
}


void KateConsole::loadConsoleIfNeeded()
{
  kdDebug(13001)<<"================================ loadConsoleIfNeeded()"<<endl;
  if (part!=0) return;
  if (!kapp->loopLevel()) {
	connect(kapp,SIGNAL(onEventLoopEnter()),this,SLOT(loadConsoleIfNeeded()));
	return;
  }

  if (!topLevelWidget() || !parentWidget()) return;
  if (!topLevelWidget() || !isVisibleTo(topLevelWidget())) return;

  kdDebug(13001)<<"CREATING A CONSOLE PART"<<endl;

    KLibFactory *factory = 0;
    factory = KLibLoader::self()->factory("libkonsolepart");
    part = 0L;
      if (factory)
        {
          part = static_cast<KParts::ReadOnlyPart *>(factory->create(this,"libkonsolepart",
		"KParts::ReadOnlyPart"));
	  if (part)
	    {
              KGlobal::locale()->insertCatalogue("konsole");
              part->widget()->show();
              lo->addWidget(part->widget());
              connect ( part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );
	      if (m_kvm->activeView())
	      	if (m_kvm->activeView()->getDoc()->url().isValid())
			cd(m_kvm->activeView()->getDoc()->url().path());
            }
        }
}

void KateConsole::showEvent(QShowEvent *)
{
	if (!part) loadConsoleIfNeeded();
}

void KateConsole::cd (KURL url)
{
  if (part) part->openURL (url);
}

void KateConsole::slotDestroyed ()
{
  part=0;
  
  // hide the dockwidget
  if (parentWidget())
    parentWidget()->hide ();
}
