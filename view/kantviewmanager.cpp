/***************************************************************************
                          kantviewmanager.cpp  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kantviewmanager.h"
#include "kantviewmanager.moc"

#include "../mainwindow/kantmainwindow.h"
#include "../mainwindow/kantIface.h"
#include "../document/kantdocmanager.h"
#include "../document/kantdocument.h"
#include "../app/kantapp.h"
#include "kantview.h"
#include "kantviewspace.h"

#include <dcopclient.h>
#include <kaction.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <kdiroperator.h>
#include <kdockwidget.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstddirs.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <qobjectlist.h>
#include <qstringlist.h>
#include <qvbox.h>


#include "kantsplitter.h"

KantViewManager::KantViewManager (QWidget *parent, KantDocManager *docManager) : KantViewManagerIface  (parent)
{
  // no memleaks
  viewList.setAutoDelete(true);
  viewSpaceList.setAutoDelete(true);

  myViewID = 0;

  this->docManager = docManager;

  // sizemanagment
  grid = new QGridLayout( this, 1, 1 );

  KantViewSpace* vs = new KantViewSpace( this );
  connect(this, SIGNAL(statusChanged(KantView *, int, int, int, int, QString)), vs, SLOT(slotStatusChanged(KantView *, int, int, int, int, QString)));
  vs->setActive( true );
  vs->installEventFilter( this );
  grid->addWidget( vs, 0, 0);
  viewSpaceList.append(vs);

  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
}

KantViewManager::~KantViewManager ()
{
  viewList.setAutoDelete(false);
  viewSpaceList.setAutoDelete(false);
}

bool KantViewManager::createView ( bool newDoc, KURL url, KantView *origView, KantDocument *doc )
{
  // create doc
  if (newDoc && !doc)
  {
    QFileInfo* fi;
    if ( (!url.isEmpty()) && (url.filename() != 0) ) {
      // if this is a local file, get some information
      if ( url.isLocalFile() ) {
         //kdDebug(13030)<<"local file!"<<endl;
         fi = new QFileInfo(url.path());
         if (!fi->exists()) {
           kdDebug(13030)<<QString("createView(): ABORTING: %1 dosen't exist").arg(url.path())<<endl;
           return false;
         }
      }
      ((KantMainWindow*)topLevelWidget())->fileOpenRecent->addURL( KURL( url.prettyURL() ) );
    }
    doc = docManager->createDoc ();//fi);
  }
  else
  {
    if (!doc)
      doc = (KantDocument *)origView->doc();
  }

  // create view
  KantView *view = new KantView (doc, this, (QString("KantViewIface%1").arg(myViewID)).latin1(), false, false);
  connect(view,SIGNAL(newStatus()),this,SLOT(setWindowCaption()));
  myViewID++;
  viewList.append (view);

  KConfig *config = ((KantMainWindow*)topLevelWidget())->config;
  config->setGroup("kwrite");
  doc->readConfig( config );
  view->readConfig( config );

  if (!newDoc && origView)
    view->copySettings(origView);

  if (newDoc)
  {
    view->newDoc();

    if (!url.isEmpty())
    {
      view->doc()->openURL ( url );

      if (url.filename() != 0) {
        QString name = url.filename();

        // anders avoid two views w/ same caption
        QListIterator<KantView> it (viewList);
        int hassamename = 0;
        for (; it.current(); ++it)
        {
           if ( it.current()->doc()->url().filename().compare( name ) == 0 )
             hassamename++;
        }

        if (hassamename > 1)
          name = QString(name+"<%1>").arg(hassamename);

        ((KantDocument *)view->doc())->setDocName (name);
      }
      else
      {
        ((KantDocument *)view->doc())->setDocName (i18n("Untitled %1").arg(doc->docID()));
      }
    }
    else
    {
      ((KantDocument *)view->doc())->setDocName (i18n("Untitled %1").arg(doc->docID()));
    }
  }
  else
  {
    ((KantDocument *)view->doc())->setDocName (doc->docName ());
  }

  view->installPopup ((QPopupMenu*)((KMainWindow *)topLevelWidget ())->factory()->container("view_popup", (KMainWindow *)topLevelWidget ()) );
  connect(view,SIGNAL(newCurPos()),this,SLOT(statusMsgOther()));
  connect(view,SIGNAL(newStatus()),this,SLOT(statusMsgOther()));
  connect(view, SIGNAL(newUndo()), this, SLOT(statusMsgOther()));
  connect(view,SIGNAL(statusMsg(const QString &)),this,SLOT(statusMsg(const QString &)));
  connect(view,SIGNAL(dropEventPass(QDropEvent *)), (KMainWindow *)topLevelWidget (),SLOT(slotDropEvent(QDropEvent *)));
  connect(view,SIGNAL(gotFocus(KantView *)),this,SLOT(activateSpace(KantView *)));

  activeViewSpace()->addView( view );
  activateView( view );

  return true;
}

bool KantViewManager::deleteView (KantView *view, bool force, bool delViewSpace, bool createNew)
{
  if (!view) return true;

  KantViewSpace *viewspace = (KantViewSpace *)view->parentWidget()->parentWidget();

  bool removeDoc = false;
  if (view->isLastView())
  {
    // If !force query user to close doc
    if (!force)
    {
      if (!view->canDiscard())
        return false;
    }

    removeDoc = true;
  }

  // clear caption of mainwindow if this is the current view ;-)
  if ( view == activeView() )
    topLevelWidget()->setCaption ( "" );

  viewspace->removeView (view);

  // Remove hole doc
  KantDocument *dDoc = 0L;
  if (removeDoc)
    dDoc = (KantDocument *) view->doc();

  // remove view from list and memory !!
  viewList.remove (view);

  if (removeDoc)
    docManager->deleteDoc (dDoc);

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  if (createNew && (viewList.count() < 1))
    createView (true, 0L, 0L);

  return true;
}

KantViewSpace* KantViewManager::activeViewSpace ()
{
  QListIterator<KantViewSpace> it(viewSpaceList);
  for (; it.current(); ++it)
  {
    if ( it.current()->isActiveSpace() )
      return it.current();
  }

  if (viewSpaceList.count() > 0)
  {
    viewSpaceList.first()->setActive( true );
    return viewSpaceList.first();
  }

  return 0L;
}

KantView* KantViewManager::activeView ()
{
  QListIterator<KantView> it(viewList);
  for (; it.current(); ++it)
  {
    if ( it.current()->isActive() )
      return it.current();
  }

  // if we get to here, no view isActive()
  // first, try to get one from activeViewSpace()
  KantViewSpace* vs;
  if ( (vs = activeViewSpace()) ) {
    //kdDebug(13030)<<"Attempting to pick a view from activeViewSpace()"<<endl;
    if ( vs->currentView() ) {
      vs->currentView()->setActive( true );
      return vs->currentView();
    }
  }

  // last attempt: just pick first
  if (viewList.count() > 0)
  {
   //kdDebug(13030)<<"desperately choosing first view!"<<endl;
   viewList.first()->setActive( true );
    return viewList.first();
  }

  // no views exists!
  return 0L;
}

void KantViewManager::setActiveSpace ( KantViewSpace* vs )
{
   if (activeViewSpace())
     activeViewSpace()->setActive( false );

   vs->setActive( true, viewSpaceCount() > 1 );
}

void KantViewManager::setActiveView ( KantView* view )
{
//kdDebug(13030)<<QString("setActiveView(): " + view->doc()->url().filename())<<endl;
   if (activeView())
     activeView()->setActive( false );

   view->setActive( true );
}

void KantViewManager::activateSpace (KantView* v)
{
  if (!v) return;

  KantViewSpace* vs = (KantViewSpace*)v->parentWidget()->parentWidget();

  if (!vs->isActiveSpace()) {
    setActiveSpace (vs);
    activateView(v);
  }
}

void KantViewManager::activateView ( KantView *view )
{
  if (!view) return;
//kdDebug(13030)<<"activateView"<<endl;
  ((KantDocument*)view->doc())->isModOnHD();
  if (!view->isActive())
  {
    if ( !activeViewSpace()->showView (view) )
    {
      // since it wasn't found, give'em a new one
      kdDebug(13030)<<"Sometimes things aren't what they seem..."<<endl;
      createView (false, 0L, view );
      return;
    }
//kdDebug(13030)<<"setting view as active"<<endl;
    setActiveView (view);
    viewList.findRef (view);

    setWindowCaption();
    statusMsgOther();

    emit viewChanged ();
  }
}

void KantViewManager::activateView( int docID )
{
  if ( activeViewSpace()->showView(docID) ) {
    activateView( activeViewSpace()->currentView() );
  }
  else
  {
    QListIterator<KantView> it(viewList);
    for ( ;it.current(); ++it)
    {
      if ( ( (KantDocument *)it.current()->doc() )->docID() == docID  )
      {
        createView( false, 0L, it.current() );
        //activateView( current );
        return;
      }
    }

    createView (false, 0L, 0L, docManager->docWithID(docID));
  }
}

long KantViewManager::viewCount ()
{
  return viewList.count();
}

long KantViewManager::viewSpaceCount ()
{
  return viewSpaceList.count();
}

void KantViewManager::slotViewChanged()
{
  if ( activeView() && !activeView()->hasFocus())
    activeView()->setFocus();
}

void KantViewManager::activateNextView()
{
  viewList.findRef (activeView());
  viewList.next();

  if (viewList.current())
    viewList.current()->setFocus();
  else
    viewList.first();
}

void KantViewManager::activatePrevView()
{
  viewList.findRef (activeView());
  viewList.prev();

  if (viewList.current())
    viewList.current()->setFocus();
  else
    viewList.last();
}

void KantViewManager::deleteLastView ()
{
  deleteView (activeView (), true, true, false);
}

void KantViewManager::statusMsg (const QString &msg)
{
  if (activeView() == 0) return;
  bool readOnly =  activeView()->isReadOnly();
  int config =  activeView()->config();

  int ovr = 0;
  if (readOnly)
    ovr = 0;
  else
  {
    if (config & KantView::cfOvr)
    {
      ovr=1;
    }
    else
    {
      ovr=2;
    }
  }

  int mod = 0;
  if (activeView()->isModified())
    mod = 1;
  // anders: since activeView() loops, lets ask it only once!
  KantView* v = activeView();
  emit statusChanged (v, v->currentLine() + 1, v->currentColumn() + 1, ovr, mod, QString(msg));
  emit statChanged ();
}

void KantViewManager::statusMsgOther ()
{
  if (activeView() == 0) return;
  // anders: since activeView() loops, lets ask it only once!
  KantView* v = activeView();

  bool readOnly =  v->isReadOnly();
  int config =  v->config();

  int ovr = 0;
  if (readOnly)
    ovr = 0;
  else
  {
    if (config & KantView::cfOvr)
    {
      ovr=1;
    }
    else
    {
      ovr=2;
    }
  }

  int mod = (int)v->isModified();

  QString  msg = v->doc()->url().prettyURL();
  if (msg.isEmpty())
    msg = ((KantDocument *)v->doc())->docName();

  emit statusChanged (v, v->currentLine() + 1, v->currentColumn() + 1, ovr, mod, msg);
  emit statChanged ();
}

void KantViewManager::slotWindowNext()
{
  long id = docManager->findDoc ((KantDocument *) activeView ()->doc()) - 1;

  if (id < 0)
    id =  docManager->docCount () - 1;

  activateView (docManager->nthDoc(id)->docID());
}

void KantViewManager::slotWindowPrev()
{
  long id = docManager->findDoc ((KantDocument *) activeView ()->doc()) + 1;

  if (id >= docManager->docCount () )
    id = 0;

  activateView (docManager->nthDoc(id)->docID());
}

void KantViewManager::slotDocumentNew ()
{
  createView (true, 0L, 0L);
}

void KantViewManager::slotDocumentOpen ()
{
  KURL::List urls = KFileDialog::getOpenURLs(QString::null, QString::null, 0L, i18n("Open File..."));

  for (KURL::List::Iterator i=urls.begin(); i != urls.end(); ++i)
  {
    openURL( *i );
  }
}

void KantViewManager::slotDocumentSave ()
{
  if (activeView() == 0) return;

  KantView *current = activeView();

  if( current->doc()->isModified() )
  {
    if( !current->doc()->url().isEmpty() && current->doc()->isReadWrite() )
	{
           current->doc()->save();
	}
    else
      slotDocumentSaveAs();
  }
}

void KantViewManager::slotDocumentSaveAll ()
{
  QListIterator<KantView> it(viewList);
  for ( ;it.current(); ++it)
  {
    KantView* current = it.current();
    if( current->doc()->isModified() ) {
      if( !current->doc()->url().isEmpty() && current->doc()->isReadWrite() )
        {
          current->doc()->save();
        }
      else
        slotDocumentSaveAs();
    }
  }
}

void KantViewManager::slotDocumentSaveAs ()
{
  if (activeView() == 0) return;

  KantView *current = activeView();

  KURL url = KFileDialog::getSaveURL(QString::null, QString::null, 0L, i18n("Save File..."));

  if( !url.isEmpty() )
  {
    current->doc()->saveAs( url );
    ((KantDocument *)current->doc())->setDocName (url.filename());

    setWindowCaption();
  }
}

void KantViewManager::slotDocumentClose ()
{
  if (!activeView()) return;

  QList<KantView> closeList;
  long docID = ((KantDocument *)activeView()->doc())->docID();


  for (uint i=0; i < ((KantApp *)kapp)->mainWindowsCount (); i++ )
  {
    for (uint z=0 ; z < ((KantApp *)kapp)->mainWindows.at(i)->viewManager->viewList.count(); z++)
    {
      KantView* current = ((KantApp *)kapp)->mainWindows.at(i)->viewManager->viewList.at(z);
      if ( ((KantDocument *)current->doc())->docID() == docID )
      {
        closeList.append (current);
      }
    }

    bool done = false;
    while ( closeList.at(0) )
    {
      KantView *view = closeList.at(0);
      done = ((KantApp *)kapp)->mainWindows.at(i)->viewManager->deleteView (view);
      closeList.remove (view);

      if (!done) return;
    }
  }

  emit viewChanged ();
}

void KantViewManager::slotDocumentCloseAll ()
{
  if (docManager->docCount () == 0) return;

  QList<KantView> closeList;

  for (uint i=0; i < ((KantApp *)kapp)->mainWindowsCount (); i++ )
  {
    for (uint z=0 ; z < ((KantApp *)kapp)->mainWindows.at(i)->viewManager->viewList.count(); z++)
    {
      KantView* current = ((KantApp *)kapp)->mainWindows.at(i)->viewManager->viewList.at(z);
      closeList.append (current);
    }

    bool done = false;
    while ( closeList.at(0) )
    {
      KantView *view = closeList.at(0);
      done = ((KantApp *)kapp)->mainWindows.at(i)->viewManager->deleteView (view);
      closeList.remove (view);

      if (!done) return;
    }
  }
}

void KantViewManager::slotUndo ()
{
  if (activeView() == 0) return;

  activeView()->undo();
}

void KantViewManager::slotRedo ()
{
  if (activeView() == 0) return;

  activeView()->redo();
}

void KantViewManager::slotUndoHistory ()
{
  if (activeView() == 0) return;

  activeView()->undoHistory();
}

void KantViewManager::slotCut ()
{
  if (activeView() == 0) return;

  activeView()->cut();
}

void KantViewManager::slotCopy ()
{
  if (activeView() == 0) return;

  activeView()->copy();
}

void KantViewManager::slotPaste ()
{
  if (activeView() == 0) return;

  activeView()->paste();
}

void KantViewManager::slotSelectAll ()
{
  if (activeView() == 0) return;

  activeView()->selectAll();
}

void KantViewManager::slotDeselectAll ()
{
  if (activeView() == 0) return;

  activeView()->deselectAll();
}

void KantViewManager::slotInvertSelection ()
{
  if (activeView() == 0) return;

  activeView()->invertSelection();
}

void KantViewManager::slotFind ()
{
  if (activeView() == 0) return;

  activeView()->find();
}

void KantViewManager::slotFindAgain ()
{
  if (activeView() == 0) return;

  activeView()->findAgain();
}

void KantViewManager::slotFindAgainB ()
{
  if (activeView() == 0) return;

  activeView()->searchAgain(true);
}


void KantViewManager::slotReplace ()
{
  if (activeView() == 0) return;

  activeView()->replace();
}

void KantViewManager::slotIndent()
{
  KantView* v = activeView();
  if (v)
    v->indent();

}

void KantViewManager::slotUnIndent()
{
  KantView* v = activeView();
  if (v)
    v->unIndent();

}

void KantViewManager::slotInsertFile ()
{
  if (activeView() == 0) return;

  activeView()->insertFile();
}

void KantViewManager::slotSpellcheck ()
{
  if (activeView() == 0) return;

  activeView()->spellcheck();
}

void KantViewManager::slotGotoLine ()
{
  if (activeView() == 0) return;

  activeView()->gotoLine();
}

void KantViewManager::setEol(int which)
{
  if (activeView())
    activeView()->setEol( which );
}

void KantViewManager::slotSetHl (int n)
{
  if (activeView() == 0) return;

  activeView()->setHl(n);
}

void KantViewManager::addBookmark ()
{
  if (activeView() == 0) return;

  activeView()->addBookmark();
}

void KantViewManager::setBookmark ()
{
  if (activeView() == 0) return;

  activeView()->setBookmark();
}

void KantViewManager::clearBookmarks ()
{
  if (activeView() == 0) return;

  activeView()->clearBookmarks();
}

void KantViewManager::openURL (KURL url)
{
  if ( !docManager->isOpen( url ) )
    createView (true, url, 0L);
  else
    activateView( docManager->findDoc( url ) );
}

void KantViewManager::openConstURL (const KURL& url)
{
  if ( !docManager->isOpen( url ) )
    createView (true, url, 0L);
  else
    activateView( docManager->findDoc( url ) );
}

void KantViewManager::printNow()
{
  if (!activeView()) return;
  activeView()->printDlg ();
}

void KantViewManager::printDlg()
{
  if (!activeView()) return;
  activeView()->printDlg ();
}

void KantViewManager::splitViewSpace( KantViewSpace* vs,
                                      bool isHoriz,
                                      bool atTop,
                                      KURL newViewUrl)
{
  if (!activeView()) return;
  if (!vs)
    /*KantViewSpace**/ vs = activeViewSpace();
  bool isFirstTime = vs->parentWidget() == this;

  QValueList<int> sizes;
  if (! isFirstTime)
    sizes = ((KantSplitter*)vs->parentWidget())->sizes();

  Qt::Orientation o = isHoriz ? Qt::Vertical : Qt::Horizontal;
  KantSplitter* s = new KantSplitter(o, vs->parentWidget());
  s->setOpaqueResize( useOpaqueResize );

  if (! isFirstTime) {
    // anders: make sure the split' viewspace is allways
    // correctly positioned.
    // If viewSpace is the first child, the new splitter must be moveToFirst'd
    if ( !((KantSplitter*)vs->parentWidget())->isLastChild( vs ) )
       ((KantSplitter*)s->parentWidget())->moveToFirst( s );
  }
  vs->reparent( s, 0, QPoint(), true );
  KantViewSpace* vsNew = new KantViewSpace( s );

  if (atTop)
    s->moveToFirst( vsNew );

  if (isFirstTime)
    grid->addWidget(s, 0, 0);
  else
    ((KantSplitter*)s->parentWidget())->setSizes( sizes );

  sizes.clear();
  int sz = isHoriz ? s->height()/2 : s->width()/2;
  sizes.append(sz);
  sizes.append(sz);
  s->setSizes( sizes );

  s->show();

  connect(this, SIGNAL(statusChanged(KantView *, int, int, int, int, QString)), vsNew, SLOT(slotStatusChanged(KantView *, int, int, int, int, QString)));
  viewSpaceList.append( vsNew );
  vsNew->installEventFilter( this );
  activeViewSpace()->setActive( false );
  vsNew->setActive( true, true );
  vsNew->show();
  if (!newViewUrl.isValid())
    createView (false, 0L, (KantView *)activeView());
  else {
    // tjeck if doc is allready open
    long aDocId;
    if ( (aDocId = docManager->findDoc( newViewUrl )) )
      createView (false, 0L, 0L, docManager->docWithID( aDocId) );
    else
      createView( true, newViewUrl );
  }
}

void KantViewManager::removeViewSpace (KantViewSpace *viewspace)
{
  // abort if viewspace is 0
  if (!viewspace) return;

  // abort if this is the last viewspace
  if (viewSpaceList.count() < 2) return;

  KantSplitter* p = (KantSplitter*)viewspace->parentWidget();

  // find out if it is the first child for repositioning
  // see below
  bool pIsFirst = false;

  // save some size information
  KantSplitter* pp=0L;
  QValueList<int> ppsizes;
  if (viewSpaceList.count() > 2 && p->parentWidget() != this)
  {
    pp = (KantSplitter*)p->parentWidget();
    ppsizes = pp->sizes();
    pIsFirst = !pp->isLastChild( p ); // simple logic, right-
  }

  // Figure out where to put views that are still needed
  KantViewSpace* next;
  if (viewSpaceList.find(viewspace) == 0)
    next = viewSpaceList.next();
  else
    next = viewSpaceList.prev();

  // Reparent views in viewspace that are last views, delete the rest.
  int vsvc = viewspace->viewCount();
  while (vsvc > 0)
  {
    if (viewspace->currentView())
    {
      //kdDebug(13030)<<QString("removeViewSpace(): %1 views left").arg(vsvc)<<endl;
      KantView* v = viewspace->currentView();

      if (v->isLastView())
      {
        viewspace->removeView(v);
        next->addView( v, false );
      }
      else
      {
        deleteView( v, false, false );
        //kdDebug(13030)<<"KantViewManager::removeViewSpace(): deleting a view: "<<v->doc()->url().filename()<<endl;
      }
    }
    vsvc = viewspace->viewCount();
  }

  viewSpaceList.remove( viewspace );

  // reparent the other sibling of the parent.
  while (p->children ())
  {
    //kdDebug(13030)<<"KantViewManager::removeViewSpace(): reparenting something"<<endl;
    QWidget* other = ((QWidget *)(( QList<QObject>*)p->children())->first());
    other->reparent( p->parentWidget(), 0, QPoint(), true );
    // We also need to find the right viewspace to become active,
    // and if "other" is the last, we move it into the grid.
    if (pIsFirst)
       ((KantSplitter*)p->parentWidget())->moveToFirst( other );
    if ( other->isA("KantViewSpace") ) {
      setActiveSpace( (KantViewSpace*)other );
      if (viewSpaceList.count() == 1)
        grid->addWidget( other, 0, 0);
    }
    else {
      QObjectList* l = other->queryList( "KantViewSpace" );
      if ( l->first() != 0 ) { // I REALLY hope so!
        setActiveSpace( (KantViewSpace*)l->first() );
      }
      delete l;
    }
  }

  delete p;

  if (!ppsizes.isEmpty())
    pp->setSizes( ppsizes );

  // find the view that is now active.
  KantView* v = activeViewSpace()->currentView();
  if ( v ) {
    //kdDebug(13030)<<"removeViewSpace(): setting active view: "<<v->doc()->url().filename()<<endl;
    activateView( v );
  }

  emit viewChanged();
}

void KantViewManager::slotCloseCurrentViewSpace()
{
  removeViewSpace(activeViewSpace());
}

void KantViewManager::setWindowCaption()
{
  if (activeView())
  {
    QString c;
    if (activeView()->doc()->url().isEmpty() || (! showFullPath))
      c = ((KantDocument *)activeView()->doc())->docName();
    else
      c = activeView()->doc()->url().prettyURL();

    ((KantMainWindow*)topLevelWidget())->setCaption( c,activeView()->isModified());
  }
}

void KantViewManager::setShowFullPath( bool enable )
{
  showFullPath = enable;
  setWindowCaption();
}

void KantViewManager::setUseOpaqueResize( bool enable )
{
  useOpaqueResize = enable;
  // TODO: loop through splitters and set this prop
}

void KantViewManager::reloadCurrentDoc()
{
  if (! activeView() )
    return;
  if (! activeView()->canDiscard())
    return;
  KantView* v = activeView();
  // save cursor position
  int cl = v->currentLine();
  int cc = v->currentColumn();
  // save bookmarks
  ((KantDocument*)v->doc())->reloadFile();
  if (v->numLines() >= cl)
    v->setCursorPosition( cl, cc );
}

///////////////////////////////////////////////////////////
// session config functions
///////////////////////////////////////////////////////////

void KantViewManager::saveAllDocsAtCloseDown()
{
  QValueList<long> seen;
  KantView* v;
  int id;
  QStringList list;
  int vc = viewCount();
  uint i = 0;
  KSimpleConfig* scfg = new KSimpleConfig("kantsessionrc", false);
  while ( i <= vc )
  {
    v = activeView();
    id =  ((KantDocument*)v->doc())->docID();
    // save to config if not seen
    if ( ! seen.contains( id ) && ! v->doc()->url().isEmpty() ) {
      seen.append( id );

      scfg->setGroup( v->doc()->url().prettyURL() );
      v->writeSessionConfig(scfg);
      v->doc()->writeSessionConfig(scfg);
      // TODO: should we tjeck for local file here?
      // TODO: LASTMOD
      // write entry
      scfg->setGroup("open files");
      scfg->writeEntry( QString("File%1").arg(id), v->doc()->url().prettyURL() );
      list.append( QString("File%1").arg(id) );
    }
    if( ! deleteView( v ) )
      return;  // this will hopefully never happen, since - WHAT THEN???
    i++;
  }
  scfg->setGroup("open files");
  scfg->writeEntry( "list", list );
  scfg->sync();
}

void KantViewManager::reopenDocuments(bool isRestore)
{
  KSimpleConfig* scfg = new KSimpleConfig("kantsessionrc", false);
  KConfig* config = kapp->config();
  config->setGroup("General");
  bool resVC = config->readBoolEntry("restore views", true);
  ////////////////////////////////////////////////////////////////////////
  // RESTORE VIEW CONFIG TEST
  if ( scfg->hasGroup("splitter0") && (isRestore || resVC) ) {
    kdDebug(13030)<<"reopenDocumentw(): calling restoreViewConfig()"<<endl;

    // commented out - tooooooooo buggy
    //restoreViewConfig();
    return;
  }
  ////////////////////////////////////////////////////////////////////////
  // read the list and loop around it.
  scfg->setGroup("open files");
  if (config->readBoolEntry("reopen at startup", true) || isRestore )
  {
    QStringList list = /*config*/scfg->readListEntry("list");

    for ( int i = list.count() - 1; i > -1; i-- ) {
      scfg->setGroup("open files");
      QString fn = scfg->readEntry(list[i]);
      openURL( KURL( fn ) );
      scfg->setGroup( fn );
      KantView* v = activeView();
      if (v)
      {
        v->readSessionConfig( scfg );
        v->doc()->readSessionConfig( scfg );
      }
      scfg->deleteGroup( fn );
    }
  }
  // truncate sessionconfig file.
  scfg->deleteGroup("open files");
  scfg->sync();
}

void KantViewManager::saveViewSpaceConfig()
{
   if (viewSpaceCount() == 1) {
     kdDebug(13030)<<"saveViewSpaceConfig(): only one vs, aborting"<<endl;
     return;
   }

   KSimpleConfig* scfg = new KSimpleConfig("kantsessionrc", false);

   // I need the first splitter, the one which has this as parent.
   KantSplitter* s;
   QObjectList *l = queryList("KantSplitter", 0, false, false);
   QObjectListIt it( *l );
   if ( (s = (KantSplitter*)it.current()) != 0 )
     saveSplitterConfig( s, 0, scfg );
   else
     kdDebug(13030)<<"saveViewSpaceConfig(): PANIC! can't find starting point!"<<endl;
   delete l;

   scfg->sync();
}

void KantViewManager::saveSplitterConfig( KantSplitter* s, int idx, KSimpleConfig* config )
{
   QString grp = QString("splitter%1").arg(idx);
   config->setGroup(grp);

   // Save sizes, orient, children for this splitter
   config->writeEntry( "sizes", s->sizes() );
   config->writeEntry( "orientation", s->orientation() );

   QStringList childList;
   // a kantsplitter has two children, of which one may be a KantSplitter.
   const QObjectList* l = s->children();
   QObjectListIt it( *l );
   QObject* obj;
   for (; it.current(); ++it) {
     obj = it.current();
     QString n;  // name for child list, see below
     // For KantViewSpaces, ask them to save the file list.
     if ( obj->isA("KantViewSpace") ) {
       n = QString("viewspace%1").arg( viewSpaceList.find((KantViewSpace*)obj) );
       ((KantViewSpace*)obj)->saveFileList( config, viewSpaceList.find((KantViewSpace*)obj) );
       // save active viewspace
       if ( ((KantViewSpace*)obj)->isActiveSpace() ) {
         config->setGroup("general");
         config->writeEntry("activeviewspace", viewSpaceList.find((KantViewSpace*)obj) );
       }
     }
     // For KantSplitters, recurse
     else if ( obj->isA("KantSplitter") ) {
       idx++;
       saveSplitterConfig( (KantSplitter*)obj, idx, config);
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

void KantViewManager::restoreViewConfig()
{
   // This is called *instead* of reopenDocuments if view config needs to be restored.
   // Consequently, it has the task of opening all documents.
   KSimpleConfig* scfg = new KSimpleConfig("kantsessionrc", false);
   // if group splitter0 does not exist, call reopenDocuments() and return
   if ( ! scfg->hasGroup("splitter0") ) {
     //reopenDocuments();
     return;
   }
   // remove the initial viewspace.
   viewSpaceList.clear();
   // call restoreSplitter for splitter0
   restoreSplitter( scfg, QString("splitter0"), this );
   // finally, make the correct view active.
   scfg->setGroup("general");
   activateSpace( viewSpaceList.at( scfg->readNumEntry("activeviewspace") )->currentView() );
   scfg->deleteGroup("general");
   if ( scfg->hasGroup("open files") )
     scfg->deleteGroup("open files");
   scfg->sync();
}

void KantViewManager::restoreSplitter( KSimpleConfig* config, QString group, QWidget* parent)
{
   config->setGroup( group );

   // create a splitter with orientation
   kdDebug(13030)<<"restoreSplitter():creating a splitter: "<<group<<endl;
   if (parent == this)
     kdDebug(13030)<<"parent is this"<<endl;
   KantSplitter* s = new KantSplitter((Qt::Orientation)config->readNumEntry("orientation"), parent);
   if ( group.compare("splitter0") == 0 )
     grid->addWidget(s, 0, 0);

   QStringList children = config->readListEntry( "children" );
   for (QStringList::Iterator it=children.begin(); it!=children.end(); ++it) {
     // for a viewspace, create it and open all documents therein.
     // TODO: restore cursor position.
     if ( (*it).startsWith("viewspace") ) {
       kdDebug(13030)<<"Adding a viewspace: "<<(*it)<<endl;
       KantViewSpace* vs;
       kdDebug(13030)<<"creating a viewspace for "<<(*it)<<endl;
       vs = new KantViewSpace( s );
       connect(this, SIGNAL(statusChanged(KantView *, int, int, int, int, QString)), vs, SLOT(slotStatusChanged(KantView *, int, int, int, int, QString)));
       vs->installEventFilter( this );
       viewSpaceList.append( vs );
       vs->show();
       setActiveSpace( vs );
       // open documents
       config->setGroup( (*it) );
       int idx = 0;
       QString key = QString("file%1").arg( idx );
       while ( config->hasKey( key ) ) {
         QStringList data = config->readListEntry( key );
         if ( ! docManager->isOpen( KURL(data[0]) ) ) {
           openURL( KURL( data[0] ) );
           config->setGroup( data[0] );
           KantView* v = activeView();
           if (v)
           {
             v->readSessionConfig( config );
             v->doc()->readSessionConfig( config );
           }
           else
           {
             createView (true, 0L, 0L);
           }
           config->deleteGroup( data[0] );
         }
         else { // if the group has been deleted, we can find a document
           kdDebug(13030)<<"document allready open, creating extra view"<<endl;
           // ahem, tjeck if this document actually exists.
           long docID = docManager->findDoc( KURL(data[0]) );
           if (docID >= 0)
             createView( false, 0L, 0L, docManager->nthDoc( docID ) );
         }
         // cursor pos
         KantView* v = activeView();
         v->setCursorPosition( data[1].toInt(), data[2].toInt() );
         idx++;
         key = QString("file%1").arg( idx );
         config->setGroup(*it);
       }
       config->deleteGroup( (*it) );
       // If the viewspace have no documents due to bad luck, create a blank.
       if ( vs->viewCount() < 1)
         createView( true );
     }
     // for a splitter, recurse.
     else if ( (*it).startsWith("splitter") )
       restoreSplitter( config, QString(*it), s );
   }
   // set sizes
   config->setGroup( group );
   kdDebug(13030)<<QString("splitter has %1 children").arg( s->children()->count() )<<endl;
   s->setSizes( config->readIntListEntry("sizes") );
   s->show();
   config->deleteGroup( group );
}
