/***************************************************************************
                          kantconsole.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Anders Lund, anders@alweb.dk
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

#include "kantconsole.h"
#include "kantconsole.moc"

#include <kurl.h>
#include <qlayout.h>
#include <stdlib.h>
#include <klibloader.h>
#include <klocale.h>
#include <kglobal.h>
#include <qlabel.h>

KantConsole::KantConsole (QWidget* parent, const char* name) : QWidget (parent, name)
{

  lo = new QVBoxLayout(this);
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

KantConsole::~KantConsole ()
{
}

void KantConsole::cd (KURL url)
{
  part->openURL (url);
}

void KantConsole::slotDestroyed ()
{
  if (!topLevelWidget() || !parentWidget()->isVisible()) return;

 KLibFactory *factory = 0;
    factory = KLibLoader::self()->factory("libkonsolepart");
      if (factory)
        {
          part = static_cast<KParts::ReadOnlyPart *>(factory->create(this,"libkonsolepart",
		"KParts::ReadOnlyPart"));
	  if (part)
	    {
              part->widget()->show();
              lo->addWidget(part->widget());
              connect ( part, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );
            }
        }
}
