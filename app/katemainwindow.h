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
#include "kateIface.h"

#include "kateviewmanager.h"

#include "kateprojectmanager.h"

#include <kate/view.h>
#include <kate/document.h>

#include <kparts/part.h>
#include <kparts/dockmainwindow.h>

#include <qguardedptr.h>

#include <scriptmanager.h>
#include <kaction.h>

#define DEFAULT_STYLE "Modern"

class GrepDialog;

class KFileItem;
class KRecentFilesAction;

class KateMainWindow : public KParts::DockMainWindow, virtual public KateMainWindowDCOPIface, virtual public KParts::PartBase
{
  Q_OBJECT

  friend class KateConfigDialog;
  friend class KateApp;
  friend class KateViewManager;
  friend class KateView;
  friend class KateDocument;

  public:
    KateMainWindow(KateDocManager *_docManager, KatePluginManager *_pluginManager, KateProjectManager *projectMan);
    ~KateMainWindow();

    Kate::MainWindow *mainWindow () { return m_mainWindow; };
    Kate::ToolViewManager *toolViewManager () { return m_toolViewManager; };

    Kate::Project *activeProject () { return m_project; };

    /**
     * Creates a new project file at give url of given type + opens it
     * @param type projecttype
     * @param filename name of the new project file
     * @return Project new created project object
     */
     Kate::Project *createProject (const QString &type, const QString &name, const QString &filename);

    /**
     * @param filename name of the project file
     * @return Project opened project
     */
    Kate::Project *openProject (const QString &filename);

    void activateProject (Kate::Project *project);

    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL activeDocumentUrl();

    // dockwidgets
    KDockWidget *mainDock;
    KDockWidget *consoleDock;
    KDockWidget *filelistDock;
    KDockWidget *projectsDock;
    KDockWidget *fileselectorDock;
    KDockWidget *grepWidgetDock;

    KDockWidget *m_leftDock;
    KDockWidget *m_rightDock;
    KDockWidget *m_topDock;
    KDockWidget *m_bottomDock;
    KActionMenu *m_settingsShowToolViews;
    // console
    KateConsole *console;

    // management items
    KateDocManager *m_docManager;
    KateViewManager *m_viewManager;
    KateProjectManager *m_projectManager;

    // should be protected, and kateviewmanager a friend class.
    KRecentFilesAction *fileOpenRecent;

    KateFileList *filelist;
    class KateProjectList *projectlist;
    class KateProjectViews *projectviews;
    KateFileSelector *fileselector;

  private:
    uint myID;
    bool syncKonsole;

  protected:
    KatePluginManager *m_pluginManager;

    // Anders: I moved the config object to protected, because
    // other objects needs access.
    KConfig* config;

  private:
    QGuardedPtr<Kate::Project> m_project;
    uint m_projectNumber;
    QGuardedPtr<Kate::View> activeView;

    KAction *closeCurrentViewSpace;

    KAction *goNext;
    KAction *goPrev;

    KAction *saveProject;
    KAction *closeProject;
    KRecentFilesAction *recentProjects;

    KActionMenu* documentOpenWith;

    KAction *gotoLine;
    KAction* windowNext;
    KAction* windowPrev;

    QPopupMenu *documentMenu;

    KToggleAction* settingsShowFilelist;
    KToggleAction* settingsShowFileselector;

    KAction* settingsConfigure;

    KActionMenu *scriptMenu;
    KScriptManager* kscript;

  public slots:
    void newWindow ();

    void slotConfigure();

    void slotOpenWithMenuAction(int idx);

 private:
     GrepDialog* grep_dlg;

  public slots:
    void slotGrepDialogItemSelected ( const QString &filename, int linenumber );
    void runScript( int menuItemId);
    void slotMail();

  public:
    void saveWindowConfiguration (KConfig *config);
    void restoreWindowConfiguration (KConfig *config);

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
    void slotDocumentChanged();
    void slotWindowActivated ();
    void documentMenuAboutToShow();
    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();
    void slotGoNext();
    void slotGoPrev();

    void fileSelected(const KFileItem *file);

    void tipOfTheDay();

    /* to update the caption */
    void slotDocumentCreated (Kate::Document *doc);
    void updateCaption (Kate::Document *doc);

  public:
    void openURL (const QString &name=0L);

  protected:
    static uint uniqueID;
    Kate::MainWindow *m_mainWindow;
    Kate::ToolViewManager *m_toolViewManager;

  public:
    Kate::ViewManager *viewManager () {return m_viewManager->viewManager(); };
    KateViewManager *kateViewManager () { return m_viewManager; };
    KDockWidget *centralDock () { return mainDock; };

  // For dcop interface. -anders
  public:
    /** @return the interface number for the current document */
    int currentDocumentIfaceNumber();

  public: //ToolViewManager stuff
    virtual KDockWidget *addToolViewWidget(KDockWidget::DockPosition pos,QWidget *widget,const QPixmap &icon, const QString& caption);
    virtual bool removeToolViewWidget(QWidget *);
    virtual KDockWidget *addToolView(KDockWidget::DockPosition pos,const char* name,const QPixmap &icon,const QString&);
    virtual bool removeToolView(KDockWidget *);

    virtual bool hideToolView(class KDockWidget*);
    virtual bool showToolView(class KDockWidget*);
    virtual bool hideToolView(const QString& name);
    virtual bool showToolView(const QString& name);


  private slots:
    void pluginHelp ();
    void slotFullScreen(bool);

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
};

class KateToggleToolViewAction:public KToggleAction
{
Q_OBJECT
public:
	KateToggleToolViewAction( const QString& text, const KShortcut& cut = KShortcut(),KDockWidget *dw=0, QObject* parent = 0, KateMainWindow* mw=0, const char* name = 0 );
	virtual ~KateToggleToolViewAction();

private:
	KDockWidget *m_dw;
	KateMainWindow *m_mw;
protected slots:
	void slotToggled(bool);
	void anDWChanged();
	void slotWidgetDestroyed();
};

#endif


