/***************************************************************************
                          kantdocument.cpp  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "kantview.h"
#include "kantview.moc"

#include "../document/kantdocument.h"

#include "../kwrite/kwview.h"
#include "../kwrite/kwattribute.h"
#include "../kwrite/kwdoc.h"
#include "../kwrite/kwdialog.h"
#include "../kwrite/highlight.h"
#include "../kwrite/kwrite_factory.h"

#include <kaction.h>

#include <qfocusdata.h>
#include <kdebug.h>
#include <kapp.h>

KantView::KantView(QWidget *parent, KantDocument *doc, const char * name, bool HandleOwnURIDrops)
    : KWrite (doc, parent, name, HandleOwnURIDrops, false), DCOPObject (name)
{
  active = false;

  setFocusPolicy(QWidget::ClickFocus);

  kWriteView->installEventFilter( this );
}

KantView::~KantView()
{
}

void KantView::setActive (bool b)
{
  active = b;
}

bool KantView::isActive ()
{
  return active;
}

void KantView::setFocus ()
{
  KWrite::setFocus ();

  emit gotFocus (this);
}

bool KantView::eventFilter(QObject* o, QEvent* e)
{
  if (e->type() == QEvent::FocusIn)
    emit gotFocus (this);

  return QWidget::eventFilter(o, e);
}

void KantView::searchAgain(bool back) {
  bool b= (searchFlags & sfBackward) > 0;
  initSearch(s, (searchFlags & ((b==back)?~sfBackward:~0))  // clear flag for forward searching
                | sfFromCursor | sfPrompt | sfAgain | ((b!=back)?sfBackward:0) );
  if (s.flags & sfReplace)
    replaceAgain();
  else
    KWrite::searchAgain(s);
}
