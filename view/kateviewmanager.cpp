/***************************************************************************
                          kateviewmanager.cpp  -  description
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

#include "kateviewmanager.h"
#include "kateviewmanager.moc"

#include "../mainwindow/katemainwindow.h"
#include "../mainwindow/kateIface.h"
#include "../document/katedocmanager.h"
#include "../document/katedocument.h"
#include "../app/kateapp.h"
#include "kateview.h"
#include "kateviewspace.h"

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


#include "katesplitter.h"

uint KateViewManager::myViewID = 0;

KateViewManager::KateViewManager (QWidget *parent, KateDocManager *docManager) : KateViewManagerIface  (parent)
{
  // no memleaks
  viewList.setAutoDelete(true);
  viewSpaceList.setAutoDelete(true);

  this->docManager = docManager;

  // sizemanagment
  grid = new QGridLayout( this, 1, 1 );

  KateViewSpace* vs = new KateViewSpace( this );
  connect(this, SIGNAL(statusChanged(KateView *, int, int, int, int, QString)), vs, SLOT(slotStatusChanged(KateView *, int, int, int, int, QString)));
  vs->setActive( true );
  vs->installEventFilter( this );
  grid->addWidget( vs, 0, 0);
  viewSpaceList.append(vs);

  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
}

KateViewManager::~KateViewManager ()
{
  viewList.setAutoDelete(false);
  viewSpaceList.setAutoDelete(false);
}

bool KateViewManager::createView ( bool newDoc, KURL url, KateView *origView, KateDocument *doc )
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
      ((KateMainWindow*)topLevelWidget())->fileOpenRecent->addURL( KURL( url.prettyURL() ) );
    }
    doc = docManager->createDoc ();//fi);
  }
  else
  {
    if (!doc)
      doc = (KateDocument *)origView->doc();
  }

  // create view
  KateView *view = new KateView (doc, this, (QString("KateView%1-%2").arg(doc->docID()).arg(myViewID)).latin1(), false, false);
  connect(view,SIGNAL(newStatus()),this,SLOT(setWindowCaption()));
  myViewID++;
  viewList.append (view);

  KConfig *config = ((KateMainWindow*)topLevelWidget())->config;
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
        QListIterator<KateView> it (viewList);
        int hassamename = 0;
        for (; it.current(); ++it)
        {
           if ( it.current()->doc()->url().filename().compare( name ) == 0 )
             hassamename++;
        }

        if (hassamename > 1)
          name = QString(name+"<%1>").arg(hassamename);

        ((KateDocument *)view->doc())->setDocName (name);
      }
      else
      {
        ((KateDocument *)view->doc())->setDocName (i18n("Untitled %1").arg(doc->docID()));
      }
    }
    else
    {
      ((KateDocument *)view->doc())->setDocName (i18n("Untitled %1").arg(doc->docID()));
    }
  }
  else
  {
    ((KateDocument *)view->doc())->setDocName (doc->docName ());
  }

  view->installPopup ((QPopupMenu*)((KMainWindow *)topLevelWidget ())->factory()->container("view_popup", (KMainWindow *)topLevelWidget ()) );
  connect(view,SIGNAL(newCurPos()),this,SLOT(statusMsgOther()));
  connect(view,SIGNAL(newStatus()),this,SLOT(statusMsgOther()));
  connect(view, SIGNAL(newUndo()), this, SLOT(statusMsgOther()));
  connect(view,SIGNAL(statusMsg(const QString &)),this,SLOT(statusMsg(const QString &)));
  connect(view,SIGNAL(dropEventPass(QDropEvent *)), (KMainWindow *)topLevelWidget (),SLOT(slotDropEvent(QDropEvent *)));
  connect(view,SIGNAL(gotFocus(KateView *)),this,SLOT(activateSpace(KateView *)));

  activeViewSpace()->addView( view );
  activateView( view );

  return true;
}

bool KateViewManager::deleteView (KateView *view, bool force, bool delViewSpace, bool createNew)
{
  if (!view) return true;

  KateViewSpace *viewspace = (KateViewSpace *)view->parentWidget()->parentWidget();

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
  KateDocument *dDoc = 0L;
  if (removeDoc)
    dDoc = (KateDocument *) view->doc();

  // remove view from list and memory !!
  viewList.remove (view);

  if (removeDoc)
    docManager->deleteDoc (dDoc);

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  if (createNew && (viewList.count() < 1) && (docManager->docCount() < 1) )
    createView (true, 0L, 0L);
  else if (createNew && (viewList.count() < 1) && (docManager->docCount() > 0) )
    createView (false, 0L, 0L, docManager->nthDoc(docManager->docCount()-1));

  return true;
}

KateViewSpace* KateViewManager::activeViewSpace ()
{
  QListIterator<KateViewSpace> it(viewSpaceList);
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

KateView* KateViewManager::activeView ()
{
  QListIterator<KateView> it(viewList);
  for (; it.current(); ++it)
  {
    if ( it.current()->isActive() )
      return it.current();
  }

  // if we get to here, no view isActive()
  // first, try to get one from activeViewSpace()
  KateViewSpace* vs;
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

void KateViewManager::setActiveSpace ( KateViewSpace* vs )
{
   if (activeViewSpace())
     activeViewSpace()->setActive( false );

   vs->setActive( true, viewSpaceCount() > 1 );
}

void KateViewManager::setActiveView ( KateView* view )
{
//kdDebug(13030)<<QString("setActiveView(): " + view->doc()->url().filename())<<endl;
   if (activeView())
     activeView()->setActive( false );

   view->setActive( true );
}

void KateViewManager::activateSpace (KateView* v)
{
  if (!v) return;

  KateViewSpace* vs = (KateViewSpace*)v->parentWidget()->parentWidget();

  if (!vs->isActiveSpace()) {
    setActiveSpace (vs);
    activateView(v);
  }
}

void KateViewManager::activateView ( KateView *view )
{
  if (!view) return;
//kdDebug(13030)<<"activateView"<<endl;
  ((KateDocument*)view->doc())->isModOnHD();
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

void KateViewManager::activateView( uint docID )
{
  if ( activeViewSpace()->showView(docID) ) {
    activateView( activeViewSpace()->currentView() );
  }
  else
  {
    QListIterator<KateView> it(viewList);
    for ( ;it.current(); ++it)
    {
      if ( ( (KateDocument *)it.current()->doc() )->docID() == docID  )
      {
        createView( false, 0L, it.current() );
        //activateView( current );
        return;
      }
    }

    createView (false, 0L, 0L, docManager->docWithID(docID));
  }
}

uint KateViewManager::viewCount ()
{
  return viewList.count();
}

uint KateViewManager::viewSpaceCount ()
{
  return viewSpaceList.count();
}

void KateViewManager::slotViewChanged()
{
  if ( activeView() && !activeView()->hasFocus())
    activeView()->setFocus();
}

void KateViewManager::activateNextView()
{
  uint i = viewSpaceList.find (activeViewSpace())+1;
 
  if (i >= viewSpaceList.count())
    i=0;

  setActiveSpace (viewSpaceList.at(i));
  activateView(viewSpaceList.at(i)->currentView());
}

void KateViewManager::activatePrevView()
{
  int i = viewSpaceList.find (activeViewSpace())-1;
 
  if (i < 0)
    i=viewSpaceList.count()-1;

  setActiveSpace (viewSpaceList.at(i));
  activateView(viewSpaceList.at(i)->currentView());  
}

void KateViewManager::deleteLastView ()
{
  deleteView (activeView (), true, true, false);
}

void KateViewManager::statusMsg (const QString &msg)
{
  if (activeView() == 0) return;
  bool readOnly =  activeView()->isReadOnly();
  int config =  activeView()->config();

  int ovr = 0;
  if (readOnly)
    ovr = 0;
  else
  {
    if (config & KateView::cfOvr)
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
  KateView* v = activeView();
  emit statusChanged (v, v->currentLine() + 1, v->currentColumn() + 1, ovr, mod, QString(msg));
  emit statChanged ();
}

void KateViewManager::statusMsgOther ()
{
  if (activeView() == 0) return;
  // anders: since activeView() loops, lets ask it only once!
  KateView* v = activeView();

  bool readOnly =  v->isReadOnly();
  int config =  v->config();

  int ovr = 0;
  if (readOnly)
    ovr = 0;
  else
  {
    if (config & KateView::cfOvr)
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
    msg = ((KateDocument *)v->doc())->docName();

  emit statusChanged (v, v->currentLine() + 1, v->currentColumn() + 1, ovr, mod, msg);
  emit statChanged ();
}

void KateViewManager::slotWindowNext()
{
  int id = docManager->findDoc ((KateDocument *) activeView ()->doc()) - 1;

  if (id < 0)
    id =  docManager->docCount () - 1;

  activateView (docManager->nthDoc(id)->docID());
}

void KateViewManager::slotWindowPrev()
{
  uint id = docManager->findDoc ((KateDocument *) activeView ()->doc()) + 1;

  if (id >= docManager->docCount () )
    id = 0;

  activateView (docManager->nthDoc(id)->docID());
}

void KateViewManager::slotDocumentNew ()
{
  createView (true, 0L, 0L);
}

void KateViewManager::slotDocumentOpen ()
{
  QString path = QString::null;
  if (activeView())
    path = activeView()->doc()->url().url();

  KURL::List urls = KFileDialog::getOpenURLs(path, QString::null, 0L, i18n("Open File..."));

  for (KURL::List::Iterator i=urls.begin(); i != urls.end(); ++i)
  {
    openURL( *i );
  }
}

void KateViewManager::slotDocumentSave ()
{
  if (activeView() == 0) return;

  KateView *current = activeView();

  if( current->doc()->isModified() )
  {
    if( !current->doc()->url().isEmpty() && current->doc()->isReadWrite() )
      current->doc()->save();
    else
      slotDocumentSaveAs();
  }
}

void KateViewManager::slotDocumentSaveAll ()
{
  QListIterator<KateView> it(viewList);
  for ( ;it.current(); ++it)
  {
    KateView* current = it.current();
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

void KateViewManager::slotDocumentSaveAs ()
{
  if (activeView() == 0) return;

  KateView *current = activeView();

  KURL url = KFileDialog::getSaveURL(current->doc()->url().url(), QString::null, 0L, i18n("Save File..."));

  if( !url.isEmpty() )
  {
    current->doc()->saveAs( url );
    ((KateDocument *)current->doc())->setDocName (url.filename());

    setWindowCaption();
  }
}

void KateViewManager::slotDocumentClose ()
{
  if (!activeView()) return;

  QList<KateView> closeList;
  uint docID = ((KateDocument *)activeView()->doc())->docID();


  for (uint i=0; i < ((KateApp *)kapp)->mainWindowsCount (); i++ )
  {
    for (uint z=0 ; z < ((KateApp *)kapp)->mainWindows.at(i)->viewManager->viewList.count(); z++)
    {
      KateView* current = ((KateApp *)kapp)->mainWindows.at(i)->viewManager->viewList.at(z);
      if ( ((KateDocument *)current->doc())->docID() == docID )
      {
        closeList.append (current);
      }
    }

    bool done = false;
    while ( closeList.at(0) )
    {
      KateView *view = closeList.at(0);
      done = ((KateApp *)kapp)->mainWindows.at(i)->viewManager->deleteView (view);
      closeList.remove (view);

      if (!done) return;
    }
  }

  emit viewChanged ();
}

void KateViewManager::slotDocumentCloseAll ()
{
  if (docManager->docCount () == 0) return;

  QList<KateView> closeList;

  for (uint i=0; i < ((KateApp *)kapp)->mainWindowsCount (); i++ )
  {
    for (uint z=0 ; z < ((KateApp *)kapp)->mainWindows.at(i)->viewManager->viewList.count(); z++)
    {
      KateView* current = ((KateApp *)kapp)->mainWindows.at(i)->viewManager->viewList.at(z);
      closeList.append (current);
    }

    bool done = false;
    while ( closeList.at(0) )
    {
      KateView *view = closeList.at(0);
      done = ((KateApp *)kapp)->mainWindows.at(i)->viewManager->deleteView (view);
      closeList.remove (view);

      if (!done) return;
    }
  }
}

void KateViewManager::slotUndo ()
{
  if (activeView() == 0) return;

  activeView()->undo();
}

void KateViewManager::slotRedo ()
{
  if (activeView() == 0) return;

  activeView()->redo();
}

void KateViewManager::slotUndoHistory ()
{
  if (activeView() == 0) return;

  activeView()->undoHistory();
}

void KateViewManager::slotCut ()
{
  if (activeView() == 0) return;

  activeView()->cut();
}

void KateViewManager::slotCopy ()
{
  if (activeView() == 0) return;

  activeView()->copy();
}

void KateViewManager::slotPaste ()
{
  if (activeView() == 0) return;

  activeView()->paste();
}

void KateViewManager::slotSelectAll ()
{
  if (activeView() == 0) return;

  activeView()->selectAll();
}

void KateViewManager::slotDeselectAll ()
{
  if (activeView() == 0) return;

  activeView()->deselectAll();
}

void KateViewManager::slotInvertSelection ()
{
  if (activeView() == 0) return;

  activeView()->invertSelection();
}

void KateViewManager::slotFind ()
{
  if (activeView() == 0) return;

  activeView()->find();
}

void KateViewManager::slotFindAgain ()
{
  if (activeView() == 0) return;

  activeView()->findAgain(false);
}

void KateViewManager::slotFindAgainB ()
{
  if (activeView() == 0) return;

  activeView()->findAgain(true);
}


void KateViewManager::slotReplace ()
{
  if (activeView() == 0) return;

  activeView()->replace();
}

void KateViewManager::slotEditCommand ()
{
  if (activeView() == 0) return;

  activeView()->slotEditCommand();
}

void KateViewManager::slotIndent()
{
  KateView* v = activeView();
  if (v)
    v->indent();

}

void KateViewManager::slotUnIndent()
{
  KateView* v = activeView();
  if (v)
    v->unIndent();

}

void KateViewManager::slotSpellcheck ()
{
  if (activeView() == 0) return;

  activeView()->spellcheck();
}

void KateViewManager::slotGotoLine ()
{
  if (activeView() == 0) return;

  activeView()->gotoLine();
}

void KateViewManager::setEol(int which)
{
  if (activeView())
    activeView()->setEol( which );
}

void KateViewManager::slotSetHl (int n)
{
  if (activeView() == 0) return;

  activeView()->setHl(n);
}

void KateViewManager::addBookmark ()
{
  if (activeView() == 0) return;

  activeView()->addBookmark();
}

void KateViewManager::setBookmark ()
{
  if (activeView() == 0) return;

  activeView()->setBookmark();
}

void KateViewManager::clearBookmarks ()
{
  if (activeView() == 0) return;

  activeView()->clearBookmarks();
}

void KateViewManager::openURL (KURL url)
{
  if ( !docManager->isOpen( url ) )
    createView (true, url, 0L);
  else
    activateView( docManager->findDoc( url ) );
}

void KateViewManager::openConstURL (const KURL& url)
{
  if ( !docManager->isOpen( url ) )
    createView (true, url, 0L);
  else
    activateView( docManager->findDoc( url ) );
}

void KateViewManager::printNow()
{
  if (!activeView()) return;
  activeView()->printDlg ();
}

void KateViewManager::printDlg()
{
  if (!activeView()) return;
  activeView()->printDlg ();
}

void KateViewManager::splitViewSpace( KateViewSpace* vs,
                                      bool isHoriz,
                                      bool atTop,
                                      KURL newViewUrl)
{
  if (!activeView()) return;
  if (!vs)
    /*KateViewSpace**/ vs = activeViewSpace();
  bool isFirstTime = vs->parentWidget() == this;

  QValueList<int> sizes;
  if (! isFirstTime)
    sizes = ((KateSplitter*)vs->parentWidget())->sizes();

  Qt::Orientation o = isHoriz ? Qt::Vertical : Qt::Horizontal;
  KateSplitter* s = new KateSplitter(o, vs->parentWidget());
  s->setOpaqueResize( useOpaqueResize );

  if (! isFirstTime) {
    // anders: make sure the split' viewspace is allways
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
    grid->addWidget(s, 0, 0);
  else
    ((KateSplitter*)s->parentWidget())->setSizes( sizes );

  sizes.clear();
  int sz = isHoriz ? s->height()/2 : s->width()/2;
  sizes.append(sz);
  sizes.append(sz);
  s->setSizes( sizes );

  s->show();

  connect(this, SIGNAL(statusChanged(KateView *, int, int, int, int, QString)), vsNew, SLOT(slotStatusChanged(KateView *, int, int, int, int, QString)));
  viewSpaceList.append( vsNew );
  vsNew->installEventFilter( this );
  activeViewSpace()->setActive( false );
  vsNew->setActive( true, true );
  vsNew->show();
  if (!newViewUrl.isValid())
    createView (false, 0L, (KateView *)activeView());
  else {
    // tjeck if doc is allready open
    uint aDocId;
    if ( (aDocId = docManager->findDoc( newViewUrl )) )
      createView (false, 0L, 0L, docManager->docWithID( aDocId) );
    else
      createView( true, newViewUrl );
  }
}

void KateViewManager::removeViewSpace (KateViewSpace *viewspace)
{
  // abort if viewspace is 0
  if (!viewspace) return;

  // abort if this is the last viewspace
  if (viewSpaceList.count() < 2) return;

  KateSplitter* p = (KateSplitter*)viewspace->parentWidget();

  // find out if it is the first child for repositioning
  // see below
  bool pIsFirst = false;

  // save some size information
  KateSplitter* pp=0L;
  QValueList<int> ppsizes;
  if (viewSpaceList.count() > 2 && p->parentWidget() != this)
  {
    pp = (KateSplitter*)p->parentWidget();
    ppsizes = pp->sizes();
    pIsFirst = !pp->isLastChild( p ); // simple logic, right-
  }

  // Figure out where to put views that are still needed
  KateViewSpace* next;
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
      KateView* v = viewspace->currentView();

      if (v->isLastView())
      {
        viewspace->removeView(v);
        next->addView( v, false );
      }
      else
      {
        deleteView( v, false, false );
        //kdDebug(13030)<<"KateViewManager::removeViewSpace(): deleting a view: "<<v->doc()->url().filename()<<endl;
      }
    }
    vsvc = viewspace->viewCount();
  }

  viewSpaceList.remove( viewspace );

  // reparent the other sibling of the parent.
  while (p->children ())
  {
    //kdDebug(13030)<<"KateViewManager::removeViewSpace(): reparenting something"<<endl;
    QWidget* other = ((QWidget *)(( QList<QObject>*)p->children())->first());
    other->reparent( p->parentWidget(), 0, QPoint(), true );
    // We also need to find the right viewspace to become active,
    // and if "other" is the last, we move it into the grid.
    if (pIsFirst)
       ((KateSplitter*)p->parentWidget())->moveToFirst( other );
    if ( other->isA("KateViewSpace") ) {
      setActiveSpace( (KateViewSpace*)other );
      if (viewSpaceList.count() == 1)
        grid->addWidget( other, 0, 0);
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
  KateView* v = activeViewSpace()->currentView();
  if ( v ) {
    //kdDebug(13030)<<"removeViewSpace(): setting active view: "<<v->doc()->url().filename()<<endl;
    activateView( v );
  }

  emit viewChanged();
}

void KateViewManager::slotCloseCurrentViewSpace()
{
  removeViewSpace(activeViewSpace());
}

void KateViewManager::setWindowCaption()
{
  if (activeView())
  {
    QString c;
    if (activeView()->doc()->url().isEmpty() || (! showFullPath))
      c = ((KateDocument *)activeView()->doc())->docName();
    else
      c = activeView()->doc()->url().prettyURL();

    ((KateMainWindow*)topLevelWidget())->setCaption( c,activeView()->isModified());
  }
}

void KateViewManager::setShowFullPath( bool enable )
{
  showFullPath = enable;
  setWindowCaption();
}

void KateViewManager::setUseOpaqueResize( bool enable )
{
  useOpaqueResize = enable;
  // TODO: loop through splitters and set this prop
}

void KateViewManager::reloadCurrentDoc()
{
  if (! activeView() )
    return;
  if (! activeView()->canDiscard())
    return;
  KateView* v = activeView();
  // save cursor position
  int cl = v->currentLine();
  int cc = v->currentColumn();
  // save bookmarks
  ((KateDocument*)v->doc())->reloadFile();
  if (v->numLines() >= cl)
    v->setCursorPosition( cl, cc );
}

///////////////////////////////////////////////////////////
// session config functions
///////////////////////////////////////////////////////////

void KateViewManager::saveAllDocsAtCloseDown()
{
  QValueList<uint> seen;
  KateView* v;
  uint id;
  QStringList list;
  uint vc = viewCount();
  uint i = 0;
  KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);
  while ( i <= vc )
  {
    v = activeView();
    id =  ((KateDocument*)v->doc())->docID();
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

void KateViewManager::reopenDocuments(bool isRestore)
{
  KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);
  KConfig* config = kapp->config();
  config->setGroup("General");

  if ( scfg->hasGroup("splitter0") && (isRestore || config->readBoolEntry("restore views", false)) )
  {
    kdDebug(13030)<<"reopenDocumentw(): calling restoreViewConfig()"<<endl;

    restoreViewConfig();
    return;
  }

  scfg->setGroup("open files");
  if (config->readBoolEntry("reopen at startup", true) || isRestore )
  {
    QStringList list = /*config*/scfg->readListEntry("list");

    for ( int i = list.count() - 1; i > -1; i-- ) {
      scfg->setGroup("open files");
      QString fn = scfg->readEntry(list[i]);
      openURL( KURL( fn ) );
      scfg->setGroup( fn );
      KateView* v = activeView();
      if (v)
      {
        v->readSessionConfig( scfg );
        v->doc()->readSessionConfig( scfg );
      }
      scfg->deleteGroup( fn );
    }
  }

  // delete the open files from sessionrc file
  scfg->deleteGroup("open files");
  scfg->sync();
}

void KateViewManager::saveViewSpaceConfig()
{
   if (viewSpaceCount() == 1) {
     kdDebug(13030)<<"saveViewSpaceConfig(): only one vs, aborting"<<endl;
     return;
   }

   KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

   // I need the first splitter, the one which has this as parent.
   KateSplitter* s;
   QObjectList *l = queryList("KateSplitter", 0, false, false);
   QObjectListIt it( *l );
   if ( (s = (KateSplitter*)it.current()) != 0 )
     saveSplitterConfig( s, 0, scfg );
   else
     kdDebug(13030)<<"saveViewSpaceConfig(): PANIC! can't find starting point!"<<endl;
   delete l;

   scfg->sync();
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
       n = QString("viewspace%1").arg( viewSpaceList.find((KateViewSpace*)obj) );
       ((KateViewSpace*)obj)->saveFileList( config, viewSpaceList.find((KateViewSpace*)obj) );
       // save active viewspace
       if ( ((KateViewSpace*)obj)->isActiveSpace() ) {
         config->setGroup("general");
         config->writeEntry("activeviewspace", viewSpaceList.find((KateViewSpace*)obj) );
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

void KateViewManager::restoreSplitter( KSimpleConfig* config, QString group, QWidget* parent)
{
   config->setGroup( group );

   // create a splitter with orientation
   kdDebug(13030)<<"restoreSplitter():creating a splitter: "<<group<<endl;
   if (parent == this)
     kdDebug(13030)<<"parent is this"<<endl;
   KateSplitter* s = new KateSplitter((Qt::Orientation)config->readNumEntry("orientation"), parent);
   if ( group.compare("splitter0") == 0 )
     grid->addWidget(s, 0, 0);

   QStringList children = config->readListEntry( "children" );
   for (QStringList::Iterator it=children.begin(); it!=children.end(); ++it) {
     // for a viewspace, create it and open all documents therein.
     // TODO: restore cursor position.
     if ( (*it).startsWith("viewspace") ) {
       kdDebug(13030)<<"Adding a viewspace: "<<(*it)<<endl;
       KateViewSpace* vs;
       kdDebug(13030)<<"creating a viewspace for "<<(*it)<<endl;
       vs = new KateViewSpace( s );
       connect(this, SIGNAL(statusChanged(KateView *, int, int, int, int, QString)), vs, SLOT(slotStatusChanged(KateView *, int, int, int, int, QString)));
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
           KateView* v = activeView();
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
           int docID = docManager->findDoc( KURL(data[0]) );
           if (docID >= 0)
             createView( false, 0L, 0L, docManager->nthDoc( docID ) );
         }
         // cursor pos
         KateView* v = activeView();
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


