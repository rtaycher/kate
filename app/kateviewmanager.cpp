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
#include "kateviewmanager.h"
#include "kateviewmanager.moc"

#include "katemainwindow.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "katesplitter.h"
#include "kateviewspace.h"

#include <dcopclient.h>
#include <kaction.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kdiroperator.h>
#include <kdockwidget.h>
#include <kencodingfiledialog.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <ktoolbar.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>

#include <ktexteditor/encodinginterface.h>

#include <qlayout.h>
#include <qobjectlist.h>
#include <qstringlist.h>
#include <qvbox.h>
#include <qtimer.h>
#include <qfileinfo.h>
//END Includes

KateViewManager::KateViewManager (QWidget *parent, KateDocManager *m_docManager, KateMainWindow *mainWindow)
 : QWidget  (parent),
   m_activeViewRunning (false),m_mainWindow(mainWindow)
{
  m_viewManager = new Kate::ViewManager (this);

  m_blockViewCreationAndActivation=false;

  useOpaqueResize = KGlobalSettings::opaqueResize();

  // no memleaks
  m_viewList.setAutoDelete(true);
  m_viewSpaceList.setAutoDelete(true);

  this->m_docManager = m_docManager;

  // sizemanagement
  m_grid = new QGridLayout( this, 1, 1 );

  KateViewSpace* vs = new KateViewSpace( this, this );
  connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, const QString&)), vs, SLOT(slotStatusChanged(Kate::View *, int, int, int, bool, int, const QString&)));
  vs->setActive( true );
  m_grid->addWidget( vs, 0, 0);
  m_viewSpaceList.append(vs);
  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
  connect(m_docManager, SIGNAL(initialDocumentReplaced()), this, SIGNAL(viewChanged()));
}

KateViewManager::~KateViewManager ()
{
  m_viewList.setAutoDelete(false);
  m_viewSpaceList.setAutoDelete(false);
}

bool KateViewManager::createView ( Kate::Document *doc )
{
  if (m_blockViewCreationAndActivation) return false;

  // create doc
  if (!doc)
    doc = m_docManager->createDoc ();

  // create view
  Kate::View *view = (Kate::View *) doc->createView (this, 0L);

  m_viewList.append (view);

  // disable settings dialog action
  view->actionCollection()->remove (view->actionCollection()->action( "set_confdlg" ));

  // popup menu
  view->installPopup ((QPopupMenu*)(m_mainWindow->factory()->container("ktexteditor_popup", m_mainWindow)) );

  connect(view->getDoc(),SIGNAL(nameChanged(Kate::Document *)),this,SLOT(statusMsg()));
  connect(view,SIGNAL(cursorPositionChanged()),this,SLOT(statusMsg()));
  connect(view,SIGNAL(newStatus()),this,SLOT(statusMsg()));
  connect(view->getDoc(), SIGNAL(undoChanged()), this, SLOT(statusMsg()));
  connect(view,SIGNAL(dropEventPass(QDropEvent *)), m_mainWindow,SLOT(slotDropEvent(QDropEvent *)));
  connect(view,SIGNAL(gotFocus(Kate::View *)),this,SLOT(activateSpace(Kate::View *)));

  activeViewSpace()->addView( view );
  activateView( view );
  connect( doc, SIGNAL(modifiedOnDisc(Kate::Document *, bool, unsigned char)),
      activeViewSpace(), SLOT(modifiedOnDisc(Kate::Document *, bool, unsigned char)) );

  return true;
}

bool KateViewManager::deleteView (Kate::View *view, bool delViewSpace)
{
  if (!view) return true;

  KateViewSpace *viewspace = (KateViewSpace *)view->parentWidget()->parentWidget();

  viewspace->removeView (view);

  m_mainWindow->guiFactory ()->removeClient (view);

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
      createView ( view->getDoc() );
      return;
    }

    setActiveView (view);
    m_viewList.findRef (view);

   m_mainWindow->toolBar ()->setUpdatesEnabled (false);

    if (m_mainWindow->activeView)
      m_mainWindow->guiFactory()->removeClient (m_mainWindow->activeView );

    m_mainWindow->activeView = view;

    if (!m_blockViewCreationAndActivation)
      m_mainWindow->guiFactory ()->addClient( view );

    m_mainWindow->toolBar ()->setUpdatesEnabled (true);

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
        createView( it.current()->getDoc() );
        return;
      }
    }

    Kate::Document *d = (Kate::Document *)m_docManager->documentWithID(documentNumber);
    createView (d);
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
        ((KateApp *)kapp)->kateMainWindow(i2)->kateViewManager()->createView ();
      else if ((m_viewList.count() < 1) && (m_docManager->documents() > 0) )
        ((KateApp *)kapp)->kateMainWindow(i2)->kateViewManager()->createView (m_docManager->document(m_docManager->documents()-1));
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

void KateViewManager::slotDocumentNew ()
{
  createView ();
}

void KateViewManager::slotDocumentOpen ()
{
  Kate::View *cv = activeView();

  KEncodingFileDialog::Result r=KEncodingFileDialog::getOpenURLsAndEncoding(
     (cv ? KTextEditor::encodingInterface(cv->document())->encoding() : Kate::Document::defaultEncoding()),
     (cv ? cv->document()->url().url() : QString::null),
     QString::null,this,i18n("Open File"));

  uint lastID = 0;
  for (KURL::List::Iterator i=r.URLs.begin(); i != r.URLs.end(); ++i)
    lastID = openURL( *i, r.encoding, false );

  if (lastID > 0)
    activateView (lastID);
}

void KateViewManager::slotDocumentSaveAll()
{
  for( QPtrListIterator<Kate::View> it( m_viewList ); it.current(); ++it )
    it.current()->save();
}

void KateViewManager::slotDocumentClose ()
{
  // no active view, do nothing
  if (!activeView()) return;

  // prevent close document if only one view alive and the document of
  // it is not modified and empty !!!
  if ( (m_viewList.count() == 1)
       && !activeView()->getDoc()->isModified()
       && activeView()->getDoc()->url().isEmpty()
       && (activeView()->getDoc()->length() == 0) )
  {
    activeView()->getDoc()->closeURL();
    return;
  }

  // close document
  m_docManager->closeDocument (activeView()->getDoc());

  // create new one, if none alive
  openNewIfEmpty();
}

void KateViewManager::slotDocumentCloseAll ()
{
  if (m_docManager->documents () == 0) return;

  kdDebug(13001)<<"CLOSE ALL DOCUMENTS *****************"<<endl;

  m_blockViewCreationAndActivation=true;
  m_docManager->closeAllDocuments();
  m_blockViewCreationAndActivation=false;

  openNewIfEmpty();
}

uint KateViewManager::openURL (const KURL &url, const QString& encoding, bool activate)
{
  uint id = 0;
  Kate::Document *doc = m_docManager->openURL (url, encoding, &id);

  if (!doc->url().isEmpty())
    m_mainWindow->fileOpenRecent->addURL( doc->url() );

  if (activate)
    activateView( id );

  return id;
}

void KateViewManager::openURL (const KURL &url)
{
  openURL (url, QString::null);
}

void KateViewManager::splitViewSpace( KateViewSpace* vs,
                                      bool isHoriz,
                                      bool atTop)
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
  KateViewSpace* vsNew = new KateViewSpace( this, s );

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

  createView (activeView()->getDoc());

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
  statusMsg ();
  m_mainWindow->slotWindowActivated ();
}

/**
 * session config functions
 */

void KateViewManager::saveViewConfiguration(KConfig *config,const QString& group)
{
  bool weHaveSplittersAlive (viewSpaceCount() > 1);

  config->setGroup (group); //"View Configuration");
  config->writeEntry ("Splitters", weHaveSplittersAlive);

  // no splitters around
  if (!weHaveSplittersAlive)
  {
    config->writeEntry("Active Viewspace", 0);
    m_viewSpaceList.first()->saveConfig ( config, 0,group );

    return;
  }

  // I need the first splitter, the one which has this as parent.
  KateSplitter* s;
  QObjectList *l = queryList("KateSplitter", 0, false, false);
  QObjectListIt it( *l );

  if ( (s = (KateSplitter*)it.current()) != 0 )
    saveSplitterConfig( s, 0, config , group);

  delete l;
}

void KateViewManager::restoreViewConfiguration (KConfig *config, const QString& group)
{
  config->setGroup(group);
  //config->setGroup ("View Configuration");

  // no splitters around, ohhh :()
  if (!config->readBoolEntry ("Splitters"))
  {
    // only add the new views needed, let the old stay, won't hurt if one around
    m_viewSpaceList.first ()->restoreConfig (this, config, QString(group+"-ViewSpace 0"));
  }
  else
  {
    // send all views + their gui to **** ;)
    for (uint i=0; i < m_viewList.count(); i++)
      m_mainWindow->guiFactory ()->removeClient (m_viewList.at(i));

    m_viewList.clear ();

    // cu viewspaces
    m_viewSpaceList.clear();

    // call restoreSplitter for Splitter 0
    restoreSplitter( config, QString(group+"-Splitter 0"), this,group );
  }

  // finally, make the correct view active.
  config->setGroup (group);
/*
  KateViewSpace *vs = m_viewSpaceList.at( config->readNumEntry("Active ViewSpace") );
  if ( vs )
    activateSpace( vs->currentView() );
  */
}


void KateViewManager::saveSplitterConfig( KateSplitter* s, int idx, KConfig* config, const QString& viewConfGrp )
{
  QString grp = QString(viewConfGrp+"-Splitter %1").arg(idx);
  config->setGroup(grp);

  // Save sizes, orient, children for this splitter
  config->writeEntry( "Sizes", s->sizes() );
  config->writeEntry( "Orientation", s->orientation() );

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
     n = QString(viewConfGrp+"-ViewSpace %1").arg( m_viewSpaceList.find((KateViewSpace*)obj) );
     ((KateViewSpace*)obj)->saveConfig ( config, m_viewSpaceList.find((KateViewSpace*)obj), viewConfGrp);
     // save active viewspace
     if ( ((KateViewSpace*)obj)->isActiveSpace() ) {
       config->setGroup(viewConfGrp);
       config->writeEntry("Active Viewspace", m_viewSpaceList.find((KateViewSpace*)obj) );
     }
   }
   // For KateSplitters, recurse
   else if ( obj->isA("KateSplitter") ) {
     idx++;
     saveSplitterConfig( (KateSplitter*)obj, idx, config,viewConfGrp);
     n = QString(viewConfGrp+"-Splitter %1").arg( idx );
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
  config->writeEntry("Children", childList);
}

void KateViewManager::restoreSplitter( KConfig* config, const QString &group, QWidget* parent, const QString& viewConfGrp)
{
  config->setGroup( group );

  KateSplitter* s = new KateSplitter((Qt::Orientation)config->readNumEntry("Orientation"), parent);

  if ( group.compare(viewConfGrp+"-Splitter 0") == 0 )
   m_grid->addWidget(s, 0, 0);

  QStringList children = config->readListEntry( "Children" );
  for (QStringList::Iterator it=children.begin(); it!=children.end(); ++it)
  {
    // for a viewspace, create it and open all documents therein.
    if ( (*it).startsWith(viewConfGrp+"-ViewSpace") )
    {
     KateViewSpace* vs = new KateViewSpace( this, s );

     connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, const QString &)), vs, SLOT(slotStatusChanged(Kate::View *, int, int, int, bool, int, const QString &)));

     if (m_viewSpaceList.isEmpty())
       vs->setActive (true);

     m_viewSpaceList.append( vs );

     vs->show();
     setActiveSpace( vs );

     vs->restoreConfig (this, config, *it);
    }
    else
    {
      // for a splitter, recurse.
      restoreSplitter( config, QString(*it), s, viewConfGrp );
    }
  }

  // set sizes
  config->setGroup( group );
  s->setSizes( config->readIntListEntry("Sizes") );
  s->show();
}

KateMainWindow *KateViewManager::mainWindow() {
        return m_mainWindow;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
