/***************************************************************************
                          kantsidebar.cpp  -  description
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
#include "kantsidebar.h"
#include "kantsidebar.moc"

#include <qtabbar.h>
#include <kconfig.h>

KantSidebar::KantSidebar(QWidget* parent, const char* name) : QTabWidget (parent, name)
{
}

KantSidebar::~KantSidebar()
{
}

void KantSidebar::addWidget(QWidget* widget, const QString & label )
{
  addTab (widget, label);
  showPage (widget);
  widget->setFocus();
}

void KantSidebar::removeWidget(QWidget* widget)
{
  removePage ( widget );
}

void KantSidebar::focusNextWidget()
{
  int id = currentPageIndex ();

  if ( id < tabBar()->count()-1 )
    id++;
  else
    id = 0;

  setCurrentPage ( id );
  currentPage ()->setFocus ();
}

void KantSidebar::readConfig(KConfig* config, const char* group)
{
  config->setGroup(group);
  QString t = config->readEntry("Current", "Files");
  for (int i=0; i<tabBar()->count()-1; i++)
  {
    setCurrentPage ( i );
    if ( tabLabel ( currentPage() ).compare( t ) == 0 )
    {
      break;
    }
  }
}

void KantSidebar::saveConfig(KConfig* config, const char* group)
{
  config->setGroup(group);
  config->writeEntry("Current", tabLabel (currentPage ()));
}
