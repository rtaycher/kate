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

#ifndef __KATE_FILELIST_H__
#define __KATE_FILELIST_H__

#include "katemain.h"

#include <kate/document.h>

#include <klistbox.h>

#include <qtooltip.h>
#include <qcolor.h>

class KateFileListItem : public QListBoxItem
{
  public:
    KateFileListItem( KateDocManager *_docManager, Kate::Document *doc, uint documentNumber, const QString& text);
    ~KateFileListItem();

    uint documentNumber ();

    void setText(const QString &text);

    int height( const QListBox* lb ) const;

    int width( const QListBox* lb ) const;

  protected:
    void paint( QPainter *painter );

  private:
    uint myDocID;
    Kate::Document *doc;
    KateDocManager *docManager;
};

class KateFileList : public KListBox
{
  Q_OBJECT

  public:
    KateFileList (KateDocManager *_docManager, KateViewManager *_viewManager, QWidget * parent = 0, const char * name = 0 );
    ~KateFileList ();

    /** called by KFLToolTip::maybeTip() to get a string
     * and a rect based on the point.
     * Returns the URL for the doc which item is under p
     * if any.
     */
    void tip( const QPoint &p, QRect &r, QString &str );

    void setSortType (int s);
    int sortType () const { return m_sort; };
    void updateSort ();

    enum sorting {
      sortByID = 0,
      sortByName = 1
    };
    
  public slots:
    void slotNextDocument();
    void slotPrevDocument();

  private:
    KateDocManager *docManager;
    KateViewManager *viewManager;
    int m_sort;
    bool notify;

  private slots:
    void slotDocumentCreated (Kate::Document *doc);
    void slotDocumentDeleted (uint documentNumber);
    void slotActivateView( QListBoxItem *item );
    void slotModChanged (Kate::Document *doc);
    void slotModifiedOnDisc (Kate::Document *doc, bool b, unsigned char reason);
    void slotNameChanged (Kate::Document *doc);
    void slotViewChanged ();
    void slotMenu ( QListBoxItem *item, const QPoint &p );

  protected:
  	virtual void keyPressEvent(QKeyEvent *e); 
    
  private:
    /////////////////////////////////////////////////////////////////////
    // A private tooltip class to display the URL of a document in the
    // tooltip.
    // Thanks to KDevelop team for the code:)
    /////////////////////////////////////////////////////////////////////
    class KFLToolTip : public QToolTip
    {
      public:
        KFLToolTip(QWidget *parent);
        
      protected:
        void maybeTip( const QPoint & );
    };
    KFLToolTip* tooltip;
};

#endif
