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

#include <kiconloader.h>
#include <klocale.h>

KantFileList::KantFileList (KantDocManager *_docManager, QWidget * parent, const char * name ):  KListBox (parent, name)
{
  docManager = _docManager;

  connect(docManager,SIGNAL(documentCreated(KantDocument *)),this,SLOT(slotDocumentCreated(KantDocument *)));
}

KantFileList::~KantFileList ()
{
}

void KantFileList::slotDocumentCreated (KantDocument *doc)
{
  insertItem (new KantFileListItem (doc->docID(), SmallIcon("null"), QString ("test")) );
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
	_bold=bold;
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
      return QMAX( pm.width() + lb->fontMetrics().width( text() ) + 6,
		   QApplication::globalStrut().width() );
    }

void KantFileListItem::paint( QPainter *painter )
    {
      painter->drawPixmap( 3, 0, pm );
      QFont f=painter->font();
	f.setBold(_bold);
      painter->setFont(f);
      if ( !text().isEmpty() ) {
        QFontMetrics fm = painter->fontMetrics();
        int yPos;                       // vertical text position
        if ( pm.height() < fm.height() )
	  yPos = fm.ascent() + fm.leading()/2;
        else
	  yPos = pm.height()/2 - fm.height()/2 + fm.ascent();
        painter->drawText( pm.width() + 5, yPos, text() );
      }
}
