/***************************************************************************
                          kantfilelist.h  -  description
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

#ifndef KANTFILELIST_H
#define KANTFILELIST_H

#include "../kantmain.h"

#include <klistbox.h>
#include <qapplication.h>

class KantFileListItem : public QListBoxItem
{
  public:
    KantFileListItem( long docID, const QPixmap &pix, const QString& text);
    ~KantFileListItem();

    long docID ();

    const QPixmap *pixmap() const { return &pm; };

    void setText(const QString &text);
    void setPixmap(const QPixmap &pixmap);

    void setBold(bool bold);

    int height( const QListBox* lb ) const;

    int width( const QListBox* lb ) const;

  protected:
    void paint( QPainter *painter );

   private:
   long myDocID;
    QPixmap pm;
    bool _bold;
};

class KantFileList : public KListBox
{
  Q_OBJECT

  public:
    KantFileList (KantDocManager *_docManager, KantViewManager *_viewManager, QWidget * parent = 0, const char * name = 0 );
    ~KantFileList ();

  private:
    KantDocManager *docManager;
    KantViewManager *viewManager;

  private slots:
      void slotDocumentCreated (KantDocument *doc);
      void slotDocumentDeleted (long docID);
      void slotActivateView( QListBoxItem *item );
      void slotModChanged (KantDocument *doc);
};

#endif
