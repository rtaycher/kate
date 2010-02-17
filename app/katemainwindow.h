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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KATE_MAINWINDOW_H__
#define __KATE_MAINWINDOW_H__

#include "katemain.h"
#include "katemdi.h"
#include "katefilelist.h"

#include <KTextEditor/View>
#include <KTextEditor/Document>

#include <KParts/Part>

#include <KAction>

#include <QDragEnterEvent>
#include <QEvent>
#include <QDropEvent>
#include <QVBoxLayout>
#include <QModelIndex>
#include <QHash>

class QMenu;


namespace Kate
{
  class MainWindow;
  class Plugin;
  class PluginView;
  class PluginConfigPageInterface;
}

class KFileItem;
class KRecentFilesAction;

class KateExternalToolsMenuAction;
class KateViewDocumentProxyModel;
class KateViewManager;
class KateMwModOnHdDialog;

#include <QtGui/QStackedLayout>
// Helper layout class to always provide minimum size
class KateContainerStackedLayout : public QStackedLayout
{
  Q_OBJECT
public:
  KateContainerStackedLayout(QWidget* parent);
  virtual QSize sizeHint() const;
  virtual QSize minimumSize() const;
};


class KateMainWindow : public KateMDI::MainWindow, virtual public KParts::PartBase
{
    Q_OBJECT

    friend class KateConfigDialog;
    friend class KateViewManager;

  public:
    /**
     * Construct the window and restore it's state from given config if any
     * @param sconfig session config for this window, 0 if none
     * @param sgroup session config group to use
     */
    KateMainWindow (KConfig *sconfig, const QString &sgroup);

    /**
     * Destruct the nice window
     */
    ~KateMainWindow();

    /**
     * Accessor methodes for interface and child objects
     */
  public:
    Kate::MainWindow *mainWindow ()
    {
      return m_mainWindow;
    }

    KateViewManager *viewManager ()
    {
      return m_viewManager;
    }

    QString dbusObjectPath() const
    {
      return m_dbusObjectPath;
    }
    /**
     * various methodes to get some little info out of this
     */
  public:
    /** Returns the URL of the current document.
     * anders: I add this for use from the file selector. */
    KUrl activeDocumentUrl();

    uint mainWindowNumber () const
    {
      return myID;
    }

    /**
     * Prompts the user for what to do with files that are modified on disk if any.
     * This is optionally run when the window receives focus, and when the last
     * window is closed.
     * @return true if no documents are modified on disk, or all documents were
     * handled by the dialog; otherwise (the dialog was canceled) false.
     */
    bool showModOnDiskPrompt();

  public:
    /*reimp*/ void readProperties(const KConfigGroup& config);
    /*reimp*/ void saveProperties(KConfigGroup& config);
    /*reimp*/ void saveGlobalProperties( KConfig* sessionConfig );

  public:
    bool queryClose_internal(KTextEditor::Document *doc = NULL);

    /**
     * save the settings, size and state of this window in
     * the provided config group
     */
    void saveWindowConfig(const KConfigGroup &);
    /**
     * restore the settings, size and state of this window from
     * the provided config group.
     */
    void restoreWindowConfig(const KConfigGroup &);

  private:
    /**
     * Setup actions which pointers are needed already in setupMainWindow
     */
    void setupImportantActions ();

    void setupMainWindow();
    void setupActions();
    bool queryClose();

    /**
     * read some global options from katerc
     */
    void readOptions();

    /**
     * save some global options to katerc
     */
    void saveOptions();

    void dragEnterEvent( QDragEnterEvent * );
    void dropEvent( QDropEvent * );

  public Q_SLOTS:
    void slotFileQuit();

    /**
     * slots used for actions in the menus/toolbars
     * or internal signal connections
     */
  private Q_SLOTS:
    void newWindow ();

    void slotConfigure();

    void slotOpenWithMenuAction(QAction* a);

    void slotEditToolbars();
    void slotNewToolbarConfig();
    void slotWindowActivated ();
    void slotUpdateOpenWith();

    void documentMenuAboutToShow();
    void activateDocumentFromDocMenu (QAction *action);

    void slotDropEvent(QDropEvent *);
    void editKeys();
    void mSlotFixOpenWithMenu();

    //void fileSelected(const KFileItem &file);

    void tipOfTheDay();

    /* to update the caption */
    void slotDocumentCreated (KTextEditor::Document *doc);
    void updateCaption (KTextEditor::Document *doc);
    // calls updateCaption(doc) with the current document
    void updateCaption ();

    void pluginHelp ();
    void aboutEditor();
    void slotFullScreen(bool);

  private Q_SLOTS:
    void toggleShowStatusBar ();

  public:
    bool showStatusBar ();

  Q_SIGNALS:
    void statusBarToggled ();

  public:
    void openUrl (const QString &name = 0L);
    QModelIndex modelIndexForDocument(KTextEditor::Document *document);

    QHash<Kate::Plugin*, Kate::PluginView*> &pluginViews ()
    {
      return m_pluginViews;
    }

    inline QWidget *horizontalViewBarContainer() {return m_horizontalViewBarContainer;}
    inline void addToHorizontalViewBarContainer(KTextEditor::View *view,QWidget *bar){m_containerstack->addWidget (bar); m_viewBarMapping[view]=BarState(bar);}
    inline void hideHorizontalViewBarForView(KTextEditor::View *view) {QWidget *bar; BarState state=m_viewBarMapping.value(view); bar=state.bar(); if (bar) {m_containerstack->setCurrentWidget(bar); bar->hide(); state.setState(false); m_viewBarMapping[view]=state;} m_horizontalViewBarContainer->hide();}
    inline void showHorizontalViewBarForView(KTextEditor::View *view) {QWidget *bar; BarState state=m_viewBarMapping.value(view); bar=state.bar();  if (bar) {m_containerstack->setCurrentWidget(bar); bar->show(); state.setState(true); m_viewBarMapping[view]=state;  m_horizontalViewBarContainer->show();}}
    inline void deleteHorizontalViewBarForView(KTextEditor::View *view) {QWidget *bar; BarState state=m_viewBarMapping.take(view); bar=state.bar();  if (bar) {if (m_containerstack->currentWidget()==bar) m_horizontalViewBarContainer->hide(); delete bar;}}

    void switchToNextDocument() { m_fileList->slotNextDocument(); }
    void switchToPreviousDocument() { m_fileList->slotPrevDocument(); }

  private Q_SLOTS:
    void slotUpdateHorizontalViewBar();


  private Q_SLOTS:
    void showFileListPopup(const QPoint& pos);
  protected:
    bool event( QEvent * );

  private Q_SLOTS:
    void slotDocumentCloseAll();
    void slotDocumentCloseOther();
    void slotDocumentCloseOther(KTextEditor::Document *document);
    void slotDocumentCloseSelected(const QList<KTextEditor::Document*>&);
    void slotDocModified(KTextEditor::Document *document);
  private:
    static uint uniqueID;
    uint myID;

    Kate::MainWindow *m_mainWindow;

    bool modNotification;

    // management items
    KateViewManager *m_viewManager;

    KRecentFilesAction *fileOpenRecent;

    class KateFileList *m_fileList;

    KActionMenu* documentOpenWith;

    QMenu *documentMenu;
    QActionGroup *documentsGroup;

    KToggleAction* settingsShowFilelist;
    KToggleAction* settingsShowFileselector;

    KateExternalToolsMenuAction *externalTools;
    bool m_modignore, m_grrr;

    QString m_dbusObjectPath;
    KateViewDocumentProxyModel *m_documentModel;

    // all plugin views for this mainwindow, used by the pluginmanager
    QHash<Kate::Plugin*, Kate::PluginView*> m_pluginViews;

    // options: show statusbar + show path
    KToggleAction *m_paShowPath;
    KToggleAction *m_paShowStatusBar;
    QWidget *m_horizontalViewBarContainer;
    KateContainerStackedLayout *m_containerstack;
    class BarState{
      public:
        BarState():m_bar(0),m_state(false){}
        BarState(QWidget* bar):m_bar(bar),m_state(false){}
        ~BarState(){}
        QWidget *bar(){return m_bar;}
        bool state(){return m_state;}
        void setState(bool state){m_state=state;}
      private:
        QWidget *m_bar;
        bool m_state;
    };
    QHash<KTextEditor::View*,BarState> m_viewBarMapping;


  public:
    void queueModifiedOnDisc(KTextEditor::Document *doc);
    static void unsetModifiedOnDiscDialogIfIf(KateMwModOnHdDialog* diag) {
      if (s_modOnHdDialog==diag) s_modOnHdDialog=0;
    }
  private:
    static KateMwModOnHdDialog *s_modOnHdDialog;

  public:
    void showPluginConfigPage(Kate::PluginConfigPageInterface *configpageinterface,uint id);  

};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
