/***************************************************************************
                          kantviewspace.h  -  description
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

#ifndef __kant_view_space_h__
#define __kant_view_space_h__

#include "../kantmain.h"
#include "kantview.h"

#include <qlist.h>
#include <qwidget.h>
#include <qvbox.h>

class KSimpleConfig;
class KantVSStatusBar : public QWidget
{
  Q_OBJECT

   public:
      KantVSStatusBar ( KantViewSpace *parent = 0L, const char *name = 0L );
      virtual ~KantVSStatusBar ();

      void showActiveViewIndicator ( bool b );

   public slots:
      void slotDisplayStatusText (const QString& text);
      void slotClear ();

   signals:
      void clicked();

   protected:
      virtual bool eventFilter (QObject*,QEvent *);
      virtual void showMenu ();

      virtual void paintEvent (QPaintEvent *e);
      KantViewSpace* viewspace;
      QLabel *m_pStatusLabel;
      int m_yOffset;
      bool m_showLed;
};

class KantViewSpace : public QVBox
{
  Q_OBJECT

  public:
    KantViewSpace(QWidget* parent=0, const char* name=0);
    ~KantViewSpace();
    bool isActiveSpace();
    void setActive(bool b, bool showled=false);
    QWidgetStack* stack;
    void addView(KantView* v, bool show=true);
    void removeView(KantView* v);
    bool showView(KantView* v);
    bool showView(int docID);
    KantView* currentView();
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
    KantVSStatusBar* mStatusBar;
    QLabel* l;
    QPixmap i_active;
    QPixmap i_empty;
    QList<KantView> mViewList;
    int mViewCount;

  private slots:
    void slotStatusChanged (KantView *view, int r, int c, int ovr, int mod, QString msg);
};

#endif
