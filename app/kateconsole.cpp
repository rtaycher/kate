/***************************************************************************
                          kateconsole.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/**************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kateconsole.h"
#include "kateconsole.moc"

#include <kurl.h>
#include <qlayout.h>
#include <klibloader.h>
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>

KateConsole::KateConsole (QWidget* parent, const char* name) : QWidget (parent, name),part(0)
{

    lo = new QVBoxLayout(this);
}

KateConsole::~KateConsole ()
{
}


void KateConsole::loadConsoleIfNeeded()
{
  kdDebug()<<"================================ loadConsoleIfNeeded()"<<endl;
  if (part!=0) return;
  if (!kapp->loopLevel()) {
	connect(kapp,SIGNAL(onEventLoopEnter()),this,SLOT(loadConsoleIfNeeded()));
	return;
  }

  if (!topLevelWidget() || !parentWidget()) return;
  if (!topLevelWidget() || !isVisibleTo(topLevelWidget())) return;

  kdDebug()<<"CREATING A CONSOLE PART"<<endl;

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
  loadConsoleIfNeeded();
}
