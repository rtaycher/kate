/***************************************************************************
                          katefileselector.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Matt Newell, 2002 by Anders Lund
    email                : newellm@proaxis.com, anders@alweb.dk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KATE_FILESELECTOR_H__
#define __KATE_FILESELECTOR_H__

#include "katemain.h"
#include "katedocmanager.h"
#include <kate/document.h>

#include <qwidget.h>
#include <kfile.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <qframe.h>

class KateMainWindow;
class KateViewManager;
class KActionCollection;
class KActionSelector;
 
/*
    The kate file selector presents a directory view, in which the default action is
    to open the activated file.
    Additinally, a toolbar for managing the kdiroperator widget + sync that to
    the directory of the current file is available, as well as a filter widget
    allowing to filter the displayed files using a name filter.
*/


/* I think this fix for not moving toolbars is better */
class KateFileSelectorToolBar: public KToolBar
{
	Q_OBJECT
public:
	KateFileSelectorToolBar(QWidget *parent);
	virtual ~KateFileSelectorToolBar();

	 virtual void setMovingEnabled( bool b );

};


class KateFileSelector : public QWidget
{
  Q_OBJECT
  
  friend class KFSConfigPage;

  public:
    /* When to sync to current document directory */
    enum AutoSyncEvent { DocumentChanged=1, DocumentOpened=2, GotVisible=4 };

    KateFileSelector( KateMainWindow *mainWindow=0, KateViewManager *viewManager=0,
                      QWidget * parent = 0, const char * name = 0 );
    ~KateFileSelector();

    void readConfig( KConfig *, const QString & );
    void writeConfig( KConfig *, const QString & );
    void setupToolbar( KConfig * );
    void setView( KFile::FileView );
    KDirOperator *dirOperator(){ return dir; }
    KActionCollection *actionCollection() { return mActionCollection; };
    
  public slots:
    void slotFilterChange(const QString&);
    void setDir(KURL);
    void setDir( const QString& url ) { setDir( KURL( url ) ); };

  private slots:
    void cmbPathActivated( const KURL& u );
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KURL& u );
    void dirFinishedLoading();
    void setActiveDocumentDir();
    void kateViewChanged();
    void btnFilterClick();
    void autoSync();
    void autoSync( Kate::Document * );
  protected:
    void focusInEvent( QFocusEvent * );
    void showEvent( QShowEvent * );
    bool eventFilter( QObject *, QEvent * );

  private:
    /*class */KateFileSelectorToolBar *toolbar;
    KActionCollection *mActionCollection;
    class KBookmarkHandler *bookmarkHandler;
    KURLComboBox *cmbPath;
    KDirOperator * dir;
    class KAction *acSyncDir;
    class TBContainer *tbparent;
    KHistoryCombo * filter;
    class QToolButton *btnFilter;

    KateMainWindow *mainwin;
    KateViewManager *viewmanager;

    QString lastFilter;
    int autoSyncEvents; // enabled autosync events
    QString waitingUrl; // maybe display when we gets visible

};

/*  TODO anders
    KFSFilterHelper
    A popup widget presenting a listbox with checkable items
    representing the mime types available in the current directory, and
    providing a name filter based on those.
*/

/*
    Config page for file selector.
    Allows for configuring the toolbar, the history length
    of the path and file filter combos, and how to handle
    user closed session.
*/
class KFSConfigPage : public Kate::ConfigPage {
  Q_OBJECT
  public:
    KFSConfigPage( QWidget* parent=0, const char *name=0, KateFileSelector *kfs=0);
    virtual ~KFSConfigPage() {};

    virtual void apply();
    virtual void reload();

  private:
    void init();
    
    KateFileSelector *fileSelector;
    bool bDirty;
    //class QListBox *lbAvailableActions, *lbUsedActions;
    KActionSelector *acSel;
    class QSpinBox *sbPathHistLength, *sbFilterHistLength;
    class QCheckBox *cbSyncOpen, *cbSyncActive, *cbSyncShow;
    class QCheckBox *cbSesLocation, *cbSesFilter;
};


#endif //__KATE_FILESELECTOR_H__
