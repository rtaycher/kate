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

KateFileList::KateFileList (KateDocManager *_docManager, KateViewManager *_viewManager, QWidget * parent, const char * name ):  KListBox (parent, name)
, m_sort( KateFileList::sortByName )
{
  setFocusPolicy ((QWidget::FocusPolicy)0);

  docManager = _docManager;
  viewManager = _viewManager;
  tooltip = new KFLToolTip( this );

  for (uint i = 0; i < docManager->documents(); i++)
  {
    slotDocumentCreated (docManager->document(i));
    slotModChanged (docManager->document(i));
  }

  connect(docManager,SIGNAL(documentCreated(Kate::Document *)),this,SLOT(slotDocumentCreated(Kate::Document *)));
  connect(docManager,SIGNAL(documentDeleted(uint)),this,SLOT(slotDocumentDeleted(uint)));

  // Honour KDE single/double click setting
  connect(this,SIGNAL(executed(QListBoxItem *)),this,SLOT(slotActivateView(QListBoxItem *)));
  connect(this,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivateView(QListBoxItem *)));

  connect(viewManager,SIGNAL(viewChanged()), this,SLOT(slotViewChanged()));

  connect(this,SIGNAL(rightButtonPressed ( QListBoxItem *, const QPoint & )), this,SLOT(slotMenu ( QListBoxItem *, const QPoint & )));
}

KateFileList::~KateFileList ()
{
}

void KateFileList::slotDocumentCreated (Kate::Document *doc)
{
  insertItem (new KateFileListItem (doc->documentNumber(), SmallIcon("null"), doc->docName(), KGlobalSettings::textColor()) );
  connect(doc,SIGNAL(modStateChanged(Kate::Document *)),this,SLOT(slotModChanged(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(slotNameChanged(Kate::Document *)));
  connect(doc,SIGNAL(modifiedOnDisc(Kate::Document *, bool)),this,SLOT(slotModifiedOnDisc(Kate::Document *, bool)));

  updateSort ();
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
}

void KateFileList::slotActivateView( QListBoxItem *item )
{
  viewManager->activateView( ((KateFileListItem *)item)->documentNumber() );
}

void KateFileList::slotModChanged (Kate::Document *doc)
{
  if (!doc) return;

  uint i;

  if( doc->isModified() )
  {
    for (i = 0; i < count(); i++)
    {
      if (((KateFileListItem *) item (i)) ->documentNumber() == doc->documentNumber())
      {
        ((KateFileListItem *)item(i))->setPixmap(SmallIcon("modified"));
        ((KateFileListItem *)item(i))->setBold(true);

        triggerUpdate(false);
        break;
      }
    }
  }
  else
  {
    for (i = 0; i < count(); i++)
    {
      if (((KateFileListItem *) item (i)) ->documentNumber() == doc->documentNumber())
      {
        ((KateFileListItem *)item(i))->setPixmap(SmallIcon("null"));
        ((KateFileListItem *)item(i))->setBold(false);

        triggerUpdate(false);
        break;
      }
    }
  }
}

void KateFileList::slotModifiedOnDisc (Kate::Document *doc, bool b)
{
  for (uint i = 0; i < count(); i++)
  {
    if (((KateFileListItem *) item (i)) ->documentNumber() == doc->documentNumber())
    {
     ((KateFileListItem *)item(i))->setColor (b ? QColor ("red") : KGlobalSettings::textColor());

      kdDebug() << "testing mod works" << endl;

      triggerUpdate(false);
      break;
    }
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

  QString c;
  if (doc->url().isEmpty() || (!viewManager->getShowFullPath()))
  {
    c = doc->docName();

    //File name shouldn't be too long - Maciek
    if (c.length() > 64)
      c = c.left(64) + "...";
  }
  else
  {
    c = doc->url().prettyURL();

    //File name shouldn't be too long - Maciek
    if (c.length() > 64)
      c = "..." + c.right(64);
  }

  ((KateMainWindow*)topLevelWidget())->setCaption( c, doc->isModified());

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

  QPopupMenu *menu = (QPopupMenu*) ((KMainWindow *)(viewManager->topLevelWidget)())->factory()->container("filelist_popup", (KMainWindow *)(viewManager->topLevelWidget ()));
  menu->exec(p);
}

void KateFileList::tip( const QPoint &p, QRect &r, QString &str )
{
  KateFileListItem *i = (KateFileListItem*)itemAt( p );
  r = itemRect( i );

  if( i != NULL && r.isValid() )
    str = docManager->documentWithID(i->documentNumber())->url().prettyURL();
  else
    str = "";
}

KateFileListItem::KateFileListItem( uint documentNumber, const QPixmap &pix, const QString& text, const QColor &col): QListBoxItem()
{
  _bold=false;
  _color = col;
  myDocID = documentNumber;
  setPixmap(pix);
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

void KateFileListItem::setColor (const QColor &col)
{
  _color = col;
}

void KateFileListItem::setPixmap(const QPixmap &pixmap)
{
  pm=pixmap;
}

void KateFileListItem::setBold(bool bold)
{
  bold=bold;
}

int KateFileListItem::height( const QListBox* lb ) const
{
  int h;

  if ( text().isEmpty() )
    h = pm.height();
  else
    h = QMAX( pm.height(), lb->fontMetrics().lineSpacing() + 1 );

  return QMAX( h, QApplication::globalStrut().height() );
}

int KateFileListItem::width( const QListBox* lb ) const
{
  if ( text().isEmpty() )
    return QMAX( pm.width() + 6, QApplication::globalStrut().width() );

  return QMAX( pm.width() + lb->fontMetrics().width( text() ) + 6, QApplication::globalStrut().width() );
}

void KateFileListItem::paint( QPainter *painter )
{
  painter->drawPixmap( 3, 0, pm );
  QFont f=painter->font();
  f.setBold(_bold);
  painter->setFont(f);

  if ( !text().isEmpty() )
  {
    QFontMetrics fm = painter->fontMetrics();
    int yPos;                       // vertical text position

    if ( pm.height() < fm.height() )
      yPos = fm.ascent() + fm.leading()/2;
    else
      yPos = pm.height()/2 - fm.height()/2 + fm.ascent();

    painter->setPen (_color);
    painter->drawText( pm.width() + 5, yPos, text() );
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
