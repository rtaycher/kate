/***************************************************************************
                          katefilelist.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "katefilelist.h"
#include "katefilelist.moc"

#include "katedocmanager.h"
#include "kateviewmanager.h"
#include "katemainwindow.h"

#include <qapplication.h>
#include <qpainter.h>

#include <kiconloader.h>
#include <klocale.h>

KateFileList::KateFileList (KateDocManager *_docManager, KateViewManager *_viewManager, QWidget * parent, const char * name ):  KListBox (parent, name)
{
  docManager = _docManager;
  viewManager = _viewManager;
  tooltip = new KFLToolTip( this );

  for (uint i = 0; i < docManager->docCount(); i++)
  {
    slotDocumentCreated (docManager->nthDoc(i));
    slotModChanged (docManager->nthDoc(i));
  }

  connect(docManager,SIGNAL(documentCreated(Kate::Document *)),this,SLOT(slotDocumentCreated(Kate::Document *)));
  connect(docManager,SIGNAL(documentDeleted(uint)),this,SLOT(slotDocumentDeleted(uint)));

  connect(this,SIGNAL(highlighted(QListBoxItem *)),this,SLOT(slotActivateView(QListBoxItem *)));
  connect(this,SIGNAL(selected(QListBoxItem *)), this,SLOT(slotActivateView(QListBoxItem *)));

  connect(viewManager,SIGNAL(viewChanged()), this,SLOT(slotViewChanged()));

  connect(this,SIGNAL(rightButtonPressed ( QListBoxItem *, const QPoint & )), this,SLOT(slotMenu ( QListBoxItem *, const QPoint & )));
}

KateFileList::~KateFileList ()
{
}

void KateFileList::slotDocumentCreated (Kate::Document *doc)
{
  insertItem (new KateFileListItem (doc->documentNumber(), SmallIcon("null"), doc->docName()) );
  connect(doc,SIGNAL(modStateChanged(Kate::Document *)),this,SLOT(slotModChanged(Kate::Document *)));
  connect(doc,SIGNAL(nameChanged(Kate::Document *)),this,SLOT(slotNameChanged(Kate::Document *)));
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
    str = docManager->docWithID(i->documentNumber())->url().prettyURL();
  else
    str = "";
}

KateFileListItem::KateFileListItem( uint documentNumber, const QPixmap &pix, const QString& text): QListBoxItem()
{
  _bold=false;
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

