/***************************************************************************
                          kantmainwindow.h  -  description
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
#ifndef __kant_mainwindow_h__
#define __kant_mainwindow_h__

#include "../main/kantmain.h"
#include "kantIface.h"

#include <kdockwidget.h>
#include <kparts/part.h>
#include <kxmlgui.h>

class KantMainWindow : public KDockMainWindow, virtual public KantIface , virtual public KParts::PartBase
{
  Q_OBJECT

  friend class KantApp;
  friend class KantViewManager;
  friend class KantView;
  friend class KantDocument;

  public:
    KantMainWindow(KantDocManager *_docManager, KantPluginManager *_pluginManager);
    ~KantMainWindow();

    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL currentDocUrl();

    // dockwidgets
    KDockWidget *mainDock;
    KDockWidget *sidebarDock;
    KDockWidget *consoleDock;

    // sidebar
    KantSidebar *sidebar;

    // console
    KantConsole *console;

    // managment items
    KantDocManager *docManager;
    KantViewManager *viewManager;

    // should be protected, and kantviewmanager a friend class.
    KRecentFilesAction *fileOpenRecent;

    KantFileList *filelist;
    KantFileSelector *fileselector;

    virtual QStringList containerTags() const;
    virtual QWidget *createContainer( QWidget *parent, int index,
      const QDomElement &element, int &id );
  protected:

    KantPluginManager *pluginManager;

    /** just calls viewmanager */
    void restore(bool isRestored);

    void focusInEvent(QFocusEvent*);
    // Anders: I moved the config object to protected, because
    // other objects needs access.
    KConfig* config;

  private:
    KAction *fileSave;
    KAction *fileSaveAll;
    KAction *fileSaveAs;
    KAction *filePrint;
    KAction *fileClose;
    KAction *fileCloseAll;
    KAction *editUndo;
    KAction *editRedo;
    KAction *editUndoHist;
    KAction *editCut;
    KAction *editCopy;
    KAction *editPaste;
    KAction *editSelectAll;
    KAction *editDeselectAll;
    KAction *editInvertSelection;
    KAction *editFind;
    KAction *editFindNext;
    KAction *editFindPrev;
    KAction *editReplace;
    KAction *editIndent;
    KAction *editUnIndent;
    KAction *editInsert;
    KAction *bookmarkAdd;
    KAction *bookmarkSet;
    KAction *bookmarkClear;
    KAction *toolsSpell;
    KAction *editCmd;

    KAction *viewSplitVert;
    KAction *viewSplitHoriz;
    KAction *closeCurrentViewSpace;

    KAction *goNext;
    KAction *goPrev;

    KSelectAction* setEndOfLine;
    KAction* documentReload;

    KAction *setHighlightConf;
    KSelectAction *setHighlight;

    KAction *gotoLine;
    KAction* windowNext;
    KAction* windowPrev;
    KActionMenu *docListMenu;
    KActionMenu *bookmarkMenu;

    KToggleAction* settingsShowSidebar;
    KToggleAction* settingsShowFullPath;
    KToggleAction* settingsShowToolbar;
    KToggleAction* settingsShowConsole;
    KAction* settingsConfigure;

    KToggleAction* settingsShowFullScreen;

    KAction* sidebarFocusNext;
   
    QString tagSidebar;

  public slots:
    void newWindow ();

    void slotSettingsShowSidebar();
    void slotSettingsShowConsole();
    void slotSettingsShowToolbar();

    void slotConfigure();
    void slotHlConfigure();

    void slotSidebarFocusNext();

  private:
    void setupMainWindow();
    void setupActions();
    void setupPlugins();

    virtual bool queryClose();

    void readOptions(KConfig *);
    void saveOptions(KConfig *);
    virtual void saveProperties(KConfig*);

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

  private slots:
    void slotFileQuit();
    void slotEditToolbars();
    void slotWindowActivated ();
    void slotCurrentDocChanged();
    void docListMenuAboutToShow();
    void setEOLMenuAboutToShow();
    void bookmarkMenuAboutToShow();
    void settingsMenuAboutToShow();
    void setHighlightMenuAboutToShow();
    void slotSettingsShowFullPath();
    void slotDropEvent(QDropEvent *);
    void editKeys();

    void slotGoNext();
    void slotGoPrev();
    void slotSettingsShowFullScreen();

    void fileSelected(const KFileViewItem *file);

  public:
    void openURL (const QString &name=0L);
    void ShowErrorMessage (const QString & strFileName = 0, int nLine = 1, const QString & strMessage=0);

  protected:
     bool eventFilter (QObject* o, QEvent* e);
};

#endif
