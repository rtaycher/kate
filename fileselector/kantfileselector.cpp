/***************************************************************************
                          KantFileSelector.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Matt Newell
    email                : newellm@proaxis.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantfileselector.h"
#include "kantfileselector.moc"

#include <qlayout.h>
#include <qpushbutton.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qstrlist.h>

#include <kiconloader.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kprotocolinfo.h>
#include <kdiroperator.h>
#include <kconfig.h>
#include <klocale.h>
#include <kcombobox.h>

#include <kdebug.h>

KantFileSelector::KantFileSelector( QWidget * parent, const char * name ): QWidget(parent, name)
{
  QVBoxLayout* lo = new QVBoxLayout(this);

  QHBox *hlow = new QHBox (this);
  lo->addWidget(hlow);

  QPushButton *up = new QPushButton( "&Up", hlow );
  QPushButton *back = new QPushButton( "&Back", hlow );
  QPushButton *forward = new QPushButton( "&Next", hlow );

  hlow->setMaximumHeight(up->height());

  cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
  cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  KURLCompletion* cmpl = new KURLCompletion();
  connect ( cmpl, SIGNAL(matches(const QStringList &)), this, SLOT(cmplMatch(const QStringList &)) );
  cmbPath->setCompletionObject( cmpl );
  lo->addWidget(cmbPath);

  dir = new KDirOperator(QString::null, this, "operator");
  dir->setView(KFile::Simple);
  lo->addWidget(dir);
  lo->setStretchFactor(dir, 2);


  QHBox* filterBox = new QHBox(this);
  filterIcon = new QLabel(filterBox);
  filterIcon->setPixmap( BarIcon("filter") );
  filter = new KHistoryCombo(filterBox, "filter");
  filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  filterBox->setStretchFactor(filter, 2);
  lo->addWidget(filterBox);

  connect( filter, SIGNAL( activated(const QString&) ), SLOT( slotFilterChange(const QString&) ) );
  connect( filter, SIGNAL( returnPressed(const QString&) ),filter, SLOT( addToHistory(const QString&) ) );

  connect( up, SIGNAL( clicked() ), dir, SLOT( cdUp() ) );
  connect( back, SIGNAL( clicked() ), dir, SLOT( back() ) );
  connect( forward, SIGNAL( clicked() ), dir, SLOT( forward() ) );

  connect( cmbPath, SIGNAL( urlActivated( const KURL&  )),
             this,  SLOT( cmbPathActivated( const KURL& ) ));
  connect( cmbPath, SIGNAL( returnPressed( const QString&  )),
             this,  SLOT( cmbPathReturnPressed( const QString& ) ));
  connect(dir, SIGNAL(urlEntered(const KURL&)),
             this, SLOT(dirUrlEntered(const KURL&)) );

}

KantFileSelector::~KantFileSelector()
{
}

void KantFileSelector::readConfig(KConfig *config, const QString & name)
{
  dir->readConfig(config, name + ":dir");

  config->setGroup( name );
  cmbPath->setURLs( config->readListEntry("dir history") );
  cmbPathReturnPressed( cmbPath->currentText() );

  filter->setHistoryItems( config->readListEntry("filter history") );

  if ( config->readNumEntry("current filter") )
    filter->setCurrentItem( config->readNumEntry("current filter") );

  slotFilterChange( filter->currentText() );
}

void KantFileSelector::saveConfig(KConfig *config, const QString & name)
{
  dir->saveConfig(config,name + ":dir");

  config->setGroup( name );
  QStringList l;
  for (int i = 0; i < cmbPath->count(); i++) {
    l.append( cmbPath->text( i ) );
  }
  config->writeEntry("dir history", l );

  config->writeEntry("filter history", filter->historyItems());
  config->writeEntry("current filter", filter->currentItem());
}

void KantFileSelector::setView(KFile::FileView view)
{
 dir->setView(view);
}

void KantFileSelector::slotFilterChange( const QString & nf )
{
  dir->setNameFilter( nf );
  dir->rereadDir();
}

void KantFileSelector::cmbPathActivated( const KURL& u )
{
   dir->setURL( u, true );
}

void KantFileSelector::cmbPathReturnPressed( const QString& u )
{
   dir->setFocus();
   dir->setURL( KURL(u), true );
}

void KantFileSelector::dirUrlEntered( const KURL& u )
{
kdDebug()<<"dirUrlEneterd(): "<<u.url()<<endl;
kdDebug()<<QString("max items: %1").arg(cmbPath->maxItems())<<endl;
   cmbPath->removeURL( u );
   QStringList urls = cmbPath->urls();
   urls.prepend( u.url() );
   while ( urls.count() >= cmbPath->maxItems() )
      urls.remove( urls.last() );
   cmbPath->setURLs( urls );

}

void KantFileSelector::cmplMatch( const QStringList & s )
{
  //if (KURL(s).isLocalFile())
    kdDebug()<<"matches: "<<s.join(",")<<endl;
}

void KantFileSelector::focusInEvent(QFocusEvent*)
{
   dir->setFocus();
}
