/***************************************************************************
                          kantfilelist.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
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

#include "kantfilelist.h"
#include "kantfilelist.moc"

#include "../document/kantdocument.h"
#include "../document/kantdocmanager.h"
#include "../view/kantview.h"
#include "../view/kantviewmanager.h"
#include "../mainwindow/kantmainwindow.h"

#include <qapplication.h>

#include <kiconloader.h>
#include <klocale.h>

KantFileList::KantFileList (KantDocManager *_docManager, KantViewManager *_viewManager, QWidget * parent, const char * name ):  KListBox (parent, name)
{
  docManager = _docManager;
  viewManager = _viewManager;

  tooltip = new KFLToolTip( this );

  for (uint i = 0; i < docManager->docCount(); i++)
  {
    slotDocumentCreated (docManager->nthDoc(i));
    slotModChanged (docManager->nthDoc(i));
  }

  connect(docManager,SIGNAL(documentCreated(KantDocument *)),this,SLOT(slotDocumentCreated(KantDocument *)));
  connect(docManager,SIGNAL(documentDeleted(long)),this,SLOT(slotDocumentDeleted(long)));

  connect(this,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivateView(QListBoxItem *)));
  connect(this,SIGNAL(selected(QListBoxItem *)), this,SLOT(slotActivateView(QListBoxItem *)));

  connect(viewManager,SIGNAL(viewChanged()), this,SLOT(slotViewChanged()));

  connect(this,SIGNAL(rightButtonPressed ( QListBoxItem *, const QPoint & )), this,SLOT(slotMenu ( QListBoxItem *, const QPoint & )));
}

KantFileList::~KantFileList ()
{
}

void KantFileList::slotDocumentCreated (KantDocument *doc)
{
  insertItem (new KantFileListItem (doc->docID(), SmallIcon("null"), doc->docName()) );
  connect(doc,SIGNAL(modStateChanged(KantDocument *)),this,SLOT(slotModChanged(KantDocument *)));
  connect(doc,SIGNAL(nameChanged(KantDocument *)),this,SLOT(slotNameChanged(KantDocument *)));
  sort();
}

void KantFileList::slotDocumentDeleted (long docID)
{
  for (uint i = 0; i < count(); i++)
  {
    if (((KantFileListItem *) item (i)) ->docID() == docID)
    {
      if (count() > 1)
        removeItem( i );
      else
        clear();
    }
  }
}

void KantFileList::slotActivateView( QListBoxItem *item )
{
  viewManager->activateView( ((KantFileListItem *)item)->docID() );
}

void KantFileList::slotModChanged (KantDocument *doc)
{
  if (!doc) return;

  uint i;

  if( doc->isModified() )
  {
    for (i = 0; i < count(); i++)
    {
      if (((KantFileListItem *) item (i)) ->docID() == doc->docID())
      {
        ((KantFileListItem *)item(i))->setPixmap(SmallIcon("modified"));
        ((KantFileListItem *)item(i))->setBold(true);

        triggerUpdate(false);
        break;
      }
    }
  }
  else
  {
    for (i = 0; i < count(); i++)
    {
      if (((KantFileListItem *) item (i)) ->docID() == doc->docID())
      {
        ((KantFileListItem *)item(i))->setPixmap(SmallIcon("null"));
        ((KantFileListItem *)item(i))->setBold(false);

        triggerUpdate(false);
        break;
      }
    }
  }
}

void KantFileList::slotNameChanged (KantDocument *doc)
{
  if (!doc) return;

  for (uint i = 0; i < count(); i++)
  {
    if (((KantFileListItem *) item (i)) ->docID() == doc->docID())
    {
      ((KantFileListItem *)item(i))->setText(doc->docName());
      triggerUpdate(false);
      break;
    }
  }
  sort();
}

void KantFileList::slotViewChanged ()
{
  if (!viewManager->activeView()) return;

  KantView *view = viewManager->activeView();

  for (uint i = 0; i < count(); i++)
  {
    if (((KantFileListItem *) item (i)) ->docID() == ((KantDocument *)view->doc())->docID())
    {
      setCurrentItem (i);
      if ( !isSelected( item(i) ) )
        setSelected( i, true );
      break;
    }
  }
}

void KantFileList::slotMenu ( QListBoxItem *item, const QPoint &p )
{
  if (!item)
    return;

  QPopupMenu *menu = (QPopupMenu*) ((KMainWindow *)topLevelWidget ())->factory()->container("filelist_popup", (KMainWindow *)topLevelWidget ());
  menu->exec(p);
}

void KantFileList::tip( const QPoint &p, QRect &r, QString &str )
{
  KantFileListItem *i = (KantFileListItem*)itemAt( p );
  r = itemRect( i );

  if( i != NULL && r.isValid() )
    str = docManager->docWithID(i->docID())->url().prettyURL();
  else
    str = "";
}

KantFileListItem::KantFileListItem( long docID, const QPixmap &pix, const QString& text): QListBoxItem()
{
  _bold=false;
  myDocID = docID;
  setPixmap(pix);
  setText( text );
}

KantFileListItem::~KantFileListItem()
{
}

long KantFileListItem::docID ()
{
  return myDocID;
}


void KantFileListItem::setText(const QString &text)
{
  QListBoxItem::setText(text);
}

void KantFileListItem::setPixmap(const QPixmap &pixmap)
{
  pm=pixmap;
}

void KantFileListItem::setBold(bool bold)
{
  bold=bold;
}

int KantFileListItem::height( const QListBox* lb ) const
{
  int h;

  if ( text().isEmpty() )
    h = pm.height();
  else
    h = QMAX( pm.height(), lb->fontMetrics().lineSpacing() + 1 );

  return QMAX( h, QApplication::globalStrut().height() );
}

int KantFileListItem::width( const QListBox* lb ) const
{
  if ( text().isEmpty() )
    return QMAX( pm.width() + 6, QApplication::globalStrut().width() );

  return QMAX( pm.width() + lb->fontMetrics().width( text() ) + 6, QApplication::globalStrut().width() );
}

void KantFileListItem::paint( QPainter *painter )
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

    painter->drawText( pm.width() + 5, yPos, text() );
  }
}

/////////////////////////////////////////////////////////////////////
// KantFileList::KFLToolTip implementation

KantFileList::KFLToolTip::KFLToolTip( QWidget *parent )
  : QToolTip( parent )
{
}

void KantFileList::KFLToolTip::maybeTip( const QPoint &p )
{
  QString str;
  QRect r;

  ((KantFileList*)parentWidget())->tip( p, r, str );

  if( !str.isEmpty() && r.isValid() )
    tip( r, str );
}

