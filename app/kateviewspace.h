/***************************************************************************
                          kateviewspace.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Anders Lund
    email                : anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
    bool eventFilter(QObject* o, QEvent* e);

  private:
    bool mIsActiveSpace;
    KateVSStatusBar* mStatusBar;
    QLabel* l;
    QPixmap i_active;
    QPixmap i_empty;
    QPtrList<Kate::View> mViewList;
    int mViewCount;

  private slots:
    void slotStatusChanged (Kate::View *view, int r, int c, int ovr, bool block, int mod, QString msg);

  public slots:
    void polish();
  
};

#endif
