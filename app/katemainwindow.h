/***************************************************************************
                          katemainwindow.h  -  description
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_MAINWINDOW_H__
#define __KATE_MAINWINDOW_H__

#include "katemain.h"
#include "../interfaces/mainwindow.h"
#include "../interfaces/document.h"
#include "../part/kateviewhighlightaction.h"
#include "kateIface.h"

#include <kparts/part.h>

#include <kscript/scriptloader.h>
#include <kscript/scriptinterface.h>

class GrepDialog;
class KFileItem;

class KateMainWindow : public Kate::MainWindow, virtual public KateMainWindowDCOPIface, virtual public KParts::PartBase
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateApp;
  friend class KateViewManager;
  friend class KateView;
  friend class KateDocument;

  public:
    KateMainWindow(KateDocManager *_docManager, KatePluginManager *_pluginManager);
    ~KateMainWindow();

    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL currentDocUrl();

    // dockwidgets
    KDockWidget *mainDock;
    KDockWidget *consoleDock;
    KDockWidget *filelistDock;
    KDockWidget *fileselectorDock;

    // console
    KateConsole *console;

    // managment items
    KateDocManager *docManager;
    KateViewManager *viewManager;

    // should be protected, and kateviewmanager a friend class.
    KRecentFilesAction *fileOpenRecent;

    KateFileList *filelist;
    KateFileSelector *fileselector;

  private:
    uint myID;
    bool syncKonsole;

  protected:

    KatePluginManager *pluginManager;

    /** just calls viewmanager */
    void restore(bool isRestored);

    // Anders: I moved the config object to protected, because
    // other objects needs access.
    KConfig* config;

  private:
    KAction *editUndo;
    KAction *editRedo;
    KAction *bookmarkToggle;
    KAction *bookmarkClear;

    KAction *closeCurrentViewSpace;
    KToggleAction *viewBorder;

    KAction *goNext;
    KAction *goPrev;

    KSelectAction* setEndOfLine;
    KAction* documentReload;
    KActionMenu* documentOpenWith;
    KateViewHighlightAction *setHighlight;

    KAction *gotoLine;
    KAction* windowNext;
    KAction* windowPrev;

    QPopupMenu *bookmarkMenu, *documentMenu;
    QPtrList<Kate::Mark> list;

    KToggleAction* settingsShowFilelist;
    KToggleAction* settingsShowFileselector;
    KToggleAction* settingsShowFullPath;
    KToggleAction* settingsShowToolbar;
    KToggleAction* settingsShowConsole;
    KToggleAction* setVerticalSelection;

    KAction* settingsConfigure;
    
    KSelectAction* scriptMenu;
    ScriptLoader* kscript;

  public slots:
    void newWindow ();

    void slotSettingsShowConsole();
    void slotSettingsShowToolbar();

    void slotConfigure();

    void slotOpenWithMenuAction(int idx);

 private:
     GrepDialog* grep_dlg;

  public slots:
    void slotGrepDialogItemSelected ( QString filename, int linenumber );
    void slotFindInFiles ();

  private:
    void setupMainWindow();
    void setupActions();
//    void setupPlugins();

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
    void documentMenuAboutToShow();
    void setEOLMenuAboutToShow();
    void bookmarkMenuAboutToShow();
    void settingsMenuAboutToShow();
    void slotSettingsShowFullPath();
    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();
    void slotGoNext();
    void slotGoPrev();

    void gotoBookmark (int n);

    void fileSelected(const KFileItem *file);

  public:
    void openURL (const QString &name=0L);

  protected:
     bool eventFilter (QObject* o, QEvent* e);

  protected:
    static uint uniqueID;

  public:
    Kate::ViewManager *getViewManager ();
    Kate::DocManager *getDocManager ();
    KDockWidget *getMainDock ();

  private slots:
    void pluginHelp ();
};

#endif
