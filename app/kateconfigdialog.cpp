/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>

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

// $Id$

#include "kateconfigdialog.h"
#include "kateconfigdialog.moc"

#include "katemainwindow.h"

#include "kateconsole.h"
#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
#include "katefileselector.h"
#include "katefilelist.h"

#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qhbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include <kinstance.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kglobalaccel.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kkeydialog.h>
#include <klistbox.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kwin.h>
#include <kseparator.h>

KateConfigDialog::KateConfigDialog (KateMainWindow *parent, const char *name)
 : KDialogBase (KDialogBase::TreeList, i18n("Configure Kate"), KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel, KDialogBase::Ok, parent, name)
{
  config = parent->config;
  docManager = ((KateApp *)kapp)->kateDocumentManager();
  viewManager = parent->kateViewManager();
  pluginManager = ((KateApp *)kapp)->katePluginManager();
  mainWindow = parent;

  setMinimumSize(600,400);

  v = viewManager->activeView();

  if (!v) return;

  pluginPages.setAutoDelete (false);
  editorPages.setAutoDelete (false);

  KWin kwin;
  kwin.setIcons(winId(), kapp->icon(), kapp->miniIcon());

  QStringList path;

  setShowIconsInTreeList(true);

  path.clear();
  path << i18n("Application");
  setFolderIcon (path, SmallIcon("kate", KIcon::SizeSmall));

  path.clear();

  // General page
  path << i18n("Application") << i18n("General");
  QFrame* frGeneral = addPage(path, i18n("General Options"), BarIcon("gohome", KIcon::SizeSmall));

  QVBoxLayout *lo = new QVBoxLayout( frGeneral );
  lo->setSpacing(KDialog::spacingHint());
  config->setGroup("General");

  // GROUP with the one below: "Startup"
  QButtonGroup *bgStartup = new QButtonGroup( 1, Qt::Horizontal, i18n("Startup"), frGeneral );
  lo->addWidget( bgStartup );
  // reopen files
  cb_reopenFiles = new QCheckBox( bgStartup );
  cb_reopenFiles->setText(i18n("Reopen &files at startup"));
  //config->setGroup("General");
  cb_reopenFiles->setChecked( config->readBoolEntry("reopen at startup", false) );
  QWhatsThis::add(cb_reopenFiles, i18n(
        "If this is enabled Kate will attempt to reopen files that were open when you closed "
        "last time. Cursor position will be recovered if possible. Non-existent files will "
        "not be opened."));
  connect( cb_reopenFiles, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  //config->setGroup("General");
  // restore view  config
  cb_restoreVC = new QCheckBox( bgStartup );
  cb_restoreVC->setText(i18n("Restore &view configuration"));
  cb_restoreVC->setChecked( config->readBoolEntry("restore views", false) );
  QWhatsThis::add(cb_restoreVC, i18n(
        "Check this if you want all your views and frames restored each time you open Kate"));
  connect( cb_restoreVC, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // How instances should be handled
  cb_singleInstance = new QCheckBox(frGeneral);
  lo->addWidget( cb_singleInstance );
  cb_singleInstance->setText(i18n("Allow Kate to use more than one UN&IX process"));
  config->setGroup("KDE");
  cb_singleInstance->setChecked(config->readBoolEntry("MultipleInstances",false));
  QWhatsThis::add( cb_singleInstance, i18n(
        "If this is unchecked, Kate will only use one UNIX process. If you try running it again, the current "
        "process will get the focus, and open any files you requested to be opened. If it is checked, each time "
        "you start Kate, a new UNIX process will be started.") );
  connect( cb_singleInstance, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // show full path in title
  config->setGroup("General");
  cb_fullPath = new QCheckBox( i18n("Show full &path in title"), frGeneral);
  lo->addWidget( cb_fullPath );
  cb_fullPath->setChecked( config->readBoolEntry("Show Full Path in Title", false ) );
  QWhatsThis::add(cb_fullPath,i18n("If this option is checked, the full document path will be shown in the window caption."));
  connect( cb_fullPath, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // opaque resize of view splitters
  cb_opaqueResize = new QCheckBox( frGeneral );
  lo->addWidget( cb_opaqueResize );
  cb_opaqueResize->setText(i18n("&Show content when resizing views"));
  cb_opaqueResize->setChecked( viewManager->useOpaqueResize );
  QWhatsThis::add( cb_opaqueResize, i18n(
        "If this is disabled, resizing views will display a <i>rubberband</i> to show the "
        "new sizes until you release the mouse button.") );
  connect( cb_opaqueResize, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // sync the konsole ?
  cb_syncKonsole = new QCheckBox(frGeneral);
  lo->addWidget( cb_syncKonsole );
  cb_syncKonsole->setText(i18n("Sync &terminal emulator with active document"));
  cb_syncKonsole->setChecked(parent->syncKonsole);
  QWhatsThis::add( cb_syncKonsole, i18n(
        "If this is checked, the built in Konsole will <code>cd</code> to the directory "
        "of the active document when started and whenever the active document changes, "
        "if the document is a local file.") );
  connect( cb_syncKonsole, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // sync the konsole ?
  cb_sortFiles = new QCheckBox(frGeneral);
  lo->addWidget( cb_sortFiles );
  cb_sortFiles->setText(i18n("Sort &files alpabetically in the file list."));
  cb_sortFiles->setChecked(parent->filelist->sortType() == KateFileList::sortByName);
  QWhatsThis::add( cb_sortFiles, i18n(
        "If this is checked, the files in the file list will be sorted alphabetically.") );
  connect( cb_sortFiles, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // number of recent files
  QHBox *hbNrf = new QHBox( frGeneral );
  lo->addWidget( hbNrf );
  QLabel *lNrf = new QLabel( i18n("&Number of recent files:"), hbNrf );
  sb_numRecentFiles = new QSpinBox( 0, 1000, 1, hbNrf );
  sb_numRecentFiles->setValue( mainWindow->fileOpenRecent->maxItems() );
  lNrf->setBuddy( sb_numRecentFiles );
  QString youwouldnotbelieveit ( i18n(
        "<qt>Sets the number of recent files remembered by Kate.<p><strong>NOTE: </strong>"
        "If you set this lower than the current value, the list will be truncated and "
        "some items forgotten.</qt>") );
  QWhatsThis::add( lNrf, youwouldnotbelieveit );
  QWhatsThis::add( sb_numRecentFiles, youwouldnotbelieveit );
  connect( sb_numRecentFiles, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );

  KSeparator *sep=new KSeparator(frGeneral);
  sep->setOrientation(KSeparator::HLine);
  lo->addWidget(sep);
  lo->addWidget(new QLabel(i18n("Toolview mode:"),frGeneral));
  cb_mode=new QComboBox(frGeneral);
  cb_mode->insertItem(i18n("Modern Style"));
  cb_mode->insertItem(i18n("Classic Style"));
  lo->addWidget(cb_mode);
  QWhatsThis::add(cb_mode,i18n("Choose how you want the toolviews managed.<BR><ul>"
  "<li><b>Modern Style</b> The toolviews will behave similar to the views in Konqueror's navigation panel.</li>"
  "<li><b>Classic Style</b> The toolviews (filelist, fileselector, ...) can be docked anywhere and made floating.</li></ul>"));

  config->setGroup("General");
  cb_mode->setCurrentItem((config->readEntry("viewMode",DEFAULT_STYLE)=="Modern")?0:1);
  connect( cb_mode, SIGNAL( activated ( int ) ), this, SLOT( slotChanged() ) );

  lo->addStretch(1); // :-] works correct without autoadd
  // END General page


  path.clear();

  // file selector page
  path << i18n("Application") << i18n("File Selector");

  QVBox *page = addVBoxPage( path, i18n("File Selector Settings"),
                              BarIcon("fileopen", KIcon::SizeSmall) );
  fileSelConfigPage = new KFSConfigPage( page, "file selector config page",
                                         mainWindow->fileselector );
  connect( fileSelConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );

  path.clear();
  path << i18n("Application") << i18n("Plugins");
  /*QVBox **/page=addVBoxPage(path,i18n("Plugin Manager"),
                          BarIcon("connect_established",KIcon::SizeSmall));
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );

  // editor widgets from kwrite/kwdialog
  path.clear();
  path << i18n("Editor");
  setFolderIcon (path, SmallIcon("edit", KIcon::SizeSmall));

  for (uint i = 0; i < KTextEditor::configInterfaceExtension (v->document())->configPages (); i++)
  {
    path.clear();
    path << i18n("Editor") << KTextEditor::configInterfaceExtension (v->document())->configPageName (i);
    /*QVBox **/page = addVBoxPage(path, KTextEditor::configInterfaceExtension (v->document())->configPageFullName (i),
                              KTextEditor::configInterfaceExtension (v->document())->configPagePixmap(i, KIcon::SizeSmall) );

    KTextEditor::ConfigPage *cPage = KTextEditor::configInterfaceExtension (v->document())->configPage(i, page);
    connect( cPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    editorPages.append (cPage);
  }

  #if 0
  path.clear();
  path << i18n("Plugins");
  setFolderIcon (path, SmallIcon("kate", KIcon::SizeSmall));
  #endif
  for (uint i=0; i<pluginManager->pluginList().count(); i++)
  {
    if  ( pluginManager->pluginList().at(i)->load && Kate::pluginConfigInterfaceExtension(pluginManager->pluginList().at(i)->plugin) )
      addPluginPage (pluginManager->pluginList().at(i)->plugin);
  }

  enableButtonSeparator(true);
  dataChanged = false;
  enableButton(Apply, false);
  unfoldTreeList ();
}

KateConfigDialog::~KateConfigDialog()
{
}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigInterfaceExtension(plugin))
    return;

  for (uint i=0; i<Kate::pluginConfigInterfaceExtension(plugin)->configPages(); i++)
  {
    QStringList path;
    path.clear();
    path << i18n("Application")<<i18n("Plugins") << Kate::pluginConfigInterfaceExtension(plugin)->configPageName(i);
    QVBox *page=addVBoxPage(path, Kate::pluginConfigInterfaceExtension(plugin)->configPageFullName(i), Kate::pluginConfigInterfaceExtension(plugin)->configPagePixmap(i, KIcon::SizeSmall));

    PluginPageListItem *info=new PluginPageListItem;
    info->plugin = plugin;
    info->page = Kate::pluginConfigInterfaceExtension(plugin)->configPage (i, page);
    pluginPages.append(info);
  }
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
   if (!Kate::pluginConfigInterfaceExtension(plugin))
    return;

  for (uint i=0; i<pluginPages.count(); i++)
  {
    if  ( pluginPages.at(i)->plugin == plugin )
    {
      QWidget *w = pluginPages.at(i)->page->parentWidget();
      delete pluginPages.at(i)->page;
      delete w;
      pluginPages.remove(pluginPages.at(i));
    }
  }
}

void KateConfigDialog::slotOk()
{
  if( dataChanged )
    slotApply();
  accept();
}

void KateConfigDialog::slotApply()
{
  viewManager->setUseOpaqueResize(cb_opaqueResize->isChecked());
  config->setGroup("KDE");
  config->writeEntry("MultipleInstances",cb_singleInstance->isChecked());
  config->setGroup("General");
  config->writeEntry("reopen at startup", cb_reopenFiles->isChecked());

  if (((config->readEntry("viewMode",DEFAULT_STYLE)=="Modern")? 0 : 1)!=cb_mode->currentItem())
  {
    config->writeEntry("viewMode",(cb_mode->currentItem()==0)?"Modern":"Classic");
    config->writeEntry("deleteKDockWidgetConfig",true);
  }

  mainWindow->syncKonsole = cb_syncKonsole->isChecked();

  mainWindow->filelist->setSortType(cb_sortFiles->isChecked() ? KateFileList::sortByName : KateFileList::sortByID);

  //config->setGroup("General");
  config->writeEntry("restore views", cb_restoreVC->isChecked());

  config->writeEntry( "Number of recent files", sb_numRecentFiles->value() );
  mainWindow->fileOpenRecent->setMaxItems( sb_numRecentFiles->value() );

  fileSelConfigPage->apply();

  for (uint i=0; i<editorPages.count(); i++)
  {
    editorPages.at(i)->apply();
  }

  v->getDoc()->writeConfig();

  viewManager->setShowFullPath( cb_fullPath->isChecked() ); // hm, stored 2 places :(
  config->writeEntry( "Show Full Path in Title", cb_fullPath->isChecked() );
  config->sync();

  for (uint i=0; i<pluginPages.count(); i++)
  {
    pluginPages.at(i)->page->apply();
  }
  dataChanged = false;
  enableButton(Apply, false);
}

void KateConfigDialog::slotChanged()
{
  dataChanged = true;
  enableButton(Apply, true);
}
