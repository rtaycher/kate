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

#include <klistview.h>

#include <qtooltip.h>
#include <qcolor.h>

#define RTTI_KateFileListItem 1001

class KateMainWindow;

class KAction;

class KateFileListItem : public QListViewItem
{
  public:
    KateFileListItem( QListView *lv,
		      Kate::Document *doc );
    ~KateFileListItem();

    inline uint documentNumber () { return doc->documentNumber(); }
    inline Kate::Document * document() { return doc; }

    int height() const;
    int width( const QFontMetrics &fm, const QListView* lv, int column ) const;
    int rtti() const { return RTTI_KateFileListItem; }

  protected:
    void paintCell( QPainter *painter, const QColorGroup & cg, int column, int width, int align );
    /**
     * Reimplemented so we can sort by a number of different document properties.
     */
    int compare ( QListViewItem * i, int col, bool ascending ) const;

  private:
    Kate::Document *doc;
//     uint myDocID;
};

class KateFileList : public KListView
{
  Q_OBJECT

  public:
    KateFileList (KateMainWindow *main, KateViewManager *_viewManager, QWidget * parent = 0, const char * name = 0 );
    ~KateFileList ();

    int sortType () const { return m_sort; };
    void updateSort ();

    enum sorting {
      sortByID = 0,
      sortByName = 1,
      sortByURL = 2
    };

    QString tooltip( QListViewItem *item, int );


  public slots:
    void setSortType (int s);
    void slotNextDocument();
    void slotPrevDocument();

  private slots:
    void slotDocumentCreated (Kate::Document *doc);
    void slotDocumentDeleted (uint documentNumber);
    void slotActivateView( QListViewItem *item );
    void slotModChanged (Kate::Document *doc);
    void slotModifiedOnDisc (Kate::Document *doc, bool b, unsigned char reason);
    void slotNameChanged (Kate::Document *doc);
    void slotViewChanged ();
    void slotMenu ( QListViewItem *item, const QPoint &p, int col );

  protected:
    virtual void keyPressEvent( QKeyEvent *e );
    /**
     * Reimplemented to force Single mode for real:
     * don't let a mouse click outside items deselect.
     */
    virtual void contentsMousePressEvent( QMouseEvent *e );
    /**
     * Reimplemented to make sure the first (and only) column is at least
     * the width of the viewport
     */
    virtual void resizeEvent( QResizeEvent *e );

  private:
    void setupActions ();
    void updateActions ();

  private:
    KateMainWindow *m_main;
    KateViewManager *viewManager;

    int m_sort;
    bool notify;

    KAction* windowNext;
    KAction* windowPrev;

    class ToolTip *m_tooltip;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
