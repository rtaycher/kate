/***************************************************************************
                          katesidebar.cpp  -  description
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
#include "katesidebar.h"
#include "katesidebar.moc"

#include <kconfig.h>
#include <qwidgetstack.h>

KateSidebar::KateSidebar(QWidget* parent, const char* name,bool stacked) : KateStackTabWidget (parent, name,stacked)
{
}

KateSidebar::~KateSidebar()
{
}

void KateSidebar::addWidget(QWidget* widget, const QString & label )
{
  addPage (widget, label);
  showPage (widget);
  widget->setFocus();
}

void KateSidebar::removeWidget(QWidget* widget)
{
//  removePage ( widget );
}

void KateSidebar::focusNextWidget()
{
/*  int id = currentPageIndex ();

  if ( id < tabBar()->count()-1 )
    id++;
  else
    id = 0;

  setCurrentPage ( id );
  currentPage ()->setFocus (); */
}

void KateSidebar::readConfig(KConfig* config, const char* group)
{
  config->setGroup(group);
  int c = config->readNumEntry("Current", 0);

  setMode(config->readBoolEntry("KOWStyle",true));
  showPage(c);
}

void KateSidebar::saveConfig(KConfig* config, const char* group)
{
  config->setGroup(group);
  config->writeEntry("Current", stack()->id(stack()->visibleWidget()));
}
