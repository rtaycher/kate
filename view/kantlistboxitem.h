/***************************************************************************
                          kantlistboxitem.h  -  description
                             -------------------
    begin                : Sun Feb 18 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/
 
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/***************************************************************************
 *   This is a workaround for QListBoxPixmap, because this class can only  *
 *   set the pixmap in the constructor and doesn't allow changes           *
 ***************************************************************************/

#ifndef _KANTLISTBOX_H_
#define _KANTLISTBOX_H_

#include <qlistbox.h>
#include <qapplication.h>

class KantListBoxItem : public QListBoxItem
{
 public:
  KantListBoxItem( const QPixmap &pix, const QString& text): QListBoxItem()
    {
      _bold=false;
      setPixmap(pix);
      setText( text );
    };

  ~KantListBoxItem(){};
 
  const QPixmap *pixmap() const { return &pm; };
 
  void setText(const QString &text)
    {
      QListBoxItem::setText(text);
    };
 
  void setPixmap(const QPixmap &pixmap)
    {
      pm=pixmap;
    };
 
  void setBold(bool bold)
    {
	_bold=bold;
    }
 
  int height( const QListBox* lb ) const
    {
      int h;
      if ( text().isEmpty() )
        h = pm.height();
      else
        h = QMAX( pm.height(), lb->fontMetrics().lineSpacing() + 1 );
      return QMAX( h, QApplication::globalStrut().height() );
    };

  int width( const QListBox* lb ) const
    {
      if ( text().isEmpty() )
        return QMAX( pm.width() + 6, QApplication::globalStrut().width() );
      return QMAX( pm.width() + lb->fontMetrics().width( text() ) + 6,
		   QApplication::globalStrut().width() );
    };
 
 protected:
  void paint( QPainter *painter )
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
    };
 
 private:
  QPixmap pm;
  bool _bold;
};

#endif
