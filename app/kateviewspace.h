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

#ifndef __KATE_VIEWSPACE_H__
#define __KATE_VIEWSPACE_H__

#include "katemain.h"

#include <kate/view.h>
#include <kate/document.h>

#include <qptrlist.h>
#include <qwidget.h>
#include <qvbox.h>
#include <kstatusbar.h>

class KSimpleConfig;
class KateVSStatusBar : public KStatusBar
{
  Q_OBJECT

   public:
      KateVSStatusBar ( KateViewSpace *parent = 0L, const char *name = 0L );
      virtual ~KateVSStatusBar ();
      
   public slots:
      void setStatus( int r, int c, int ovr, bool block, int mod, QString msg );
      
   protected:
      virtual bool eventFilter (QObject*,QEvent *);
      virtual void showMenu ();
      
   private:
      QLabel* m_lineColLabel;
      QLabel* m_modifiedLabel;
      QLabel* m_insertModeLabel;
      QLabel* m_selectModeLabel;
      QLabel* m_fileNameLabel;
};

class KVSSBSep;
class KateViewSpace : public QVBox
{
  Q_OBJECT

  public:
    KateViewSpace(QWidget* parent=0, const char* name=0);
    ~KateViewSpace();
    bool isActiveSpace();
    void setActive(bool b, bool showled=false);
    QWidgetStack* stack;
    void addView(Kate::View* v, bool show=true);
    void removeView(Kate::View* v);
    bool showView(Kate::View* v);
    bool showView(uint docID);
    Kate::View* currentView();
    int viewCount() { return mViewList.count(); }
    /** Saves the list of documents represented in this viewspace.
     * Documents with an invalid URL is discarded.
     * myIndex is used as identifyer for a config group.
     */
    void saveFileList(KSimpleConfig* config, int myIndex);
  protected:
    /** reimplemented to catch QEvent::PaletteChange, 
    since we use a modified palette for the statusbar */
    bool event( QEvent * );

  private:
    bool mIsActiveSpace;
    KateVSStatusBar* mStatusBar;
    QLabel* l;
    QPixmap i_active;
    QPixmap i_empty;
    QPtrList<Kate::View> mViewList;
    int mViewCount;
    KVSSBSep *sep;
  private slots:
    void slotStatusChanged (Kate::View *view, int r, int c, int ovr, bool block, int mod, QString msg);

  public slots:
    void polish();
  
};

#endif
