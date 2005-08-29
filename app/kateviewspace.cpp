/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2005 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateviewspace.h"
#include "kateviewspace.moc"

#include "katemainwindow.h"
#include "kateviewspacecontainer.h"
#include "katedocmanager.h"
#include "kateapp.h"
#include "katesession.h"

#include <ktexteditor/sessionconfiginterface.h>

#include <kiconloader.h>
#include <klocale.h>
#include <ksqueezedtextlabel.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kstringhandler.h>

#include <QStackedWidget>
#include <qpainter.h>
#include <qlabel.h>
#include <qcursor.h>
#include <QMenu>
#include <qpixmap.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QEvent>
#include <QMouseEvent>

//BEGIN KVSSBSep
/*
   "KateViewSpaceStatusBarSeparator"
   A 2 px line to separate the statusbar from the view.
   It is here to compensate for the lack of a frame in the view,
   I think Kate looks very nice this way, as QScrollView with frame
   looks slightly clumsy...
   Slight 3D effect. I looked for suitable QStyle props or methods,
   but found none, though maybe it should use QStyle::PM_DefaultFrameWidth
   for height (TRY!).
   It does look a bit funny with flat styles (Light, .Net) as is,
   but there are on methods to paint panel lines separately. And,
   those styles tends to look funny on their own, as a light line
   in a 3D frame next to a light contents widget is not functional.
   Also, QStatusBar is up to now completely ignorant to style.
   -anders
*/
class KVSSBSep : public QWidget {
public:
  KVSSBSep( KateViewSpace *parent=0) : QWidget(parent)
  {
    setFixedHeight( 2 );
  }
protected:
  void paintEvent( QPaintEvent *e )
  {
    QPainter p( this );
    p.setPen( colorGroup().shadow() );
    p.drawLine( e->rect().left(), 0, e->rect().right(), 0 );
    p.setPen( ((KateViewSpace*)parentWidget())->isActiveSpace() ? colorGroup().light() : colorGroup().midlight() );
    p.drawLine( e->rect().left(), 1, e->rect().right(), 1 );
  }
};
//END KVSSBSep

//BEGIN KateViewSpace
KateViewSpace::KateViewSpace( KateViewSpaceContainer *viewManager,
                              QWidget* parent, const char* name )
  : Q3VBox(parent, name),
    m_viewManager( viewManager )
{
  mViewList.setAutoDelete(false);

  stack = new QStackedWidget( this );
  stack->setFocus();
  stack->setSizePolicy (QSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));

  //sep = new KVSSBSep( this );
  mStatusBar = new KateVSStatusBar(this);
  mIsActiveSpace = false;
  mViewCount = 0;

  setMinimumWidth (mStatusBar->minimumWidth());
  m_group = QString::null;
}

KateViewSpace::~KateViewSpace()
{
}

void KateViewSpace::polish()
{
  mStatusBar->show();
}

void KateViewSpace::addView(KTextEditor::View* v, bool show)
{
  // restore the config of this view if possible
  if ( !m_group.isEmpty() )
  {
    QString fn = v->document()->url().prettyURL();
    if ( ! fn.isEmpty() )
    {
      QString vgroup = QString("%1 %2").arg(m_group).arg(fn);

      KateSession::Ptr as = KateSessionManager::self()->activeSession ();
      if ( as->configRead() && as->configRead()->hasGroup( vgroup ) )
      {
        as->configRead()->setGroup( vgroup );

        if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v))
          iface->readSessionConfig ( as->configRead() );
      }
    }
  }

  stack->addWidget(v);
  if (show) {
    mViewList.append(v);
    showView( v );
  }
  else {
    KTextEditor::View* c = mViewList.current();
    mViewList.prepend( v );
    showView( c );
  }

  // signals for the statusbar
  connect(v, SIGNAL(cursorPositionChanged(KTextEditor::View *)), mStatusBar, SLOT(cursorPositionChanged(KTextEditor::View *)));
  connect(v, SIGNAL(viewModeChanged(KTextEditor::View *)), mStatusBar, SLOT(viewModeChanged(KTextEditor::View *)));
  connect(v, SIGNAL(selectionChanged (KTextEditor::View *)), mStatusBar, SLOT(selectionChanged (KTextEditor::View *)));
  connect(v, SIGNAL(informationMessage (KTextEditor::View *, const QString &)), mStatusBar, SLOT(informationMessage (KTextEditor::View *, const QString &)));
  connect(v->document(), SIGNAL(modifiedChanged(KTextEditor::Document *)), mStatusBar, SLOT(modifiedChanged()));
  connect(v->document(), SIGNAL(modifiedOnDisk(KTextEditor::Document *, bool, KTextEditor::ModificationInterface::ModifiedOnDiskReason)), mStatusBar, SLOT(modifiedChanged()) );
  connect(v->document(), SIGNAL(documentNameChanged(KTextEditor::Document *)), mStatusBar, SLOT(documentNameChanged()));
}

void KateViewSpace::removeView(KTextEditor::View* v)
{
  bool active = ( v == currentView() );

  mViewList.remove (v);
  stack->removeWidget (v);

  if ( ! active )
    return;

  if (currentView() != 0L)
    showView(mViewList.current());
  else if (mViewList.count() > 0)
    showView(mViewList.last());
}

bool KateViewSpace::showView(KTextEditor::Document *document)
{
  Q3PtrListIterator<KTextEditor::View> it (mViewList);
  it.toLast();
  for( ; it.current(); --it )
  {
    if (it.current()->document() == document)
    {
      KTextEditor::View* kv = it.current();

      mViewList.removeRef( kv );
      mViewList.append( kv );
      stack->setCurrentWidget( kv );
      kv->show();

      mStatusBar->updateStatus ();

      return true;
    }
  }
   return false;
}


KTextEditor::View* KateViewSpace::currentView()
{
  if (mViewList.count() > 0)
    return (KTextEditor::View*)stack->currentWidget();

  return 0L;
}

bool KateViewSpace::isActiveSpace()
{
  return mIsActiveSpace;
}

void KateViewSpace::setActive( bool active, bool )
{
  mIsActiveSpace = active;

  // change the statusbar palette and make sure it gets updated
  QPalette pal( palette() );
  if ( ! active )
  {
    pal.setColor( QColorGroup::Background, pal.active().mid() );
    pal.setColor( QColorGroup::Light, pal.active().midlight() );
  }

  mStatusBar->setPalette( pal );
  mStatusBar->update();
  //sep->update();
}

bool KateViewSpace::event( QEvent *e )
{
  if ( e->type() == QEvent::PaletteChange )
  {
    setActive( mIsActiveSpace );
    return true;
  }
  return Q3VBox::event( e );
}

void KateViewSpace::saveConfig ( KConfig* config, int myIndex ,const QString& viewConfGrp)
{
//   kdDebug()<<"KateViewSpace::saveConfig("<<myIndex<<", "<<viewConfGrp<<") - currentView: "<<currentView()<<")"<<endl;
  QString group = QString(viewConfGrp+"-ViewSpace %1").arg( myIndex );

  config->setGroup (group);
  config->writeEntry ("Count", mViewList.count());

  if (currentView())
    config->writeEntry( "Active View", currentView()->document()->url().prettyURL() );

  // Save file list, includeing cursor position in this instance.
  Q3PtrListIterator<KTextEditor::View> it(mViewList);

  int idx = 0;
  for (; it.current(); ++it)
  {
    if ( !it.current()->document()->url().isEmpty() )
    {
      config->setGroup( group );
      config->writeEntry( QString("View %1").arg( idx ), it.current()->document()->url().prettyURL() );

      // view config, group: "ViewSpace <n> url"
      QString vgroup = QString("%1 %2").arg(group).arg(it.current()->document()->url().prettyURL());
      config->setGroup( vgroup );

      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(it.current()))
          iface->writeSessionConfig( config );
    }

    idx++;
  }
}

void KateViewSpace::restoreConfig ( KateViewSpaceContainer *viewMan, KConfig* config, const QString &group )
{
  config->setGroup (group);
  QString fn = config->readEntry( "Active View" );

  if ( !fn.isEmpty() )
  {
    KTextEditor::Document *doc = KateDocManager::self()->findDocument (KURL(fn));

    if (doc)
    {
      // view config, group: "ViewSpace <n> url"
      QString vgroup = QString("%1 %2").arg(group).arg(fn);
      config->setGroup( vgroup );

      viewMan->createView (doc);

      KTextEditor::View *v = viewMan->activeView ();

      if (KTextEditor::SessionConfigInterface *iface = qobject_cast<KTextEditor::SessionConfigInterface *>(v))
        iface->readSessionConfig( config );
    }
  }

  if (mViewList.isEmpty())
    viewMan->createView (KateDocManager::self()->document(0));

  m_group = group; // used for restroing view configs later
}
//END KateViewSpace

//BEGIN KateVSStatusBar
KateVSStatusBar::KateVSStatusBar ( KateViewSpace *parent, const char *name )
  : KStatusBar( parent, name ),
    m_viewSpace( parent )
{
  m_lineColLabel = new QLabel( this );
  addWidget( m_lineColLabel, 0, false );
  m_lineColLabel->setAlignment( Qt::AlignCenter );
  m_lineColLabel->installEventFilter( this );

  m_modifiedLabel = new QLabel( QString("   "), this );
  addWidget( m_modifiedLabel, 0, false );
  m_modifiedLabel->setAlignment( Qt::AlignCenter );
  m_modifiedLabel->installEventFilter( this );

  m_insertModeLabel = new QLabel( i18n(" INS "), this );
  addWidget( m_insertModeLabel, 0, false );
  m_insertModeLabel->setAlignment( Qt::AlignCenter );
  m_insertModeLabel->installEventFilter( this );

  m_selectModeLabel = new QLabel( i18n(" NORM "), this );
  addWidget( m_selectModeLabel, 0, false );
  m_selectModeLabel->setAlignment( Qt::AlignCenter );
  m_selectModeLabel->installEventFilter( this );

  m_fileNameLabel=new KSqueezedTextLabel( this );
  addWidget( m_fileNameLabel, 1, true );
  m_fileNameLabel->setMinimumSize( 0, 0 );
  m_fileNameLabel->setSizePolicy(QSizePolicy( QSizePolicy::Ignored, QSizePolicy::Fixed ));
  m_fileNameLabel->setAlignment( /*Qt::AlignRight*/Qt::AlignLeft );
  m_fileNameLabel->installEventFilter( this );

  installEventFilter( this );
  m_modPm = SmallIcon("modified");
  m_modDiscPm = SmallIcon("modonhd");
  m_modmodPm = SmallIcon("modmod");
  m_noPm = SmallIcon("null");
}

KateVSStatusBar::~KateVSStatusBar ()
{
}

void KateVSStatusBar::showMenu()
{
   KMainWindow* mainWindow = static_cast<KMainWindow*>( window() );
   QMenu* menu = static_cast<QMenu*>( mainWindow->factory()->container("viewspace_popup", mainWindow ) );

   if (menu)
     menu->exec(QCursor::pos());
}

bool KateVSStatusBar::eventFilter(QObject*,QEvent *e)
{
  if (e->type()==QEvent::MouseButtonPress)
  {
    if ( m_viewSpace->currentView() )
      m_viewSpace->currentView()->setFocus();

    if ( ((QMouseEvent*)e)->button()==Qt::RightButton)
      showMenu();

    return true;
  }

  return false;
}

void KateVSStatusBar::updateStatus ()
{
  if (!m_viewSpace->currentView())
    return;

  viewModeChanged (m_viewSpace->currentView());
  cursorPositionChanged (m_viewSpace->currentView());
  selectionChanged (m_viewSpace->currentView());
  modifiedChanged ();
  documentNameChanged ();
}

void KateVSStatusBar::viewModeChanged ( KTextEditor::View *view )
{
  if (view != m_viewSpace->currentView())
    return;

  m_insertModeLabel->setText( view->viewMode() );
}

void KateVSStatusBar::cursorPositionChanged ( KTextEditor::View *view )
{
  if (view != m_viewSpace->currentView())
    return;

  KTextEditor::Cursor position (view->cursorPositionVirtual());

  m_lineColLabel->setText(
    i18n(" Line: %1 Col: %2 ").arg(KGlobal::locale()->formatNumber(position.line()+1, 0))
                              .arg(KGlobal::locale()->formatNumber(position.column()+1, 0)) );
}

void KateVSStatusBar::selectionChanged (KTextEditor::View *view)
{
  if (view != m_viewSpace->currentView())
    return;

  m_selectModeLabel->setText( view->blockSelection() ? i18n(" BLK ") : i18n(" NORM ") );
}

void KateVSStatusBar::informationMessage (KTextEditor::View *view, const QString &message)
{
  if (view != m_viewSpace->currentView())
    return;

  m_fileNameLabel->setText( message );

  // timer to reset this after 4 seconds
  QTimer::singleShot(4000, this, SLOT(documentNameChanged()));
}

void KateVSStatusBar::modifiedChanged()
{
  KTextEditor::View *v = m_viewSpace->currentView();

  if ( v )
  {
    bool mod = v->document()->isModified();

    const KateDocumentInfo *info
      = KateDocManager::self()->documentInfo ( v->document() );

    bool modOnHD = info && info->modifiedOnDisc;

    m_modifiedLabel->setPixmap(
        mod ?
          info && modOnHD ?
            m_modmodPm :
            m_modPm :
          info && modOnHD ?
            m_modDiscPm :
        m_noPm
        );
  }
}

void KateVSStatusBar::documentNameChanged ()
{
  KTextEditor::View *v = m_viewSpace->currentView();

  if ( v )
    m_fileNameLabel->setText( KStringHandler::lsqueeze(v->document()->documentName (), 64) );
}

//END KateVSStatusBar

// kate: space-indent on; indent-width 2; replace-tabs on;
