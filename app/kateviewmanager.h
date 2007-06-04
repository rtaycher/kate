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

#include <KTextEditor/View>
#include <KTextEditor/Document>

#include <QPointer>
#include <QList>
#include <QModelIndex>
#include <QSplitter>
#include <QStackedWidget>

class KConfigGroup;
class KConfigBase;
class KateMainWindow;
class KateViewSpaceContainer;
class KateViewSpace;
class QAction;

class QToolButton;

class KateViewManager : public QSplitter
{
    Q_OBJECT

    friend class KateViewSpace;
    friend class KateVSStatusBar;

  public:
    KateViewManager (QWidget *parentW, KateMainWindow *parent);
    ~KateViewManager ();

    void updateViewSpaceActions ();

  private:
    /**
     * create all actions needed for the view manager
     */
    void setupActions ();

  public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfigGroup& group);

    /* restore it */
    void restoreViewConfiguration (const KConfigGroup& group);

    KTextEditor::Document *openUrl (const KUrl &url, const QString& encoding, bool activate = true, bool isTempFile = false);

    KTextEditor::View *openUrlWithView (const KUrl &url, const QString& encoding);

  public Q_SLOTS:
    void openUrl (const KUrl &url);

  public:

    void setViewActivationBlocked (bool block);

  public:
    void closeViews(KTextEditor::Document *doc);
    KateMainWindow *mainWindow();

  private Q_SLOTS:
    void activateView ( KTextEditor::View *view );
    void activateSpace ( KTextEditor::View* v );

  public Q_SLOTS:
    void activateDocument(const QModelIndex &index);

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentClose ();

    void setActiveSpace ( KateViewSpace* vs );
    void setActiveView ( KTextEditor::View* view );

    void activateNextView();
    void activatePrevView();

  protected:
    friend class KateViewSpaceContainer;

    QPointer<KTextEditor::View> guiMergedView;

  Q_SIGNALS:
    void statChanged ();
    void viewChanged ();

  public:
    inline QList<KTextEditor::View*> &viewList ()
    {
      return m_viewList;
    };

  private:
    /**
     * create and activate a new view for doc, if doc == 0, then
     * create a new document
     */
    bool createView ( KTextEditor::Document *doc = 0L );

    bool deleteView ( KTextEditor::View *view, bool delViewSpace = true);

    void moveViewtoSplit (KTextEditor::View *view);
    void moveViewtoStack (KTextEditor::View *view);

    /* Save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    void saveSplitterConfig(QSplitter* s, KConfigBase* config, const QString& viewConfGrp);

    /** Restore a single splitter.
     * This is all the work is done for @see saveSplitterConfig()
     */
    void restoreSplitter ( KConfigBase* config, const QString &group, QSplitter* parent, const QString& viewConfGrp);

    void removeViewSpace (KateViewSpace *viewspace);

  public:
    KTextEditor::View* activeView ();
    KateViewSpace* activeViewSpace ();

    int viewCount () const;
    int viewSpaceCount () const;

    bool isViewActivationBlocked()
    {
      return m_blockViewCreationAndActivation;
    };

  private Q_SLOTS:
    void slotViewChanged();
    void reactivateActiveView();
    void slotPendingDocumentNameChanged();

    void documentCreated (KTextEditor::Document *doc);
    void documentDeleted (KTextEditor::Document *doc);

  public Q_SLOTS:
    /**
     * Splits a KateViewSpace into two in the following steps:
     * 1. create a QSplitter in the parent of the KateViewSpace to be split
     * 2. move the to-be-splitted KateViewSpace to the new splitter
     * 3. create new KateViewSpace and added to the new splitter
     * 4. create KateView to populate the new viewspace.
     * 5. The new KateView is made the active one, because createView() does that.
     * If no viewspace is provided, the result of activeViewSpace() is used.
     * The orientation of the new splitter is determined by the value of o.
     * Note: horizontal splitter means vertically aligned views.
     */
    void splitViewSpace( KateViewSpace* vs = 0L, Qt::Orientation o = Qt::Horizontal );

    /**
     * activate view for given document
     * @param doc document to activate view for
     */
    KTextEditor::View *activateView ( KTextEditor::Document *doc );

    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz ()
    {
      splitViewSpace(0L, Qt::Vertical);
    }
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert ()
    {
      splitViewSpace();
    }

    void slotCloseCurrentViewSpace();

  private:
    KateMainWindow *m_mainWindow;
    bool m_init;

    QToolButton *m_closeTabButton;
    QAction *m_closeView;
    QAction *m_closeTab;
    QAction *m_activateNextTab;
    QAction *m_activatePrevTab;
    QAction *goNext;
    QAction *goPrev;

    QList<KateViewSpace*> m_viewSpaceList;
    QList<KTextEditor::View*> m_viewList;
    QHash<KTextEditor::View*, bool> m_activeStates;

    bool m_blockViewCreationAndActivation;

    bool m_activeViewRunning;

    bool m_pendingViewCreation;
    QPointer<KTextEditor::Document> m_pendingDocument;

    int m_splitterIndex; // used during saving splitter config.
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

