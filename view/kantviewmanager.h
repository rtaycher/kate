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
#include "../pluginmanager/kantpluginiface.h"
#include "kantview.h"

class KantViewManager : public KantPluginIface
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

  public slots:
    void openURL (KURL url=0L);
    void openConstURL (const KURL&url=0L);
    void reloadCurrentDoc();

  private:
    QList<KantViewSpace> viewSpaceList;
    long myViewID;

    KantDocManager *docManager;
    QGridLayout *grid;

    bool createView ( bool newDoc=true, KURL url=0L, KantView *origView=0L );
    bool deleteView ( KantView *view, bool force=false, bool delViewSpace = true, bool createNew = true );

    void moveViewtoSplit (KantView *view);
    void moveViewtoStack (KantView *view);

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

    void splitViewSpace( bool isHoriz=true );

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

    void slotSplitViewSpaceHoriz () { splitViewSpace(); }
    void slotSplitViewSpaceVert () { splitViewSpace( false ); }

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
