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

#ifndef __KATE_FILESELECTOR_H__
#define __KATE_FILESELECTOR_H__


#include <ktexteditor/document.h>
#include <ktexteditor/configpage.h>
#include <kate/interfaces/plugin.h>
#include <kate/interfaces/pluginconfigpageinterface.h>
#include <kdiroperator.h>
#include <kurlcombobox.h>

#include <kvbox.h>
//Added by qt3to4:
#include <QShowEvent>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QEvent>
#include <kfile.h>
#include <kurl.h>
#include <ktoolbar.h>


class KActionCollection;
class KActionSelector;
class QToolButton;
class QCheckBox;
class QSpinBox;

namespace Kate {
class MainWindow;
namespace Private {
namespace Plugin {
class KateFileSelector;

class KateFileSelectorPlugin:public Kate::Plugin,public Kate::PluginViewInterface,public Kate::PluginConfigPageInterface {
    Q_OBJECT
    Q_INTERFACES(Kate::PluginViewInterface)
    Q_INTERFACES(Kate::PluginConfigPageInterface)
  public:
    KateFileSelectorPlugin( QObject* parent = 0, const char* name = 0, const QStringList& = QStringList() );
    virtual ~KateFileSelectorPlugin(){}
    void addView (Kate::MainWindow *win);
    void removeView (Kate::MainWindow *win);

    void storeViewConfig(KConfig*,Kate::MainWindow *,const QString&) {}
    void loadViewConfig(KConfig*,Kate::MainWindow *,const QString&);
    void storeGeneralConfig(KConfig*,const QString&) {}
    void loadGeneralConfig(KConfig*,const QString&) {}
    
    uint configPages() const;
    PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name=0);
    QString configPageName (uint number = 0) const;
    QString configPageFullName (uint number = 0) const;
    QPixmap configPagePixmap (uint number = 0, int size = KIcon::SizeSmall) const;    
    
  private:
    QLinkedList<KateFileSelector*> m_views;  
};


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

class KateFileSelectorToolBarParent: public QFrame
{
	Q_OBJECT
public:
	KateFileSelectorToolBarParent(QWidget *parent);
	~KateFileSelectorToolBarParent();
	void setToolBar(KateFileSelectorToolBar *tb);
private:
	KateFileSelectorToolBar *m_tb;
protected:
	virtual void resizeEvent ( QResizeEvent * );
};

class KateFileSelector : public KVBox
{
  Q_OBJECT

  friend class KFSConfigPage;

  public:
    /* When to sync to current document directory */
    enum AutoSyncEvent { DocumentChanged=1, GotVisible=2 };

    KateFileSelector( Kate::MainWindow *mainWindow=0,
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
    void kateViewChanged();

  private slots:
    void fileSelected(const KFileItem * /*file*/);
    void cmbPathActivated( const KURL& u );
    void cmbPathReturnPressed( const QString& u );
    void dirUrlEntered( const KURL& u );
    void dirFinishedLoading();
    void setActiveDocumentDir();
    void btnFilterClick();

  protected:
    KURL activeDocumentUrl();
    void focusInEvent( QFocusEvent * );
    void showEvent( QShowEvent * );
    bool eventFilter( QObject *, QEvent * );
    void initialDirChangeHack();

  public:
    Kate::MainWindow* mainWindow() {return mainwin;}
  private:
    class KateFileSelectorToolBar *toolbar;
    KActionCollection *mActionCollection;
    class KBookmarkHandler *bookmarkHandler;
    KURLComboBox *cmbPath;
    KDirOperator * dir;
    class KAction *acSyncDir;
    KHistoryCombo * filter;
    QToolButton *btnFilter;

    Kate::MainWindow *mainwin;

    QString lastFilter;
    int autoSyncEvents; // enabled autosync events
    QString waitingUrl; // maybe display when we gets visible
    QString waitingDir;
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
class KFSConfigPage : public Kate::PluginConfigPage {
  Q_OBJECT
  public:
    KFSConfigPage( QWidget* parent=0, const char *name=0, KateFileSelector *kfs=0);
    virtual ~KFSConfigPage() {};

    virtual void apply();
    virtual void reset();
    virtual void defaults() {}

  private slots:
    void slotMyChanged();

  private:
    void init();

    KateFileSelector *fileSelector;
    KActionSelector *acSel;
    QSpinBox *sbPathHistLength, *sbFilterHistLength;
    QCheckBox *cbSyncActive, *cbSyncShow;
    QCheckBox *cbSesLocation, *cbSesFilter;

    bool m_changed;
};

}
}
}

#endif //__KATE_FILESELECTOR_H__
// kate: space-indent on; indent-width 2; replace-tabs on;
