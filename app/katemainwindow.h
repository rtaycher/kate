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

#include "kateviewmanager.h"
#include "kateprojectmanager.h"

#include <kate/view.h>
#include <kate/document.h>

#include <kparts/part.h>
#include <kparts/dockmainwindow.h>
#include <kmdi/mainwindow.h>

#include <qguardedptr.h>

#include <scriptmanager.h>
#include <kaction.h>

class GrepTool;

class KFileItem;
class KRecentFilesAction;
class DCOPObject;

class KateExternalToolsMenuAction;
class KateProjectList;
class KateProjectViews;

class KateMainWindow : public KMDI::MainWindow, virtual public KParts::PartBase
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateViewManager;

  public:
    KateMainWindow ();
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
   * ToolView Managment, used to create/access/delete toolviews
   */
  public:
    KMDI::ToolViewAccessor *addToolView(KDockWidget::DockPosition position, QWidget *widget, const QPixmap &icon, const QString &sname, const QString &tabToolTip = 0, const QString &tabCaption = 0);

    bool removeToolView(QWidget *);
    bool removeToolView(KMDI::ToolViewAccessor *);

    bool showToolView(QWidget *);
    bool showToolView(KMDI::ToolViewAccessor *);

    bool hideToolView(QWidget *);
    bool hideToolView(KMDI::ToolViewAccessor *);

  /**
   * Project section
   */
  public:
    /**
     * current active project
     * @return active project
     */
    Kate::Project *activeProject () { return m_project; }

    /**
     * Creates a new project file at give url of given type + opens it
     * @param type projecttype
     * @param name project name
     * @param filename filename of the new project file
     * @return new created project object
     */
    Kate::Project *createProject (const QString &type, const QString &name, const QString &filename);

    /**
     * @param filename name of the project file
     * @return opened project
     */
    Kate::Project *openProject (const QString &filename);

    /**
     * activate given project
     * @param project project to activate
     */
    void activateProject (Kate::Project *project);

  /**
   * various methodes to get some little info out of this
   */
  public:
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL activeDocumentUrl();

    bool notifyMod() const { return modNotification; }

    uint mainWindowNumber () const { return myID; }

  public:
    void readProperties(KConfig *config);
    void saveProperties(KConfig *config);
    void saveGlobalProperties( KConfig* sessionConfig );

  private:
    void setupMainWindow();
    void setupActions();
    void setupScripts();
    bool queryClose();
    bool queryClose_internal();

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

  // slots for the project GUI actions: new/open/save/close
  public slots:
    void slotProjectNew ();
    void slotProjectOpen ();
    void slotProjectSave ();
    void slotProjectClose ();

    // recent files
    void openConstURLProject (const KURL&);

  private slots:
    void projectDeleted (uint projectNumber);
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
    KateProjectList *projectlist;
    KateProjectViews *projectviews;
    KateFileSelector *fileselector;

    QGuardedPtr<Kate::Project> m_project;
    uint m_projectNumber;

    KAction *saveProject;
    KAction *closeProject;
    KRecentFilesAction *recentProjects;

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
    bool m_modignore;
};

#endif


