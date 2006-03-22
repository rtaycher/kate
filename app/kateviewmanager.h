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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_VIEWMANAGER_H__
#define __KATE_VIEWMANAGER_H__

#include "katemain.h"

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <qpointer.h>
#include <Q3PtrList>

class KateMainWindow;
class KateViewSpaceContainer;

class KConfig;
class KAction;

class QToolButton;

class KateViewManager : public QObject
{
  Q_OBJECT

  public:
    KateViewManager (KateMainWindow *parent);
    ~KateViewManager ();

    KateViewSpaceContainer *activeContainer () { return m_currentContainer; }

    Q3PtrList<KateViewSpaceContainer> *containers() { return &m_viewSpaceContainerList; }

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

    KTextEditor::Document *openURL (const KUrl &url, const QString& encoding, bool activate = true, bool isTempFile=false);

  public Q_SLOTS:
    void openURL (const KUrl &url);

  private:
    void removeViewSpace (KateViewSpace *viewspace);

    bool showFullPath;

  public:
    KTextEditor::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

    void setViewActivationBlocked (bool block);

  public:
    void closeViews(KTextEditor::Document *doc);
    KateMainWindow *mainWindow();

  private Q_SLOTS:
    void activateView ( KTextEditor::View *view );
    void activateSpace ( KTextEditor::View* v );

    void tabChanged(QWidget*);

  public Q_SLOTS:
    bool getShowFullPath() const { return showFullPath; }

    void activateView ( KTextEditor::Document *doc );

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
    void setActiveView ( KTextEditor::View* view );

    void setShowFullPath(bool enable);

    void activateNextView();
    void activatePrevView();

  protected:
    friend class KateViewSpaceContainer;

    QPointer<KTextEditor::View> guiMergedView;

  Q_SIGNALS:
    void statChanged ();
    void viewChanged ();

  private:
    Q3PtrList<KateViewSpaceContainer> m_viewSpaceContainerList;
    KateViewSpaceContainer *m_currentContainer;

    KateMainWindow *m_mainWindow;
    bool m_init;

    QToolButton *m_closeTabButton;
    KAction *m_closeView;
    KAction *m_closeTab;
    KAction *m_activateNextTab;
    KAction *m_activatePrevTab;
    KAction *goNext;
    KAction *goPrev;
};

#endif
