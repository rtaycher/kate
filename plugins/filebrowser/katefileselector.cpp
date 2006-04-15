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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

//BEGIN Includes
#include "katefileselector.h"
#include "katefileselector.moc"

#include "kbookmarkhandler.h"

#include "kactionselector.h"

#include <qlayout.h>
#include <qtoolbutton.h>
#include <khbox.h>
#include <kvbox.h>
#include <qlabel.h>
#include <q3strlist.h>
#include <qtooltip.h>
#include <qapplication.h>
#include <QListWidget>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <q3groupbox.h>
#include <qcheckbox.h>
#include <qregexp.h>
#include <qtimer.h>
#include <qdir.h>
//Added by qt3to4:
#include <QPixmap>
#include <QFocusEvent>
#include <QEvent>
#include <QFrame>
#include <QListWidget>
#include <QShowEvent>
#include <QResizeEvent>
#include <QVBoxLayout>

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
#include <kmenu.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <kate/interfaces/mainwindow.h>
#include <ktexteditor/view.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kfileitem.h>
//END Includes


K_EXPORT_COMPONENT_FACTORY( katefilebrowserplugin, KGenericFactory<Kate::Private::Plugin::KateFileSelectorPlugin>( "katefilebrowserplugin" ) )

using namespace Kate::Private::Plugin;

KateFileSelectorPlugin::KateFileSelectorPlugin( QObject* parent, const char* name, const QStringList&):
  Kate::Plugin ( (Kate::Application*)parent, name ) {
}

void KateFileSelectorPlugin::addView(Kate::MainWindow *win) {
    QWidget *toolview=win->createToolView ("kate_private_plugin_katefileselectorplugin", MainWindow::Left, SmallIcon("fileopen"), i18n("Filesystem Browser"));
    m_views.append(new KateFileSelector(win,toolview));
}

void KateFileSelectorPlugin::removeView(Kate::MainWindow *win) {
  for(QLinkedList<KateFileSelector*>::iterator it=m_views.begin();it!=m_views.end();++it) {
    if ((*it)->mainWindow()==win) {
      QWidget *pw=(*it)->parentWidget();
      delete *it;
      delete pw;
      m_views.erase(it);
      break;
    }
  }
}

void KateFileSelectorPlugin::loadViewConfig(KConfig* config,Kate::MainWindow *win,const QString& group) {
  for(QLinkedList<KateFileSelector*>::iterator it=m_views.begin();it!=m_views.end();++it) {
    if ((*it)->mainWindow()==win) {
      (*it)->readConfig(config,group);
      break;
    }
  }
}


uint KateFileSelectorPlugin::configPages() const {return 1;}
Kate::PluginConfigPage *KateFileSelectorPlugin::configPage (uint number, QWidget *parent, const char *name) {
#ifdef __GNUC__
#warning "fixme"
#endif
  if (number!=0) return 0;
  return new KFSConfigPage(parent,name,*(m_views.begin()));
}

QString KateFileSelectorPlugin::configPageName (uint number) const {
  if (number!=0) return QString();
  kDebug()<<"Returning a config page name"<<endl;
  return i18n("File Selector");
}

QString KateFileSelectorPlugin::configPageFullName (uint number) const {
  if (number!=0) return QString();
  return i18n("File Selector Settings");
}

QPixmap KateFileSelectorPlugin::configPagePixmap (uint number, int size) const {
  if (number!=0) return QPixmap();
  return BarIcon("fileopen", size);
}

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

Kate::Private::Plugin::KateFileSelector::KateFileSelector( Kate::MainWindow *mainWindow,
                                    QWidget * parent, const char * name )
    : KVBox (parent),
      mainwin(mainWindow)
{
  setObjectName(name);
  mActionCollection = new KActionCollection( this );

  QtMsgHandler oldHandler = qInstallMsgHandler( silenceQToolBar );

  KateFileSelectorToolBarParent *tbp=new KateFileSelectorToolBarParent(this);
  toolbar = new KateFileSelectorToolBar(tbp);
  tbp->setToolBar(toolbar);
  toolbar->setMovable(false);
  qInstallMsgHandler( oldHandler );

  cmbPath = new KUrlComboBox( KUrlComboBox::Directories, true, this);
  cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  KUrlCompletion* cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
  cmbPath->setCompletionObject( cmpl );
  cmbPath->setAutoDeleteCompletionObject( true );

// FIXME
//  cmbPath->listBox()->installEventFilter( this );

  dir = new KDirOperator(KUrl(), this);
  dir->setView(KFile::/* Simple */Detail);
  dir->view()->setSelectionMode(KFile::Multi);
  setStretchFactor(dir, 2);
  dir->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  KActionCollection *coll = dir->actionCollection();
  // some shortcuts of diroperator that clashes with Kate
  coll->action( "delete" )->setShortcut( KShortcut( Qt::ALT + Qt::Key_Delete ) );
  coll->action( "reload" )->setShortcut( KShortcut( Qt::ALT + Qt::Key_F5 ) );
  coll->action( "back" )->setShortcut( KShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Left ) );
  coll->action( "forward" )->setShortcut( KShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Right ) );
  // some consistency - reset up for dir too
  coll->action( "up" )->setShortcut( KShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Up ) );
  coll->action( "home" )->setShortcut( KShortcut( Qt::CTRL + Qt::ALT + Qt::Key_Home ) );

  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu( KIcon("bookmark"), i18n("Bookmarks"),
        mActionCollection, "bookmarks" );
  acmBookmarks->setDelayed( false );
  bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->popupMenu() );
  KHBox* filterBox = new KHBox(this);

  btnFilter = new QToolButton( filterBox );
  btnFilter->setIcon( SmallIconSet("filter" ) );
  btnFilter->setCheckable( true );
  filter = new KHistoryCombo( true, filterBox);
  filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  filterBox->setStretchFactor(filter, 2);
  connect( btnFilter, SIGNAL( clicked() ), this, SLOT( btnFilterClick() ) );

  connect( filter, SIGNAL( activated(const QString&) ),
                   SLOT( slotFilterChange(const QString&) ) );
  connect( filter, SIGNAL( returnPressed(const QString&) ),
           filter, SLOT( addToHistory(const QString&) ) );

  // kaction for the dir sync method
  acSyncDir = new KAction( KIcon("curfiledir"), i18n("Current Document Folder"), mActionCollection, "sync_dir" );
  connect( acSyncDir, SIGNAL( triggered() ), this, SLOT( setActiveDocumentDir() ) );
  toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
  toolbar->setIconDimensions( 16 );
  toolbar->setEnableContextMenu( false );

  connect( cmbPath, SIGNAL( urlActivated( const KUrl&  )),
             this, SLOT( cmbPathActivated( const KUrl& ) ));
  connect( cmbPath, SIGNAL( returnPressed( const QString&  )),
             this, SLOT( cmbPathReturnPressed( const QString& ) ));
  connect(dir, SIGNAL(urlEntered(const KUrl&)),
             this, SLOT(dirUrlEntered(const KUrl&)) );

  connect(dir, SIGNAL(finishedLoading()),
             this, SLOT(dirFinishedLoading()) );

  // enable dir sync button if current doc has a valid URL
  connect ( mainwin, SIGNAL( viewChanged() ),
              this, SLOT( kateViewChanged() ) );

  // Connect the bookmark handler
  connect( bookmarkHandler, SIGNAL( openURL( const QString& )),
           this, SLOT( setDir( const QString& ) ) );

  waitingUrl.clear();

  // whatsthis help
  cmbPath->setWhatsThis(       i18n("<p>Here you can enter a path for a folder to display."
            "<p>To go to a folder previously entered, press the arrow on "
            "the right and choose one. <p>The entry has folder "
            "completion. Right-click to choose how completion should behave.") );
  filter->setWhatsThis(        i18n("<p>Here you can enter a name filter to limit which files are displayed."
             "<p>To clear the filter, toggle off the filter button to the left."
             "<p>To reapply the last filter used, toggle on the filter button." ) );
  btnFilter->setWhatsThis(        i18n("<p>This button clears the name filter when toggled off, or "
             "reapplies the last filter used when toggled on.") );

  connect(dir,SIGNAL(fileSelected(const KFileItem*)),this,SLOT(fileSelected(const KFileItem*)));
}

Kate::Private::Plugin::KateFileSelector::~KateFileSelector()
{
}
//END Constroctor/Destrctor

//BEGIN Public Methods

void Kate::Private::Plugin::KateFileSelector::readConfig(KConfig *config, const QString & name)
{
  kDebug()<<"===================================================================Kate::Private::Plugin::KateFileSelector::readConfig"<<endl;
#ifdef __GNUC__
#warning THIS WILL CRASH - setViewConfig keeps the pointer to the KConfigGroup! That API needs to be revised.
#endif
  KConfigGroup confGroup(config, name + ":view");
  dir->setViewConfig(&confGroup );
  KConfigGroup confDirGroup(config, name + ":dir");
  dir->readConfig(&confDirGroup);
  dir->setView( KFile::Default );
  dir->view()->setSelectionMode(KFile::Multi);
  config->setGroup( name );

  // set up the toolbar
  setupToolbar( config );

  cmbPath->setMaxItems( config->readEntry( "pathcombo history len", 9 ) );
  cmbPath->setURLs( config->readPathListEntry( "dir history" ) );
  // if we restore history
  if ( config->readEntry( "restore location", QVariant(true )).toBool() || qApp->isSessionRestored() ) {
    QString loc( config->readPathEntry( "location" ) );
    if ( ! loc.isEmpty() ) {
//       waitingDir = loc;
//       QTimer::singleShot(0, this, SLOT(initialDirChangeHack()));
      setDir( loc );
    }
  }

  // else is automatic, as cmpPath->setURL is called when a location is entered.

  filter->setMaxCount( config->readEntry( "filter history len", 9 ) );
  filter->setHistoryItems( config->readEntry("filter history",QStringList()), true );
  lastFilter = config->readEntry( "last filter" );
  QString flt("");
  if ( config->readEntry( "restore last filter", true ) || qApp->isSessionRestored() )
    flt = config->readEntry("current filter");
  filter->lineEdit()->setText( flt );
  slotFilterChange( flt );

  autoSyncEvents = config->readEntry( "AutoSyncEvents", 0 );
}

void Kate::Private::Plugin::KateFileSelector::initialDirChangeHack()
{
  setDir( waitingDir );
}

void Kate::Private::Plugin::KateFileSelector::setupToolbar( KConfig *config )
{
  toolbar->clear();
  QStringList tbactions = config->readEntry( "toolbar actions", QStringList(),',' );
  if ( tbactions.isEmpty() ) {
    // reasonable collection for default toolbar
    tbactions << "up" << "back" << "forward" << "home" <<
                "short view" << "detailed view" <<
                "bookmarks" << "sync_dir";
  }
  KAction *ac;
  for ( QStringList::Iterator it=tbactions.begin(); it != tbactions.end(); ++it ) {
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = mActionCollection->action( (*it).toLatin1().constData() );
    else
      ac = dir->actionCollection()->action( (*it).toLatin1().constData() );
    if ( ac )
      ac->plug( toolbar );
  }
}

void Kate::Private::Plugin::KateFileSelector::writeConfig(KConfig *config, const QString & name)
{
  KConfigGroup confGroup(config,name + ":dir");
  dir->writeConfig(&confGroup);

  config->setGroup( name );
  config->writeEntry( "pathcombo history len", cmbPath->maxItems() );
  QStringList l;
  for (int i = 0; i < cmbPath->count(); i++) {
    l.append( cmbPath->itemText( i ) );
  }
  config->writePathEntry( "dir history", l );
  config->writePathEntry( "location", cmbPath->currentText() );

  config->writeEntry( "filter history len", filter->maxCount() );
  config->writeEntry( "filter history", filter->historyItems() );
  config->writeEntry( "current filter", filter->currentText() );
  config->writeEntry( "last filter", lastFilter );
  config->writeEntry( "AutoSyncEvents", autoSyncEvents );
}

void Kate::Private::Plugin::KateFileSelector::setView(KFile::FileView view)
{
  dir->setView(view);
  dir->view()->setSelectionMode(KFile::Multi);
}

//END Public Methods

//BEGIN Public Slots

void Kate::Private::Plugin::KateFileSelector::slotFilterChange( const QString & nf )
{
  QString f = nf.trimmed();
  bool empty = f.isEmpty() || f == "*";
  if ( empty ) {
    dir->clearFilter();
    filter->lineEdit()->setText( QString() );
    btnFilter->setToolTip(
        i18n("Apply last filter (\"%1\")", lastFilter )) ;
  }
  else {
    dir->setNameFilter( f );
    lastFilter = f;
    btnFilter->setToolTip( i18n("Clear filter") );
  }
  btnFilter->setChecked( !empty );
  dir->updateDir();
  // this will be never true after the filter has been used;)
  btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );

}

bool kateFileSelectorIsReadable ( const KUrl& url )
{
  if ( !url.isLocalFile() )
    return true; // what else can we say?

  QDir dir (url.path());
  return dir.exists ();
}

void Kate::Private::Plugin::KateFileSelector::setDir( KUrl u )
{
  KUrl newurl;

  if ( !u.isValid() )
    newurl.setPath( QDir::homePath() );
  else
    newurl = u;

  QString pathstr = newurl.path(+1);
  newurl.setPath(pathstr);

  if ( !kateFileSelectorIsReadable ( newurl ) )
    newurl.cd(QString::fromLatin1(".."));

  if ( !kateFileSelectorIsReadable (newurl) )
     newurl.setPath( QDir::homePath() );

  dir->setURL(newurl, true);
}

//END Public Slots

//BEGIN Private Slots

void Kate::Private::Plugin::KateFileSelector::fileSelected(const KFileItem * /*file*/)
{
  const KFileItemList *list=dir->selectedItems();

  KFileItem *tmp;
  KFileItemList::const_iterator it = list->begin();
  const KFileItemList::const_iterator end = list->end();
  for ( ; it != end; ++it )
  {
    tmp = (*it);
	mainwin->openURL(tmp->url());
    dir->view()->setSelected(tmp,false);
  }
}




void Kate::Private::Plugin::KateFileSelector::cmbPathActivated( const KUrl& u )
{
   cmbPathReturnPressed( u.url() );
}

void Kate::Private::Plugin::KateFileSelector::cmbPathReturnPressed( const QString& u )
{
  KUrl typedURL( u );
  if ( typedURL.hasPass() )
    typedURL.setPass( QString() );

  QStringList urls = cmbPath->urls();
  urls.removeAll( typedURL.url() );
  urls.prepend( typedURL.url() );
  cmbPath->setURLs( urls, KUrlComboBox::RemoveBottom );
  dir->setFocus();
  dir->setURL( KUrl(u), true );
}

void Kate::Private::Plugin::KateFileSelector::dirUrlEntered( const KUrl& u )
{
  cmbPath->setURL( u );
}

void Kate::Private::Plugin::KateFileSelector::dirFinishedLoading()
{
}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void Kate::Private::Plugin::KateFileSelector::btnFilterClick()
{
  if ( !btnFilter->isChecked() ) {
    slotFilterChange( QString() );
  }
  else {
    filter->lineEdit()->setText( lastFilter );
    slotFilterChange( lastFilter );
  }
}

//FIXME crash on shutdown
void Kate::Private::Plugin::KateFileSelector::setActiveDocumentDir()
{
//   kDebug(13001)<<"KateFileSelector::setActiveDocumentDir()"<<endl;
  KUrl u = activeDocumentUrl();
//   kDebug(13001)<<"URL: "<<u.prettyURL()<<endl;
  if (!u.isEmpty())
    setDir( u.upURL() );
//   kDebug(13001)<<"... setActiveDocumentDir() DONE!"<<endl;
}

void Kate::Private::Plugin::KateFileSelector::kateViewChanged()
{
  if ( autoSyncEvents & DocumentChanged )
  {
//     kDebug(13001)<<"KateFileSelector::do a sync ()"<<endl;
    // if visible, sync
    if ( isVisible() ) {
      setActiveDocumentDir();
      waitingUrl.clear();
    }
    // else set waiting url
    else {
      KUrl u = activeDocumentUrl();
      if (!u.isEmpty())
        waitingUrl = u.directory();
    }
  }

  // TODO: make sure the button is disabled if the directory is unreadable, eg
  //       the document URL has protocol http
  acSyncDir->setEnabled( ! activeDocumentUrl().directory().isEmpty() );
}

//END Private Slots

//BEGIN Protected

KUrl Kate::Private::Plugin::KateFileSelector::activeDocumentUrl() {
  KTextEditor::View *v = mainwin->activeView();
  if ( v )
    return v->document()->url();
  return KUrl();
}

void Kate::Private::Plugin::KateFileSelector::focusInEvent( QFocusEvent * )
{
   dir->setFocus();
}

void Kate::Private::Plugin::KateFileSelector::showEvent( QShowEvent * )
{
    // sync if we should
    if ( autoSyncEvents & GotVisible ) {
//     kDebug(13001)<<"syncing fs on show"<<endl;
      setActiveDocumentDir();
      waitingUrl.clear();
    }
    // else, if we have a waiting URL set it
    else if ( ! waitingUrl.isEmpty() ) {
      setDir( waitingUrl );
      waitingUrl.clear();
   }
}

bool Kate::Private::Plugin::KateFileSelector::eventFilter( QObject* o, QEvent *e )
{
  /*
      This is rather unfortunate, but:
      QComboBox does not support setting the size of the listbox to something
      reasonable. Even using listbox->setVariableWidth() does not yield a
      satisfying result, something is wrong with the handling of the sizehint.
      And the popup is rather useless, if the paths are only partly visible.
  */
/* FIXME Q3ListBox *lb = cmbPath->listBox();
  if ( o == lb && e->type() == QEvent::Show ) {
    int add = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
    int w = qMin( mainwin->width(), lb->contentsWidth() + add );
    lb->resize( w, lb->height() );
    // TODO - move the listbox to a suitable place if nessecary
    // TODO - decide if it is worth caching the size while untill the contents
    //        are changed.
  }
  */// TODO - same thing for the completion popup?
  return QWidget::eventFilter( o, e );
}

//END Protected

//BEGIN ACtionLBItem
/*
   QListboxItem that can store and return a string,
   used for the toolbar action selector.
*/
class ActionLBItem : public QListWidgetItem {
  public:
  ActionLBItem( QListWidget *lb=0,
                const QIcon &pm = QIcon(),
                const QString &text=QString::null,
                const QString &str=QString::null ) :
    QListWidgetItem(pm, text,lb,0 ),
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
KFSConfigPage::KFSConfigPage( QWidget *parent, const char *, KateFileSelector *kfs )
  : Kate::PluginConfigPage( parent ),
    fileSelector( kfs ),
    m_changed( false )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  lo->setSpacing( spacing );

  // Toolbar - a lot for a little...
  Q3GroupBox *gbToolbar = new Q3GroupBox( 1, Qt::Vertical, i18n("Toolbar"), this );
  acSel = new KActionSelector( gbToolbar );
  acSel->setAvailableLabel( i18n("A&vailable actions:") );
  acSel->setSelectedLabel( i18n("S&elected actions:") );
  lo->addWidget( gbToolbar );
  connect( acSel, SIGNAL( added( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );
  connect( acSel, SIGNAL( removed( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );
  connect( acSel, SIGNAL( movedUp( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );
  connect( acSel, SIGNAL( movedDown( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );

  // Sync
  Q3GroupBox *gbSync = new Q3GroupBox( 1, Qt::Horizontal, i18n("Auto Synchronization"), this );
  cbSyncActive = new QCheckBox( i18n("When a docu&ment becomes active"), gbSync );
  cbSyncShow = new QCheckBox( i18n("When the file selector becomes visible"), gbSync );
  lo->addWidget( gbSync );
  connect( cbSyncActive, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );
  connect( cbSyncShow, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );

  // Histories
  KHBox *hbPathHist = new KHBox ( this );
  QLabel *lbPathHist = new QLabel( i18n("Remember &locations:"), hbPathHist );
  sbPathHistLength = new QSpinBox( hbPathHist );
  lbPathHist->setBuddy( sbPathHistLength );
  lo->addWidget( hbPathHist );
  connect( sbPathHistLength, SIGNAL( valueChanged ( int ) ), this, SLOT( slotMyChanged() ) );

  KHBox *hbFilterHist = new KHBox ( this );
  QLabel *lbFilterHist = new QLabel( i18n("Remember &filters:"), hbFilterHist );
  sbFilterHistLength = new QSpinBox( hbFilterHist );
  lbFilterHist->setBuddy( sbFilterHistLength );
  lo->addWidget( hbFilterHist );
  connect( sbFilterHistLength, SIGNAL( valueChanged ( int ) ), this, SLOT( slotMyChanged() ) );

  // Session
  Q3GroupBox *gbSession = new Q3GroupBox( 1, Qt::Horizontal, i18n("Session"), this );
  cbSesLocation = new QCheckBox( i18n("Restore loca&tion"), gbSession );
  cbSesFilter = new QCheckBox( i18n("Restore last f&ilter"), gbSession );
  lo->addWidget( gbSession );
  connect( cbSesLocation, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );
  connect( cbSesFilter, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );

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
        "<p>Decides how many locations to keep in the history of the location "
        "combo box.") );
  lbPathHist->setWhatsThis(lhwt );
  sbPathHistLength->setWhatsThis(lhwt );
  QString fhwt( i18n(
        "<p>Decides how many filters to keep in the history of the filter "
        "combo box.") );
  lbFilterHist->setWhatsThis(fhwt );
  sbFilterHistLength->setWhatsThis(fhwt );
  QString synwt( i18n(
        "<p>These options allow you to have the File Selector automatically "
        "change location to the folder of the active document on certain "
        "events."
        "<p>Auto synchronization is <em>lazy</em>, meaning it will not take "
        "effect until the file selector is visible."
        "<p>None of these are enabled by default, but you can always sync the "
        "location by pressing the sync button in the toolbar.") );
  gbSync->setWhatsThis(synwt );
  cbSesLocation->setWhatsThis(i18n(
        "<p>If this option is enabled (default), the location will be restored "
        "when you start Kate.<p><strong>Note</strong> that if the session is "
        "handled by the KDE session manager, the location is always restored.") );
  cbSesFilter->setWhatsThis(i18n(
        "<p>If this option is enabled (default), the current filter will be "
        "restored when you start Kate.<p><strong>Note</strong> that if the "
        "session is handled by the KDE session manager, the filter is always "
        "restored."
        "<p><strong>Note</strong> that some of the autosync settings may "
        "override the restored location if on.") );

  init();

}

void KFSConfigPage::apply()
{
  if ( ! m_changed )
    return;

  m_changed = false;

  KConfig *config = KGlobal::config();
  config->setGroup( "fileselector" );
  // toolbar
  QStringList l;
  ActionLBItem *aItem;
  QList<QListWidgetItem *> list = acSel->selectedListWidget()->findItems(QString("*"), Qt::MatchRegExp);
  foreach(QListWidgetItem *item, list)
  {
    aItem = (ActionLBItem*)item;
      l << aItem->idstring();
  }
  config->writeEntry( "toolbar actions", l );
  fileSelector->setupToolbar( config );
  // sync
  int s = 0;
  if ( cbSyncActive->isChecked() )
    s |= KateFileSelector::DocumentChanged;
  if ( cbSyncShow->isChecked() )
    s |= KateFileSelector::GotVisible;
  fileSelector->autoSyncEvents = s;

  // histories
  fileSelector->cmbPath->setMaxItems( sbPathHistLength->value() );
  fileSelector->filter->setMaxCount( sbFilterHistLength->value() );
  // session - theese are read/written directly to the app config,
  //           as they are not needed during operation.
  config->writeEntry( "restore location", cbSesLocation->isChecked() );
  config->writeEntry( "restore last filter", cbSesFilter->isChecked() );
}

void KFSConfigPage::reset()
{
  // hmm, what is this supposed to do, actually??
  init();
  m_changed = false;
}
void KFSConfigPage::init()
{
  KConfig *config = KGlobal::config();
  config->setGroup( "fileselector" );
  // toolbar
  QStringList l = config->readEntry( "toolbar actions", QStringList(),',' );
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
  QListWidget *lb;
  for ( QStringList::Iterator it=allActions.begin(); it != allActions.end(); ++it ) {
    lb = l.contains( *it ) ? acSel->selectedListWidget() : acSel->availableListWidget();
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = fileSelector->actionCollection()->action( (*it).toLatin1().constData() );
    else
      ac = fileSelector->dirOperator()->actionCollection()->action( (*it).toLatin1().constData() );
    if ( ac )
      new ActionLBItem( lb, ac->icon(), ac->text().replace( re, "" ), *it );
  }

  // sync
  int s = fileSelector->autoSyncEvents;
  cbSyncActive->setChecked( s & KateFileSelector::DocumentChanged );
  cbSyncShow->setChecked( s & KateFileSelector::GotVisible );
  // histories
  sbPathHistLength->setValue( fileSelector->cmbPath->maxItems() );
  sbFilterHistLength->setValue( fileSelector->filter->maxCount() );
  // session
  cbSesLocation->setChecked( config->readEntry( "restore location", QVariant(true )).toBool() );
  cbSesFilter->setChecked( config->readEntry( "restore last filter", QVariant(true )).toBool() );
}

void KFSConfigPage::slotMyChanged()
{
  m_changed = true;
  emit changed();
}
//END KFSConfigPage
// kate: space-indent on; indent-width 2; replace-tabs on;
