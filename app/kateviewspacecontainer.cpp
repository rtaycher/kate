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
#include "kateviewspacecontainer.h"
#include "kateviewspacecontainer.moc"

#include "katetabwidget.h"
#include "katemainwindow.h"
#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "kateviewspace.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kstringhandler.h>
#include <kxmlguifactory.h>
#include <ktoolbar.h>

#include <qlayout.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qfileinfo.h>
#include <QDropEvent>
#include <QMenu>
#include <QStackedWidget>
//END Includes

KateViewSpaceContainer::KateViewSpaceContainer (KateViewManager *viewManager, QWidget *parent)
 : QSplitter  (parent)
 , m_viewManager(viewManager)
 , m_blockViewCreationAndActivation (false)
 , m_activeViewRunning (false)
 , m_pendingViewCreation(false)
{
  // resize mode
  setOpaqueResize( KGlobalSettings::opaqueResize() );

  KateViewSpace* vs = new KateViewSpace( this, 0 );
  addWidget (vs);

  vs->setActive( true );
  m_viewSpaceList.append(vs);
  connect( this, SIGNAL(viewChanged()), this, SLOT(slotViewChanged()) );
  connect(KateDocManager::self(), SIGNAL(initialDocumentReplaced()), this, SIGNAL(viewChanged()));

  connect(KateDocManager::self(),SIGNAL(documentCreated(KTextEditor::Document *)),this,SLOT(documentCreated(KTextEditor::Document *)));
  connect(KateDocManager::self(),SIGNAL(documentDeleted(KTextEditor::Document *)),this,SLOT(documentDeleted(KTextEditor::Document *)));
}

KateViewSpaceContainer::~KateViewSpaceContainer ()
{
}

void KateViewSpaceContainer::documentCreated (KTextEditor::Document *doc)
{
  if (m_blockViewCreationAndActivation) return;

  if (!activeView())
    activateView (doc);
}

void KateViewSpaceContainer::documentDeleted (KTextEditor::Document *)
{
  if (m_blockViewCreationAndActivation) return;

  // just for the case we close a document out of many and this was the active one
  // if all docs are closed, this will be handled by the documentCreated
  if (!activeView() && (KateDocManager::self()->documents() > 0))
    createView (KateDocManager::self()->document(KateDocManager::self()->documents()-1));
}

bool KateViewSpaceContainer::createView ( KTextEditor::Document *doc )
{
  if (m_blockViewCreationAndActivation) return false;

  // create doc
  if (!doc)
    doc = KateDocManager::self()->createDoc ();

  // create view, registers its XML gui itself
  KTextEditor::View *view = (KTextEditor::View *) doc->createView (activeViewSpace()->stack);

  m_viewList.append (view);
  m_activeStates[view] = false;

  // disable settings dialog action
  delete view->actionCollection()->action( "set_confdlg" );
  delete view->actionCollection()->action( "editor_options" );

  // popup menu
  QMenu *menu = qobject_cast<QMenu*> (mainWindow()->factory()->container("ktexteditor_popup", mainWindow()));
  if (menu)
    view->setContextMenu (menu);

  connect(view,SIGNAL(dropEventPass(QDropEvent *)), mainWindow(),SLOT(slotDropEvent(QDropEvent *)));
  connect(view,SIGNAL(focusIn(KTextEditor::View *)),this,SLOT(activateSpace(KTextEditor::View *)));

  activeViewSpace()->addView( view );
  activateView( view );

  return true;
}

bool KateViewSpaceContainer::deleteView (KTextEditor::View *view, bool delViewSpace)
{
  if (!view) return true;

  KateViewSpace *viewspace = (KateViewSpace *)view->parentWidget()->parentWidget();

  viewspace->removeView (view);

  mainWindow()->guiFactory ()->removeClient (view);

  // remove view from list and memory !!
  m_viewList.removeAt ( m_viewList.indexOf (view) );
  m_activeStates.remove (view);
  delete view;
  view = 0L;

  if (delViewSpace)
    if ( viewspace->viewCount() == 0 )
      removeViewSpace( viewspace );

  return true;
}

KateViewSpace* KateViewSpaceContainer::activeViewSpace ()
{
  for ( QList<KateViewSpace*>::const_iterator it = m_viewSpaceList.begin();
        it != m_viewSpaceList.end(); ++it )
  {
    if ( (*it)->isActiveSpace() )
      return *it;
  }

  // none active, so use the first we grab
  if ( !m_viewSpaceList.isEmpty() )
  {
    m_viewSpaceList.first()->setActive( true );
    return m_viewSpaceList.first();
  }

  return 0L;
}

KTextEditor::View* KateViewSpaceContainer::activeView ()
{
  if (m_activeViewRunning)
    return 0L;

  m_activeViewRunning = true;

  for ( QList<KTextEditor::View*>::const_iterator it = m_viewList.begin();
        it != m_viewList.end(); ++it )
  {
    if ( m_activeStates[*it] )
    {
      m_activeViewRunning = false;
      return *it;
    }
  }

  // if we get to here, no view isActive()
  // first, try to get one from activeViewSpace()
  KateViewSpace* vs = activeViewSpace();
  if ( vs && vs->currentView() )
  {
    activateView (vs->currentView());

    m_activeViewRunning = false;
    return vs->currentView();
  }

  // last attempt: just pick first
  if ( !m_viewList.isEmpty() )
  {
    activateView (m_viewList.first());

    m_activeViewRunning = false;
    return m_viewList.first();
  }

  m_activeViewRunning = false;

  // no views exists!
  return 0L;
}

void KateViewSpaceContainer::setActiveSpace ( KateViewSpace* vs )
{
   if (activeViewSpace())
     activeViewSpace()->setActive( false );

   vs->setActive( true, viewSpaceCount() > 1 );
}

void KateViewSpaceContainer::setActiveView ( KTextEditor::View* view )
{
  if (activeView())
    m_activeStates[activeView()] = false;

  m_activeStates[view] = true;
}

void KateViewSpaceContainer::activateSpace (KTextEditor::View* v)
{
  if (!v) return;

  KateViewSpace* vs = (KateViewSpace*)v->parentWidget()->parentWidget();

  if (!vs->isActiveSpace()) {
    setActiveSpace (vs);
    activateView(v);
  }
}

void KateViewSpaceContainer::reactivateActiveView() {
  KTextEditor::View *view = activeView();
  if (view) {
    m_activeStates[view] = false;
    activateView(view);
  } else if (m_pendingViewCreation) {
    m_pendingViewCreation=false;
    disconnect(m_pendingDocument,SIGNAL(documentNameChanged(Document *)),this,SLOT(slotPendingDocumentNameChanged()));
    createView(m_pendingDocument);
  }
}

void KateViewSpaceContainer::activateView ( KTextEditor::View *view )
{
  if (!view) return;

  if (!m_activeStates[view])
  {
    if ( !activeViewSpace()->showView (view) )
    {
      // since it wasn't found, give'em a new one
      createView ( (KTextEditor::Document *)view->document() );
      return;
    }

    setActiveView (view);

    mainWindow()->toolBar ()->setUpdatesEnabled (false);

    if (m_viewManager->guiMergedView)
      mainWindow()->guiFactory()->removeClient (m_viewManager->guiMergedView );

    m_viewManager->guiMergedView = view;

    if (!m_blockViewCreationAndActivation)
      mainWindow()->guiFactory ()->addClient( view );

    mainWindow()->toolBar ()->setUpdatesEnabled (true);

    emit viewChanged ();
  }

  KateDocManager::self()->setActiveDocument((KTextEditor::Document *)view->document());
}

void KateViewSpaceContainer::activateView( KTextEditor::Document *d )
{
  // no doc with this id found
  if (!d)
    return;

  // activate existing view if possible
  if ( activeViewSpace()->showView(d) )
  {
    activateView( activeViewSpace()->currentView() );
    return;
  }

  // create new view otherwise
  createView (d);
}

int KateViewSpaceContainer::viewCount () const
{
  return m_viewList.count();
}

int KateViewSpaceContainer::viewSpaceCount () const
{
  return m_viewSpaceList.count();
}

void KateViewSpaceContainer::slotViewChanged()
{
  if ( activeView() && !activeView()->hasFocus())
    activeView()->setFocus();
}

void KateViewSpaceContainer::activateNextView()
{
  int i = m_viewSpaceList.indexOf (activeViewSpace()) + 1;

  if (i >= m_viewSpaceList.count())
    i=0;

  setActiveSpace (m_viewSpaceList.at(i));
  activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewSpaceContainer::activatePrevView()
{
  int i = m_viewSpaceList.indexOf (activeViewSpace()) - 1;

  if (i < 0)
    i=m_viewSpaceList.count()-1;

  setActiveSpace (m_viewSpaceList.at(i));
  activateView(m_viewSpaceList.at(i)->currentView());
}

void KateViewSpaceContainer::closeViews(KTextEditor::Document *doc)
{
  QList<KTextEditor::View*> closeList;

  for ( QList<KTextEditor::View*>::const_iterator it = m_viewList.begin();
        it != m_viewList.end(); ++it)
  {
    if ( (*it)->document() == doc )
      closeList.append( *it );
  }

  while ( !closeList.isEmpty() )
    deleteView( closeList.takeFirst(), true );

  if (m_blockViewCreationAndActivation) return;
  QTimer::singleShot(0,this,SIGNAL(viewChanged()));
  //emit m_viewManager->viewChanged ();
}

void KateViewSpaceContainer::slotPendingDocumentNameChanged() {
          QString c;
          if (m_pendingDocument->url().isEmpty() || (!showFullPath))
          {
            c = m_pendingDocument->documentName();
          }
          else
          {
            c = m_pendingDocument->url().prettyURL();
          }
          setWindowTitle(KStringHandler::lsqueeze(c,32));
}

void KateViewSpaceContainer::splitViewSpace( KateViewSpace* vs,
                                      bool isHoriz,
                                      bool atTop)
{
  // fallback to activeViewSpace
  if (!vs)
    vs = activeViewSpace();

  // found no viewspace, bah
  if (!vs)
    return;

  // get current splitter
  QSplitter *currentSplitter = qobject_cast<QSplitter*>(vs->parentWidget());

  // no splitter found, bah
  if (!currentSplitter)
    return;

  // which orientation we want?
  Qt::Orientation orientation = isHoriz ? Qt::Vertical : Qt::Horizontal;

  // index where to insert new splitter/viewspace
  int index = currentSplitter->indexOf (vs);

  // if we are not atTop, inc this
  if (!atTop)
    ++index;

  // create new viewspace
  KateViewSpace* vsNew = new KateViewSpace( this, 0);

  // first try if we not can fix the orientation issue
  if (currentSplitter->orientation() != orientation && currentSplitter->count() == 1)
    currentSplitter->setOrientation (orientation);

  // now we have 2 cases, currentSplitter has right orientation or not ;)
  // only if not, we need new splitter and move current vs over to it ;)
  if (currentSplitter->orientation() != orientation)
  {
    QSplitter *oldSplitter = currentSplitter;

    currentSplitter = new QSplitter (orientation);
    oldSplitter->insertWidget (index,currentSplitter);
    
    currentSplitter->setOpaqueResize( KGlobalSettings::opaqueResize() );
    currentSplitter->addWidget (vs);
    currentSplitter->addWidget (vsNew);
    currentSplitter->show ();
  }
  else
  {
    currentSplitter->insertWidget (index, vsNew);
  }

  m_viewSpaceList.append( vsNew );
  activeViewSpace()->setActive( false );
  vsNew->setActive( true, true );
  vsNew->show();

  createView ((KTextEditor::Document*)activeView()->document());

  if (this == m_viewManager->activeContainer())
    m_viewManager->updateViewSpaceActions ();
}

void KateViewSpaceContainer::removeViewSpace (KateViewSpace *viewspace)
{
  // abort if viewspace is 0
  if (!viewspace) return;

  // abort if this is the last viewspace
  if (m_viewSpaceList.count() < 2) return;

  // get current splitter
  QSplitter *currentSplitter = qobject_cast<QSplitter*>(viewspace->parentWidget());

  // no splitter found, bah
  if (!currentSplitter)
    return;

  // delete views of the viewspace
  while (viewspace->viewCount() > 0 && viewspace->currentView())
  {
    deleteView( viewspace->currentView(), false );
  }

  // cu viewspace
  m_viewSpaceList.removeAt( m_viewSpaceList.indexOf( viewspace ) );
  delete viewspace;

  // only do magic with the splitter if it's not the basic one ;)
  // and it has now only one child
  if ((currentSplitter != this) && (currentSplitter->count() == 1))
  {
    // get parent splitter
    QSplitter *parentSplitter = qobject_cast<QSplitter*>(currentSplitter->parentWidget());

    // only do magic if found ;)
    if (parentSplitter)
    {
      int index = parentSplitter->indexOf (currentSplitter);

      // save current splitter size, as the removal of currentSplitter looses the info
      QList<int> parentSizes = parentSplitter->sizes();
      parentSplitter->insertWidget (index, currentSplitter->widget (0));
      delete currentSplitter;
      // now restore the sizes again
      parentSplitter->setSizes(parentSizes);
    }
  }

  // find the view that is now active.
  KTextEditor::View* v = activeViewSpace()->currentView();
  if ( v )
    activateView( v );

  if (this == m_viewManager->activeContainer())
    m_viewManager->updateViewSpaceActions ();

  emit viewChanged();
}

void KateViewSpaceContainer::slotCloseCurrentViewSpace()
{
  removeViewSpace(activeViewSpace());
}

void KateViewSpaceContainer::setShowFullPath( bool enable )
{
  showFullPath = enable;
}

/**
 * session config functions
 */

void KateViewSpaceContainer::saveViewConfiguration(KConfig *config,const QString& group)
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
  const QList<QObject*> &l = children();
  for (QList<QObject*>::const_iterator it( l.begin() ); it != l.end(); ++it)
  {
   QObject *obj = *it;

   if (qobject_cast<QSplitter*>(obj))
   {
     saveSplitterConfig( qobject_cast<QSplitter*>(obj), 0, config , group);
     break;
   }
  }
}

void KateViewSpaceContainer::restoreViewConfiguration (KConfig *config, const QString& group)
{
  config->setGroup(group);
  //config->setGroup ("View Configuration");

  // no splitters around, ohhh :()
  if (!config->readEntry ("Splitters", QVariant(false)).toBool())
  {
    // only add the new views needed, let the old stay, won't hurt if one around
    m_viewSpaceList.first ()->restoreConfig (this, config, QString(group+"-ViewSpace 0"));
  }
  else
  {
    // send all views + their gui to **** ;)
    for (int i=0; i < m_viewList.count(); i++)
      mainWindow()->guiFactory ()->removeClient (m_viewList.at(i));

    // kill views and viewspaces
    qDeleteAll( m_viewList );
    m_viewList.clear();
    qDeleteAll( m_viewSpaceList );
    m_viewSpaceList.clear();
//     while (!m_viewList.isEmpty()) delete m_viewList.takeFirst();
//     while (!m_viewSpaceList.isEmpty()) delete m_viewSpaceList.takeFirst();

    // call restoreSplitter for Splitter 0
    restoreSplitter( config, QString(group+"-Splitter 0"), this,group );
  }

  // finally, make the correct view active.
  config->setGroup (group);
/*
  KateViewSpace *vs = m_viewSpaceList.at( config->readEntry("Active ViewSpace") );
  if ( vs )
    activateSpace( vs->currentView() );
  */
}


void KateViewSpaceContainer::saveSplitterConfig( QSplitter* s, int idx, KConfig* config, const QString& viewConfGrp )
{
  QString grp = QString(viewConfGrp+"-Splitter %1").arg(idx);
  config->setGroup(grp);

  // Save sizes, orient, children for this splitter
  config->writeEntry( "Sizes", s->sizes() );
  config->writeEntry( "Orientation", int(s->orientation()) );

  QStringList childList;
  // a katesplitter has two children, of which one may be a KateSplitter.
  const QList<QObject*> &l = s->children();
  for (QList<QObject*>::const_iterator it( l.begin() ); it != l.end(); ++it)
  {
   QObject *obj = *it;
   KateViewSpace* kvs = qobject_cast<KateViewSpace*>(obj);

   QString n;  // name for child list, see below
   // For KateViewSpaces, ask them to save the file list.
   if ( kvs ) {
     n = QString(viewConfGrp+"-ViewSpace %1").arg( m_viewSpaceList.indexOf(kvs) );
     kvs->saveConfig ( config, m_viewSpaceList.indexOf(kvs), viewConfGrp);
     // save active viewspace
     if ( kvs->isActiveSpace() ) {
       config->setGroup(viewConfGrp);
       config->writeEntry("Active Viewspace", m_viewSpaceList.indexOf(kvs) );
     }
   }
   // For KateSplitters, recurse
   else if ( QSplitter* splitter = qobject_cast<QSplitter*>(obj) ) {
     idx++;
     saveSplitterConfig( splitter, idx, config,viewConfGrp);
     n = QString(viewConfGrp+"-Splitter %1").arg( idx );
   }
   // make sure list goes in right place!
   if (!n.isEmpty()) {
     if ( childList.count() > 0 && ! (s->indexOf( (QWidget*)obj ) > 1) )
       childList.prepend( n );
     else
       childList.append( n );
   }
  }

  // reset config group.
  config->setGroup(grp);
  config->writeEntry("Children", childList);
}

void KateViewSpaceContainer::restoreSplitter( KConfig* config, const QString &group, QWidget* parent, const QString& viewConfGrp)
{
  config->setGroup( group );

  QSplitter* s = new QSplitter((Qt::Orientation)config->readEntry("Orientation",0), parent);

  QStringList children = config->readEntry( "Children",QStringList() );
  for (QStringList::Iterator it=children.begin(); it!=children.end(); ++it)
  {
    // for a viewspace, create it and open all documents therein.
    if ( (*it).startsWith(viewConfGrp+"-ViewSpace") )
    {
     KateViewSpace* vs = new KateViewSpace( this, s );

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
  s->setSizes( config->readEntry("Sizes",QList<int>()) );
  s->show();
}

KateMainWindow *KateViewSpaceContainer::mainWindow() {
  return m_viewManager->mainWindow();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
