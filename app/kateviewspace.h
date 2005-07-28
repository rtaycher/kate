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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_VIEWSPACE_H__
#define __KATE_VIEWSPACE_H__

#include "katemain.h"

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/modificationinterface.h>

#include <q3ptrlist.h>
#include <qwidget.h>
#include <q3vbox.h>
//Added by qt3to4:
#include <QPixmap>
#include <QLabel>
#include <QEvent>
#include <kstatusbar.h>

class KVSSBSep;

class KConfig;
class KSqueezedTextLabel;
class KateViewSpaceContainer;

class QStackedWidget;

class KateVSStatusBar : public KStatusBar
{
  Q_OBJECT

   public:
      KateVSStatusBar ( KateViewSpace *parent = 0L, const char *name = 0L );
      virtual ~KateVSStatusBar ();

   /**
    * stuff to update the statusbar on view changes
    */
  public slots:
    void updateStatus ();

    void viewModeChanged ( KTextEditor::View *view );

    void cursorPositionChanged ( KTextEditor::View *view );

    void selectionChanged (KTextEditor::View *view);

    void modifiedChanged();

    void documentNameChanged ();

    void informationMessage (KTextEditor::View *view, const QString &message);

   protected:
      virtual bool eventFilter (QObject*,QEvent *);
      virtual void showMenu ();

   private:
      QLabel* m_lineColLabel;
      QLabel* m_modifiedLabel;
      QLabel* m_insertModeLabel;
      QLabel* m_selectModeLabel;
      KSqueezedTextLabel* m_fileNameLabel;
      QPixmap m_modPm, m_modDiscPm, m_modmodPm, m_noPm;
      class KateViewSpace *m_viewSpace;
};

class KateViewSpace : public Q3VBox
{
  friend class KateViewSpaceContainer;
  friend class KateVSStatusBar;

  Q_OBJECT

  public:
    KateViewSpace(KateViewSpaceContainer *, QWidget* parent=0, const char* name=0);
    ~KateViewSpace();
    bool isActiveSpace();
    void setActive(bool b, bool showled=false);
    QStackedWidget* stack;
    void addView(KTextEditor::View* v, bool show=true);
    void removeView(KTextEditor::View* v);

    bool showView(KTextEditor::View *view) { return showView(view->document()); }
    bool showView(KTextEditor::Document *document);

    KTextEditor::View* currentView();
    int viewCount() const { return mViewList.count(); }

    void saveConfig (KConfig* config, int myIndex,const QString& viewConfGrp);
    void restoreConfig ( class KateViewSpaceContainer *viewMan, KConfig* config, const QString &group );


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
    Q3PtrList<KTextEditor::View> mViewList;
    int mViewCount;
    KVSSBSep *sep;
    KateViewSpaceContainer *m_viewManager;
    QString m_group;

  public slots:
    void polish();
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
