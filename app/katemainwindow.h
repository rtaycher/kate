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
#include "kateIface.h"

#include <kate/view.h>
#include <kate/document.h>

#include <kparts/part.h>

#include <qguardedptr.h>

#include <scriptmanager.h>

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
    QGuardedPtr<Kate::View> activeView;
  
    KAction *closeCurrentViewSpace;

    KAction *goNext;
    KAction *goPrev;

    KActionMenu* documentOpenWith;

    KAction *gotoLine;
    KAction* windowNext;
    KAction* windowPrev;

    QPopupMenu *documentMenu;

    KToggleAction* settingsShowFilelist;
    KToggleAction* settingsShowFileselector;
    KToggleAction* settingsShowToolbar;
    KToggleAction* settingsShowConsole;

    KAction* settingsConfigure;

    KSelectAction* scriptMenu;
    KScriptManager* kscript;

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
    void runScript();

  private:
    void setupMainWindow();
    void setupActions();
    void setupScripts();
    bool queryClose();

    void readOptions(KConfig *);
    void saveOptions(KConfig *);
    void saveProperties(KConfig*);

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

  private slots:
    void slotFileQuit();
    void slotEditToolbars();
    void slotWindowActivated ();
    void documentMenuAboutToShow();
    void settingsMenuAboutToShow();
    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();
    void slotGoNext();
    void slotGoPrev();

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
