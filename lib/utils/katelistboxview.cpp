/*
        katelistboxview.cpp
        List box view for kate plugins
        Copyright (C) 2002 by Anders Lund <anders@alweb.dk>

        $Id:$
        ---

        This program is free software; you can redistribute it and/or
        modify it under the terms of the GNU General Public License
        as published by the Free Software Foundation; either version 2
        of the License, or (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program; if not, write to the Free Software
        Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "katelistboxview.h"
#include <klistbox.h>

// private storage
class KateListboxViewPrivate {
  public:
    KListbox *listbox;
};


KateListboxView::KateListboxView( QWidget *parent, const char *name )
  : KateDockViewBase( parent, name),
    d( new KateListboxViewPrivate)
{
  d->listbox = new KListbox( this );
}

KateListboxView::KateListboxView( const QString &titlePrefix, const QString &title, QWidget *parent, const char *name )
  : KateDockViewBase( titlePrefix, title, parent, name),
    d( new KateListboxViewPrivate)
{
  d->listbox = new KListbox( this );
}

KateListboxView::~KateListboxView()
{
  delete d;
}

KListbox *KateListboxView::listbox()
{
  return d->listbox;
}