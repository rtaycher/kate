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
#include "../interfaces/toolviewmanager.h"
#include "kateIface.h"

#include "kateviewmanager.h"

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

class KateMainWindow : public KParts::DockMainWindow, virtual public KateMainWindowDCOPIface, virtual public KParts::PartBase
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
    
    enum DockWidgetMode
    {
      ModernStyle,
      ClassicStyle
    };
    
    Kate::MainWindow *mainWindow () { return m_mainWindow; };
    Kate::ToolViewManager *toolViewManager () { return m_toolViewManager; };
    
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KURL activeDocumentUrl();

    // dockwidgets
    KDockWidget *mainDock;
    KDockWidget *consoleDock;
    KDockWidget *filelistDock;
    KDockWidget *fileselectorDock;

    KDockWidget *m_leftDock;
    KDockWidget *m_rightDock;
    KDockWidget *m_topDock;
    KDockWidget *m_bottomDock;
    KActionMenu *m_settingsShowToolViews;
    // console
    KateConsole *console;

    // managment items
    KateDocManager *m_docManager;
    KateViewManager *m_viewManager;

    // should be protected, and kateviewmanager a friend class.
    KRecentFilesAction *fileOpenRecent;

    KateFileList *filelist;
    KateFileSelector *fileselector;

  private:
    uint myID;
    bool syncKonsole;
  
    enum DockWidgetMode m_dockStyle;

  public:
   int dockStyle() { return m_dockStyle;}
  protected:

    KatePluginManager *m_pluginManager;

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
    void slotGrepDialogItemSelected ( QString filename, int linenumber );
    void slotFindInFiles ();
    void runScript( int menuItemId);
    void slotMail();

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
    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();
    void slotGoNext();
    void slotGoPrev();

    void fileSelected(const KFileItem *file);
    
    void tipOfTheDay();

  public:
    void openURL (const QString &name=0L);
    
  protected:
     bool eventFilter (QObject* o, QEvent* e);

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


