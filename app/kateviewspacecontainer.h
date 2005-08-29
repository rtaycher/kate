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

#ifndef __KATE_VIEWSPACE_CONTAINER_H__
#define __KATE_VIEWSPACE_CONTAINER_H__

#include "katemain.h"

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include "katemdi.h"

#include <Q3PtrList>
#include <QHash>

class KConfig;
class KateMainWindow;

class KateViewSpaceContainer: public QSplitter
{
  Q_OBJECT

  friend class KateViewSpace;
  friend class KateVSStatusBar;

  public:
    KateViewSpaceContainer (QWidget *parent, KateViewManager *viewManager);

    ~KateViewSpaceContainer ();

    inline Q3PtrList<KTextEditor::View> &viewList () { return m_viewList; };

  public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfig *config,const QString& group);

    /* restore it */
    void restoreViewConfiguration (KConfig *config,const QString& group);

  private:
    /**
     * create and activate a new view for doc, if doc == 0, then
     * create a new document
     */
    bool createView ( KTextEditor::Document *doc =0L );

    bool deleteView ( KTextEditor::View *view, bool delViewSpace = true);

    void moveViewtoSplit (KTextEditor::View *view);
    void moveViewtoStack (KTextEditor::View *view);

    /* Save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    void saveSplitterConfig(QSplitter* s, int idx=0, KConfig* config=0L, const QString& viewConfGrp="");

    /** Restore a single splitter.
     * This is all the work is done for @see saveSplitterConfig()
     */
    void restoreSplitter ( KConfig* config, const QString &group, QWidget* parent , const QString& viewConfGrp);

    void removeViewSpace (KateViewSpace *viewspace);

    bool showFullPath;

  public:
    KTextEditor::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

    bool isViewActivationBlocked(){return m_blockViewCreationAndActivation;};

  public:
    void closeViews(uint documentNumber);
    KateMainWindow *mainWindow();
  friend class KateViewManager;

  private slots:
    void activateView ( KTextEditor::View *view );
    void activateSpace ( KTextEditor::View* v );
    void slotViewChanged();
    void reactivateActiveView();
    void slotPendingDocumentNameChanged();

    void documentCreated (KTextEditor::Document *doc);
    void documentDeleted (uint docNumber);

  public slots:
     /* Splits a KateViewSpace into two.
      * The operation is performed by creating a KateMDI::Splitter in the parent of the KateViewSpace to be split,
      * which is then moved to that splitter. Then a new KateViewSpace is created and added to the splitter,
      * and a KateView is created to populate the new viewspace. The new KateView is made the active one,
      * because createView() does that.
      * If no viewspace is provided, the result of activeViewSpace() is used.
      * The isHoriz, true pr default, decides the orientation of the splitting action.
      * If atTop is true, the new viewspace will be moved to the first position in the new splitter.
      * If a newViewUrl is provided, the new view will show the document in that URL if any, otherwise
      * the document of the current view in the viewspace to be split is used.
      */
    void splitViewSpace( KateViewSpace* vs=0L, bool isHoriz=true, bool atTop=false );

    bool getShowFullPath() const { return showFullPath; }

    /**
     * activate view for given document
     * @param doc document to activate view for
     */
    void activateView ( KTextEditor::Document *doc );

    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz () { splitViewSpace(); }
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert () { splitViewSpace( 0L, false ); }

    void slotCloseCurrentViewSpace();

    void setActiveSpace ( KateViewSpace* vs );
    void setActiveView ( KTextEditor::View* view );

    void setShowFullPath(bool enable);

    void activateNextView();
    void activatePrevView();

  signals:
    void viewChanged ();

  private:
    KateViewManager *m_viewManager;
    Q3PtrList<KateViewSpace> m_viewSpaceList;
    Q3PtrList<KTextEditor::View> m_viewList;
    QHash<KTextEditor::View*, bool> m_activeStates;

    bool m_blockViewCreationAndActivation;

    bool m_activeViewRunning;

    bool m_pendingViewCreation;
    QPointer<KTextEditor::Document> m_pendingDocument;
};

#endif
