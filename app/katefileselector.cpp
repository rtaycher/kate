/***************************************************************************
                          KateFileSelector.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Matt Newell <newellm@proaxis.com>
			   (C) 2002 by Anders Lund <anders@alweb.dk>
			   (C) 2002 by Joseph Wenninger <jowenn@kde.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//BEGIN Includes
#include "katefileselector.h"
#include "katefileselector.moc"

#include "katemainwindow.h"
#include "kateviewmanager.h"
#include "kbookmarkhandler.h"

#include "kactionselector.h"

#include <qlayout.h>
#include <qtoolbutton.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlabel.h>
#include <qstrlist.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qapplication.h>
#include <qlistbox.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qdockarea.h>


#include <kapplication.h>
#include <kiconloader.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
#include <kprotocolinfo.h>
#include <kdiroperator.h>
#include <kconfig.h>
#include <klocale.h>
#include <kcombobox.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <ktoolbarbutton.h>
#include <qtoolbar.h>
#include <kpopupmenu.h>
#include <kdialog.h>

#include <kdebug.h>
//END Includes

 // from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar(QtMsgType, const char *){}


KateFileSelectorToolBar::KateFileSelectorToolBar(QWidget *parent):KToolBar( parent, "Kate FileSelector Toolbar", true )
{
	setMinimumWidth(10);
}

KateFileSelectorToolBar::~KateFileSelectorToolBar(){}

void KateFileSelectorToolBar::setMovingEnabled( bool)
{
	//kdDebug()<<"JoWenn's setMovingEnabled called ******************************"<<endl;
	KToolBar::setMovingEnabled(false);
}


KateFileSelectorToolBarParent::KateFileSelectorToolBarParent(QWidget *parent)
	:QFrame(parent),m_tb(0){}
KateFileSelectorToolBarParent::~KateFileSelectorToolBarParent(){}
void KateFileSelectorToolBarParent::setToolBar(KateFileSelectorToolBar *tb)
{
	m_tb=tb;
}

void KateFileSelectorToolBarParent::resizeEvent ( QResizeEvent * )
{
	if (m_tb)
	{
		setMinimumHeight(m_tb->sizeHint().height());
		m_tb->resize(width(),height());
	}
}


//BEGIN Constructor/destructor

KateFileSelector::KateFileSelector( KateMainWindow *mainWindow, KateViewManager *viewManager,
                                    QWidget * parent, const char * name )
    : QWidget(parent, name),
      mainwin(mainWindow),
      viewmanager(viewManager)
{
  mActionCollection = new KActionCollection( this );
  
  QVBoxLayout* lo = new QVBoxLayout(this);

 // tbparent = new TBContainer( this );
//  lo->addWidget( tbparent );
//    QDockArea *tbParentDock=new QDockArea(Horizontal,QDockArea::Reverse,this);
//    lo->addWidget (tbParentDock);
  QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );
 
  KateFileSelectorToolBarParent *tbp=new KateFileSelectorToolBarParent(this);
  toolbar = new KateFileSelectorToolBar(tbp);
  tbp->setToolBar(toolbar);
  lo->addWidget(tbp);
//  lo->addWidget(toolbar);
//  lo->addWidget(toolbar);
  toolbar->setMovingEnabled(false);
  toolbar->setFlat(true);
//  tbparent->setTb( toolbar );
  qInstallMsgHandler( oldHandler );

  cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
  cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  KURLCompletion* cmpl = new KURLCompletion(KURLCompletion::DirCompletion);
  cmbPath->setCompletionObject( cmpl );
  lo->addWidget(cmbPath);
  cmbPath->listBox()->installEventFilter( this );

  dir = new KDirOperator(QString::null, this, "operator");
  dir->setView(KFile::/*Simple*/Detail);

  KActionCollection *coll = dir->actionCollection();
  // some shortcuts of diroperator that clashes with Kate
  coll->action( "delete" )->setShortcut( KShortcut( ALT + Key_Delete ) );
  coll->action( "reload" )->setShortcut( KShortcut( ALT + Key_F5 ) );
  coll->action( "back" )->setShortcut( KShortcut( ALT + SHIFT + Key_Left ) );
  coll->action( "forward" )->setShortcut( KShortcut( ALT + SHIFT + Key_Right ) );
  // some consistency - reset up for dir too
  coll->action( "up" )->setShortcut( KShortcut( ALT + SHIFT + Key_Up ) );
  coll->action( "home" )->setShortcut( KShortcut( CTRL + ALT + Key_Home ) );
  
  lo->addWidget(dir);
  lo->setStretchFactor(dir, 2);

  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu( i18n("Bookmarks"), "bookmark",
        mActionCollection, "bookmarks" );
  acmBookmarks->setDelayed( false );
  bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->popupMenu() );
  QHBox* filterBox = new QHBox(this);

  btnFilter = new QToolButton( filterBox );
  btnFilter->setIconSet( SmallIconSet("filter" ) );
  btnFilter->setToggleButton( true );
  filter = new KHistoryCombo( true, filterBox, "filter");
  filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  filterBox->setStretchFactor(filter, 2);
  connect( btnFilter, SIGNAL( clicked() ), this, SLOT( btnFilterClick() ) );
  lo->addWidget(filterBox);

  connect( filter, SIGNAL( activated(const QString&) ), SLOT( slotFilterChange(const QString&) ) );
  connect( filter, SIGNAL( returnPressed(const QString&) ),filter, SLOT( addToHistory(const QString&) ) );

  // kaction for the dir sync method
  acSyncDir = new KAction( i18n("Current Document Directory"), "curfiledir", 0, 
        this, SLOT( setActiveDocumentDir() ), mActionCollection, "sync_dir" );
  toolbar->setIconText( KToolBar::IconOnly );
  toolbar->setIconSize( 16 );
  toolbar->setEnableContextMenu( false );
//  toolbar->setMovingEnabled( FALSE ); DOES NOT WORK: GRRRRRRRRRRRRRRRRRRRR
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
              
  // Connect the bookmark handler
  connect(bookmarkHandler, SIGNAL( openURL( const QString& )), this, SLOT( setDir( const QString& ) ) );
              
  waitingUrl = QString::null;

  // whatsthis help
  QWhatsThis::add( cmbPath,
       i18n("<p>Here you can enter a path for a directory to display."
            "<p>To go to a directory previously entered, press the arrow on the right end and choose one."
            "<p>The entry has directory completion. Right-click to choose how completion should behave.") );
  QWhatsThis::add( filter,
        i18n("<p>Here you can enter a name filter to limit which files are displayed."
             "<p>To clear the filter, toggle off the filter button to the left."
             "<p>To reapply the last filter used, toggle on the filter button." ) );
  QWhatsThis::add( btnFilter,
        i18n("<p>This button clears the name filter when toggled off, or reapplies the "
             "last filter used when toggled on.") );

}

KateFileSelector::~KateFileSelector()
{
}
//END Constroctor/Destrctor

//BEGIN Public Methods

void KateFileSelector::readConfig(KConfig *config, const QString & name)
{
  dir->readConfig(config, name + ":dir");
  dir->setView( KFile::Default );
  
  config->setGroup( name );
  
  // set up the toolbar
  setupToolbar( config );
  
  cmbPath->setMaxItems( config->readNumEntry( "pathcombo history len", 9 ) );
  cmbPath->setURLs( config->readListEntry("dir history") );
  // if we restore history
  if ( config->readBoolEntry( "restore location", true ) || kapp->isRestored() ) {
    QString loc( config->readEntry( "location" ) );
    if ( ! loc.isEmpty() )
      setDir( loc );
  }
  // else is automatic, as cmpPath->setURL is called when a location is entered.

  filter->setMaxCount( config->readNumEntry( "filter history len", 9 ) );
  filter->setHistoryItems( config->readListEntry("filter history"), true );
  lastFilter = config->readEntry( "last filter" );
  QString flt("");
  if ( config->readBoolEntry( "restore last filter", true ) || kapp->isRestored() )
    flt = config->readEntry("current filter");
  filter->lineEdit()->setText( flt );
  slotFilterChange( flt );

  autoSyncEvents = config->readNumEntry( "AutoSyncEvents", 0 );
  // connect events as needed
  // TODO - solve startup problem: no need to set location for each doc opened!
  if ( autoSyncEvents & DocumentChanged )
    connect( viewmanager, SIGNAL( viewChanged() ), this, SLOT( autoSync() ) );
  
  if ( autoSyncEvents & DocumentOpened )
    connect( mainwin->m_docManager, SIGNAL( documentCreated(Kate::Document *) ),
                this, SLOT( autoSync(Kate::Document *) ) );

}

void KateFileSelector::setupToolbar( KConfig *config )
{
  toolbar->clear();
  QStringList tbactions = config->readListEntry( "toolbar actions", ',' );
  if ( tbactions.isEmpty() ) {
    // resonable collection for default toolbar
    tbactions << "up" << "back" << "forward" << "home" <<
                "short view" << "detailed view" <<
                "bookmarks" << "sync_dir";
  }
  KAction *ac;
  for ( QStringList::Iterator it=tbactions.begin(); it != tbactions.end(); ++it ) {
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = mActionCollection->action( (*it).latin1() );
    else
      ac = dir->actionCollection()->action( (*it).latin1() );
    if ( ac )
      ac->plug( toolbar );
  }
}

void KateFileSelector::writeConfig(KConfig *config, const QString & name)
{
  dir->writeConfig(config,name + ":dir");

  config->setGroup( name );
  config->writeEntry( "pathcombo history len", cmbPath->maxItems() );
  QStringList l;
  for (int i = 0; i < cmbPath->count(); i++) {
    l.append( cmbPath->text( i ) );
  }
  config->writeEntry("dir history", l );
  config->writeEntry( "location", cmbPath->currentText() );

  config->writeEntry( "filter history len", filter->maxCount() );
  config->writeEntry( "filter history", filter->historyItems() );
  config->writeEntry( "current filter", filter->currentText() );
  config->writeEntry( "last filter", lastFilter );
  config->writeEntry( "AutoSyncEvents", autoSyncEvents );
}

void KateFileSelector::setView(KFile::FileView view)
{
  dir->setView(view);
}

//END Public Methods

//BEGIN Public Slots

void KateFileSelector::slotFilterChange( const QString & nf )
{
  QString f = nf.stripWhiteSpace();
  bool empty = f.isEmpty() || f == "*";
  if ( empty ) {
    dir->clearFilter();
    filter->lineEdit()->setText( QString::null );
    QToolTip::add( btnFilter, QString( i18n("Apply Last Filter (\"%1\")") ).arg( lastFilter ) );
  }
  else {
    dir->setNameFilter( f );
    lastFilter = f;
    QToolTip::add( btnFilter, i18n("Clear Filter") );
  }
  btnFilter->setOn( !empty );
  dir->updateDir();
  // this will be never true after the filter has been used;)
  btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}
void KateFileSelector::setDir( KURL u )
{
  dir->setURL(u, true);
}

//END Public Slots

//BEGIN Private Slots

void KateFileSelector::cmbPathActivated( const KURL& u )
{
   cmbPathReturnPressed( u.url() );
}

void KateFileSelector::cmbPathReturnPressed( const QString& u )
{
   QStringList urls = cmbPath->urls();
   urls.remove( u );
   urls.prepend( u );
   cmbPath->setURLs( urls, KURLComboBox::RemoveBottom );
   dir->setFocus();
   dir->setURL( KURL(u), true );
}

void KateFileSelector::dirUrlEntered( const KURL& u )
{
  cmbPath->setURL( u );
}

void KateFileSelector::dirFinishedLoading()
{
}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void KateFileSelector::btnFilterClick()
{
  if ( !btnFilter->isOn() ) {
    slotFilterChange( QString::null );
  }
  else {
    filter->lineEdit()->setText( lastFilter );
    slotFilterChange( lastFilter );
  }
}


void KateFileSelector::autoSync()
{
  kdDebug()<<"KateFileSelector::autoSync()"<<endl;
  // if visible, sync
  if ( isVisible() ) {
    setActiveDocumentDir();
    waitingUrl = QString::null;
  }
  // else set waiting url
  else {
    KURL u = mainwin->activeDocumentUrl();
    if (!u.isEmpty())
      waitingUrl = u.directory();
  }
}

void KateFileSelector::autoSync( Kate::Document *doc )
{
  // as above, but using document url.
  kdDebug()<<"KateFileSelector::autoSync( Kate::Document )"<<endl;
  KURL u ( doc->url() );
  if ( u.isEmpty() ) {
    waitingUrl = QString::null;
    return;
  }
  if ( isVisible() ) {
    setDir( u.directory() );
    waitingUrl = QString::null;
  }
  else {
    waitingUrl = u.directory();
  }
}
//FIXME crash on shutdown
void KateFileSelector::setActiveDocumentDir()
{
kdDebug()<<"KateFileSelector::setActiveDocumentDir()"<<endl;
  KURL u = mainwin->activeDocumentUrl();
kdDebug()<<"KateFileSelector::setActiveDocumentDir(): URL is "<<u.url()<<endl;
  if (!u.isEmpty())
    setDir( u.directory() );
}

void KateFileSelector::kateViewChanged()
{
  // TODO: make sure the button is disabled if the directory is unreadable, eg the document URL
  //       has protocol http
  acSyncDir->setEnabled( ! mainwin->activeDocumentUrl().directory().isEmpty() );
}

//END Private Slots

//BEGIN Protected

void KateFileSelector::focusInEvent( QFocusEvent * )
{
   dir->setFocus();
}

void KateFileSelector::showEvent( QShowEvent * )
{
    // sync if we should
    if ( autoSyncEvents & GotVisible ) {
    kdDebug()<<"syncing fs on show"<<endl;
      setActiveDocumentDir();
      waitingUrl = QString::null;
    }
    // else, if we have a waiting URL set it
    else if ( ! waitingUrl.isEmpty() ) {
      setDir( waitingUrl );
      waitingUrl = QString::null;
   }
}

bool KateFileSelector::eventFilter( QObject* o, QEvent *e )
{
  /*
      This is rather unfortunate, but:
      QComboBox does not support setting the size of the listbox to something
      resonable. Even using listbox->setVariableWidth() does not yeld a satisfying
      result, something is wrong with the handling of the sizehint. And the popup is
      rather useless, if the paths are only partly visible.
  */
  QListBox *lb = cmbPath->listBox();
  if ( o == lb && e->type() == QEvent::Show ) {
    int add = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
    int w = QMIN( mainwin->width(), lb->contentsWidth() + add );
    lb->resize( w, lb->height() );
    // TODO - move the listbox to a suitable place if nessecary
    // TODO - decide if it is worth caching the size while untill the contents are changed.
  }
  // TODO - same thing for the completion popup?
  return QWidget::eventFilter( o, e );
}

//END Protected

//BEGIN ACtionLBItem
/*
   QListboxItem that can store and return a string,
   used for the toolbar action selector.
*/
class ActionLBItem : public QListBoxPixmap {
  public:
  ActionLBItem( QListBox *lb=0, 
                const QPixmap &pm = QPixmap(), 
                const QString &text=QString::null,
                const QString &str=QString::null ) : 
    QListBoxPixmap( lb, pm, text ),
    _str(str) {};
  QString idstring() { return _str; };
  private:
    QString _str;
};
//END ActionLBItem

//BEGIN KFSConfigPage
////////////////////////////////////////////////////////////////////////////////
// KFSConfigPage implementation
////////////////////////////////////////////////////////////////////////////////
KFSConfigPage::KFSConfigPage( QWidget *parent, const char *name, KateFileSelector *kfs )
  : Kate::ConfigPage( parent, name ),
    fileSelector( kfs ),
    bDirty( false )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  lo->setSpacing( spacing );

  // Toolbar - a lot for a little...
  QGroupBox *gbToolbar = new QGroupBox( 1, Qt::Vertical, i18n("Toolbar"), this );
  acSel = new KActionSelector( gbToolbar );
  acSel->setAvailableLabel( i18n("A&vailable actions:") );
  acSel->setSelectedLabel( i18n("S&elected actions:") );
  lo->addWidget( gbToolbar );
  // Sync
  QGroupBox *gbSync = new QGroupBox( 1, Qt::Horizontal, i18n("Auto Synchronization"), this );
  cbSyncActive = new QCheckBox( i18n("When a docu&ment becomes active"), gbSync );
  cbSyncOpen = new QCheckBox( i18n("When a document is o&pened"), gbSync );
  cbSyncShow = new QCheckBox( i18n("When the file selector becomes visible"), gbSync );
  lo->addWidget( gbSync );

  // Histories
  QHBox *hbPathHist = new QHBox ( this );
  QLabel *lbPathHist = new QLabel( i18n("Remember &locations:"), hbPathHist );
  sbPathHistLength = new QSpinBox( hbPathHist );
  lbPathHist->setBuddy( sbPathHistLength );
  lo->addWidget( hbPathHist );

  QHBox *hbFilterHist = new QHBox ( this );
  QLabel *lbFilterHist = new QLabel( i18n("Remember &filters:"), hbFilterHist );
  sbFilterHistLength = new QSpinBox( hbFilterHist );
  lbFilterHist->setBuddy( sbFilterHistLength );
  lo->addWidget( hbFilterHist );

  // Session
  QGroupBox *gbSession = new QGroupBox( 1, Qt::Horizontal, i18n("Session"), this );
  cbSesLocation = new QCheckBox( i18n("Restore loca&tion"), gbSession );
  cbSesFilter = new QCheckBox( i18n("Restore last f&ilter"), gbSession );
  lo->addWidget( gbSession );

  // make it look nice
  lo->addStretch( 1 );

  // be helpfull
  /*
  QWhatsThis::add( lbAvailableActions, i18n(
        "<p>Available actions for the toolbar. To add an action, select it here "
        "and press the add (<strong>-&gt;</strong>) button" ) );
  QWhatsThis::add( lbUsedActions, i18n(
        "<p>Actions used in the toolbar. To remove an action, select it and "
        "press the remove (<strong>&lt;-</strong>) button."
        "<p>To change the order of the actions, use the Up and Down buttons to "
        "move the selected action.") );
  */
  QString lhwt( i18n(
        "<p>Decides how many locations to keep in the history of the location combo box") );
  QWhatsThis::add( lbPathHist, lhwt );
  QWhatsThis::add( sbPathHistLength, lhwt );
  QString fhwt( i18n(
        "<p>Decides how many filters to keep in the history of the filter combo box") );
  QWhatsThis::add( lbFilterHist, fhwt );
  QWhatsThis::add( sbFilterHistLength, fhwt );
  QString synwt( i18n(
        "<p>These options allow you to have the File Selector automatically change "
        "location to the directory of the active document on certain events."
        "<p>Auto synchronization is <em>lazy</em>, meaning it will not take effect "
        "until the file selector is visible."
        "<p>None of these are enabled by default, but you can always sync the location "
        "by pressing the sync button in the toolbar.") );
  QWhatsThis::add( gbSync, synwt );
  QWhatsThis::add( cbSesLocation, i18n(
        "<p>If this option is enabled (default), the location will be restored when "
        "you start Kate.<p><strong>Note</strong> that if the session is handled by the KDE "
        "session manager, the location is always restored.") );
  QWhatsThis::add( cbSesFilter, i18n(
        "<p>If this option is enabled (default), the current filter will be restored when "
        "you start Kate.<p><strong>Note</strong> that if the session is handled by the KDE "
        "session manager, the filter is always restored."
        "<p><strong>Note</strong> that some of the autosync settings may override the "
        "restored location if on.") );

  init();

}

void KFSConfigPage::apply()
{
  KConfig *config = kapp->config();
  config->setGroup( "fileselector" );
  // toolbar
  QStringList l;
  QListBoxItem *item = acSel->selectedListBox()->firstItem();
  ActionLBItem *aItem;
  while ( item )
  {
    aItem = (ActionLBItem*)item;
    if ( aItem )
    {
      l << aItem->idstring();
    }
    item = item->next();
  }
  config->writeEntry( "toolbar actions", l );
  fileSelector->setupToolbar( config );
  // sync
  int s = 0;
  if ( cbSyncActive->isChecked() )
    s |= KateFileSelector::DocumentChanged;
  if ( cbSyncOpen->isChecked() )
    s |= KateFileSelector::DocumentOpened;
  if ( cbSyncShow->isChecked() )
    s |= KateFileSelector::GotVisible;
  fileSelector->autoSyncEvents = s;
  // reset connections
  disconnect( fileSelector->viewmanager, 0, fileSelector, SLOT( autoSync() ) );
  disconnect( fileSelector->mainwin->m_docManager, 0,
                fileSelector, SLOT( autoSync( Kate::Document *) ) );
  if ( s & KateFileSelector::DocumentChanged )
    connect( fileSelector->viewmanager, SIGNAL( viewChanged() ),
                fileSelector, SLOT( autoSync() ) );
  if ( s & KateFileSelector::DocumentOpened )
    connect( fileSelector->mainwin->m_docManager,
                SIGNAL( documentCreated(Kate::Document *) ),
                fileSelector, SLOT( autoSync(Kate::Document *) ) );

  // histories
  fileSelector->cmbPath->setMaxItems( sbPathHistLength->value() );
  fileSelector->filter->setMaxCount( sbFilterHistLength->value() );
  // session - theese are read/written directly to the app config,
  //           as they are not needed during operation.
  config->writeEntry( "restore location", cbSesLocation->isChecked() );
  config->writeEntry( "restore last filter", cbSesFilter->isChecked() );
}

void KFSConfigPage::reload()
{
  // hmm, what is this supposed to do, actually??
  init();
}
void KFSConfigPage::init()
{
  KConfig *config = kapp->config();
  config->setGroup( "fileselector" );
  // toolbar
  QStringList l = config->readListEntry( "fstoolbar", ',' );
  if ( l.isEmpty() ) // default toolbar
    l << "up" << "back" << "forward" << "home" <<
                "short view" << "detailed view" <<
                "bookmarks" << "sync_dir";
  
  // actions from diroperator + two of our own
  QStringList allActions;
  allActions << "up" << "back" << "forward" << "home" <<
                "reload" << "mkdir" << "delete" <<
                "short view" << "detailed view" /*<< "view menu" <<
                "show hidden" << "properties"*/ <<
                "bookmarks" << "sync_dir";
  QRegExp re("&(?=[^&])");
  KAction *ac;
  QListBox *lb;
  for ( QStringList::Iterator it=allActions.begin(); it != allActions.end(); ++it ) {
    lb = l.contains( *it ) ? acSel->selectedListBox() : acSel->availableListBox();
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = fileSelector->actionCollection()->action( (*it).latin1() );
    else
      ac = fileSelector->dirOperator()->actionCollection()->action( (*it).latin1() );
    if ( ac )
      new ActionLBItem( lb, SmallIcon( ac->icon() ), ac->text().replace( re, "" ), *it );
  }
  
  // sync
  int s = fileSelector->autoSyncEvents;
  cbSyncActive->setChecked( s & KateFileSelector::DocumentChanged );
  cbSyncOpen->setChecked( s & KateFileSelector::DocumentOpened );
  cbSyncShow->setChecked( s & KateFileSelector::GotVisible );
  // histories
  sbPathHistLength->setValue( fileSelector->cmbPath->maxItems() );
  sbFilterHistLength->setValue( fileSelector->filter->maxCount() );
  // session
  cbSesLocation->setChecked( config->readBoolEntry( "restore location", true ) );
  cbSesFilter->setChecked( config->readBoolEntry( "restore last filter", true ) );
}
//END KFSConfigPage
