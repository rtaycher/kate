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

#include <kate/view.h>
#include <kate/document.h>

class KateSplitter;
class KConfig;
class KateMainWindow;

class KateViewManager : public QWidget
{
  Q_OBJECT

  friend class KateViewSpace;
  friend class KateVSStatusBar;

  public:
    KateViewManager (QWidget *parent=0, KateDocManager *docManager=0,KateMainWindow* mainWindow=0);
    ~KateViewManager ();

    Kate::ViewManager *viewManager () const { return m_viewManager; };

    inline QPtrList<Kate::View> &viewList () { return m_viewList; };

  public:
    /* Query if the modified untitled docs should now be saved */
    void queryModified();

    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfig *config,const QString& group);

    /* restore it */
    void restoreViewConfiguration (KConfig *config,const QString& group);

    uint openURL (const KURL &url, const QString& encoding=QString::null, bool activate = true);
    
  public slots:
    void openConstURLCheck (const KURL&url=0L);

  private:
    bool useOpaqueResize;
  
    /**
     * create and activate a new view for doc, if doc == 0, then
     * create a new document
     */
    bool createView ( Kate::Document *doc =0L );

    bool deleteView ( Kate::View *view, bool delViewSpace = true);

    void moveViewtoSplit (Kate::View *view);
    void moveViewtoStack (Kate::View *view);

    /* Save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    void saveSplitterConfig(KateSplitter* s, int idx=0, KConfig* config=0L, const QString& viewConfGrp="");

    /** Restore a single splitter.
     * This is all the work is done for @see saveSplitterConfig()
     */
    void restoreSplitter ( KConfig* config, const QString &group, QWidget* parent , const QString& viewConfGrp);

    void removeViewSpace (KateViewSpace *viewspace);

    bool showFullPath;

  public:
    virtual Kate::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

    bool isViewActivationBlocked(){return m_blockViewCreationAndActivation;};

  public:
    void closeViews(uint documentNumber);
    KateMainWindow *mainWindow();

  private slots:
    void activateView ( Kate::View *view );
    void activateSpace ( Kate::View* v );
    void slotViewChanged();
    void openNewIfEmpty();

  public slots:
    void deleteLastView ();

     /* Splits a KateViewSpace into two.
      * The operation is performed by creating a KateSplitter in the parent of the KateViewSpace to be split,
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

    void activateView ( uint documentNumber );
    void activateView ( int documentNumber ) { activateView((uint) documentNumber); };

    void slotDocumentCloseAll ();
    void slotDocumentSaveAll();

    void slotWindowNext();
    void slotWindowPrev();

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentClose ();
    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz () { splitViewSpace(); }
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert () { splitViewSpace( 0L, false ); }

    void slotCloseCurrentViewSpace();

    void statusMsg ();

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
    QPtrList<KateViewSpace> m_viewSpaceList;
    QPtrList<Kate::View> m_viewList;

    KateDocManager *m_docManager;
    QGridLayout *m_grid;
    bool m_blockViewCreationAndActivation;

    bool m_activeViewRunning;
    KateMainWindow *m_mainWindow;
};

#endif
