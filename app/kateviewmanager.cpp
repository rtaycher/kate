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

// $Id$

//BEGIN Includes
#include "kateviewmanager.h"
#include "kateviewmanager.moc"

#include "katemainwindow.h"
#include "kateIface.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "../utils/filedialog.h"
#include "kateviewspace.h"

#include <dcopclient.h>
#include <kaction.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kdiroperator.h>
#include <kdockwidget.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <ktoolbar.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <qfileinfo.h>

#include <kio/netaccess.h>

#include <qlayout.h>
#include <qobjectlist.h>
#include <qstringlist.h>
#include <qvbox.h>
#include <qtimer.h>
#include <qprogressdialog.h>

#include "katesplitter.h"
//END Includes

KateViewManager::KateViewManager (QWidget *parent, KateDocManager *m_docManager)
 : QWidget  (parent),
   m_activeViewRunning (false)
{
  m_viewManager = new Kate::ViewManager (this);

  m_reopening=false;
  m_blockViewCreationAndActivation=false;

  // no memleaks
  m_viewList.setAutoDelete(true);
  m_viewSpaceList.setAutoDelete(true);

  this->m_docManager = m_docManager;

  // sizemanagement
  m_grid = new QGridLayout( this, 1, 1 );

  KateViewSpace* vs = new KateViewSpace( this );
  connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, const QString&)), vs, SLOT(slotStatusChanged(Kate::View *, int, int, int, bool, int, const QString&)));
  vs->setActive( true );
  m_grid->addWidget( vs, 0, 0);
  m_viewSpaceList.append(vs);
  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
}

KateViewManager::~KateViewManager ()
{
  m_viewList.setAutoDelete(false);
  m_viewSpaceList.setAutoDelete(false);
}

bool KateViewManager::createView ( bool newDoc, KURL url, Kate::View *origView, Kate::Document *doc )
{

  if (m_blockViewCreationAndActivation) return false;

  // create doc
  if (newDoc && !doc)
    doc = (Kate::Document *)m_docManager->createDoc ();
  else
    if (!doc)
      doc = (Kate::Document *)origView->getDoc();

  // create view
  Kate::View *view = (Kate::View *)doc->createView (this, 0L);
  m_viewList.append (view);

  if (newDoc)
  {
    if (!url.isEmpty())
    {
      if (view->getDoc()->openURL ( url ))
        ((KateMainWindow*)topLevelWidget())->fileOpenRecent->addURL ( view->getDoc()->url() );

      QString name = url.filename();

      // anders avoid two views w/ same caption
      QPtrListIterator<Kate::View> it (m_viewList);

      int hassamename = 0;
      for (; it.current(); ++it)
      {
        if ( it.current()->getDoc()->url().filename().compare( name ) == 0 )
          hassamename++;
      }

      if (hassamename > 1)
        name = QString(name+"<%1>").arg(hassamename);

      view->getDoc()->setDocName (name);
    }
    else
    {
      view->getDoc()->setDocName (i18n("Untitled %1").arg(doc->documentNumber()));
    }
  }
  else
  {
    // when reopening files from an empty session, this is needed to
    // get a name for the automatic document. FIXME why, niceify
    if ( doc->docName().isEmpty() )
      doc->setDocName( i18n("Untitled %1").arg( doc->documentNumber() ) );
    view->getDoc()->setDocName (doc->docName ());
  }

 // if (m_docManager->isFirstDocument())
   // view->getDoc()->setDocName (i18n("Untitled %1").arg(doc->documentNumber()));

  // disable settings dialog action
  view->actionCollection()->remove (view->actionCollection()->action( "set_confdlg" ));

  // popup menu
  view->installPopup ((QPopupMenu*)((KMainWindow *)topLevelWidget ())->factory()->container("ktexteditor_popup", (KMainWindow *)topLevelWidget ()) );

  connect(view,SIGNAL(cursorPositionChanged()),this,SLOT(statusMsg()));
  connect(view,SIGNAL(newStatus()),this,SLOT(statusMsg()));
  connect(view->getDoc(), SIGNAL(undoChanged()), this, SLOT(statusMsg()));
  connect(view,SIGNAL(dropEventPass(QDropEvent *)), (KMainWindow *)topLevelWidget (),SLOT(slotDropEvent(QDropEvent *)));
  connect(view,SIGNAL(gotFocus(Kate::View *)),this,SLOT(activateSpace(Kate::View *)));

  activeViewSpace()->addView( view );
  activateView( view );

  return true;
}

bool KateViewManager::deleteView (Kate::View *view, bool delViewSpace)
{
  if (!view) return true;

  KateViewSpace *viewspace = (KateViewSpace *)view->parentWidget()->parentWidget();

  viewspace->removeView (view);

  ((KMainWindow *)topLevelWidget ())->guiFactory ()->removeClient (view);

  // remove view from list and memory !!
  m_viewList.remove (view);

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  return true;
}

KateViewSpace* KateViewManager::activeViewSpace ()
{
  QPtrListIterator<KateViewSpace> it(m_viewSpaceList);

  for (; it.current(); ++it)
  {
    if ( it.current()->isActiveSpace() )
      return it.current();
  }

  if (m_viewSpaceList.count() > 0)
  {
    m_viewSpaceList.first()->setActive( true );
    return m_viewSpaceList.first();
  }

  return 0L;
}

Kate::View* KateViewManager::activeView ()
{
  if (m_activeViewRunning)
    return 0L;

  m_activeViewRunning = true;

  for (QPtrListIterator<Kate::View> it(m_viewList); it.current(); ++it)
  {
    if ( it.current()->isActive() )
    {
      m_activeViewRunning = false;
      return it.current();
    }
  }

  // if we get to here, no view isActive()
  // first, try to get one from activeViewSpace()
  KateViewSpace* vs;
  if ( (vs = activeViewSpace()) )
  {
    if ( vs->currentView() )
    {
      activateView (vs->currentView());

      m_activeViewRunning = false;
      return vs->currentView();
    }
  }

  // last attempt: just pick first
  if (m_viewList.count() > 0)
  {
    activateView (m_viewList.first());

    m_activeViewRunning = false;
    return m_viewList.first();
  }

  m_activeViewRunning = false;

  // no views exists!
  return 0L;
}

void KateViewManager::setActiveSpace ( KateViewSpace* vs )
{
   if (activeViewSpace())
     activeViewSpace()->setActive( false );

   vs->setActive( true, viewSpaceCount() > 1 );
}

void KateViewManager::setActiveView ( Kate::View* view )
{
  if (activeView())
    activeView()->setActive( false );

  view->setActive( true );
}

void KateViewManager::activateSpace (Kate::View* v)
{
  if (!v) return;

  KateViewSpace* vs = (KateViewSpace*)v->parentWidget()->parentWidget();

  if (!vs->isActiveSpace()) {
    setActiveSpace (vs);
    activateView(v);
  }
}

void KateViewManager::activateView ( Kate::View *view )
{
  if (!view) return;

  if (!view->isActive())
  {
    if ( !activeViewSpace()->showView (view) )
    {
      // since it wasn't found, give'em a new one
      createView (false, KURL(), view );
      return;
    }

    setActiveView (view);
    m_viewList.findRef (view);

    ((KMainWindow *)topLevelWidget ())->toolBar ()->setUpdatesEnabled (false);

    if (((KateMainWindow *)topLevelWidget ())->activeView)
      ((KMainWindow *)topLevelWidget ())->guiFactory()->removeClient ( ((KateMainWindow *)topLevelWidget ())->activeView );

    ((KateMainWindow *)topLevelWidget ())->activeView = view;

    if (!m_blockViewCreationAndActivation)
      ((KMainWindow *)topLevelWidget ())->guiFactory ()->addClient( view );

    ((KMainWindow *)topLevelWidget ())->toolBar ()->setUpdatesEnabled (true);

    statusMsg();

    emit viewChanged ();
    emit m_viewManager->viewChanged ();
  }

  m_docManager->setActiveDocument(view->getDoc());
}

void KateViewManager::activateView( uint documentNumber )
{
  if ( activeViewSpace()->showView(documentNumber) ) {
    activateView( activeViewSpace()->currentView() );
  }
  else
  {
    QPtrListIterator<Kate::View> it(m_viewList);
    for ( ;it.current(); ++it)
    {
      if ( it.current()->getDoc()->documentNumber() == documentNumber  )
      {
        createView( false, KURL(), it.current() );
        return;
      }
    }

    Kate::Document *d = (Kate::Document *)m_docManager->documentWithID(documentNumber);
    createView (false, KURL(), 0L, d );

    // the document exist, but had no views, so we try restoring the view
    // session config
    // FIXME anders: this is clumsy and probably slow, so make something nicer.
    KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);
    int n = m_viewSpaceList.findRef( activeViewSpace() );
    if ( n > -1 )
    {
      QString gr = QString("viewspace%1").arg( n );
      if ( scfg->hasGroup( gr ) )
      {
        scfg->setGroup ( gr );
        int i( 0 );
        QString fg = QString("file%1").arg( i );
        while ( scfg->hasKey( fg ) ) {
          if ( scfg->readPathEntry( fg ) == d->url().prettyURL() )
            break;
          i++;
          fg = QString("file%1").arg( i );
        }
        if ( scfg->hasGroup( gr + ":" + fg ) )
        {
          scfg->setGroup( gr + ":" + fg );
          activeView()->readSessionConfig( scfg );
        }
      }
    }

  }
}

uint KateViewManager::viewCount ()
{
  return m_viewList.count();
}

uint KateViewManager::viewSpaceCount ()
{
  return m_viewSpaceList.count();
}

void KateViewManager::slotViewChanged()
{
  if ( activeView() && !activeView()->hasFocus())
    activeView()->setFocus();
}

void KateViewManager::activateNextView()
{
  uint i = m_viewSpaceList.find (activeViewSpace())+1;

  if (i >= m_viewSpaceList.count())
    i=0;

  setActiveSpace (m_viewSpaceList.at(i));
  activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewManager::activatePrevView()
{
  int i = m_viewSpaceList.find (activeViewSpace())-1;

  if (i < 0)
    i=m_viewSpaceList.count()-1;

  setActiveSpace (m_viewSpaceList.at(i));
  activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewManager::deleteLastView ()
{
  deleteView (activeView (), true);
}

void KateViewManager::closeViews(uint documentNumber)
{
    QPtrList<Kate::View> closeList;

    for (uint z=0 ; z < m_viewList.count(); z++)
    {
      Kate::View* current = m_viewList.at(z);
      if ( current->getDoc()->documentNumber() == documentNumber )
      {
        closeList.append (current);
      }
    }

    while ( !closeList.isEmpty() )
    {
      Kate::View *view = closeList.first();
      deleteView (view, true);
      closeList.removeFirst();
    }

  if (m_blockViewCreationAndActivation) return;
  QTimer::singleShot(0,this,SIGNAL(viewChanged()));
  emit m_viewManager->viewChanged ();
}


void KateViewManager::openNewIfEmpty()
{
  if (m_blockViewCreationAndActivation) return;

  for (uint i2=0; i2 < ((KateApp *)kapp)->mainWindows (); i2++ )
  {
    if (((KateApp *)kapp)->kateMainWindow(i2)->kateViewManager()->viewCount() == 0)
    {
      if ((m_viewList.count() < 1) && (m_docManager->documents() < 1) )
        ((KateApp *)kapp)->kateMainWindow(i2)->kateViewManager()->createView (true, KURL(), 0L);
      else if ((m_viewList.count() < 1) && (m_docManager->documents() > 0) )
        ((KateApp *)kapp)->kateMainWindow(i2)->kateViewManager()->createView (false, KURL(), 0L, (Kate::Document *)m_docManager->document(m_docManager->documents()-1));
    }
  }
  emit viewChanged ();
  emit m_viewManager->viewChanged ();
}

void KateViewManager::statusMsg ()
{
  if (!activeView()) return;

  Kate::View* v = activeView();

  bool readOnly =  !v->getDoc()->isReadWrite();
  uint config =  v->getDoc()->configFlags();

  int ovr = 0;
  if (readOnly)
    ovr = 0;
  else
  {
    if (config & Kate::Document::cfOvr)
    {
      ovr=1;
    }
    else
    {
      ovr=2;
    }
  }

  int mod = (int)v->getDoc()->isModified();
  bool block=v->getDoc()->blockSelectionMode();

  QString c;
  if (v->getDoc()->url().isEmpty() || (!showFullPath))
  {
    c = v->getDoc()->docName();

    //File name shouldn't be too long - Maciek
    if (c.length() > 64)
      c = c.left(64) + "...";
  }
  else
  {
    c = v->getDoc()->url().prettyURL();

    //File name shouldn't be too long - Maciek
    if (c.length() > 64)
      c = "..." + c.right(64);
  }

  emit statusChanged (v, v->cursorLine(), v->cursorColumn(), ovr,block, mod, c);
  emit statChanged ();
}

void KateViewManager::slotWindowNext()
{
  int id = m_docManager->findDocument (activeView ()->getDoc()) - 1;

  if (id < 0)
    id =  m_docManager->documents () - 1;

  activateView (m_docManager->document(id)->documentNumber());
}

void KateViewManager::slotWindowPrev()
{
  uint id = m_docManager->findDocument (activeView ()->getDoc()) + 1;

  if (id >= m_docManager->documents () )
    id = 0;

  activateView (m_docManager->document(id)->documentNumber());
}

void KateViewManager::slotDocumentNew ()
{
  createView (true, KURL(), 0L);
}

void KateViewManager::slotDocumentOpen ()
{
  Kate::View *cv = activeView();
        Kate::FileDialog *dialog;

        //TODO: move to kdelibs
        QString DEFAULT_ENCODING = QString::fromLatin1(KGlobal::locale()->encoding());

  if (cv)
          dialog = new Kate::FileDialog (cv->getDoc()->url().url(),cv->getDoc()->encoding(), this, i18n ("Open File"));
        else
          dialog = new Kate::FileDialog (QString::null, DEFAULT_ENCODING, this, i18n ("Open File"));

        Kate::FileDialogData data = dialog->exec ();
        delete dialog;

  for (KURL::List::Iterator i=data.urls.begin(); i != data.urls.end(); ++i)
  {
    if (!KIO::NetAccess::exists(*i, true, this))
      KMessageBox::error (this, i18n("The given file could not be read, check if it exists or if it is readable for the current user."));
    else
      openURL( *i, data.encoding );
  }
}

void KateViewManager::slotDocumentSaveAll()
{
  for( QPtrListIterator<Kate::View> it( m_viewList ); it.current(); ++it )
    it.current()->save();
}

void KateViewManager::slotDocumentClose ()
{
  if (!activeView()) return;

  m_docManager->closeDocument (activeView()->getDoc());

  openNewIfEmpty();
}

void KateViewManager::slotDocumentCloseAll ()
{
  if (m_docManager->documents () == 0) return;

  kdDebug()<<"CLOSE ALL DOCUMENTS *****************"<<endl;

  m_blockViewCreationAndActivation=true;
  m_docManager->closeAllDocuments();
  m_blockViewCreationAndActivation=false;

  openNewIfEmpty();
}

void KateViewManager::openURL (KURL url, const QString& encoding)
{
  uint id;
  Kate::Document *doc=m_docManager->openURL(url,encoding,&id);

  if (!doc->url().isEmpty())
    ((KateMainWindow*)topLevelWidget())->fileOpenRecent->addURL( doc->url() );

  /* NOTE by uncommenting this, we are forced to try if a session configuration
     exists for this viewspace.
     In some cases this will cause misbehaviour, that is if viewspaces are
     created and deleted and a file is then opened first time in a viewspace
     for which a record exists...
     TODO - generally clean up the session config reading/writing */
  //Kate::View *cv = activeView();

  //if (!cv)
  //  createView(false,url,0L,doc);

  activateView( id );
}

void KateViewManager::openConstURLCheck (const KURL& url)
{
  if (url.isEmpty()) return;

  if (!KIO::NetAccess::exists(url, true, this))
  {
    KMessageBox::error (this, i18n("The given file could not be read, check if it exists or if it is readable for the current user."));
    return;
  }

  openURL (KURL (url));
}

void KateViewManager::splitViewSpace( KateViewSpace* vs,
                                      bool isHoriz,
                                      bool atTop,
                                      KURL newViewUrl)
{
  kdDebug(13001)<<"splitViewSpace()"<<endl;

  if (!activeView()) return;
  if (!vs) vs = activeViewSpace();

  bool isFirstTime = vs->parentWidget() == this;

  QValueList<int> psizes;
  if ( ! isFirstTime )
    if ( QSplitter *ps = static_cast<QSplitter*>(vs->parentWidget()->qt_cast("QSplitter")) )
      psizes = ps->sizes();

  Qt::Orientation o = isHoriz ? Qt::Vertical : Qt::Horizontal;
  KateSplitter* s = new KateSplitter(o, vs->parentWidget());
  s->setOpaqueResize( useOpaqueResize );

  if (! isFirstTime) {
    // anders: make sure the split' viewspace is always
    // correctly positioned.
    // If viewSpace is the first child, the new splitter must be moveToFirst'd
    if ( !((KateSplitter*)vs->parentWidget())->isLastChild( vs ) )
       ((KateSplitter*)s->parentWidget())->moveToFirst( s );
  }
  vs->reparent( s, 0, QPoint(), true );
  KateViewSpace* vsNew = new KateViewSpace( s );

  if (atTop)
    s->moveToFirst( vsNew );

  if (isFirstTime)
    m_grid->addWidget(s, 0, 0);
  else if ( QSplitter *ps = static_cast<QSplitter*>(s->parentWidget()->qt_cast("QSplitter")) )
    ps->setSizes( psizes );


  s->show();

  QValueList<int> sizes;
  int space = 50;//isHoriz ? s->parentWidget()->height()/2 : s->parentWidget()->width()/2;
  sizes << space << space;
  s->setSizes( sizes );

  connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, const QString &)), vsNew, SLOT(slotStatusChanged(Kate::View *, int, int,int, bool, int, const QString &)));
  m_viewSpaceList.append( vsNew );
  activeViewSpace()->setActive( false );
  vsNew->setActive( true, true );
  vsNew->show();
  if (!newViewUrl.isValid())
    createView (false, KURL(), (Kate::View *)activeView());
  else {
    // tjeck if doc is already open
    uint aDocId;
    if ( (aDocId = m_docManager->findDocument( newViewUrl )) )
      createView (false, KURL(), 0L, (Kate::Document *)m_docManager->documentWithID( aDocId) );
    else
      createView( true, newViewUrl );
  }
  kdDebug(13001)<<"splitViewSpace() - DONE!"<<endl;
}

void KateViewManager::removeViewSpace (KateViewSpace *viewspace)
{
  // abort if viewspace is 0
  if (!viewspace) return;

  // abort if this is the last viewspace
  if (m_viewSpaceList.count() < 2) return;

  KateSplitter* p = (KateSplitter*)viewspace->parentWidget();

  // find out if it is the first child for repositioning
  // see below
  bool pIsFirst = false;

  // save some size information
  KateSplitter* pp=0L;
  QValueList<int> ppsizes;
  if (m_viewSpaceList.count() > 2 && p->parentWidget() != this)
  {
    pp = (KateSplitter*)p->parentWidget();
    ppsizes = pp->sizes();
    pIsFirst = !pp->isLastChild( p ); // simple logic, right-
  }

  // Figure out where to put views that are still needed
  KateViewSpace* next;
  if (m_viewSpaceList.find(viewspace) == 0)
    next = m_viewSpaceList.next();
  else
    next = m_viewSpaceList.prev();

  // Reparent views in viewspace that are last views, delete the rest.
  int vsvc = viewspace->viewCount();
  while (vsvc > 0)
  {
    if (viewspace->currentView())
    {
      Kate::View* v = viewspace->currentView();

      if (v->isLastView())
      {
        viewspace->removeView(v);
        next->addView( v, false );
      }
      else
      {
        deleteView( v, false );
      }
    }
    vsvc = viewspace->viewCount();
  }

  m_viewSpaceList.remove( viewspace );

  // reparent the other sibling of the parent.
  while (p->children ())
  {
    QWidget* other = ((QWidget *)(( QPtrList<QObject>*)p->children())->first());

    other->reparent( p->parentWidget(), 0, QPoint(), true );
    // We also need to find the right viewspace to become active,
    // and if "other" is the last, we move it into the m_grid.
    if (pIsFirst)
       ((KateSplitter*)p->parentWidget())->moveToFirst( other );
    if ( other->isA("KateViewSpace") ) {
      setActiveSpace( (KateViewSpace*)other );
      if (m_viewSpaceList.count() == 1)
        m_grid->addWidget( other, 0, 0);
    }
    else {
      QObjectList* l = other->queryList( "KateViewSpace" );
      if ( l->first() != 0 ) { // I REALLY hope so!
        setActiveSpace( (KateViewSpace*)l->first() );
      }
      delete l;
    }
  }

  delete p;

  if (!ppsizes.isEmpty())
    pp->setSizes( ppsizes );

  // find the view that is now active.
  Kate::View* v = activeViewSpace()->currentView();
  if ( v )
    activateView( v );

  emit viewChanged();
  emit m_viewManager->viewChanged ();
}

void KateViewManager::slotCloseCurrentViewSpace()
{
  removeViewSpace(activeViewSpace());
}

void KateViewManager::setShowFullPath( bool enable )
{
  showFullPath = enable;
}

void KateViewManager::setUseOpaqueResize( bool enable )
{
  useOpaqueResize = enable;
  // TODO: loop through splitters and set this prop
}

///////////////////////////////////////////////////////////
// session config functions
///////////////////////////////////////////////////////////
void KateViewManager::saveAllDocsAtCloseDown(  )
{
  kdDebug(13001)<<"saveAllDocsAtCloseDown()"<<endl;
  if (m_docManager->documents () == 0) return;

  // Make sure all documents have a URL
  Kate::Document *d = m_docManager->firstDocument();
  while ( d )
  {
    if ( d->url().isEmpty() && d->isModified() )
    {
      // ask if the user wants to save this one
      // and if so, ask for a URL
      // else, close doc.
      bool keep( false );

      if ( KMessageBox::warningYesNo( this,
              i18n("<p>The document '%1' has been modified, but not saved."
                   "<p>Do you want to keep it?").arg( d->docName() ),
              i18n("Unsaved Document") ) == KMessageBox::Yes )
      { // FIXME add some nicer constructors to the file dialog
        Kate::FileDialogData fdd = (new Kate::FileDialog( QString::null,
                                       d->encoding(),
                                       this,
                                       i18n("Save As"),
                                       Kate::FileDialog::saveDialog ))->exec();
        d->setEncoding( fdd.encoding );

        if ( d->saveAs( fdd.url ) )
          keep = true;
        else
          return;
      }

      if ( !keep )
      {
        closeViews( d->documentNumber() );
        m_docManager->deleteDoc( d );
      }
    }

    d = m_docManager->nextDocument();
  }

  if ( ! m_docManager->documents() ) return;

  KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

  // save current document, since if we just reopens documents
  // when restarted, we want that in front.

  scfg->setGroup("open files");
  scfg->writeEntry("count",m_docManager->documents());
  scfg->writePathEntry("current file", activeView()->getDoc()->url().prettyURL());
  m_docManager->saveDocumentList(scfg);

  scfg->sync();

  m_blockViewCreationAndActivation=true;
  m_docManager->closeAllDocuments();
  m_blockViewCreationAndActivation=false;

  kdDebug(13001)<<">>>> saveAllDocsAtCloseDown() DONE"<<endl;
  delete scfg;
}

void KateViewManager::reopenDocuments(bool isRestore)
{
  m_reopening=true;
  //kdDebug(13001)<<"reopenDocuments()"<<endl;
  KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);
  KConfig* config = kapp->config();
  config->setGroup("General");
  bool restoreViews = config->readBoolEntry("restore views", false);
  bool reopenAtStart = config->readBoolEntry("reopen at startup", false);

  if (  (reopenAtStart &&  (!(((KateApp*)kapp)->doNotInitialize() & 0x1))) || isRestore )
  {
    scfg->setGroup("open files");
    // try to focus the file that had focus at close down
    QString curfile = scfg->readPathEntry("current file");

    if (curfile.isEmpty()) {
        delete scfg;
        m_reopening=false;
        return;
    }

    QString fileCountStr=scfg->readEntry("count");
    int fileCount=fileCountStr.isEmpty() ? 100 : fileCountStr.toInt();

    QProgressDialog *pd=new QProgressDialog(
        i18n("Reopening files from the last session..."),
        QString::null,
        fileCount,
        this,
        "openprog");

    m_blockViewCreationAndActivation=true;
    m_docManager->closeAllDocuments();
    m_blockViewCreationAndActivation=false;

    int i = 0;
    QString fn;
    // check all remote files for existence in a synchronous way
    // so that the password dialog only appears ones for every remote source
    while (scfg->hasKey(QString("File%1").arg(i)))
    {
      fn = scfg->readPathEntry( QString("File%1").arg( i ) );
      if ( !fn.isEmpty() && !KURL( fn ).isLocalFile() )
          KIO::NetAccess::exists(KURL( fn ), true, this);
      i++;
    }
    i = 0;
    while (scfg->hasKey(QString("File%1").arg(i)))
    {
      fn = scfg->readPathEntry( QString("File%1").arg( i ) );
      if ( !fn.isEmpty() ) {
        //kdDebug(13001)<<"reopenDocuments(): opening file : "<<fn<<endl;
        scfg->setGroup( fn );

        Kate::Document *doc = m_docManager->openURL( KURL( fn ) );
        if (doc)
          doc->readSessionConfig(scfg);

        scfg->setGroup("open files");
      }
      i++;

      pd->setProgress(pd->progress()+1);
      kapp->processEvents();

    }
    delete pd;

    if ( scfg->hasGroup("splitter0") && ( isRestore || restoreViews ) )
    {
      //kdDebug(13001)<<"calling restoreViewConfig()"<<endl;
      restoreViewConfig();
    }
    else  openURL(KURL(curfile));

  }
  m_reopening=false;
  //kdDebug(13001)<<">>>> reopenDocuments() DONE"<<endl;
  delete scfg;
}

void KateViewManager::saveViewSpaceConfig()
{
   //kdDebug(13001)<<"saveViewSpaceConfig()"<<endl;
   KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

  // TEMPORARY ??
  //kdDebug(13001)<<"clearing session config file before saving list"<<endl;
  scfg->setGroup("nogroup");
  QStringList groups(scfg->groupList());
  for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
    if ( *it != "nogroup") scfg->deleteGroup(*it);

   if (viewSpaceCount() == 1) {
     m_viewSpaceList.first()->saveFileList( scfg, 0 );
   }
   else {

     // I need the first splitter, the one which has this as parent.
     KateSplitter* s;
     QObjectList *l = queryList("KateSplitter", 0, false, false);
     QObjectListIt it( *l );
     if ( (s = (KateSplitter*)it.current()) != 0 )
       saveSplitterConfig( s, 0, scfg );

     delete l;
   }

   scfg->sync();
   delete scfg;
   //kdDebug(13001)<<">>>> saveViewSpaceConfig() DONE"<<endl;
}

void KateViewManager::saveSplitterConfig( KateSplitter* s, int idx, KSimpleConfig* config )
{

   QString grp = QString("splitter%1").arg(idx);
   config->setGroup(grp);

   // Save sizes, orient, children for this splitter
   config->writeEntry( "sizes", s->sizes() );
   config->writeEntry( "orientation", s->orientation() );

   QStringList childList;
   // a katesplitter has two children, of which one may be a KateSplitter.
   const QObjectList* l = s->children();
   QObjectListIt it( *l );
   QObject* obj;
   for (; it.current(); ++it) {
     obj = it.current();
     QString n;  // name for child list, see below
     // For KateViewSpaces, ask them to save the file list.
     if ( obj->isA("KateViewSpace") ) {
       n = QString("viewspace%1").arg( m_viewSpaceList.find((KateViewSpace*)obj) );
       ((KateViewSpace*)obj)->saveFileList( config, m_viewSpaceList.find((KateViewSpace*)obj) );
       // save active viewspace
       if ( ((KateViewSpace*)obj)->isActiveSpace() ) {
         config->setGroup("general");
         config->writeEntry("activeviewspace", m_viewSpaceList.find((KateViewSpace*)obj) );
       }
     }
     // For KateSplitters, recurse
     else if ( obj->isA("KateSplitter") ) {
       idx++;
       saveSplitterConfig( (KateSplitter*)obj, idx, config);
       n = QString("splitter%1").arg( idx );
     }
     // make sure list goes in right place!
     if (!n.isEmpty()) {
       if ( childList.count() > 0 && ! s->isLastChild( (QWidget*)obj ) )
         childList.prepend( n );
       else
         childList.append( n );
     }
   }

   // reset config group.
   config->setGroup(grp);
   config->writeEntry("children", childList);
}

void KateViewManager::restoreViewConfig()
{
   // This is called *instead* of reopenDocuments if view config needs to be restored.
   // Consequently, it has the task of opening all documents.
   KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);
   // if group splitter0 does not exist, call reopenDocuments() and return
   if ( ! scfg->hasGroup("splitter0") ) {
     //reopenDocuments();
       delete scfg;
     return;
   }

   // remove the initial viewspace.
   m_viewSpaceList.clear();
   // call restoreSplitter for splitter0
   restoreSplitter( scfg, QString("splitter0"), this );
   // finally, make the correct view active.
   //kdDebug(13001)<<"All splitters restored, setting active view"<<endl;
   scfg->setGroup("general");
   KateViewSpace *vs = m_viewSpaceList.at( scfg->readNumEntry("activeviewspace") );
   if ( vs ) // better be sure ;}
     activateSpace( vs->currentView() );
   delete scfg;
}

void KateViewManager::restoreSplitter( KSimpleConfig* config, const QString &group, QWidget* parent)
{
   config->setGroup( group );

   // create a splitter with orientation
   //kdDebug(13001)<<"restoreSplitter():creating a splitter: "<<group<<endl;
   //if (parent == this)
   //  kdDebug(13001)<<"parent is this"<<endl;
   KateSplitter* s = new KateSplitter((Qt::Orientation)config->readNumEntry("orientation"), parent);
   if ( group.compare("splitter0") == 0 )
     m_grid->addWidget(s, 0, 0);

   QStringList children = config->readListEntry( "children" );
   for (QStringList::Iterator it=children.begin(); it!=children.end(); ++it)
   {
     // for a viewspace, create it and open all documents therein.
     if ( (*it).startsWith("viewspace") ) {
       KateViewSpace* vs = new KateViewSpace( s );
       connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, const QString &)), vs, SLOT(slotStatusChanged(Kate::View *, int, int, int, bool, int, const QString &)));
       m_viewSpaceList.append( vs );
       vs->show();
       setActiveSpace( vs );

       // open documents
       int idx = 0;
       QString file = QString("file%1").arg( idx );
       config->setGroup( (*it) );  // "viewspace<n>"
       while ( config->hasKey( file ) ) {     // FIXME FIXME
         Kate::View* v;
         KURL url( config->readPathEntry( file ) );
         if ( ! m_docManager->isOpen( url ) ) {
           openURL( url );
           v = activeView();
           if (v && v->getDoc()->url() == url ) { // this is a wild assumption, but openURL() fails to return a bool :(
             // doc config is in group "<url.prettyURL()>"
             config->setGroup( url.prettyURL() );
             v->getDoc()->readSessionConfig( config );
           }
           else {
             //createView (true, KURL(), 0L);
             kdDebug(13001)<<"KateViewManager: failed to open document "<<file<<endl;
           }
         }
         else { // if the group has been deleted, we can find a document
           // ahem, tjeck if this document actually exists.
           Kate::Document *doc = m_docManager->findDocumentByUrl( url );
           if ( doc ) {
             //kdDebug(13001)<<"Document '"<<url.prettyURL()<<"' found open, creating extra view"<<endl;
             createView( false, KURL(), 0L, doc );
           }
           else
             kdDebug(13001)<<"ARGH, I can't find the document "<<url<<endl;
           v = activeView();
         }
         if ( v ) {
           // view config is in group "<group>:<file>"
           QString g = *it + ":" + file;
           //kdDebug(13001)<<"view config is group '"<<g<<"'"<<endl;
           if ( config->hasGroup( g ) ) {
             config->setGroup( g );
             v->readSessionConfig( config );
           }
         }

         idx++;
         file = QString("file%1").arg( idx );
         config->setGroup(*it);
         // done this file
       }
       // If the viewspace have no documents due to bad luck, create a blank.
       if ( vs->viewCount() < 1)
         createView( true, KURL() );
       //kdDebug(13001)<<"Done resotring a viewspace"<<endl;
     }
     // for a splitter, recurse.
     else if ( (*it).startsWith("splitter") ) {
       restoreSplitter( config, QString(*it), s );
     }
   }
   // set sizes
   config->setGroup( group );
   s->setSizes( config->readIntListEntry("sizes") );
   s->show();
   //kdDebug(13001)<<"Bye from KateViewManager::restoreSplitter() ("<<group<<")"<<endl;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
