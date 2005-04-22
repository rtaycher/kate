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

#ifndef __KATE_MAINWINDOW_H__
#define __KATE_MAINWINDOW_H__

#include "katemain.h"
#include "../interfaces/mainwindow.h"
#include "../interfaces/toolviewmanager.h"

#include "katemdi.h"
#include "kateviewmanager.h"

#include <kate/view.h>
#include <kate/document.h>

#include <kparts/part.h>
#include <kparts/dockmainwindow.h>
#include <kmdi/mainwindow.h>

#include <scriptmanager.h>
#include <kaction.h>

class KateTabWidget;
class GrepTool;

class KFileItem;
class KRecentFilesAction;
class DCOPObject;

class KateExternalToolsMenuAction;

class KateMainWindow : public KateMDI::MainWindow, virtual public KParts::PartBase
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateViewManager;

  public:
    KateMainWindow (KConfig *sconfig, const QString &sgroup);
    ~KateMainWindow();

  /**
   * Accessor methodes for interface and child objects
   */
  public:
    Kate::MainWindow *mainWindow () { return m_mainWindow; }
    Kate::ViewManager *viewManager () {return m_viewManager->viewManager(); }
    Kate::ToolViewManager *toolViewManager () { return m_toolViewManager; }

    KateViewManager *kateViewManager () { return m_viewManager; }

    DCOPObject *dcopObject () { return m_dcop; }

  /**
   * various methodes to get some little info out of this
   */
  public:
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL activeDocumentUrl();

    uint mainWindowNumber () const { return myID; }

    /**
     * Prompts the user for what to do with files that are modified on disk if any.
     * This is optionally run when the window receives focus, and when the last
     * window is closed.
     * @return true if no documents are modified on disk, or all documents were
     * handled by the dialog; otherwise (the dialog was canceled) false.
     */
    bool showModOnDiskPrompt();

    /**
     * central tabwidget ;)
     * @return tab widget
     */
    KateTabWidget *tabWidget ();

  public:
    void readProperties(KConfig *config);
    void saveProperties(KConfig *config);
    void saveGlobalProperties( KConfig* sessionConfig );

  public:
    bool queryClose_internal();

  private:
    void setupMainWindow();
    void setupActions();
    void setupScripts();
    bool queryClose();

    void readOptions(KConfig *);
    void saveOptions(KConfig *);

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

  /**
   * slots used for actions in the menus/toolbars
   * or internal signal connections
   */
  private slots:
    void newWindow ();

    void slotConfigure();

    void slotOpenWithMenuAction(int idx);

    void slotPipeToConsole ();

    void slotGrepToolItemSelected ( const QString &filename, int linenumber );
    void runScript( int menuItemId);
    void slotMail();

    void slotFileQuit();
    void slotEditToolbars();
    void slotWindowActivated ();
    void slotUpdateOpenWith();
    void documentMenuAboutToShow();
    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();

    void fileSelected(const KFileItem *file);

    void tipOfTheDay();

    /* to update the caption */
    void slotDocumentCreated (Kate::Document *doc);
    void updateCaption (Kate::Document *doc);

    void pluginHelp ();
    void slotFullScreen(bool);

  public:
    void openURL (const QString &name=0L);

  protected:
    bool eventFilter( QObject*, QEvent * );
    bool event( QEvent * );

  private slots:
    void slotDocumentCloseAll();

  private:
    static uint uniqueID;
    uint myID;

    Kate::MainWindow *m_mainWindow;
    Kate::ToolViewManager *m_toolViewManager;

    bool syncKonsole;
    bool modNotification;

    DCOPObject *m_dcop;

    // console
    KateConsole *console;

    // management items
    KateViewManager *m_viewManager;

    KRecentFilesAction *fileOpenRecent;

    KateFileList *filelist;
    KateFileSelector *fileselector;

    KActionMenu* documentOpenWith;

    QPopupMenu *documentMenu;

    KToggleAction* settingsShowFilelist;
    KToggleAction* settingsShowFileselector;
    KToggleAction* showFullScreenAction;

    KAction* settingsConfigure;

    KActionMenu *scriptMenu;
    KScriptManager* kscript;

    KateExternalToolsMenuAction *externalTools;
    GrepTool * greptool;
    bool m_modignore, m_grrr;

    KateTabWidget *m_tabWidget;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;


