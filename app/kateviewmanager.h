/***************************************************************************
                          kateviewmanager.h 
                          View Manager for the Kate Text Editor
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann, 2001, 2002 by Anders Lund
    email                : cullmann@kde.org anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_VIEWMANAGER_H__
#define __KATE_VIEWMANAGER_H__

#include "katemain.h"
#include "../interfaces/viewmanager.h"

#include <kate/view.h>
#include <kate/document.h>

class KateSplitter;
class KSimpleConfig;

class KateViewManager : public Kate::ViewManager
{
  Q_OBJECT

  public:
    KateViewManager (QWidget *parent=0, KateDocManager *docManager=0);
    ~KateViewManager ();   
    
    inline QPtrList<Kate::View> &viewList () { return m_viewList; };

  public:
    bool useOpaqueResize;
        /* Save a list of open files. */
    void saveAllDocsAtCloseDown();
    /* This will save the splitter configuration */
    void saveViewSpaceConfig();

    /* reopens documents that was open last time kate was shut down*/
    void reopenDocuments(bool isRestore);
  
  public slots:
    void openURL (KURL url=0L);
    void openConstURL (const KURL&url=0L);

  private:
    bool createView ( bool newDoc=true, KURL url=0L, Kate::View *origView=0L, Kate::Document *doc=0L );
    bool deleteView ( Kate::View *view, bool delViewSpace = true);

    void moveViewtoSplit (Kate::View *view);
    void moveViewtoStack (Kate::View *view);

    /* Save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    void saveSplitterConfig(KateSplitter* s, int idx=0, KSimpleConfig* config=0L);

    /* Restore view configuration.
     * If only one view was present at close down, calls reopenDocuemnts.
     * The configuration will be restored so that viewspaces are created, sized
     * and populated exactly like at shotdown.
     */
    void restoreViewConfig();

    /** Restore a single splitter.
     * This is all the work is done for @see saveSplitterConfig()
     */
    void restoreSplitter ( KSimpleConfig* config, QString group, QWidget* parent );

    void removeViewSpace (KateViewSpace *viewspace);

    bool showFullPath;

  public:
    virtual Kate::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

  private slots:
    void activateView ( Kate::View *view, bool checkModified = true );
    void activateSpace ( Kate::View* v );
    void slotViewChanged();
    bool closeDocWithAllViews ( Kate::View *view );
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
    void splitViewSpace( KateViewSpace* vs=0L, bool isHoriz=true, bool atTop=false, KURL newViewUrl=0L );

    bool getShowFullPath() { return showFullPath; }
    void setUseOpaqueResize( bool enable );

    void activateView ( uint documentNumber );
    void activateView ( uint documentNumber, bool checkModified );
    void activateView ( int documentNumber ) { activateView((uint) documentNumber); };

    void slotDocumentCloseAll ();
    void slotDocumentSaveAll();

    void slotWindowNext();
    void slotWindowPrev();

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentSave ();
    void slotDocumentSave( Kate::View* );
    void slotDocumentSaveAs ();
    void slotDocumentSaveAs( Kate::View* );
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

    void setWindowCaption();

    void activateNextView();
    void activatePrevView();

  signals:
    void statusChanged (Kate::View *, int, int, int, bool, int, QString);
    void statChanged ();
    
  private:
    QPtrList<KateViewSpace> m_viewSpaceList; 
    QPtrList<Kate::View> m_viewList;

    KateDocManager *m_docManager;
    QGridLayout *m_grid;
    QString m_encoding;
};

#endif
