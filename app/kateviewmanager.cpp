/***************************************************************************
                          kateviewmanager.cpp 
                          View Manager for the Kate text editor
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann, 2001, 2002 by Anders Lund
    email                : cullmann@kde.org anders@alweb.dk
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

#include "katemainwindow.h"
#include "kateIface.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "katefiledialog.h"
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
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <qfileinfo.h>
#include <qlayout.h>
#include <qobjectlist.h>
#include <qstringlist.h>
#include <qvbox.h>

#include <qtimer.h>

#include "katesplitter.h"

KateViewManager::KateViewManager (QWidget *parent, KateDocManager *docManager) : Kate::ViewManager  (parent)
{
  // no memleaks
  viewList.setAutoDelete(true);
  viewSpaceList.setAutoDelete(true);

  newOne = true;

  this->docManager = docManager;

	myEncoding = QString::fromLatin1(QTextCodec::codecForLocale()->name());

  // sizemanagment
  grid = new QGridLayout( this, 1, 1 );

  KateViewSpace* vs = new KateViewSpace( this );
  connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, QString)), vs, SLOT(slotStatusChanged(Kate::View *, int, int, int, bool, int, QString)));
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

bool KateViewManager::createView ( bool newDoc, KURL url, Kate::View *origView, Kate::Document *doc )
{
  // create doc
  if (newDoc && !doc)
    doc = (Kate::Document *)docManager->createDoc ();
  else
    if (!doc)
      doc = (Kate::Document *)origView->getDoc();

  // create view
  Kate::View *view = (Kate::View *)doc->createView (this, 0L);
  connect(view,SIGNAL(newStatus()),this,SLOT(setWindowCaption()));
  viewList.append (view);

	doc->setEncoding(myEncoding);

  if (newDoc)
  {
    if (!url.isEmpty())
    {
      if (view->getDoc()->openURL ( url ))
        ((KateMainWindow*)topLevelWidget())->fileOpenRecent->addURL( KURL( url.prettyURL() ) );

      QString name = url.filename();

      // anders avoid two views w/ same caption
      QPtrListIterator<Kate::View> it (viewList);

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
    view->getDoc()->setDocName (doc->docName ());
  }

  if (docManager->myfirstDoc)
    view->getDoc()->setDocName (i18n("Untitled %1").arg(doc->documentNumber()));

  view->installPopup ((QPopupMenu*)((KMainWindow *)topLevelWidget ())->factory()->container("view_popup", (KMainWindow *)topLevelWidget ()) );
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

  // clear caption of mainwindow if this is the current view ;-)
  if ( view == activeView() )
    topLevelWidget()->setCaption ( "" );

  viewspace->removeView (view);

  // remove view from list and memory !!
  viewList.remove (view);

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  return true;
}

KateViewSpace* KateViewManager::activeViewSpace ()
{
  QPtrListIterator<KateViewSpace> it(viewSpaceList);

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

Kate::View* KateViewManager::activeView ()
{
  QPtrListIterator<Kate::View> it(viewList);

  for (; it.current(); ++it)
  {
    if ( it.current()->isActive() )
      return it.current();
  }

  // if we get to here, no view isActive()
  // first, try to get one from activeViewSpace()
  KateViewSpace* vs;
  if ( (vs = activeViewSpace()) ) {
    if ( vs->currentView() ) {
      vs->currentView()->setActive( true );
      return vs->currentView();
    }
  }

  // last attempt: just pick first
  if (viewList.count() > 0)
  {
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

  view->getDoc()->isModOnHD();

  if (!view->isActive())
  {
    if ( !activeViewSpace()->showView (view) )
    {
      // since it wasn't found, give'em a new one
      createView (false, KURL(), view );
      return;
    }

    setActiveView (view);
    viewList.findRef (view);

    setWindowCaption();
    statusMsg();

    emit viewChanged ();
  }

  docManager->setCurrentDoc(view->getDoc());
}

void KateViewManager::activateView( uint documentNumber )
{
  if ( activeViewSpace()->showView(documentNumber) ) {
    activateView( activeViewSpace()->currentView() );
  }
  else
  {
    QPtrListIterator<Kate::View> it(viewList);
    for ( ;it.current(); ++it)
    {
      if ( it.current()->getDoc()->documentNumber() == documentNumber  )
      {
        createView( false, KURL(), it.current() );
        return;
      }
    }

    createView (false, KURL(), 0L, (Kate::Document *)docManager->docWithID(documentNumber));
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
  deleteView (activeView (), true);
}

bool KateViewManager::closeDocWithAllViews ( Kate::View *view )
{
  if (!view) return false;

  if (!view->canDiscard()) return false;

  Kate::Document *doc = view->getDoc();
  QPtrList<Kate::View> closeList;
  uint documentNumber = view->getDoc()->documentNumber();

  for (uint i=0; i < ((KateApp *)kapp)->mainWindowsCount (); i++ )
  {
    for (uint z=0 ; z < ((KateApp *)kapp)->mainWindows.at(i)->viewManager->viewList.count(); z++)
    {
      Kate::View* current = ((KateApp *)kapp)->mainWindows.at(i)->viewManager->viewList.at(z);
      if ( current->getDoc()->documentNumber() == documentNumber )
      {
        closeList.append (current);
      }
    }

    while ( !closeList.isEmpty() )
    {
      Kate::View *view = closeList.first();
      ((KateApp *)kapp)->mainWindows.at(i)->viewManager->deleteView (view, true);
      closeList.removeFirst();
    }
  }

  docManager->deleteDoc (doc);

  for (uint i2=0; i2 < ((KateApp *)kapp)->mainWindowsCount (); i2++ )
  {
    if (((KateApp *)kapp)->mainWindows.at(i2)->viewManager->viewCount() == 0)
    {
      if ((viewList.count() < 1) && (docManager->docCount() < 1) )
        ((KateApp *)kapp)->mainWindows.at(i2)->viewManager->createView (true, KURL(), 0L);
      else if ((viewList.count() < 1) && (docManager->docCount() > 0) )
        ((KateApp *)kapp)->mainWindows.at(i2)->viewManager->createView (false, KURL(), 0L, (Kate::Document *)docManager->nthDoc(docManager->docCount()-1));
    }
  }

  emit viewChanged ();
  return true;
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

  QString c = v -> getDoc()->docName();
   //File name shouldn't be too long - Maciek
   if (c.length() > 200)
     c = "..." + c.right(197);

  emit statusChanged (v, v->cursorLine(), v->cursorColumn(), ovr,block, mod, c);
  emit statChanged ();
}

void KateViewManager::slotWindowNext()
{
  int id = docManager->findDoc (activeView ()->getDoc()) - 1;

  if (id < 0)
    id =  docManager->docCount () - 1;

  activateView (docManager->nthDoc(id)->documentNumber());
}

void KateViewManager::slotWindowPrev()
{
  uint id = docManager->findDoc (activeView ()->getDoc()) + 1;

  if (id >= docManager->docCount () )
    id = 0;

  activateView (docManager->nthDoc(id)->documentNumber());
}

void KateViewManager::slotDocumentNew ()
{
  if (((KateApp *)kapp)->_isSDI)
    ((KateApp *)kapp)->newMainWindow()->viewManager->createView (true, KURL(), 0L);
  else
    createView (true, KURL(), 0L);
}

void KateViewManager::slotDocumentOpen ()
{
  Kate::View *cv = activeView();
	KateFileDialog *dialog;

  if (cv)
	  dialog = new KateFileDialog (cv->getDoc()->url().url(),cv->getDoc()->encoding(), this, i18n ("Open File"));
	else
	  dialog = new KateFileDialog (QString::null,myEncoding, this, i18n ("Open File"));

	KateFileDialogData data = dialog->exec ();
	delete dialog;

  for (KURL::List::Iterator i=data.urls.begin(); i != data.urls.end(); ++i)
  {
    myEncoding = data.encoding;
    openURL( *i );
  }
}

void KateViewManager::slotDocumentSave ()
{
  if (activeView() == 0) return;

  Kate::View *current = activeView();

  if( current->getDoc()->isModified() || current->getDoc()->url().isEmpty() )
  {
    if( !current->getDoc()->url().isEmpty() && current->getDoc()->isReadWrite() )
    {
      current->getDoc()->save();

       if ( current->getDoc()->isModified() )
         KMessageBox::sorry(this, i18n("The file could not be saved. Please check if you have write permission."));
    }
    else
      slotDocumentSaveAs();
  }
}

void KateViewManager::slotDocumentSaveAll ()
{
  QPtrListIterator<Kate::View> it(viewList);

  for ( ;it.current(); ++it)
  {
    Kate::View* current = it.current();
    if( current->getDoc()->isModified() ) {
      if( !current->getDoc()->url().isEmpty() && current->getDoc()->isReadWrite() )
        {
          current->getDoc()->save();

           if ( current->getDoc()->isModified() )
             KMessageBox::sorry(this, i18n("The file could not be saved. Please check if you have write permission."));
        }
      else
        slotDocumentSaveAs();
    }
  }
}

void KateViewManager::slotDocumentSaveAs ()
{
  if (activeView() == 0) return;

  Kate::View *current = activeView();

  if( current->saveAs() == Kate::View::SAVE_OK )
  {
    ((Kate::Document *)current->getDoc())->setDocName (current->getDoc()->url().filename());
    setWindowCaption();
  }
}

void KateViewManager::slotDocumentClose ()
{
  if (!activeView()) return;

  closeDocWithAllViews (activeView());
}

void KateViewManager::slotDocumentCloseAll ()
{
  if (docManager->docCount () == 0) return;

  QPtrList<Kate::Document> closeList;

  for (uint i=0; i < docManager->docCount(); i++ )
    closeList.append (docManager->nthDoc (i));

  bool done = false;
  while (closeList.count() > 0)
  {
    activateView (closeList.at(0)->documentNumber());
    done = closeDocWithAllViews (activeView());

    if (!done) break;

    closeList.remove (closeList.at(0));
  }
}

void KateViewManager::slotUndo ()
{
  if (activeView() == 0) return;

  activeView()->getDoc()->undo();
}

void KateViewManager::slotRedo ()
{
  if (activeView() == 0) return;

  activeView()->getDoc()->redo();
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

  activeView()->getDoc()->selectAll();
}

void KateViewManager::slotDeselectAll ()
{
  if (activeView() == 0) return;

  activeView()->getDoc()->clearSelection ();
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

  activeView()->findPrev();
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
  Kate::View* v = activeView();
  if (v)
    v->indent();

}

void KateViewManager::slotUnIndent()
{
  Kate::View* v = activeView();
  if (v)
    v->unIndent();

}

void KateViewManager::slotSpellcheck ()
{
  if (activeView() == 0) return;

  activeView()->getDoc()->spellcheck();
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

void KateViewManager::slotSetHl (uint n)
{
  if (activeView() == 0) return;

  activeView()->getDoc()->setHlMode(n);
}


void KateViewManager::exportAs(const QString& filter)
{
  if (activeView() == 0) return;
  activeView()->getDoc()->exportAs(filter);
}

void KateViewManager::toggleBookmark ()
{
  if (activeView() == 0) return;

  activeView()->toggleBookmark();
}

void KateViewManager::clearBookmarks ()
{
  if (activeView() == 0) return;

  activeView()->getDoc()->clearMarks();
}

void KateViewManager::slotComment ()
{
  if (activeView() == 0) return;

  activeView()->comment();
}

void KateViewManager::slotUnComment ()
{
  if (activeView() == 0) return;

  activeView()->uncomment();
}

void KateViewManager::openURL (KURL url)
{
  if (!((KateApp *)kapp)->_isSDI)
    openURLReal (url);
  else
  {
    if (newOne)
      openURLReal (url);
    else
      ((KateApp *)kapp)->newMainWindow()->viewManager->openURLReal (url);
  }

  newOne = false;
}

void KateViewManager::openURLReal (KURL url)
{
  // special handling if still only the first initial doc is there
  if (docManager->myfirstDoc)
  {
    createView (false, KURL(), 0L, (Kate::Document *)docManager->docList.at(0));
    docManager->docList.at(0)->setEncoding(myEncoding);

    if (docManager->docList.at(0)->openURL (url))
    {
      ((KateMainWindow*)topLevelWidget())->fileOpenRecent->addURL( KURL( url.prettyURL() ) );
    }

    docManager->docList.at(0)->setDocName (docManager->docList.at(0)->url().filename());

    setWindowCaption();

    docManager->myfirstDoc = false;

    return;
  }

  if ( !docManager->isOpen( url ) )
  {
    Kate::View *cv = activeView();

    if (cv && !cv->getDoc()->isModified() && cv->getDoc()->url().isEmpty())
    {
      cv->getDoc()->setEncoding(myEncoding);

      if (cv->getDoc()->openURL (url))
      {
        ((KateMainWindow*)topLevelWidget())->fileOpenRecent->addURL( KURL( url.prettyURL() ) );
      }

      cv->getDoc()->setDocName (cv->getDoc()->url().filename());

      setWindowCaption();
    }
    else
    {
      createView (true, url, 0L);
    }
  }
  else
    activateView( docManager->findDoc( url ) );

  newOne = false;
}

void KateViewManager::openConstURL (const KURL& url)
{
  openURL (KURL (url));
}

void KateViewManager::openConstURL_delayed1 (const KURL& url)
{
	delayedURL=url;
	QTimer::singleShot(0,this,SLOT(openConstURL_delayed2()));
//  openURL (KURL (url));
}

void KateViewManager::openConstURL_delayed2 ()
{
	openURL(KURL(delayedURL));
}



void KateViewManager::printNow()
{
  if (!activeView()) return;
  activeView()->getDoc()->print ();
}

void KateViewManager::printDlg()
{
  if (!activeView()) return;
  activeView()->getDoc()->printDialog ();
}

void KateViewManager::toggleIconBorder ()
{
  if (!activeView()) return;
  activeView()->toggleIconBorder ();
}

void KateViewManager::toggleLineNumbers()
{
  if (!activeView()) return;
  activeView()->toggleLineNumbersOn();
}

void KateViewManager::toggleVertical()
{
  if (!activeView()) return;
  activeView()->getDoc()->toggleBlockSelectionMode();
}

void KateViewManager::splitViewSpace( KateViewSpace* vs,
                                      bool isHoriz,
                                      bool atTop,
                                      KURL newViewUrl)
{
  kdDebug()<<"splitViewSpace()"<<endl;
  
  if (!activeView()) return;
  if (!vs) vs = activeViewSpace();

  bool isFirstTime = vs->parentWidget() == this;

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
  kdDebug()<<"splitViewSpace(): splitter and viewspace created"<<endl;

  if (atTop)
    s->moveToFirst( vsNew );

  if (isFirstTime)
    grid->addWidget(s, 0, 0);

  kdDebug()<<"splitViewSpace(): calling new splitter->show()"<<endl;
  s->show();
  kdDebug()<<"splitViewSpace(): splitter->show() was  called, moving on"<<endl;

  connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, QString)), vsNew, SLOT(slotStatusChanged(Kate::View *, int, int,int, bool, int, QString)));
  viewSpaceList.append( vsNew );
  vsNew->installEventFilter( this );
  activeViewSpace()->setActive( false );
  vsNew->setActive( true, true );
  vsNew->show();
  kdDebug()<<"splitViewSpace(): going to create a view!"<<endl;
  if (!newViewUrl.isValid())
    createView (false, KURL(), (Kate::View *)activeView());
  else {
    // tjeck if doc is allready open
    uint aDocId;
    if ( (aDocId = docManager->findDoc( newViewUrl )) )
      createView (false, KURL(), 0L, (Kate::Document *)docManager->docWithID( aDocId) );
    else
      createView( true, newViewUrl );
  }
  kdDebug()<<"splitViewSpace() - DONE!"<<endl;
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

  viewSpaceList.remove( viewspace );

  // reparent the other sibling of the parent.
  while (p->children ())
  {
    QWidget* other = ((QWidget *)(( QPtrList<QObject>*)p->children())->first());

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
  Kate::View* v = activeViewSpace()->currentView();
  if ( v )
    activateView( v );

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
    if (activeView()->getDoc()->url().isEmpty() || (! showFullPath))
     {
        c = ((Kate::Document *)activeView()->getDoc())->docName();
       //File name shouldn't be too long - Maciek
       if (c.length() > 200)
         c = "..." + c.right(197);
     }
      else
     {
        c = activeView()->getDoc()->url().prettyURL();
       //File name shouldn't be too long - Maciek
       if (c.length() > 200)
         c = c.left(197) + "...";
     }

    ((KateMainWindow*)topLevelWidget())->setCaption( c,activeView()->getDoc()->isModified());
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
  Kate::View* v = activeView();
  // save cursor position
  uint cl = v->cursorLine();
  uint cc = v->cursorColumn();
  // save bookmarks
  ((Kate::Document*)v->getDoc())->reloadFile();
  if (v->getDoc()->numLines() >= cl)
    v->setCursorPosition( cl, cc );
}

///////////////////////////////////////////////////////////
// session config functions
///////////////////////////////////////////////////////////

void KateViewManager::saveAllDocsAtCloseDown(  )
{
  kdDebug(13030)<<"saveAllDocsAtCloseDown()"<<endl;
  if (docManager->docCount () == 0) return;

  QPtrList<Kate::Document> closeList;

  for (uint i=0; i < docManager->docCount(); i++ )
    closeList.append (docManager->nthDoc (i));

  Kate::View *v = 0L;
  uint id = 0;

  KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

  // save current document, since if we just reopens documents
  // when restarted, we want that in front.
  scfg->setGroup("open files");
  scfg->writeEntry("current file", getActiveView()->getDoc()->url().prettyURL());

  while ( closeList.count() > 0 )
  {
    activateView (closeList.at(0)->documentNumber());
    v = activeView();
    //id = closeList.at(0)->documentNumber();

    if ( !v->getDoc()->url().isEmpty() )
    {
      scfg->setGroup( v->getDoc()->url().prettyURL() );
      v->getDoc()->writeSessionConfig(scfg);

      scfg->setGroup("open files");
      scfg->writeEntry( QString("File%1").arg(id), v->getDoc()->url().prettyURL() );
    }

    if( !closeDocWithAllViews( v ) )
      return;

    closeList.remove (closeList.at(0));
    id++;
  }

  scfg->sync();
  kdDebug(13030)<<">>>> saveAllDocsAtCloseDown() DONE"<<endl;
}

void KateViewManager::reopenDocuments(bool isRestore)
{
  kdDebug(13030)<<"reopenDocuments()"<<endl;
  KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);
  KConfig* config = kapp->config();
  config->setGroup("General");
  bool restoreViews = config->readBoolEntry("restore views", false);
  bool reopenAtStart = config->readBoolEntry("reopen at startup", true);

  if ( scfg->hasGroup("splitter0") && ( isRestore || restoreViews ) )
  {
    kdDebug(13030)<<"calling restoreViewConfig()"<<endl;
    restoreViewConfig();
  }
  else if ( reopenAtStart || isRestore )
  {
    scfg->setGroup("open files");
    // try to focus the file that had focus at close down
    QString curfile = scfg->readEntry("current file");
    Kate::View *viewtofocus = 0L;

    int i = 0;
    QString fn;
    while ( scfg->hasKey( QString("File%1").arg( i ) )  )
    {
      fn = scfg->readEntry( QString("File%1").arg( i ) );
      if ( !fn.isEmpty() ) {
        kdDebug()<<"reopenDocuments(): opening file : "<<fn<<endl;
        openURL( KURL( fn ) );
        Kate::View* v = activeView();
        if (v)
        {
          scfg->setGroup( fn );
          v->getDoc()->readSessionConfig( scfg );
          scfg->setGroup( scfg->readEntry("viewconfig") );
          v->readSessionConfig( scfg );
          if ( fn == curfile ) viewtofocus = v;
        }
      }
      scfg->setGroup("open files");
      i++;
    }
    if ( viewtofocus ) activateView( viewtofocus );
  }

  kdDebug(13030)<<">>>> reopenDocuments() DONE"<<endl;
}

void KateViewManager::saveViewSpaceConfig()
{
   kdDebug(13030)<<"saveViewSpaceConfig()"<<endl;
   KSimpleConfig* scfg = new KSimpleConfig("katesessionrc", false);

  // TEMPORARY ??
  kdDebug(13030)<<"clearing session config file before saving list"<<endl;
  scfg->setGroup("nogroup");
  QStringList groups(scfg->groupList());
  for ( QStringList::Iterator it = groups.begin(); it != groups.end(); ++it )
    if ( *it != "nogroup") scfg->deleteGroup(*it);

   if (viewSpaceCount() == 1) {
     viewSpaceList.first()->saveFileList( scfg, 0 );
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
   kdDebug(13030)<<">>>> saveViewSpaceConfig() DONE"<<endl;
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
   kdDebug(13030)<<"All splitters restored, setting active view"<<endl;
   scfg->setGroup("general");
   KateViewSpace *vs = viewSpaceList.at( scfg->readNumEntry("activeviewspace") );
   if ( vs ) // better be sure ;}
     activateSpace( vs->currentView() );
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
   for (QStringList::Iterator it=children.begin(); it!=children.end(); ++it)
   {
     // for a viewspace, create it and open all documents therein.
     if ( (*it).startsWith("viewspace") ) {
       KateViewSpace* vs = new KateViewSpace( s );
       connect(this, SIGNAL(statusChanged(Kate::View *, int, int, int, bool, int, QString)), vs, SLOT(slotStatusChanged(Kate::View *, int, int, int, bool, int, QString)));
       vs->installEventFilter( this );
       viewSpaceList.append( vs );
       vs->show();
       setActiveSpace( vs );

       // open documents
       int idx = 0;
       QString file = QString("file%1").arg( idx );
       config->setGroup( (*it) );  // "viewspace<n>"
       while ( config->hasKey( file ) ) {     // FIXME FIXME
         Kate::View* v;
         KURL url( config->readEntry( file ) );
         if ( ! docManager->isOpen( url ) ) {
           openURL( url );
           v = activeView();
           if (v && v->getDoc()->url() == url ) { // this is a wild assumption, but openURL() fails to return a bool :(
             // doc config is in group "<url.prettyURL()>"
             config->setGroup( url.prettyURL() );
             v->getDoc()->readSessionConfig( config );
           }
           else {
             //createView (true, KURL(), 0L);
             kdDebug(13030)<<"KateViewManager: failed to open document "<<file<<endl;
           }
         }
         else { // if the group has been deleted, we can find a document
           // ahem, tjeck if this document actually exists.
           Kate::Document *doc = docManager->findDocByUrl( url );
           if ( doc ) {
             kdDebug(13030)<<"Document '"<<url.prettyURL()<<"' found open, creating extra view"<<endl;
             createView( false, KURL(), 0L, doc );
           }
           else
             kdDebug(13030)<<"SOMETHING IS ROTTEN IN THE STATE OF DENMARK (or so)"<<endl;
           v = activeView(); // if shakespeare was right, this is a mistake :(((
         }
         if ( v ) {
           // view config is in group "<group>:<file>"
           QString g = *it + ":" + file;
           kdDebug()<<"view config is group '"<<g<<"'"<<endl;
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
       kdDebug(13030)<<"Done resotring a viewspace"<<endl;
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
   kdDebug(13030)<<"Bye from KateViewManager::restoreSplitter() ("<<group<<")"<<endl;
}

void KateViewManager::gotoMark (KTextEditor::Mark *mark)
{
  if (!activeView()) return;

  activeView()->gotoMark (mark);
}

void KateViewManager::slotApplyWordWrap ()
{
  if (!activeView()) return;

  activeView()->getDoc()->applyWordWrap();
}
