/*
        katedockviewbase.cpp
        Base class for dock views
        Copyright (C) 2002 by Anders Lund <anders@alweb.dk>

        $Id: $
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

#include "katedockviewbase.h"
#include "katedockviewbase.moc"

#include <qlabel.h>
#include <qlayout.h>

//#include <kdebug.h>

// data storage
class KateDockViewBasePrivate {
  public:
  QWidget *header;
  QLabel *lTitle;
  QLabel *lPrefix;
};

KateDockViewBase::KateDockViewBase( QWidget* parent, const char* name )
  : QVBox( parent, name ),
    d ( new KateDockViewBasePrivate )
{
  init( QString::null, QString::null );
}

KateDockViewBase::KateDockViewBase( const QString &prefix, const QString &title, QWidget* parent, const char* name )
  : QVBox( parent, name ),
    d ( new KateDockViewBasePrivate )
{
  init( prefix, title );
}

KateDockViewBase::~KateDockViewBase()
{
  delete d;
}

void KateDockViewBase::setTitlePrefix( const QString &prefix )
{
    d->lPrefix->setText( prefix );
    d->lPrefix->show();
}

QString KateDockViewBase::titlePrefix() const
{
  return d->lPrefix->text();
}

void KateDockViewBase::setTitle( const QString &title )
{
  d->lTitle->setText( title );
  d->lTitle->show();
}

QString KateDockViewBase::title() const
{
  return d->lTitle->text();
}

void KateDockViewBase::setTitle( const QString &prefix, const QString &title )
{
  setTitlePrefix( prefix );
  setTitle( title );
}

void KateDockViewBase::init( const QString &prefix, const QString &title )
{
  setSpacing( 4 );
  d->header = new QWidget( this );
  d->header->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed, true ) );
  QHBoxLayout *lo = new QHBoxLayout( d->header );
  lo->setSpacing( 6 );
  lo->insertSpacing( 0, 6 ); 
  d->lPrefix = new QLabel( title, d->header );
  lo->addWidget( d->lPrefix );
  d->lTitle = new QLabel( title, d->header );
  lo->addWidget( d->lTitle );
  lo->setStretchFactor( d->lTitle, 1 );
  lo->insertSpacing( -1, 6 );
  if ( prefix.isEmpty() ) d->lPrefix->hide();
  if ( title.isEmpty() ) d->lTitle->hide();
}
