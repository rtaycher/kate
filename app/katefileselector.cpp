/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

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
#include <qtimer.h>
#include <qdir.h>

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

//BEGIN Toolbar
 // from kfiledialog.cpp - avoid qt warning in STDERR (~/.xsessionerrors)
static void silenceQToolBar(QtMsgType, const char *){}

// helper classes to be able to have a toolbar without move handle
KateFileSelectorToolBar::KateFileSelectorToolBar(QWidget *parent)
  : KToolBar( parent, "Kate FileSelector Toolbar", true )
{
	setMinimumWidth(10);
}

KateFileSelectorToolBar::~KateFileSelectorToolBar(){}

void KateFileSelectorToolBar::setMovingEnabled( bool)
{
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
//END

//BEGIN Constructor/destructor

KateFileSelector::KateFileSelector( KateMainWindow *mainWindow,
                                    KateViewManager *viewManager,
                                    QWidget * parent, const char * name )
    : QWidget(parent, name),
      mainwin(mainWindow),
      viewmanager(viewManager)
{
  mActionCollection = new KActionCollection( this );

  QVBoxLayout* lo = new QVBoxLayout(this);

  QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );

  KateFileSelectorToolBarParent *tbp=new KateFileSelectorToolBarParent(this);
  toolbar = new KateFileSelectorToolBar(tbp);
  tbp->setToolBar(toolbar);
  lo->addWidget(tbp);
  toolbar->setMovingEnabled(false);
  toolbar->setFlat(true);
  qInstallMsgHandler( oldHandler );

  cmbPath = new KURLComboBox( KURLComboBox::Directories, true, this, "path combo" );
  cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  KURLCompletion* cmpl = new KURLCompletion(KURLCompletion::DirCompletion);
  cmbPath->setCompletionObject( cmpl );
  cmbPath->setAutoDeleteCompletionObject( true );
  lo->addWidget(cmbPath);
  cmbPath->listBox()->installEventFilter( this );

  dir = new KDirOperator(KURL(), this, "operator");
  dir->setView(KFile::/* Simple */Detail);
  dir->view()->setSelectionMode(KFile::Multi);

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

  connect( filter, SIGNAL( activated(const QString&) ),
                   SLOT( slotFilterChange(const QString&) ) );
  connect( filter, SIGNAL( returnPressed(const QString&) ),
           filter, SLOT( addToHistory(const QString&) ) );

  // kaction for the dir sync method
  acSyncDir = new KAction( i18n("Current Document Folder"), "curfiledir", 0,
        this, SLOT( setActiveDocumentDir() ), mActionCollection, "sync_dir" );
  toolbar->setIconText( KToolBar::IconOnly );
  toolbar->setIconSize( 16 );
  toolbar->setEnableContextMenu( false );

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
  connect( bookmarkHandler, SIGNAL( openURL( const QString& )),
           this, SLOT( setDir( const QString& ) ) );

  waitingUrl = QString::null;

  // whatsthis help
  QWhatsThis::add( cmbPath,
       i18n("<p>Here you can enter a path for a folder to display."
            "<p>To go to a folder previously entered, press the arrow on "
            "the right and choose one. <p>The entry has folder "
            "completion. Right-click to choose how completion should behave.") );
  QWhatsThis::add( filter,
        i18n("<p>Here you can enter a name filter to limit which files are displayed."
             "<p>To clear the filter, toggle off the filter button to the left."
             "<p>To reapply the last filter used, toggle on the filter button." ) );
  QWhatsThis::add( btnFilter,
        i18n("<p>This button clears the name filter when toggled off, or "
             "reapplies the last filter used when toggled on.") );

}

KateFileSelector::~KateFileSelector()
{
}
//END Constroctor/Destrctor

//BEGIN Public Methods

void KateFileSelector::readConfig(KConfig *config, const QString & name)
{
  dir->setViewConfig( config, name + ":view" );
  dir->readConfig(config, name + ":dir");
  dir->setView( KFile::Default );
  dir->view()->setSelectionMode(KFile::Multi);
  config->setGroup( name );

  // set up the toolbar
  setupToolbar( config );

  cmbPath->setMaxItems( config->readNumEntry( "pathcombo history len", 9 ) );
  cmbPath->setURLs( config->readPathListEntry( "dir history" ) );

  // restore history
  QString loc( config->readPathEntry( "location" ) );
  if ( ! loc.isEmpty() ) {
	  setDir( loc );
  }

  // else is automatic, as cmpPath->setURL is called when a location is entered.

  filter->setHistoryItems( config->readListEntry("filter history"), true );
  lastFilter = config->readEntry( "last filter" );
  QString flt = config->readEntry("current filter");
  filter->lineEdit()->setText( flt );
  slotFilterChange( flt );

  autoSyncEvents = config->readNumEntry( "AutoSyncEvents", 0 );
}

void KateFileSelector::initialDirChangeHack()
{
  setDir( waitingDir );
}

void KateFileSelector::setupToolbar( KConfig *config )
{
  toolbar->clear();
  QStringList tbactions = config->readListEntry( "toolbar actions", ',' );
  if ( tbactions.isEmpty() ) {
    // reasonable collection for default toolbar
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
  QStringList l;
  for (int i = 0; i < cmbPath->count(); i++) {
    l.append( cmbPath->text( i ) );
  }
  config->writePathEntry( "dir history", l );
  config->writePathEntry( "location", cmbPath->currentText() );

  config->writeEntry( "filter history", filter->historyItems() );
  config->writeEntry( "current filter", filter->currentText() );
  config->writeEntry( "last filter", lastFilter );
  config->writeEntry( "AutoSyncEvents", autoSyncEvents );
}

void KateFileSelector::setView(KFile::FileView view)
{
  dir->setView(view);
  dir->view()->setSelectionMode(KFile::Multi);
}

//END Public Methods

//BEGIN Public Slots

void KateFileSelector::slotFilterChange( const QString & nf )
{
  QString f = nf.stripWhiteSpace();
  bool empty = f.isEmpty() || f == "*";
  QToolTip::remove( btnFilter );
  if ( empty ) {
    dir->clearFilter();
    filter->lineEdit()->setText( QString::null );
    QToolTip::add( btnFilter,
        QString( i18n("Apply last filter (\"%1\")") ).arg( lastFilter ) );
  }
  else {
    dir->setNameFilter( f );
    lastFilter = f;
    QToolTip::add( btnFilter, i18n("Clear filter") );
  }
  btnFilter->setOn( !empty );
  dir->updateDir();
  // this will be never true after the filter has been used;)
  btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}

bool kateFileSelectorIsReadable ( const KURL& url )
{
  if ( !url.isLocalFile() )
    return true; // what else can we say?

  QDir dir (url.path());
  return dir.exists ();
}

void KateFileSelector::setDir( KURL u )
{
  KURL newurl;

  if ( !u.isValid() )
    newurl.setPath( QDir::homeDirPath() );
  else
    newurl = u;

  QString pathstr = newurl.path(+1);
  newurl.setPath(pathstr);

  if ( !kateFileSelectorIsReadable ( newurl ) )
    newurl.cd(QString::fromLatin1(".."));

  if ( !kateFileSelectorIsReadable (newurl) )
     newurl.setPath( QDir::homeDirPath() );

  dir->setURL(newurl, true);
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

//FIXME crash on shutdown
void KateFileSelector::setActiveDocumentDir()
{
//   kdDebug(13001)<<"KateFileSelector::setActiveDocumentDir()"<<endl;
  KURL u = mainwin->activeDocumentUrl();
//   kdDebug(13001)<<"URL: "<<u.prettyURL()<<endl;
  if (!u.isEmpty())
    setDir( u.upURL() );
//   kdDebug(13001)<<"... setActiveDocumentDir() DONE!"<<endl;
}

void KateFileSelector::kateViewChanged()
{
  if ( autoSyncEvents & DocumentChanged )
  {
    kdDebug(13001)<<"KateFileSelector::do a sync ()"<<endl;
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

  // TODO: make sure the button is disabled if the directory is unreadable, eg
  //       the document URL has protocol http
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
//     kdDebug(13001)<<"syncing fs on show"<<endl;
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
      reasonable. Even using listbox->setVariableWidth() does not yield a
      satisfying result, something is wrong with the handling of the sizehint.
      And the popup is rather useless, if the paths are only partly visible.
  */
  QListBox *lb = cmbPath->listBox();
  if ( o == lb && e->type() == QEvent::Show ) {
    int add = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
    int w = QMIN( mainwin->width(), lb->contentsWidth() + add );
    lb->resize( w, lb->height() );
    // TODO - move the listbox to a suitable place if nessecary
    // TODO - decide if it is worth caching the size while untill the contents
    //        are changed.
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


  // Sync
  QGroupBox *gbSync = new QGroupBox( 1, Qt::Horizontal, i18n("Auto Synchronization"), this );
  cbSyncActive = new QCheckBox( i18n("When a docu&ment becomes active"), gbSync );
  cbSyncShow = new QCheckBox( i18n("When the file selector becomes visible"), gbSync );
  lo->addWidget( gbSync );
  connect( cbSyncActive, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );
  connect( cbSyncShow, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // make it look nice
  lo->addStretch( 1 );

  // be helpfull
  QString synwt( i18n(
        "<p>These options allow you to have the File Selector automatically "
        "change location to the folder of the active document on certain "
        "events."
        "<p>Auto synchronization is <em>lazy</em>, meaning it will not take "
        "effect until the file selector is visible."
        "<p>None of these are enabled by default, but you can always sync the "
        "location by pressing the sync button in the toolbar.") );
  QWhatsThis::add( gbSync, synwt );

  init();

}

void KFSConfigPage::apply()
{
  KConfig *config = kapp->config();
  config->setGroup( "fileselector" );
  // sync
  int s = 0;
  if ( cbSyncActive->isChecked() )
    s |= KateFileSelector::DocumentChanged;
  if ( cbSyncShow->isChecked() )
    s |= KateFileSelector::GotVisible;
  fileSelector->autoSyncEvents = s;
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

  // sync
  int s = fileSelector->autoSyncEvents;
  cbSyncActive->setChecked( s & KateFileSelector::DocumentChanged );
  cbSyncShow->setChecked( s & KateFileSelector::GotVisible );
}
//END KFSConfigPage
