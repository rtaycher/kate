/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

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

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QRegExp>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <kate/mainwindow.h>
#include <ktexteditor/view.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kactionselector.h>
#include <kbookmarkhandler.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kdiroperator.h>
#include <kfileitem.h>
#include <khbox.h>
#include <khistorycombobox.h>
#include <kdeversion.h>
#include <kpluginfactory.h>
#include <ktoolbar.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>
//END Includes

K_PLUGIN_FACTORY(KateFileSelectorFactory, registerPlugin<KateFileSelectorPlugin>();)
K_EXPORT_PLUGIN(KateFileSelectorFactory(KAboutData("katefilebrowserplugin","katefilebrowserplugin",ki18n("Filesystem Browser"), "0.1", ki18n("Browse through the filesystem"), KAboutData::License_LGPL_V2)) )

KateFileSelectorPlugin::KateFileSelectorPlugin( QObject* parent, const QList<QVariant>&):
    Kate::Plugin ( (Kate::Application*)parent )
{}

Kate::PluginView *KateFileSelectorPlugin::createView (Kate::MainWindow *mainWindow)
{
  KateFileSelectorPluginView* kateFileSelectorPluginView = new KateFileSelectorPluginView (mainWindow);
  m_fileSelector = kateFileSelectorPluginView->kateFileSelector();
  return kateFileSelectorPluginView;
}

KateFileSelectorPluginView::KateFileSelectorPluginView (Kate::MainWindow *mainWindow)
    : Kate::PluginView (mainWindow)
{
  // init console
  QWidget *toolview = mainWindow->createToolView ("kate_private_plugin_katefileselectorplugin", Kate::MainWindow::Left, SmallIcon("document-open"), i18n("Filesystem Browser"));
  m_fileSelector = new KateFileSelector(mainWindow, toolview);
}

KateFileSelectorPluginView::~KateFileSelectorPluginView ()
{
  // cleanup, kill toolview + console
  delete m_fileSelector->parentWidget();
}

void KateFileSelectorPluginView::readSessionConfig(KConfigBase* config, const QString& group)
{
  m_fileSelector->readSessionConfig(config, group);
}

void KateFileSelectorPluginView::writeSessionConfig(KConfigBase* config, const QString& group)
{
  m_fileSelector->writeSessionConfig(config, group);
}

uint KateFileSelectorPlugin::configPages() const
{
  return 1;
}

Kate::PluginConfigPage *KateFileSelectorPlugin::configPage (uint number, QWidget *parent, const char *name)
{
  if (number != 0)
    return 0;
  return new KFSConfigPage(parent, name, m_fileSelector);
}

QString KateFileSelectorPlugin::configPageName (uint number) const
{
  if (number != 0) return QString();
  kDebug() << "Returning a config page name";
  return i18n("File Selector");
}

QString KateFileSelectorPlugin::configPageFullName (uint number) const
{
  if (number != 0) return QString();
  return i18n("File Selector Settings");
}

KIcon KateFileSelectorPlugin::configPageIcon (uint number) const
{
  if (number != 0) return KIcon();
  return KIcon("document-open");
}

//BEGIN Constructor/destructor
KateFileSelector::KateFileSelector( Kate::MainWindow *mainWindow,
                                      QWidget * parent, const char * name )
    : KVBox (parent),
    mainwin(mainWindow),
    autoSyncEvents(0)
{
  setObjectName(name);
  mActionCollection = new KActionCollection( this );

  toolbar = new KToolBar(this);
  toolbar->setMovable(false);

  cmbPath = new KUrlComboBox( KUrlComboBox::Directories, true, this);
  cmbPath->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  KUrlCompletion* cmpl = new KUrlCompletion(KUrlCompletion::DirCompletion);
  cmbPath->setCompletionObject( cmpl );
  cmbPath->setAutoDeleteCompletionObject( true );

// FIXME
//  cmbPath->listBox()->installEventFilter( this );

  m_dirOperator = new KDirOperator(KUrl(), this);
  m_dirOperator->installEventFilter( this );
  m_dirOperator->setView(KFile::/* Simple */Detail);
  m_dirOperator->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
  connect ( m_dirOperator, SIGNAL( viewChanged(QAbstractItemView *) ),
           this, SLOT( selectorViewChanged(QAbstractItemView *) ) );
  setStretchFactor(m_dirOperator, 2);
  m_dirOperator->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  // bookmarks action!
  KActionMenu *acmBookmarks = new KActionMenu( KIcon("bookmarks"), i18n("Bookmarks"), this );
  mActionCollection->addAction( "bookmarks", acmBookmarks );
  acmBookmarks->setDelayed( false );
  bookmarkHandler = new KBookmarkHandler( this, acmBookmarks->menu() );
  KHBox* filterBox = new KHBox(this);

  btnFilter = new QToolButton( filterBox );
  btnFilter->setIcon( KIcon("view-filter" ) );
  btnFilter->setCheckable( true );
  filter = new KHistoryComboBox( true, filterBox);
  filter->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
  filterBox->setStretchFactor(filter, 2);
  connect( btnFilter, SIGNAL( clicked() ), this, SLOT( filterButtonClicked() ) );

  connect( filter, SIGNAL( activated(const QString&) ),
           SLOT( slotFilterChange(const QString&) ) );
  connect( filter, SIGNAL( returnPressed(const QString&) ),
           filter, SLOT( addToHistory(const QString&) ) );

  // kaction for the m_dirOperator sync method
  acSyncDir = mActionCollection->addAction( "sync_dir" );
  acSyncDir->setIcon( KIcon("curfiledir") );
  acSyncDir->setText( i18n("Current Document Folder") );
  connect( acSyncDir, SIGNAL( triggered() ), this, SLOT( setActiveDocumentDir() ) );
  toolbar->setToolButtonStyle( Qt::ToolButtonIconOnly );
  toolbar->setIconDimensions( 16 );
  toolbar->setContextMenuPolicy( Qt::NoContextMenu );

  connect( cmbPath, SIGNAL( urlActivated( const KUrl&  )),
           this, SLOT( cmbPathActivated( const KUrl& ) ));
  connect( cmbPath, SIGNAL( returnPressed( const QString&  )),
           this, SLOT( cmbPathReturnPressed( const QString& ) ));
  connect(m_dirOperator, SIGNAL(urlEntered(const KUrl&)),
          this, SLOT(dirUrlEntered(const KUrl&)) );

  // enable m_dirOperator sync button if current doc has a valid URL
  connect ( mainwin, SIGNAL( viewChanged() ),
            this, SLOT( kateViewChanged() ) );

  // Connect the bookmark handler
  connect( bookmarkHandler, SIGNAL( openUrl( const QString& )),
           this, SLOT( setDir( const QString& ) ) );

  waitingUrl.clear();

  // whatsthis help
  cmbPath->setWhatsThis(       i18n("<p>Here you can enter a path for a folder to display.</p>"
                                    "<p>To go to a folder previously entered, press the arrow on "
                                    "the right and choose one.</p><p>The entry has folder "
                                    "completion. Right-click to choose how completion should behave.</p>") );
  filter->setWhatsThis(        i18n("<p>Here you can enter a name filter to limit which files are displayed.</p>"
                                    "<p>To clear the filter, toggle off the filter button to the left.</p>"
                                    "<p>To reapply the last filter used, toggle on the filter button.</p>" ) );
  btnFilter->setWhatsThis(        i18n("<p>This button clears the name filter when toggled off, or "
                                       "reapplies the last filter used when toggled on.</p>") );

  connect(m_dirOperator, SIGNAL(fileSelected(const KFileItem&)), this, SLOT(fileSelected(const KFileItem&)));

  readConfig();

  mActionCollection->addAssociatedWidget(this);
  foreach (QAction* action, mActionCollection->actions())
#if QT_VERSION < KDE_MAKE_VERSION(4,4,0)
    action->setShortcutContext(Qt::WidgetShortcut); // remove after Qt4.4 becomes mandatory
#else
    action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
#endif
}

KateFileSelector::~KateFileSelector()
{}
//END Constroctor/Destrctor

//BEGIN Public Methods

void KateFileSelector::readConfig()
{

//   m_dirOperator->setView( KFile::Default );
//   m_dirOperator->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);

  // set up the toolbar
  KConfigGroup fileselectorConfigGroup(KGlobal::config(), "fileselector");
  setupToolbar( fileselectorConfigGroup.readEntry( "toolbar actions", QStringList() ) );

  cmbPath->setMaxItems( fileselectorConfigGroup.readEntry( "pathcombo history len", 9 ) );
  // if we restore history

  filter->setMaxCount( fileselectorConfigGroup.readEntry( "filter history len", 9 ) );

  autoSyncEvents = fileselectorConfigGroup.readEntry( "AutoSyncEvents", 0 );
}

void ::KateFileSelector::readSessionConfig(KConfigBase *config, const QString & name)
{

  KConfigGroup cgView(config, name + ":view");
  m_dirOperator->setViewConfig(cgView );

  KConfigGroup cgDir(config, name + ":dir");
  m_dirOperator->readConfig(cgDir);
  m_dirOperator->setView(KFile::Default);

  KConfigGroup cg (config, name );
  cmbPath->setUrls( cg.readPathEntry( "dir history", QStringList() ) );

  KConfigGroup globalConfig( KGlobal::config(), "fileselector" );

  if ( globalConfig.readEntry( "restore location", true) || qApp->isSessionRestored() )
  {
    QString loc( cg.readPathEntry( "location", QString() ) );
    if ( ! loc.isEmpty() )
      setDir( loc );
  }
  
  m_dirOperator->setShowHiddenFiles( cg.readEntry( "show hidden files", false ) );

  filter->setHistoryItems( cg.readEntry("filter history", QStringList()), true );
  lastFilter = cg.readEntry( "last filter" );
  QString flt("");
  if ( globalConfig.readEntry( "restore last filter", true ) || qApp->isSessionRestored() )
    flt = cg.readEntry("current filter");
  filter->lineEdit()->setText( flt );
  slotFilterChange( flt );
}

void ::KateFileSelector::initialDirChangeHack()
{
  setDir( waitingDir );
}

void ::KateFileSelector::setupToolbar( QStringList actions )
{
  toolbar->clear();
  if ( actions.isEmpty() )
  {
    // reasonable collection for default toolbar
    actions << "up" << "back" << "forward" << "home" <<
    "short view" << "detailed view" << "tree view" <<
    "bookmarks" << "sync_dir";
  }
  QAction *ac;
  for ( QStringList::ConstIterator it = actions.constBegin(); it != actions.constEnd(); ++it )
  {
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = mActionCollection->action( (*it).toLatin1().constData() );
    else
      ac = m_dirOperator->actionCollection()->action( (*it).toLatin1().constData() );
    if ( ac )
      toolbar->addAction( ac );
  }
}

void KateFileSelector::writeConfig()
{
  KConfigGroup cg = KConfigGroup( KGlobal::config(), "fileselector" );

  cg.writeEntry( "pathcombo history len", cmbPath->maxItems() );
  cg.writeEntry( "filter history len", filter->maxCount() );
  cg.writeEntry( "filter history", filter->historyItems() );
  cg.writeEntry( "AutoSyncEvents", autoSyncEvents );
}

void KateFileSelector::writeSessionConfig(KConfigBase *config, const QString & name)
{
  KConfigGroup cgDir(config, name + ":dir");
  m_dirOperator->writeConfig(cgDir);

  KConfigGroup cg = KConfigGroup( config, name );
  QStringList l;
  for (int i = 0; i < cmbPath->count(); i++)
  {
    l.append( cmbPath->itemText( i ) );
  }
  cg.writePathEntry( "dir history", l );
  cg.writePathEntry( "location", cmbPath->currentText() );
  cg.writeEntry( "current filter", filter->currentText() );
  cg.writeEntry( "last filter", lastFilter );
  cg.writeEntry( "show hidden files", m_dirOperator->showHiddenFiles() );
}

void ::KateFileSelector::setView(KFile::FileView view)
{
  m_dirOperator->setView(view);
  m_dirOperator->view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//END Public Methods

//BEGIN Public Slots

void ::KateFileSelector::slotFilterChange( const QString & nf )
{
  QString f = nf.trimmed();
  const bool empty = f.isEmpty() || f == "*";

  if ( empty )
  {
    m_dirOperator->clearFilter();
    filter->lineEdit()->setText( QString() );
    btnFilter->setToolTip( i18n("Apply last filter (\"%1\")", lastFilter) ) ;
  }
  else
  {
    m_dirOperator->setNameFilter( f );
    lastFilter = f;
    btnFilter->setToolTip( i18n("Clear filter") );
  }

  btnFilter->setChecked( !empty );
  m_dirOperator->updateDir();

  // this will be never true after the filter has been used;)
  btnFilter->setEnabled( !( empty && lastFilter.isEmpty() ) );
}

bool kateFileSelectorIsReadable ( const KUrl& url )
{
  if ( !url.isLocalFile() )
    return true; // what else can we say?

  QDir dir(url.toLocalFile());
  return dir.exists ();
}

void ::KateFileSelector::setDir( KUrl u )
{
  KUrl newurl;

  if ( !u.isValid() )
    newurl.setPath( QDir::homePath() );
  else
    newurl = u;

  QString pathstr = newurl.path( KUrl::AddTrailingSlash );
  newurl.setPath(pathstr);

  if ( !kateFileSelectorIsReadable ( newurl ) )
    newurl.cd(QString::fromLatin1(".."));

  if ( !kateFileSelectorIsReadable (newurl) )
    newurl.setPath( QDir::homePath() );

  m_dirOperator->setUrl(newurl, true);
}

//END Public Slots

//BEGIN Private Slots

void ::KateFileSelector::fileSelected(const KFileItem & /*file*/)
{
  openSelectedFiles();
}

void ::KateFileSelector::openSelectedFiles()
{
  const KFileItemList list = m_dirOperator->selectedItems();

  foreach (const KFileItem& item, list)
  {
    mainwin->openUrl(item.url());
  }

  m_dirOperator->view()->selectionModel()->clear();
}




void ::KateFileSelector::cmbPathActivated( const KUrl& u )
{
  cmbPathReturnPressed( u.url() );
}

void ::KateFileSelector::cmbPathReturnPressed( const QString& u )
{
  // construct so that relative urls are ok
  KUrl typedURL( m_dirOperator->url(), u );

  //m_dirOperator->setFocus(); // is it really useful to set focus here?
  m_dirOperator->setUrl( typedURL, true );
  qobject_cast<KUrlCompletion *>( cmbPath->completionObject() )->setDir( typedURL.pathOrUrl() );

  // strip password (noop if there's none)
  typedURL.setPass( QString() );
  QStringList urls = cmbPath->urls();
  urls.removeAll( typedURL.url() );
  urls.prepend( typedURL.url() );
  cmbPath->setUrls( urls, KUrlComboBox::RemoveBottom );
}

void ::KateFileSelector::dirUrlEntered( const KUrl& u )
{
  cmbPath->setUrl( u );
  qobject_cast<KUrlCompletion *>( cmbPath->completionObject() )->setDir( u.pathOrUrl() );
}


/*
   When the button in the filter box toggles:
   If off:
   If the name filer is anything but "" or "*", reset it.
   If on:
   Set last filter.
*/
void ::KateFileSelector::filterButtonClicked()
{
  if ( !btnFilter->isChecked() )
  {
    slotFilterChange( QString() );
  }
  else
  {
    filter->lineEdit()->setText( lastFilter );
    slotFilterChange( lastFilter );
  }
}

//FIXME crash on shutdown
void ::KateFileSelector::setActiveDocumentDir()
{
//   kDebug(13001)<<"KateFileSelector::setActiveDocumentDir()";
  KUrl u = activeDocumentUrl();
//   kDebug(13001)<<"URL: "<<u.prettyUrl();
  if (!u.isEmpty())
    setDir( u.upUrl() );
//   kDebug(13001)<<"... setActiveDocumentDir() DONE!";
}

void ::KateFileSelector::kateViewChanged()
{
  if ( autoSyncEvents & DocumentChanged )
  {
//     kDebug(13001)<<"KateFileSelector::do a sync ()";
    // if visible, sync
    if ( isVisible() )
    {
      setActiveDocumentDir();
      waitingUrl.clear();
    }
    // else set waiting url
    else
    {
      KUrl u = activeDocumentUrl();
      if (!u.isEmpty())
        waitingUrl = u.directory();
    }
  }

  // TODO: make sure the button is disabled if the directory is unreadable, eg
  //       the document URL has protocol http
  acSyncDir->setEnabled( ! activeDocumentUrl().directory().isEmpty() );
}

void ::KateFileSelector::selectorViewChanged( QAbstractItemView * newView )
{
  newView->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

//END Private Slots

//BEGIN Protected

KUrl KateFileSelector::activeDocumentUrl()
{
  KTextEditor::View *v = mainwin->activeView();
  if ( v )
    return v->document()->url();
  return KUrl();
}

void KateFileSelector::focusInEvent( QFocusEvent * )
{
  m_dirOperator->setFocus();
}

void KateFileSelector::showEvent( QShowEvent * )
{
  // sync if we should
  if ( autoSyncEvents & GotVisible )
  {
//     kDebug(13001)<<"syncing fs on show";
    setActiveDocumentDir();
    waitingUrl.clear();
  }
  // else, if we have a waiting URL set it
  else if ( ! waitingUrl.isEmpty() )
  {
    setDir( waitingUrl );
    waitingUrl.clear();
  }
}

bool KateFileSelector::eventFilter( QObject* o, QEvent *e )
{
  if ( e->type() == QEvent::KeyPress && (
         ((QKeyEvent*)e)->key() == Qt::Key_Return ||
         ((QKeyEvent*)e)->key() == Qt::Key_Enter ) )
  {
    openSelectedFiles();
  }
  /*
      This is rather unfortunate, but:
      QComboBox does not support setting the size of the listbox to something
      reasonable. Even using listbox->setVariableWidth() does not yield a
      satisfying result, something is wrong with the handling of the sizehint.
      And the popup is rather useless, if the paths are only partly visible.
  */
  /* FIXME QListWidget *lb = cmbPath->listBox();
    if ( o == lb && e->type() == QEvent::Show ) {
      int add = lb->height() < lb->contentsHeight() ? lb->verticalScrollBar()->width() : 0;
      int w = qMin( mainwin->width(), lb->contentsWidth() + add );
      lb->resize( w, lb->height() );
      // TODO - move the listbox to a suitable place if necessary
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
class ActionLBItem : public QListWidgetItem
{
  public:
    ActionLBItem( QListWidget *lb = 0,
                  const QIcon &pm = QIcon(),
                  const QString &text = QString(),
                  const QString &str = QString() ) :
        QListWidgetItem(pm, text, lb, 0 ),
        _str(str)
    {}
    QString idstring()
    {
      return _str;
    }
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
  QGroupBox *gbToolbar = new QGroupBox(i18n("Toolbar"), this );
  acSel = new KActionSelector( gbToolbar );
  acSel->setAvailableLabel( i18n("A&vailable actions:") );
  acSel->setSelectedLabel( i18n("S&elected actions:") );

  QVBoxLayout *vbox = new QVBoxLayout;
  vbox->addWidget(acSel);
  gbToolbar->setLayout(vbox);

  lo->addWidget( gbToolbar );
  connect( acSel, SIGNAL( added( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );
  connect( acSel, SIGNAL( removed( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );
  connect( acSel, SIGNAL( movedUp( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );
  connect( acSel, SIGNAL( movedDown( QListWidgetItem * ) ), this, SLOT( slotMyChanged() ) );

  // Sync
  QGroupBox *gbSync = new QGroupBox(i18n("Auto Synchronization"), this );
  cbSyncActive = new QCheckBox( i18n("When a docu&ment becomes active"));
  cbSyncShow = new QCheckBox( i18n("When the file selector becomes visible"));

  vbox = new QVBoxLayout;
  vbox->addWidget(cbSyncActive);
  vbox->addWidget(cbSyncShow);
  gbSync->setLayout(vbox);

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
  QGroupBox *gbSession = new QGroupBox(i18n("Session"), this );
  cbSesLocation = new QCheckBox( i18n("Restore loca&tion"));
  cbSesFilter = new QCheckBox( i18n("Restore last f&ilter"));
  cbSesHiddenFiles = new QCheckBox( i18n("Show hidden files") );

  vbox = new QVBoxLayout;
  vbox->addWidget(cbSesLocation);
  vbox->addWidget(cbSesFilter);
  vbox->addWidget(cbSesHiddenFiles);
  gbSession->setLayout(vbox);

  lo->addWidget( gbSession );
  connect( cbSesLocation, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );
  connect( cbSesFilter, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );
  connect( cbSesHiddenFiles, SIGNAL( toggled( bool ) ), this, SLOT( slotMyChanged() ) );

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
                  "combo box.</p>") );
  lbPathHist->setWhatsThis(lhwt );
  sbPathHistLength->setWhatsThis(lhwt );
  QString fhwt( i18n(
                  "<p>Decides how many filters to keep in the history of the filter "
                  "combo box.</p>") );
  lbFilterHist->setWhatsThis(fhwt );
  sbFilterHistLength->setWhatsThis(fhwt );
  QString synwt( i18n(
                   "<p>These options allow you to have the File Selector automatically "
                   "change location to the folder of the active document on certain "
                   "events.</p>"
                   "<p>Auto synchronization is <em>lazy</em>, meaning it will not take "
                   "effect until the file selector is visible.</p>"
                   "<p>None of these are enabled by default, but you can always sync the "
                   "location by pressing the sync button in the toolbar.</p>") );
  gbSync->setWhatsThis(synwt );
  cbSesLocation->setWhatsThis(i18n(
                                "<p>If this option is enabled (default), the location will be restored "
                                "when you start Kate.</p><p><strong>Note</strong> that if the session is "
                                "handled by the KDE session manager, the location is always restored.</p>") );
  cbSesFilter->setWhatsThis(i18n(
                              "<p>If this option is enabled (default), the current filter will be "
                              "restored when you start Kate.<p><strong>Note</strong> that if the "
                              "session is handled by the KDE session manager, the filter is always "
                              "restored.</p>"
                              "<p><strong>Note</strong> that some of the autosync settings may "
                              "override the restored location if on.</p>") );
  cbSesHiddenFiles->setWhatsThis(i18n(
                              "<p>If this option is enabled, the file selector will show hidden "
                              "files in this session.</p>") );

  init();

}

void KFSConfigPage::apply()
{
  if ( ! m_changed )
    return;

  m_changed = false;

  KConfigGroup config(KGlobal::config(), "fileselector");
  // toolbar
  QStringList l;
  ActionLBItem *aItem;
  QList<QListWidgetItem *> list = acSel->selectedListWidget()->findItems(QString("*"), Qt::MatchWildcard);
  foreach(QListWidgetItem *item, list)
  {
    aItem = (ActionLBItem*)item;
    l << aItem->idstring();
  }
  config.writeEntry( "toolbar actions", l );
  fileSelector->setupToolbar( l );
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
  // session - these are read/written directly to the app config,
  //           as they are not needed during operation.
  config.writeEntry( "restore location", cbSesLocation->isChecked() );
  config.writeEntry( "restore last filter", cbSesFilter->isChecked() );
  
  // show hidden files
  fileSelector->dirOperator()->setShowHiddenFiles( cbSesHiddenFiles->isChecked() );
  
  fileSelector->writeConfig();
}

void KFSConfigPage::reset()
{
  // hmm, what is this supposed to do, actually??
  init();
  m_changed = false;
}
void KFSConfigPage::init()
{
  KConfigGroup config(KGlobal::config(), "fileselector");
  // toolbar
  QStringList l = config.readEntry( "toolbar actions", QStringList() );
  if ( l.isEmpty() ) // default toolbar
    l << "up" << "back" << "forward" << "home" <<
    "short view" << "detailed view" << "tree view" <<
    "bookmarks" << "sync_dir";

  // actions from diroperator + two of our own
  QStringList allActions;
  allActions << "up" << "back" << "forward" << "home" <<
  "reload" << "mkdir" << "delete" <<
  "short view" << "detailed view" << /* "view menu" << */
  "show hidden" /*<< "properties"*/ <<
  "tree view" << "detailed tree view" <<
  "bookmarks" << "sync_dir";
  QRegExp re("&(?=[^&])");
  QAction *ac;
  QListWidget *lb;
  for ( QStringList::Iterator it = allActions.begin(); it != allActions.end(); ++it )
  {
    lb = l.contains( *it ) ? acSel->selectedListWidget() : acSel->availableListWidget();
    if ( *it == "bookmarks" || *it == "sync_dir" )
      ac = fileSelector->actionCollection()->action( (*it).toLatin1().constData() );
    else
      ac = fileSelector->dirOperator()->actionCollection()->action( (*it).toLatin1().constData() );
    if ( ac )
    {
      QString text = ac->text().remove( re );
      // CJK languages need a filtering message for action texts in lists,
      // to remove special accelerators that they use.
      // The exact same filtering message exists in kdelibs; hence,
      // avoid extraction here and let it be sourced from kdelibs.
      #define i18ncX i18nc
      text = i18ncX( "@item:intable Action name in toolbar editor", "%1", text );
      new ActionLBItem( lb, ac->icon(), text, *it );
    }
  }

  // sync
  int s = fileSelector->autoSyncEvents;
  cbSyncActive->setChecked( s & KateFileSelector::DocumentChanged );
  cbSyncShow->setChecked( s & KateFileSelector::GotVisible );
  // histories
  sbPathHistLength->setValue( fileSelector->cmbPath->maxItems() );
  sbFilterHistLength->setValue( fileSelector->filter->maxCount() );
  // session
  cbSesLocation->setChecked( config.readEntry( "restore location", true) );
  cbSesFilter->setChecked( config.readEntry( "restore last filter", true) );
  cbSesHiddenFiles->setChecked( fileSelector->dirOperator()->showHiddenFiles() );
}

void KFSConfigPage::slotMyChanged()
{
  m_changed = true;
  emit changed();
}
//END KFSConfigPage
// kate: space-indent on; indent-width 2; replace-tabs on;
