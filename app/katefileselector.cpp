/***************************************************************************
                          KateFileSelector.cpp  -  description
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

#include "katefileselector.h"
#include "katefileselector.moc"

#include "katemainwindow.h"
#include "kateviewmanager.h"

#include <qlayout.h>
#include <qtoolbutton.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qstrlist.h>
#include <qtooltip.h>

#include <kiconloader.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kprotocolinfo.h>
#include <kdiroperator.h>
#include <kconfig.h>
#include <klocale.h>
#include <kcombobox.h>

#include <kmessagebox.h>

KateFileSelector::KateFileSelector( KateMainWindow *mainWindow, KateViewManager *viewManager,
                                    QWidget * parent, const char * name )
    : QWidget(parent, name),
      mainwin(mainWindow),
      viewmanager(viewManager)
{
  QVBoxLayout* lo = new QVBoxLayout(this);

  QHBox *hlow = new QHBox (this);
  lo->addWidget(hlow);

  home = new QToolButton( hlow );
  home->setIconSet(SmallIconSet("gohome"));
  QToolTip::add(home, i18n("Home directory"));
  up = new QToolButton( hlow );
  up->setIconSet(SmallIconSet("up"));
  QToolTip::add(up, i18n("Up one level"));
  back = new QToolButton( hlow );
  back->setIconSet(SmallIconSet("back"));
  QToolTip::add(back, i18n("Previous directory"));
  forward = new QToolButton( hlow );
  forward->setIconSet(SmallIconSet("forward"));
  QToolTip::add(forward, i18n("Next Directory"));

  // HACK
  QWidget* spacer = new QWidget(hlow);
  hlow->setStretchFactor(spacer, 1);
  hlow->setMaximumHeight(up->height());

  cfdir = new QToolButton( hlow );
  cfdir->setIconSet(SmallIconSet("curfiledir"));
  QToolTip::add(cfdir, i18n("Current Document Directory"));

  cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
  cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  KURLCompletion* cmpl = new KURLCompletion(KURLCompletion::DirCompletion);
  cmbPath->setCompletionObject( cmpl );
  lo->addWidget(cmbPath);

  dir = new KDirOperator(QString::null, this, "operator");
  dir->setView(KFile::/*Simple*/Detail);
  dir->actionCollection()->action( "delete" )->setShortcut (KShortcut (ALT+Key_Delete));
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

  connect( home, SIGNAL( clicked() ), dir, SLOT( home() ) );
  connect( up, SIGNAL( clicked() ), dir, SLOT( cdUp() ) );
  connect( back, SIGNAL( clicked() ), dir, SLOT( back() ) );
  connect( forward, SIGNAL( clicked() ), dir, SLOT( forward() ) );
  connect( cfdir, SIGNAL( clicked() ), this, SLOT( setCurrentDocDir() ) );

  connect( cmbPath, SIGNAL( urlActivated( const KURL&  )),
             this,  SLOT( cmbPathActivated( const KURL& ) ));
  connect( cmbPath, SIGNAL( returnPressed( const QString&  )),
             this,  SLOT( cmbPathReturnPressed( const QString& ) ));
  connect(dir, SIGNAL(urlEntered(const KURL&)),
             this, SLOT(dirUrlEntered(const KURL&)) );

  connect(dir, SIGNAL(finishedLoading()),
             this, SLOT(dirFinishedLoading()) );

  // enable dir sync button if current doc has a valid URL
  connect ( viewmanager, SIGNAL( viewChanged() ),
              this, SLOT( kateViewChanged() ) );
}

KateFileSelector::~KateFileSelector()
{
}

void KateFileSelector::readConfig(KConfig *config, const QString & name)
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

void KateFileSelector::writeConfig(KConfig *config, const QString & name)
{
  dir->writeConfig(config,name + ":dir");

  config->setGroup( name );
  QStringList l;
  for (int i = 0; i < cmbPath->count(); i++) {
    l.append( cmbPath->text( i ) );
  }
  config->writeEntry("dir history", l );

  config->writeEntry("filter history", filter->historyItems());
  config->writeEntry("current filter", filter->currentItem());
}

void KateFileSelector::setView(KFile::FileView view)
{
 dir->setView(view);
}

void KateFileSelector::slotFilterChange( const QString & nf )
{
  dir->setNameFilter( nf );
  dir->rereadDir();
}

void KateFileSelector::cmbPathActivated( const KURL& u )
{
   dir->setURL( u, true );
}

void KateFileSelector::cmbPathReturnPressed( const QString& u )
{
   dir->setFocus();
   dir->setURL( KURL(u), true );
}

void KateFileSelector::dirUrlEntered( const KURL& u )
{
   cmbPath->removeURL( u );
   QStringList urls = cmbPath->urls();
   urls.prepend( u.url() );
   while ( urls.count() >= (uint)cmbPath->maxItems() )
      urls.remove( urls.last() );
   cmbPath->setURLs( urls );
}

void KateFileSelector::dirFinishedLoading()
{
   // HACK - enable the nav buttons
   // have to wait for diroperator...
   up->setEnabled( dir->actionCollection()->action( "up" )->isEnabled() );
   back->setEnabled( dir->actionCollection()->action( "back" )->isEnabled() );
   forward->setEnabled( dir->actionCollection()->action( "forward" )->isEnabled() );
   home->setEnabled( dir->actionCollection()->action( "home" )->isEnabled() );
}

void KateFileSelector::focusInEvent(QFocusEvent*)
{
   dir->setFocus();
}

void KateFileSelector::setDir( KURL u )
{
  dir->setURL(u, true);
}

void KateFileSelector::setCurrentDocDir()
{
  KURL u = mainwin->currentDocUrl().directory();
  if (!u.isEmpty())
    setDir( u );
}

void KateFileSelector::kateViewChanged()
{
  // TODO: make sure the button is disabled if the directory is unreadable, eg the document URL
  //       has protocol http
  cfdir->setEnabled( ! mainwin->currentDocUrl().directory().isEmpty() );
}

