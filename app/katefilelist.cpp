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
#include "katefilelist.h"
#include "katefilelist.moc"

#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "katemainwindow.h"

#include <qapplication.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qheader.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kpassivepopup.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kstringhandler.h>
//END Includes

//BEGIN ToolTip
class ToolTip : public QToolTip
{
  public:
    ToolTip( QWidget *parent, KateFileList *lv )
      : QToolTip( parent ),
    m_listView( lv )
    {
    }
    virtual ~ToolTip() {};

    void maybeTip( const QPoint &pos )
    {
      QListViewItem *i = m_listView->itemAt( pos );
      if ( ! i ) return;

      KateFileListItem *item = ((KateFileListItem*)i);
      if ( ! item ) return;

      tip( m_listView->itemRect( i ), m_listView->tooltip( item, 0 ) );

    }

  private:
    KateFileList *m_listView;
};

//END ToolTip

//BEGIN KateFileList
KateFileList::KateFileList (KateMainWindow *main,
                            KateViewManager *_viewManager,
                            QWidget * parent, const char * name )
    :  KListView (parent, name)
    , m_sort( KateFileList::sortByID )
{
  m_main = main;
  m_tooltip = new ToolTip( viewport(), this );

  setFocusPolicy ( QWidget::NoFocus  );

  viewManager = _viewManager;

  header()->hide();
  addColumn("Document Name");

  setSelectionMode( QListView::Single );
  setSorting( 0, true );
  setShowToolTips( false );

  setupActions ();

  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
  {
    slotDocumentCreated (KateDocManager::self()->document(i));
    slotModChanged (KateDocManager::self()->document(i));
  }

  connect(KateDocManager::self(),SIGNAL(documentCreated(Kate::Document *)),
	  this,SLOT(slotDocumentCreated(Kate::Document *)));
  connect(KateDocManager::self(),SIGNAL(documentDeleted(uint)),
	  this,SLOT(slotDocumentDeleted(uint)));

  // don't Honour KDE single/double click setting, this files are already open,
  // no need for hassle of considering double-click
  connect(this,SIGNAL(selectionChanged(QListViewItem *)),
	  this,SLOT(slotActivateView(QListViewItem *)));
  connect(viewManager,SIGNAL(viewChanged()), this,SLOT(slotViewChanged()));
  connect(this,SIGNAL(contextMenuRequested( QListViewItem *, const QPoint &, int )),
	  this,SLOT(slotMenu ( QListViewItem *, const QPoint &, int )));
}

KateFileList::~KateFileList ()
{
  delete m_tooltip;
}

void KateFileList::setupActions ()
{
  windowNext = KStdAction::back(this, SLOT(slotPrevDocument()), m_main->actionCollection());
  windowPrev = KStdAction::forward(this, SLOT(slotNextDocument()), m_main->actionCollection());
  KSelectAction *a = new KSelectAction( i18n("Sort &by"), 0,
      m_main->actionCollection(), "filelist_sortby"  );
  QStringList l;
  l << i18n("Opening Order") << i18n("Document Name") << i18n("URL");
  a->setItems( l );
  connect( a, SIGNAL(activated(int)), this, SLOT(setSortType(int)) );
}

void KateFileList::updateActions ()
{
  windowNext->setEnabled(KateDocManager::self()->documents()  > 1);
  windowPrev->setEnabled(KateDocManager::self()->documents()  > 1);
}

void KateFileList::keyPressEvent(QKeyEvent *e) {
  if ( ( e->key() == Key_Return ) || ( e->key() == Key_Enter ) )
  {
    e->accept();
    slotActivateView( currentItem() );
  }
  else
  {
    KListView::keyPressEvent(e);
  }
}

// Protect single mode selection: don't let them
// leftclick outside items.
// ### if we get to accept keyboard navigation, set focus before
// returning
void KateFileList::contentsMousePressEvent( QMouseEvent *e )
{
  if ( (e->button() == LeftButton) && ! itemAt(  e->pos() ) )
    return;

  KListView::contentsMousePressEvent( e );
}

void KateFileList::resizeEvent( QResizeEvent *e )
{
  KListView::resizeEvent( e );

  // ### We may want to actually calculate the widest field,
  // since it's not automatically scrinked. If I add support for
  // tree or marks, the changes of the required width will vary
  // a lot with opening/closing of files and display changes for
  // the mark branches.
  int w = viewport()->width();
  if ( columnWidth( 0 ) < w )
    setColumnWidth( 0, w );
}

void KateFileList::slotNextDocument()
{
  if ( ! currentItem() || childCount() == 0 )
    return;

  // ### more checking once more item types are added

  if ( currentItem()->nextSibling() )
    viewManager->activateView( ((KateFileListItem*)currentItem()->nextSibling())->documentNumber() );
  else
    viewManager->activateView( ((KateFileListItem *)firstChild())->documentNumber() );
}

void KateFileList::slotPrevDocument()
{
  if ( ! currentItem() || childCount() == 0 )
    return;

  // ### more checking once more item types are added

  if ( currentItem()->itemAbove() )
    viewManager->activateView( ((KateFileListItem*)currentItem()->itemAbove())->documentNumber() );
  else
    viewManager->activateView( ((KateFileListItem *)lastItem())->documentNumber() );
}

void KateFileList::slotDocumentCreated (Kate::Document *doc)
{
  new KateFileListItem( this, doc/*, doc->documentNumber()*/ );
  connect(doc,SIGNAL(modStateChanged(Kate::Document *)),this,SLOT(slotModChanged(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(slotNameChanged(Kate::Document *)));
  connect(doc,SIGNAL(modifiedOnDisc(Kate::Document *, bool, unsigned char)),this,SLOT(slotModifiedOnDisc(Kate::Document *, bool, unsigned char)));

  sort();
  updateActions ();
}

void KateFileList::slotDocumentDeleted (uint documentNumber)
{
  QListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem *)item)->documentNumber() == documentNumber )
    {
      delete item;
      break;
    }
    item = item->nextSibling();
  }

  updateActions ();
}

void KateFileList::slotActivateView( QListViewItem *item )
{
  if ( ! item || item->rtti() != RTTI_KateFileListItem )
    return;

  viewManager->activateView( ((KateFileListItem *)item)->documentNumber() );
}

void KateFileList::slotModChanged (Kate::Document *doc)
{
  if (!doc) return;

  QListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem *)item)->documentNumber() == doc->documentNumber() )
    {
      repaintItem( item );
      break;
    }
    item = item->nextSibling();
  }
}

void KateFileList::slotModifiedOnDisc (Kate::Document *doc, bool, unsigned char r)
{
  slotModChanged( doc );

  if ( r != 0 )
  {
    QPixmap w( BarIcon("messagebox_warning", 32) );
    QString a;
    if ( r == 1 )
      a = i18n("The document<br><code>%1</code><br>was changed on disk by another process.");
    else if ( r == 2 )
      a = i18n("The document<br><code>%1</code><br>was created on disk by another process.");
    else if ( r == 3 )
      a = i18n("The document<br><code>%1</code><br>was deleted from disk by another process");

    if ( ((KateMainWindow*)topLevelWidget())->notifyMod() )
      KPassivePopup::message( i18n("Warning"),
                              a.arg( doc->url().prettyURL() ),
                              w, topLevelWidget() );
  }
}

void KateFileList::slotNameChanged (Kate::Document *doc)
{
  if (!doc) return;

  // ### using nextSibling to *only* look at toplevel items.
  // child items could be marks for example
  QListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem *)item)->documentNumber() == doc->documentNumber() )
    {
      item->setText( 0, doc->docName() );
      repaintItem( item );
      break;
    }
    item = item->nextSibling();
  }
  updateSort();
}

void KateFileList::slotViewChanged ()
{
  if (!viewManager->activeView()) return;

  Kate::View *view = viewManager->activeView();
  uint dn = view->getDoc()->documentNumber();

  QListViewItem * item = firstChild();
  while( item ) {
    if ( ((KateFileListItem *)item)->documentNumber() == dn )
    {
      setCurrentItem( item );
      item->setSelected( true );
      break;
    }
    item = item->nextSibling();
  }
}

void KateFileList::slotMenu ( QListViewItem *item, const QPoint &p, int /*col*/ )
{
  if (!item)
    return;

  QPopupMenu *menu = (QPopupMenu*) ((viewManager->mainWindow())->factory()->container("filelist_popup", viewManager->mainWindow()));

  if (menu)
    menu->exec(p);
}

QString KateFileList::tooltip( QListViewItem *item, int )
{
  KateFileListItem *i = ((KateFileListItem*)item);
  if ( ! i ) return QString::null;

  QString str;
  const KateDocumentInfo *info = KateDocManager::self()->documentInfo(i->document());

  if (info && info->modifiedOnDisc)
  {
    if (info->modifiedOnDiscReason == 1)
      str += i18n("<b>This file was changed (modified) on disc by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 2)
      str += i18n("<b>This file was changed (created) on disc by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 3)
      str += i18n("<b>This file was changed (deleted) on disc by another program.</b><br />");
  }

  str += i->document()->url().prettyURL();
  return str;
}


void KateFileList::setSortType (int s)
{
  m_sort = s;
  updateSort ();
}

void KateFileList::updateSort ()
{
  sort ();
}

//END KateFileList

//BEGIN KateFileListItem
KateFileListItem::KateFileListItem( QListView* lv,
				    Kate::Document *_doc )
  : QListViewItem( lv, _doc->docName() ),
    doc( _doc )
{
}

KateFileListItem::~KateFileListItem()
{
}

int KateFileListItem::height() const
{
  int h;

  if ( text( 0 ).isEmpty() )
    h = 16;
  else
    h = QMAX( 16, listView()->fontMetrics().lineSpacing() + 1 );

  return QMAX( h, QApplication::globalStrut().height() );
}

int KateFileListItem::width( const QFontMetrics &fm, const QListView* /*lv*/, int column ) const
{
  if ( text( 0 ).isEmpty() )
    return QMAX( 16 + 6, QApplication::globalStrut().width() );

  return QMAX( 16 + fm.width( text(column) ) + 6, QApplication::globalStrut().width() );
}

void KateFileListItem::paintCell( QPainter *painter, const QColorGroup & cg, int column, int width, int align )
{
  switch ( column ) {
    case 0:
    {
      static QPixmap noPm = SmallIcon ("null");
      static QPixmap modPm = SmallIcon("modified");
      static QPixmap discPm = SmallIcon("modonhd");
      static QPixmap modmodPm = SmallIcon("modmod");

      const KateDocumentInfo *info = KateDocManager::self()->documentInfo (doc);

      painter->fillRect( 0, 0, width, height(), isSelected() ? cg.highlight() : cg.base()  );

      if (info && info->modifiedOnDisc)
	painter->drawPixmap( 3, 0, doc->isModified() ? modmodPm : discPm );
      else
	painter->drawPixmap( 3, 0, doc->isModified() ? modPm : noPm );

      if ( !text( 0 ).isEmpty() )
      {
	QFontMetrics fm = painter->fontMetrics();
	painter->setPen( isSelected() ? cg.highlightedText() : cg.text() );

	int yPos;                       // vertical text position

	if ( 16 < fm.height() )
	  yPos = fm.ascent() + fm.leading()/2;
	else
	  yPos = 16/2 - fm.height()/2 + fm.ascent();

	painter->drawText( 16 + 4, yPos,
			   KStringHandler::rPixelSqueeze( text(0), painter->fontMetrics(), width - 20 ) );
      }
      break;
    }
    default:
      QListViewItem::paintCell( painter, cg, column, width, align );
  }
}

int KateFileListItem::compare ( QListViewItem * i, int col, bool ascending ) const
{
  if ( i->rtti() == RTTI_KateFileListItem )
  {
    switch( ((KateFileList*)listView())->sortType() )
    {
      case KateFileList::sortByID:
      {

        int d = (int)doc->documentNumber() - ((KateFileListItem*)i)->documentNumber();
        return ascending ? d : -d;
        break;
      }
      case KateFileList::sortByURL:
        return doc->url().prettyURL().compare( ((KateFileListItem*)i)->document()->url().prettyURL() );
        break;
      default:
        return QListViewItem::compare( i, col, ascending );
    }
  }
  return 0;
}
//END KateFileListItem


// kate: space-indent on; indent-width 2; replace-tabs on;
