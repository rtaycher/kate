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

#include <kconfig.h>
#include <qlayout.h>
#include <qwidgetstack.h>
#include <qcombobox.h>

KantSidebar::KantSidebar(QWidget* parent, const char* name) : QWidget(parent, name)
{
  setFocusPolicy(QWidget::ClickFocus);

  QVBoxLayout* lo = new QVBoxLayout(this);
  cmb = new QComboBox( this );
  cmb->setFocusPolicy(QWidget::ClickFocus);
  lo->addWidget( cmb );
  stack = new QWidgetStack(this);
  lo->addWidget( stack );
  stack->setFocusPolicy(QWidget::ClickFocus);
  connect(cmb, SIGNAL( activated( int ) ), stack, SLOT( raiseWidget( int ) ) );
}

KantSidebar::~KantSidebar()
{
}

void KantSidebar::addWidget(QWidget* widget, const QString & label )
{
  int id = cmb->count();
  stack->addWidget(widget, id);
  cmb->insertItem(label, id);
  cmb->setCurrentItem(id);
  stack->raiseWidget( id );
  cmb->setFocus();
  widget->setFocus();
  widget->setFocusPolicy(QWidget::ClickFocus);
}

void KantSidebar::removeWidget(QWidget* widget)
{
  cmb->removeItem( stack->id( widget ) );
  stack->removeWidget( widget );
}

void KantSidebar::focusNextWidget()
{
  int id = cmb->currentItem();

  if ( id < cmb->count()-1 )
    id++;
  else
    id = 0;

  cmb->setCurrentItem( id );
  stack->raiseWidget( id );
  stack->visibleWidget()->setFocus();
}

void KantSidebar::readConfig(KConfig* config, const char* group)
{
  config->setGroup(group);
  QString t = config->readEntry("Current", "Files");
  for (int i=0; i<cmb->count()-1; i++)
  {
    if ( cmb->text( i ).compare( t ) == 0 )
    {
      cmb->setCurrentItem( i );
      stack->raiseWidget( i );
      break;
    }
  }
}

void KantSidebar::saveConfig(KConfig* config, const char* group)
{
  config->setGroup(group);
  config->writeEntry("Current", cmb->currentText());
}
