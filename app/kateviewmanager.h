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

#ifndef __KATE_VIEWMANAGER_H__
#define __KATE_VIEWMANAGER_H__

#include "katemain.h"
#include "../interfaces/viewmanager.h"

#include "kateviewspacecontainer.h"

#include <kate/view.h>
#include <kate/document.h>
#include <kmdi/tabwidget.h>
#include <qguardedptr.h>

class KateSplitter;
class KateMainWindow;

class KConfig;
class KAction;

class KateViewManager : public QWidget
{
  Q_OBJECT

  public:
    KateViewManager (KateMainWindow *parent, KMDI::TabWidget *tabWidget, KateDocManager *docManager);
    ~KateViewManager ();

    Kate::ViewManager *viewManager () const { return m_viewManager; };
    
    KateViewSpaceContainer *activeContainer () { return m_currentContainer; }

    QPtrList<KateViewSpaceContainer> *containers() { return &m_viewSpaceContainerList; }
    
    void updateViewSpaceActions ();
    
  private:
    /**
     * create all actions needed for the view manager
     */
    void setupActions ();

  public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfig *config,const QString& group);

    /* restore it */
    void restoreViewConfiguration (KConfig *config,const QString& group);

    uint openURL (const KURL &url, const QString& encoding, bool activate = true);

  public slots:
    void openURL (const KURL &url);

  private:
    bool useOpaqueResize;

    void removeViewSpace (KateViewSpace *viewspace);

    bool showFullPath;

  public:
    virtual Kate::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

    void setViewActivationBlocked (bool block);

  public:
    void closeViews(uint documentNumber);
    KateMainWindow *mainWindow();

  private slots:
    void activateView ( Kate::View *view );
    void activateSpace ( Kate::View* v );
    void slotViewChanged();

    void tabChanged(QWidget*);

  public slots:
    bool getShowFullPath() const { return showFullPath; }

    void activateView ( uint documentNumber );
    void activateView ( int documentNumber ) { activateView((uint) documentNumber); };

    void slotDocumentSaveAll();

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentClose ();
    
    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz ();
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert ();

    void slotNewTab();
    void slotCloseTab ();
    void activateNextTab ();
    void activatePrevTab ();

    void slotCloseCurrentViewSpace();

    void setActiveSpace ( KateViewSpace* vs );
    void setActiveView ( Kate::View* view );

    void setShowFullPath(bool enable);

    void activateNextView();
    void activatePrevView();

  signals:
    void statusChanged (Kate::View *, int, int, int, bool, int, const QString &);
    void statChanged ();
    void viewChanged ();

  private:
    Kate::ViewManager *m_viewManager;
    QPtrList<KateViewSpaceContainer> m_viewSpaceContainerList;
    KateViewSpaceContainer *m_currentContainer;

    KateDocManager *m_docManager;
    KateMainWindow *m_mainWindow;
    QGuardedPtr<KMDI::TabWidget> m_tabWidget;
    bool m_init;

    KAction *m_closeView;
    KAction *m_closeTab;
    KAction *m_activateNextTab;
    KAction *m_activatePrevTab;
    KAction *goNext;
    KAction *goPrev;

  protected:
    friend class KateViewSpaceContainer;
    bool eventFilter(QObject *o,QEvent *e);
    QGuardedPtr<Kate::View> guiMergedView;
};

#endif
