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

//#include "../kantmain.h"
#include "kantIface.h"
//#include "../sidebar/kantsidebar.h"
//#include "../document/kantdocmanager.h"
//#include "../document/kantdocument.h"
//#include "../view/kantview.h"
//#include "../view/kantviewspace.h"
//#include "../view/kantviewmanager.h"
//#include "../project/kantprojectmanager.h"
//#include "../console/kantconsole.h"
//#include "../pluginmanager/kantpluginmanager.h"

//#include <qwidgetstack.h>
//#include <qlayout.h>
//#include <kdiroperator.h>
//#include <kfiledialog.h>
//#include <kapp.h>
#include <kdockwidget.h>
//#include <kuniqueapp.h>
//#include <qsplitter.h>
//#include <kiconloader.h>
//#include <kstddirs.h>
//#include <kaction.h>
//#include <klocale.h>
//#include <kcmdlineargs.h>
//#include <kstdaction.h>
//#include <qvbox.h>
//#include <qlayout.h>
//#include <qsplitter.h>
//#include <dcopclient.h>
//#include <klistbox.h>
//#include <kedittoolbar.h>
//#include <kglobal.h>
//#include <kglobalaccel.h>
//#include <kglobalsettings.h>
//#include <kurldrag.h>

#include <kparts/part.h>
#include <kxmlgui.h>

class KAction;
class KActionMenu;
class KantApp;
class KantConfigDlg;
class KantConsole;
class KantDocManager;
class KantPluginManager;
class KantProjectManager;
class KantSidebar;
class KantViewManager;
class KEditToolbar;
class KProcess;
class KRecentFilesAction;
class KSelectAction;
class KShellProcess;
class KToggleAction;

//namespace KParts
//{

class KantMainWindow : public KDockMainWindow, virtual public KantIface , virtual public KParts::PartBase
{
  Q_OBJECT

  friend class KantApp;
  friend class KantViewManager;
  friend class KantView;
  friend class KantDocument;

  public:
    KantMainWindow(KantPluginManager *_pluginManager);
    ~KantMainWindow();

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
    KantProjectManager *projectManager;
    // should be protected, and kantviewmanager a friend class.
    KRecentFilesAction *fileOpenRecent;

  protected:
    /** reopens documents that was open last time kant was shut down*/
    void reopenDocuments(bool isRestore=false);

    KantPluginManager *pluginManager;

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
    KAction *projectNew;
    KAction *projectOpen;
    KAction *projectSave;
    KAction *projectSaveAs;
    KAction *projectConfigure;
    KAction *projectCompile;
    KAction *projectRun;
    KAction *pluginsAdd;
    KAction *pluginsDelete;

    KAction *viewSplitVert;
    KAction *viewSplitHoriz;
    KAction *closeCurrentViewSpace;

    KAction *goNext;
    KAction *goPrev;

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

    QString  m_strFilterOutput;
    KShellProcess * m_pFilterShellProcess;

  public slots:
    void newWindow ();

    void slotSettingsShowSidebar();
    void slotSettingsShowConsole();
    void slotSettingsShowToolbar();

    void slotConfigure();

    void slotSidebarFocusNext();

  private:
    void setupMainWindow();
    void setupActions();

    virtual bool queryClose();

    virtual void readProperties(KConfig *);
    virtual void saveProperties(KConfig *);
    virtual void saveGlobalProperties(KConfig *);

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

  private slots:
    void slotFileQuit();
    void slotEditToolbars();
    void slotWindowActivated ();
    void slotCurrentDocChanged();
    void docListMenuAboutToShow();
    void bookmarkMenuAboutToShow();
    void projectMenuAboutToShow();
    void settingsMenuAboutToShow();
    void slotPluginsAdd();
    void slotPluginsDelete();
    void slotSettingsShowFullPath();
    void slotDropEvent(QDropEvent *);
    void slotEditFilter ();
    void slotFilterReceivedStdout (KProcess * pProcess, char * got, int len);
    void slotFilterReceivedStderr (KProcess * pProcess, char * got, int len);
    void slotFilterProcessExited (KProcess * pProcess);
    void slotFilterCloseStdin (KProcess *);
    void editKeys();

    void slotGoNext();
    void slotGoPrev();
    void slotSettingsShowFullScreen();

  public:
    void openURL (const QString &name=0L);
    void ShowErrorMessage (const QString & strFileName = 0, int nLine = 1, const QString & strMessage=0);

  protected:
     bool eventFilter (QObject* o, QEvent* e);
};

//};

#endif
