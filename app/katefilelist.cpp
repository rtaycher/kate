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

#include "katefilelist.h"
#include "katefilelist.moc"

#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "katemainwindow.h"

#include <qapplication.h>
#include <qpainter.h>
#include <qpopupmenu.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kpassivepopup.h>
// #include <knotifyclient.h>
#include <kdebug.h>
#include <kapplication.h>

KateFileList::KateFileList (KateMainWindow *main,
                            KateViewManager *_viewManager,
                            QWidget * parent, const char * name )
    :  KListBox (parent, name)
    , m_sort( KateFileList::sortByID )
{
  m_main = main;
  
  setFocusPolicy ((QWidget::FocusPolicy)0);

  viewManager = _viewManager;
  tooltip = new KFLToolTip( this );
  
  setupActions ();

  for (uint i = 0; i < KateDocManager::self()->documents(); i++)
  {
    slotDocumentCreated (KateDocManager::self()->document(i));
    slotModChanged (KateDocManager::self()->document(i));
  }

  connect(KateDocManager::self(),SIGNAL(documentCreated(Kate::Document *)),this,SLOT(slotDocumentCreated(Kate::Document *)));
  connect(KateDocManager::self(),SIGNAL(documentDeleted(uint)),this,SLOT(slotDocumentDeleted(uint)));

  // don't Honour KDE single/double click setting, this files are already open, no need for hassle of considering double-click
  connect(this,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivateView(QListBoxItem *)));

  connect(viewManager,SIGNAL(viewChanged()), this,SLOT(slotViewChanged()));

  connect(this,SIGNAL(contextMenuRequested ( QListBoxItem *, const QPoint & )), this,SLOT(slotMenu ( QListBoxItem *, const QPoint & )));
}

KateFileList::~KateFileList ()
{
  delete tooltip;
}

void KateFileList::setupActions ()
{
  windowNext = KStdAction::back(this, SLOT(slotPrevDocument()), m_main->actionCollection());
  windowPrev = KStdAction::forward(this, SLOT(slotNextDocument()), m_main->actionCollection());
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
    slotActivateView(item(currentItem()));
  }
  else
  {
    KListBox::keyPressEvent(e);
  }
}

void KateFileList::slotNextDocument()
{
  int c = currentItem ();

  if ((c == -1) || (count() == 0))
    return;

  if (uint(c+1) < count())
    viewManager->activateView( ((KateFileListItem *)item(c+1))->documentNumber() );
  else
    viewManager->activateView( ((KateFileListItem *)item(0))->documentNumber() );
}

void KateFileList::slotPrevDocument()
{
  int c = currentItem ();

  if ((c == -1) || (count() == 0))
    return;

  if ((c-1) >= 0)
    viewManager->activateView( ((KateFileListItem *)item(c-1))->documentNumber() );
  else
    viewManager->activateView( ((KateFileListItem *)item(count()-1))->documentNumber() );

}

void KateFileList::slotDocumentCreated (Kate::Document *doc)
{
  insertItem (new KateFileListItem (doc, doc->documentNumber(), doc->docName()) );
  connect(doc,SIGNAL(modStateChanged(Kate::Document *)),this,SLOT(slotModChanged(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(slotNameChanged(Kate::Document *)));
  connect(doc,SIGNAL(modifiedOnDisc(Kate::Document *, bool, unsigned char)),this,SLOT(slotModifiedOnDisc(Kate::Document *, bool, unsigned char)));

  updateSort ();
  updateActions ();
}

void KateFileList::slotDocumentDeleted (uint documentNumber)
{
  for (uint i = 0; i < count(); i++)
  {
    if (((KateFileListItem *) item (i)) ->documentNumber() == documentNumber)
    {
      if (count() > 1)
        removeItem( i );
      else
        clear();
    }
  }
  
  updateActions ();
}

void KateFileList::slotActivateView( QListBoxItem *item )
{
  viewManager->activateView( ((KateFileListItem *)item)->documentNumber() );
}

void KateFileList::slotModChanged (Kate::Document *doc)
{
  if (!doc) return;

  for (uint i = 0; i < count(); i++)
  {
    if (((KateFileListItem *) item (i)) ->documentNumber() == doc->documentNumber())
    {
      triggerUpdate(false);
      break;
    }
  }
}

void KateFileList::slotModifiedOnDisc (Kate::Document *doc, bool, unsigned char r)
{
  for (uint i = 0; i < count(); i++)
  {
    if (((KateFileListItem *) item (i)) ->documentNumber() == doc->documentNumber())
    {
      triggerUpdate(false);
      break;
    }
  }

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

//     KNotifyClient::instance();
//     int n = KNotifyClient::event( "file_modified_on_disc",
//           i18n("The document<br><code>%1</code><br>%2").arg( doc->url().prettyURL() ).arg( a ) );
//     kdDebug(13001)<<"The BASTARD returned "<<n<<endl;
    if ( ((KateMainWindow*)topLevelWidget())->notifyMod() )
      KPassivePopup::message( i18n("Warning"),
                              a.arg( doc->url().prettyURL() ),
                              w, topLevelWidget() );
  }
}

void KateFileList::slotNameChanged (Kate::Document *doc)
{
  if (!doc) return;

  for (uint i = 0; i < count(); i++)
  {
    if (((KateFileListItem *) item (i)) ->documentNumber() == doc->documentNumber())
    {
      //File name shouldn't be too long - Maciek
     QString c = doc -> docName();
     if (c.length() > 200)
       c = "..." + c.right(197);

     ((KateFileListItem *)item(i))->setText(c);

      triggerUpdate(false);
      break;
    }
  }

  updateSort ();
}

void KateFileList::slotViewChanged ()
{
  if (!viewManager->activeView()) return;

  Kate::View *view = viewManager->activeView();

  for (uint i = 0; i < count(); i++)
  {
    if (((KateFileListItem *) item (i)) ->documentNumber() == ((Kate::Document *)view->getDoc())->documentNumber())
    {
      setCurrentItem (i);
      if ( !isSelected( item(i) ) )
        setSelected( i, true );
      break;
    }
  }
}

void KateFileList::slotMenu ( QListBoxItem *item, const QPoint &p )
{
  if (!item)
    return;

  QPopupMenu *menu = (QPopupMenu*) ((viewManager->mainWindow())->factory()->container("filelist_popup", viewManager->mainWindow()));

  if (menu)
    menu->exec(p);
}

void KateFileList::tip( const QPoint &p, QRect &r, QString &str )
{
  KateFileListItem *i = (KateFileListItem*)itemAt( p );
  r = itemRect( i );
  str = "";

  if ( !i || !r.isValid() )
    return;

  Kate::Document *doc = KateDocManager::self()->documentWithID(i->documentNumber());

  if (!doc)
    return;

  const KateDocumentInfo *info = KateDocManager::self()->documentInfo(doc);

  if (info && info->modifiedOnDisc)
  {
    if (info->modifiedOnDiscReason == 1)
      str += i18n("<b>This file was changed (modified) on disc by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 2)
      str += i18n("<b>This file was changed (created) on disc by another program.</b><br />");
    else if (info->modifiedOnDiscReason == 3)
      str += i18n("<b>This file was changed (deleted) on disc by another program.</b><br />");
  }

  str += doc->url().prettyURL();
}

KateFileListItem::KateFileListItem( Kate::Document *doc, uint documentNumber, const QString& text): QListBoxItem()
{
  this->doc = doc;
  myDocID = documentNumber;
  setText( text );
}

KateFileListItem::~KateFileListItem()
{
}

uint KateFileListItem::documentNumber ()
{
  return myDocID;
}

void KateFileListItem::setText(const QString &text)
{
  QListBoxItem::setText(text);
}

int KateFileListItem::height( const QListBox* lb ) const
{
  int h;

  if ( text().isEmpty() )
    h = 16;
  else
    h = QMAX( 16, lb->fontMetrics().lineSpacing() + 1 );

  return QMAX( h, QApplication::globalStrut().height() );
}

int KateFileListItem::width( const QListBox* lb ) const
{
  if ( text().isEmpty() )
    return QMAX( 16 + 6, QApplication::globalStrut().width() );

  return QMAX( 16 + lb->fontMetrics().width( text() ) + 6, QApplication::globalStrut().width() );
}

void KateFileListItem::paint( QPainter *painter )
{
  static QPixmap noPm = SmallIcon ("null");
  static QPixmap modPm = SmallIcon("modified");
  static QPixmap discPm = SmallIcon("modonhd");
  static QPixmap modmodPm = SmallIcon("modmod");

  const KateDocumentInfo *info = KateDocManager::self()->documentInfo (doc);

  if (info && info->modifiedOnDisc)
    painter->drawPixmap( 3, 0, doc->isModified() ? modmodPm : discPm );
  else
    painter->drawPixmap( 3, 0, doc->isModified() ? modPm : noPm );

  if ( !text().isEmpty() )
  {
    QFontMetrics fm = painter->fontMetrics();

    int yPos;                       // vertical text position

     if ( 16 < fm.height() )
      yPos = fm.ascent() + fm.leading()/2;
    else
      yPos = 16/2 - fm.height()/2 + fm.ascent();

    painter->drawText( 16 + 4, yPos, text() );
  }
}

/////////////////////////////////////////////////////////////////////
// KateFileList::KFLToolTip implementation

KateFileList::KFLToolTip::KFLToolTip( QWidget *parent )
  : QToolTip( parent )
{
}

void KateFileList::KFLToolTip::maybeTip( const QPoint &p )
{
  QString str;
  QRect r;

  ((KateFileList*)parentWidget())->tip( p, r, str );

  if( !str.isEmpty() && r.isValid() )
    tip( r, str );
}

void KateFileList::setSortType (int s)
{
  m_sort = s;
  updateSort ();
}

void KateFileList::updateSort ()
{
  if (m_sort == KateFileList::sortByName)
    sort ();
}
