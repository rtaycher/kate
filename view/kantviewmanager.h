/***************************************************************************
                          kantviewmanager.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph "Crossfire" Cullmann
    email                : crossfire@babylon2k.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef kant_viewmanager_h__
#define kant_viewmanager_h__

#include "../kantmain.h"
#include "../interfaces/kantviewmanagerIface.h"
#include "kantview.h"

class KantSplitter;
class KSimpleConfig;
class KantViewManager : public KantViewManagerIface
{
  Q_OBJECT

  friend class KantMainWindow;

  public:
    KantViewManager (QWidget *parent=0, KantDocManager *docManager=0);
    ~KantViewManager ();

  protected:
    bool useOpaqueResize;
    QList<KantView> viewList;
    void saveAllDocsAtCloseDown(KConfig* config);
    /** This will save the splitter configuration */
    void saveViewSpaceConfig();

  public slots:
    void openURL (KURL url=0L);
    void openConstURL (const KURL&url=0L);
    void reloadCurrentDoc();

  private:
    QList<KantViewSpace> viewSpaceList;
    long myViewID;

    KantDocManager *docManager;
    QGridLayout *grid;

    bool createView ( bool newDoc=true, KURL url=0L, KantView *origView=0L, KantDocument *doc=0L );
    bool deleteView ( KantView *view, bool force=false, bool delViewSpace = true, bool createNew = true );

    void moveViewtoSplit (KantView *view);
    void moveViewtoStack (KantView *view);

    /** This will save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    void saveSplitterConfig(KantSplitter* s, int idx=0, KSimpleConfig* config=0L);

    void removeViewSpace (KantViewSpace *viewspace);

    bool showFullPath;

  public:
    virtual KantView* activeView ();
    KantViewSpace* activeViewSpace ();

    long viewCount ();
    long viewSpaceCount ();

  private slots:
    void activateView ( KantView *view );
    void activateSpace ( KantView* v );
    void slotViewChanged();

  public:
    void deleteLastView ();

    /** Splits a KantViewSpace into two.
      * The operation is performed by creating a KantSplitter in the parent of the KantViewSpace to be split,
      * which is then moved to that splitter. Then a new KantViewSpace is created and added to the splitter,
      * and a KantView is created to populate the new viewspace. The new KantView is made the active one,
      * because createView() does that.
      * If no viewspace is provided, the result of activeViewSpace() is used.
      * The isHoriz, true pr default, decides the orientation of the splitting action.
      * If atTop is true, the new viewspace will be moved to the first position in the new splitter.
      * If a newViewUrl is provided, the new view will show the document in that URL if any, otherwise
      * the document of the current view in the viewspace to be split is used.
      */
    void splitViewSpace( KantViewSpace* vs=0L, bool isHoriz=true, bool atTop=false, KURL newViewUrl=0L );

    bool getShowFullPath() { return showFullPath; }
    void setUseOpaqueResize( bool enable );

  public slots:
    void activateView ( int docID );

    void slotDocumentCloseAll ();
    void slotDocumentSaveAll();

    void slotWindowNext();
    void slotWindowPrev();

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentSave ();
    void slotDocumentSaveAs ();
    void slotDocumentClose ();
    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz () { splitViewSpace(); }
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert () { splitViewSpace( 0L, false ); }

    void slotCloseCurrentViewSpace();

    void slotUndo ();
    void slotRedo ();
    void slotUndoHistory ();

    void slotCut ();
    void slotCopy ();
    void slotPaste ();

    void slotSelectAll ();
    void slotDeselectAll ();
    void slotInvertSelection ();

    void slotFind ();
    void slotFindAgain ();
    void slotFindAgainB ();
    void slotReplace ();

    void slotIndent();
    void slotUnIndent();

    void slotInsertFile ();

    void slotHlDlg ();
    void slotSetHl (int n);

    void slotSpellcheck ();
    void slotGotoLine ();

    void statusMsg (const QString &msg);
    void statusMsgOther ();

    void printNow();
    void printDlg();

    void setActiveSpace ( KantViewSpace* vs );
    void setActiveView ( KantView* view );

    void setShowFullPath(bool enable);

    void setWindowCaption();

    void activateNextView();
    void activatePrevView();

    void addBookmark();
    void setBookmark();
    void clearBookmarks();

    void setEol(int);

  signals:
    void statusChanged (KantView *, int, int, int, int, QString);
    void statChanged ();
    void viewChanged ();

  public:  //KantPluginIface
  virtual KWrite *getActiveView(){return activeView();};
};

#endif
