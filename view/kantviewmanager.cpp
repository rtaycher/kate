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
#include "../sidebar/kantsidebar.h"
#include "../document/kantdocmanager.h"
#include "../document/kantdocument.h"
#include "kantview.h"
#include "kantviewspace.h"
#include "../fileselector/kantfileselector.h"

#include "../kwrite/kwview.h"
#include "../kwrite/kwattribute.h"
#include "../kwrite/kwdoc.h"
#include "../kwrite/kwdialog.h"
#include "../kwrite/highlight.h"
#include "../kwrite/kwrite_factory.h"

#include <qwidgetstack.h>
#include <qlayout.h>
#include <kdiroperator.h>
#include <kfiledialog.h>
#include <kdockwidget.h>
#include <kiconloader.h>
#include <kstddirs.h>
#include <kaction.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kstdaction.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qsplitter.h>
#include <dcopclient.h>
#include <klistbox.h>
#include <qobjectlist.h>
#include <kdebug.h>
#include <qstringlist.h>
#include <qfileinfo.h>

KantVMListBoxItem::KantVMListBoxItem (const QPixmap &pixmap,const QString &text, long docID) : KantListBoxItem (pixmap, text )
{
  myDocID = docID;
}

KantVMListBoxItem::~KantVMListBoxItem ()
{
}

long KantVMListBoxItem::docID ()
{
  return myDocID;
}

KantViewManager::KantViewManager (QWidget *parent, KantDocManager *docManager, KantSidebar *sidebar) : KantPluginIface  (parent)
{
  // no memleaks
  viewList.setAutoDelete(true);
  viewSpaceList.setAutoDelete(true);

  myViewID = 0;

  this->docManager = docManager;
  this->sidebar = sidebar;

  // sizemanagment
  grid = new QGridLayout( this, 1, 1 );

  KantViewSpace* vs = new KantViewSpace( this );
  connect(this, SIGNAL(statusChanged(KantView *, int, int, int, int, QString)), vs, SLOT(slotStatusChanged(KantView *, int, int, int, int, QString)));
  vs->setActive( true );
  vs->installEventFilter( this );
  grid->addWidget( vs, 0, 0);
  viewSpaceList.append(vs);

  listbox = new KListBox (sidebar);
  fileselector = new KantFileSelector(0, "operator");
  fileselector->dirOperator()->setView(KFile::Simple);

  sidebar->addWidget (fileselector, i18n("Fileselector"));
  sidebar->addWidget (listbox, i18n("Files"));

  connect(listbox,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(activateView(QListBoxItem *)));
  connect(listbox,SIGNAL(selected(QListBoxItem *)),this,SLOT(activateView(QListBoxItem *)));

  connect(fileselector->dirOperator(),SIGNAL(fileSelected(const KFileViewItem*)),this,SLOT(fileSelected(const KFileViewItem*)));

  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
}

KantViewManager::~KantViewManager ()
{
}

bool KantViewManager::createView ( bool newDoc, KURL url, KantView *origView )
{
  KantDocument *doc;

  // create doc
  if (newDoc)
  {
    QFileInfo* fi;
    if ( (!url.isEmpty()) && (url.filename() != 0) ) {
      // if this is a local file, get some information
      if ( url.isLocalFile() ) {
         fi = new QFileInfo(url.path());
         if (!fi->exists()) {
           kdDebug()<<QString("createView(): ABORTING: %1 dosen't exist").arg(url.path())<<endl;
           return false;
         }
      }
      ((KantMainWindow*)topLevelWidget())->fileOpenRecent->addURL( KURL( url.prettyURL() ) );
    }
    else
      fi = new QFileInfo();
    doc = docManager->createDoc (fi);
  }
  else
    doc = (KantDocument *)origView->doc();

  // create view
  KantView *view = new KantView (this, doc, (QString("KantViewIface%1").arg(myViewID)).latin1());
  connect(view,SIGNAL(newStatus()),this,SLOT(setWindowCaption()));
  connect(view,SIGNAL(newStatus()),this,SLOT(slotSetModified()));
  myViewID++;
  viewList.append (view);        

  KConfig *config = ((KantMainWindow*)topLevelWidget())->config;
  config->setGroup("kwrite");
  doc->readConfig( config );
  view->readConfig( config );

  if (!newDoc)
    view->copySettings(origView);

  view->init();

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
        for (; it.current(); ++it) {
           if ( it.current()->doc()->url().filename().compare( name ) == 0 )
             hassamename++;
        }
        if (hassamename > 1)
          name = QString(name+"<%1>").arg(hassamename);
        view->setCaption ( name );
      }
      else
      {
        view->setCaption (i18n("Untitled %1").arg(doc->docID()));
      }
    }
    else
    {
      view->setCaption (i18n("Untitled %1").arg(doc->docID()));
    }
  }
  else
    view->setCaption (origView->caption());

  // if new document insert KantListItem in listbox
  if (newDoc)
    listbox->insertItem (new KantVMListBoxItem (SmallIcon("null"),view->caption(), doc->docID()) );
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
  KantDocument *dDoc;
  if (removeDoc)
  {
    for (uint i = 0; i < listbox->count(); i++)
    {
      if (((KantVMListBoxItem *) listbox->item (i)) ->docID() == ((KantDocument *) view->doc())->docID())
      {
        // QT BUGFIX - if you remove the last item of a listbox it crashs after the next insert !!!!!
        if (listbox->count() > 1)
          listbox->removeItem( i );
        else
          listbox->clear();
      }
    }

    dDoc = (KantDocument *) view->doc();
  }

  // remove view from list and memory !!
  viewList.remove (view);

  if (removeDoc)
    docManager->deleteDoc (dDoc);

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  if (createNew && (viewList.count() < 1))
    createView (true, 0L, 0L);

  emit viewChanged ();

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

  if (viewList.count() > 0)
  {
   viewList.first()->setActive( true );
    return viewList.first();
  }

  return 0L;
}

void KantViewManager::setActiveSpace ( KantViewSpace* vs )
{
   if (activeViewSpace())
     activeViewSpace()->setActive( false );

   vs->setActive( true );
}

void KantViewManager::setActiveView ( KantView* view )
{
//kdDebug()<<"setActiveView()"<<endl;
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
//kdDebug()<<"activateView"<<endl;
  ((KantDocument*)view->doc())->isModOnHD();
  if (!view->isActive())
  {
    if ( !activeViewSpace()->showView (view) )
    {
      // since it wasn't found, give'em a new one
      kdDebug()<<"Sometimes things aren't what they seem..."<<endl;
      createView (false, 0L, view );
      return;
    }
//kdDebug()<<"setting view as active"<<endl;
    setActiveView (view);
    viewList.findRef (view);

    setWindowCaption();
    statusMsgOther();
    for (int i = 0; i < listbox->count(); i++)
    {
      if ( ((KantVMListBoxItem *) listbox->item (i))->docID() == ((KantDocument *) view->doc())->docID() )
        listbox->setCurrentItem( i );
    }
    emit viewChanged ();
  }
}

void KantViewManager::activateView( QListBoxItem *item )
{
  activateView( ((KantVMListBoxItem *)item)->docID() );
}

void KantViewManager::activateView( int docID )
{
  if ( activeViewSpace()->showView(docID) ) {
    activateView( activeViewSpace()->currentView() );
  }
  else {
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
    if (config & KWrite::cfOvr)
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
    if (config & KWrite::cfOvr)
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
    msg = v->caption();

  emit statusChanged (v, v->currentLine() + 1, v->currentColumn() + 1, ovr, mod, msg);
  emit statChanged ();
}

void KantViewManager::fileSelected(const KFileViewItem *file)
{
  KURL u(file->urlString());
  openURL( u );
}

void KantViewManager::slotWindowNext()
{
  long id = docManager->findDoc ((KantDocument *) activeView ()->doc()) - 1;

  if (id < 0)
    id =  docManager->docCount () - 1;

  listbox->setCurrentItem( id );
}

void KantViewManager::slotWindowPrev()
{
  long id = docManager->findDoc ((KantDocument *) activeView ()->doc()) + 1;

  if (id >= docManager->docCount () )
    id = 0;

  listbox->setCurrentItem( id );
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


void KantViewManager::setUnmodified(long docId, const QString &text)
{
  	   for (uint i = 0; i < listbox->count(); i++)
    	     {
	       if (((KantVMListBoxItem *) listbox->item (i)) ->docID() == docId)
		 {
           	   ((KantVMListBoxItem *)listbox->item(i))->setPixmap(SmallIcon("null"));
           	   ((KantVMListBoxItem *)listbox->item(i))->setBold(false);
		   if (!text.isNull()) ((KantVMListBoxItem *)listbox->item(i))->setText(text);
           	   listbox->triggerUpdate(false);
		   break;
		 }
             }

}

void KantViewManager::setModified(long docId, const QString &text)
{
  	   for (uint i = 0; i < listbox->count(); i++)
    	     {
	       if (((KantVMListBoxItem *) listbox->item (i)) ->docID() == docId)
		 {
           	   ((KantVMListBoxItem *)listbox->item(i))->setPixmap(SmallIcon("modified"));
           	   ((KantVMListBoxItem *)listbox->item(i))->setBold(true);
		   if (!text.isNull()) ((KantVMListBoxItem *)listbox->item(i))->setText(text);
           	   listbox->triggerUpdate(false);
		   break;
		 }
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
	   setUnmodified(((KantDocument *)current->doc())->docID(),QString());
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
          setUnmodified(((KantDocument *)current->doc())->docID(),QString());
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
    current->setCaption (url.filename());

    setWindowCaption();

    for (uint i = 0; i < listbox->count(); i++)
    {
      if (((KantVMListBoxItem *) listbox->item (i)) ->docID() == ((KantDocument *) current->doc())->docID())
        setUnmodified(((KantDocument *)current->doc())->docID(),current->caption());
    }
  }
}

void KantViewManager::slotDocumentClose ()
{
  if (!activeView()) return;

  long docID = ((KantDocument *)activeView()->doc())->docID();

  QList<KantView> closeList;

  QListIterator<KantView> it(viewList);
  for ( ;it.current(); ++it)
  {
    KantView* current = it.current();
    if ( ((KantDocument *)current->doc())->docID() == docID )
    {
      closeList.append (current);
    }
  }

  bool done = false;
  while ( closeList.at(0) )
  {
    KantView *view = closeList.at(0);
    done = deleteView (view);
    closeList.remove (view);

    if (!done) return;
  }

  if (activeView()) {
    listbox->setSelected(listbox->currentItem(), true);
  }
}

void KantViewManager::slotDocumentCloseAll ()
{
  if (docManager->docCount () == 0) return;

  bool done = false;
  uint viewCounter = viewCount();
  uint i = 0;

  while (i <= viewCounter)
  {
    done = deleteView(activeView());

    if (!done) return;
    else
      i++;
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

void KantViewManager::slotHlDlg ()
{
  if (activeView() == 0) return;

  activeView()->hlDlg();

  KWriteFactory::instance()->config()->sync();

  KConfig *config = KWriteFactory::instance()->config();
  activeView()->writeConfig(config);
  activeView()->doc()->writeConfig(config);

  KWriteFactory::instance()->config()->sync();
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

void KantViewManager::splitViewSpace(bool isHoriz)
{
  if (!activeView()) return;

  bool isFirstTime = activeViewSpace()->parentWidget() == this;

  QValueList<int> sizes;
  if (! isFirstTime)
    sizes = ((QSplitter*)activeViewSpace()->parentWidget())->sizes();

  Qt::Orientation o = isHoriz ? Qt::Vertical : Qt::Horizontal;
  QSplitter* s = new QSplitter(o, activeViewSpace()->parentWidget());
  s->setOpaqueResize( useOpaqueResize );

  if (! isFirstTime) {
    viewSpaceList.findRef(activeViewSpace());
    uint at =  viewSpaceList.at();
    if ( (at < (viewSpaceList.count() - 1))
       && (viewSpaceList.at(at +1)->parentWidget() == activeViewSpace()->parentWidget() ) )
         ((QSplitter*)s->parentWidget())->moveToFirst( s );
  }
  activeViewSpace()->reparent( s, 0, QPoint(), true );
  KantViewSpace* vs = new KantViewSpace( s );

  if (isFirstTime)
    grid->addWidget(s, 0, 0);
  else
    ((QSplitter*)s->parentWidget())->setSizes( sizes );

  sizes.clear();
  int sz = isHoriz ? s->height()/2 : s->width()/2;
  sizes.append(sz);
  sizes.append(sz);
  s->setSizes( sizes );

  s->show();

  connect(this, SIGNAL(statusChanged(KantView *, int, int, int, int, QString)), vs, SLOT(slotStatusChanged(KantView *, int, int, int, int, QString)));
  viewSpaceList.append( vs );
  vs->installEventFilter( this );
  //s->moveToLast( vs );
  activeViewSpace()->setActive( false );
  vs->setActive( true );
  vs->show();
  createView (false, 0L, (KantView *)activeView());
}

void KantViewManager::removeViewSpace (KantViewSpace *viewspace)
{
  // abort if viewspace is 0
  if (!viewspace) return;

  // abort if this is the last viewspace
  if (viewSpaceList.count() < 2) return;

  QSplitter* p = (QSplitter*)viewspace->parentWidget();

  QSplitter* pp=0L;
  QValueList<int> ppsizes;
  if (viewSpaceList.count() > 2 && p->parentWidget() != this)
  {
    pp = (QSplitter*)p->parentWidget();
    ppsizes = pp->sizes();
  }

  //KantViewSpace *next = viewSpaceList.prev();
  KantViewSpace* next;
  if (viewSpaceList.find(viewspace) == 0)
    next = viewSpaceList.next();
  else
    next = viewSpaceList.prev();

  // here, reparent views in viewspace that are last views, delete the rest.
  int vsvc = viewspace->viewCount();
  while (vsvc > 0)
  {
    if (viewspace->currentView())
    {
    kdDebug()<<QString("removeViewSpace(): %1 views left").arg(vsvc)<<endl;
      KantView* v = viewspace->currentView();

      if (v->isLastView())
      {
        viewspace->removeView(v);
        next->addView( v, false );
      }
      else
      {
        deleteView( v, false, false, false );
      }
    }
    else
      kdDebug()<<"removeViewSpace(): PANIC!!"<<endl;
    vsvc = viewspace->viewCount();
  }

  viewSpaceList.remove( viewspace );

  while (p->children ())
  {
    ((QWidget *)(( QList<QObject>*)p->children())->first())->reparent( p->parentWidget(), 0, QPoint(), true );
  }

  delete p;

  if (!ppsizes.isEmpty())
    pp->setSizes( ppsizes );

  viewSpaceList.find( activeViewSpace() );

  if (viewSpaceList.current()->parentWidget() == this)
    grid->addWidget( viewSpaceList.current(), 0, 0);

  viewSpaceList.current()->setActive( true );
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
      c = activeView()->caption();
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


void KantViewManager::slotSetModified()
{
  if (!activeView()) return;

  if (  ( ((KantDocument *) activeView()->doc())->isModified() ) )
    setModified(((KantDocument *) activeView()->doc())->docID(),QString());
  else
    setUnmodified(((KantDocument *) activeView()->doc())->docID(),QString());
}


void KantViewManager::saveAllDocsAtCloseDown(KConfig* config)
{
  QValueList<long> seen;
  KantView* v;
  int id;
  QStringList data;
  QStringList list;
  int vc = viewCount();
  uint i = 0;
  while ( i <= vc )
  {
    v = activeView();
    id =  ((KantDocument*)v->doc())->docID();
    // save to config if not seen
    if ( ! seen.contains( id ) && ! v->doc()->url().isEmpty() ) {
      seen.append( id );
      data.clear();
      // TODO: should we tjeck for local file here?
      // add URL, cursor position
      data.append( v->doc()->url().prettyURL() );//URL
      data.append( QString("%1.%2").arg(v->currentLine()).arg(v->currentColumn()) );// CURSOR
      //data.append();// LASTMOD
      // write entry
      config->setGroup("open files");
      config->writeEntry( QString("File%1").arg(id), data );
      list.append( QString("File%1").arg(id) );
    }
    if( ! deleteView( v ) )
      return;
    i++;
  }
  config->setGroup("open files");
  config->writeEntry( "list", list );
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

