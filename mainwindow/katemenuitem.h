/***************************************************************************
                          katemenuitem.h  -  description
                             -------------------
    begin                : Sun Feb 19 2001
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
 *   This items are for the document list menu, but have some troubles,    *
 *   because KDE 2.1's stylecode is not correctly, 		           *
 *   btw completely implemented                                            *
 ***************************************************************************/

#ifndef _KANT_MENUITEM_H_
#define _KANT_MENUITEM_H_

#include <qpopupmenu.h>
#include <qmenudata.h>

#define max(x,y) x>y ? x:y

class KateMenuItem : public QCustomMenuItem
{
public:
    KateMenuItem( const QString& s, const QFont& f,const QPixmap &p )
        : string( s ), font( f ), pixmap(p){qDebug((" KateMenuItem-Constructor:"+s).latin1());};

    ~KateMenuItem(){};

    virtual void paint( QPainter* p, const QColorGroup& /*cg*/, bool /*act*/, bool /*enabled*/, int x, int y, int w, int h )
    {
	qDebug(("Drawing: String is: "+string).latin1());
        p->setFont ( font );
        p->drawText( x, y, w, h, AlignLeft | AlignVCenter | ShowPrefix | DontClip, string );
        p->drawPixmap( x+QFontMetrics(font).size( AlignLeft | AlignVCenter | ShowPrefix | DontClip,  string).width()+3, y, pixmap); 
    };

    virtual QSize sizeHint()
    {
	qDebug("KateMenuItem : sizeHint()");
        QSize size=QFontMetrics( font ).size( AlignLeft | AlignVCenter | ShowPrefix | DontClip,  string );
        size.setHeight(max(size.height(),pixmap.height()));
        size.setWidth(size.width()+pixmap.width()+3);
        return size;
    };
private:
    QString string;
    QFont font;
    QPixmap pixmap;
};


#endif
