/***************************************************************************
                          katemainwindow.cpp  -  description
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

#include "kateconfigdialog.h"
#include "kateconfigdialog.moc"

#include "katemainwindow.h"

#include "../sidebar/katesidebar.h"
#include "../console/kateconsole.h"
#include "../document/katedocument.h"
#include "../document/katedocmanager.h"
#include "../pluginmanager/katepluginmanager.h"
#include "../pluginmanager/kateconfigplugindialogpage.h"
#include "../view/kateviewmanager.h"
#include "../app/kateapp.h"
#include "../fileselector/katefileselector.h"
#include "../filelist/katefilelist.h"
#include "../factory/katefactory.h"

#include "../view/kateviewdialog.h"
#include "../document/katedialogs.h"
#include "../document/katehighlight.h"

#include <qcheckbox.h>
#include <qinputdialog.h>
#include <qlayout.h>
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
#include <kstddirs.h>

KateConfigDialog::KateConfigDialog (KateMainWindow *parent, const char *name)
 : KDialogBase (KDialogBase::TreeList, i18n("Configure Kate"), KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, parent, name)
{
  config = parent->config;
  docManager = parent->docManager;
  viewManager = parent->viewManager;
  pluginManager = parent->pluginManager;
  mainWindow = parent;

  v = viewManager->activeView();

  if (!v) return;

  QStringList path;

  path.clear();
  path << i18n("App") << i18n("General");
  QFrame* frGeneral = addPage(path, i18n("General Options"), BarIcon("misc", KIcon::SizeMedium));
  QGridLayout* gridFrG = new QGridLayout(frGeneral);
  gridFrG->setSpacing( 6 );

  // opaque resize of view splitters
  cb_opaqueResize = new QCheckBox( frGeneral );
  cb_opaqueResize->setText(i18n("Show &Content when resizing views"));
  gridFrG->addMultiCellWidget( cb_opaqueResize, 0, 0, 0, 1 );
  cb_opaqueResize->setChecked(viewManager->useOpaqueResize);
  QWhatsThis::add(cb_opaqueResize, i18n("If this is disabled, resizing views will display a <i>rubberband</i> to show the new sizes untill you release the mouse button."));

  // reopen files
  cb_reopenFiles = new QCheckBox( frGeneral );
  cb_reopenFiles->setText(i18n("Reopen &Files at startup"));
  gridFrG->addMultiCellWidget( cb_reopenFiles, 1, 1, 0, 1 );
  config->setGroup("open files");
  cb_reopenFiles->setChecked( config->readBoolEntry("reopen at startup", true) );
  QWhatsThis::add(cb_reopenFiles, i18n("If this is enabled Kate will attempt to reopen files that was open when you closed last time. Cursor position will be recovered if possible. Non-existing files will not be opened."));

  // restore view  config
  cb_restoreVC = new QCheckBox( frGeneral );
  cb_restoreVC->setText(i18n("Restore &View Configuration"));
  gridFrG->addMultiCellWidget( cb_restoreVC, 2, 2, 0, 1 );
  config->setGroup("General");
  cb_restoreVC->setChecked( config->readBoolEntry("restore views", false) );
  QWhatsThis::add(cb_restoreVC, i18n("Check this if you want all your views restored each time you open Kate"));


  // How instances should be handled
  cb_singleInstance = new QCheckBox(frGeneral);
  cb_singleInstance->setText(i18n("Restrict to single instance"));
  gridFrG->addMultiCellWidget(cb_singleInstance,3,3,0,1);
  config->setGroup("startup");
  cb_singleInstance->setChecked(config->readBoolEntry("singleinstance",true));

  // FileSidebar style
  cb_fileSidebarStyle = new QCheckBox(frGeneral);
  cb_fileSidebarStyle->setText(i18n("Show Filebar in KOffice Workspace style"));
  gridFrG->addMultiCellWidget(cb_fileSidebarStyle,4,4,0,1);
  config->setGroup("Sidebar");
  cb_fileSidebarStyle->setChecked(config->readBoolEntry("KOWStyle",true));

  // sync the konsole ?
  cb_syncKonsole = new QCheckBox(frGeneral);
  cb_syncKonsole->setText(i18n("Sync Konsole with active Document"));
  gridFrG->addMultiCellWidget(cb_syncKonsole,5,5,0,1);
  cb_syncKonsole->setChecked(parent->syncKonsole);

  config->setGroup("General");

  // editor widgets from kwrite/kwdialog
  // color options
  path.clear();
  path << i18n("Editor") << i18n("Colors");
  QVBox *page = addVBoxPage(path, i18n("Colors"),
                              BarIcon("colorize", KIcon::SizeMedium) );
  colorConfig = new ColorConfig(page);
  // some kwrite tabs needs a kwrite as an arg!

  // indent options
  path.clear();
  path << i18n("Editor") << i18n("Indent");
  page=addVBoxPage(path, i18n("Indent Options"),
                       BarIcon("rightjust", KIcon::SizeMedium) );
  indentConfig = new IndentConfigTab(page, v);

  // select options
  path.clear();
  path << i18n("Editor") << i18n("Select");
  page=addVBoxPage(path, i18n("Selection behavior"),
                       BarIcon("misc") );
  selectConfig = new SelectConfigTab(page, v);

  // edit options
  path.clear();
  path << i18n("Editor") << i18n("Edit");
  page=addVBoxPage(path, i18n("Editing Options"),
                       BarIcon("edit", KIcon::SizeMedium ) );
  editConfig = new EditConfigTab(page, v);

  // spell checker
  path.clear();
  path << i18n("Editor") << i18n("Spelling");
  page = addVBoxPage( path, i18n("Spell checker behavior"),
                          BarIcon("spellcheck", KIcon::SizeMedium) );
  ksc = new KSpellConfig(page, 0L, v->ksConfig(), false );
  colors = v->getColors();
  colorConfig->setColors( colors );

  path.clear();
  path << i18n("App") << i18n("Plugins");
  page=addVBoxPage(path,i18n("Configure plugins"),
                          BarIcon("misc",KIcon::SizeMedium));
  (void)new KateConfigPluginPage(page);

  hlManager = HlManager::self();

  defaultStyleList.setAutoDelete(true);
  hlManager->getDefaults(defaultStyleList,defaultFont);

  hlDataList.setAutoDelete(true);
  //this gets the data from the KConfig object
  hlManager->getHlDataList(hlDataList);

  path.clear();
  path << i18n("Editor") << i18n("Highlighting");
  page=addVBoxPage(path,i18n("Highlighting configuration"),
                        SmallIcon("highlighting", KIcon::SizeMedium));
  hlPage = new HighlightDialogPage(hlManager, &defaultStyleList, &defaultFont, &hlDataList,
    0, page);
}

KateConfigDialog::~KateConfigDialog()
{
}

int KateConfigDialog::exec()
{
  if (!KDialogBase::exec()) return 0;

  viewManager->setUseOpaqueResize(cb_opaqueResize->isChecked());
  config->setGroup("startup");
  config->writeEntry("singleinstance",cb_singleInstance->isChecked());
  config->setGroup("open files");
  config->writeEntry("reopen at startup", cb_reopenFiles->isChecked());

  config->setGroup("Sidebar");
  config->writeEntry("KOWStyle",cb_fileSidebarStyle->isChecked());
  mainWindow->sidebar->setMode(cb_fileSidebarStyle->isChecked());

  mainWindow->syncKonsole = cb_syncKonsole->isChecked();

  config->setGroup("General");
  config->writeEntry("restore views", cb_restoreVC->isChecked());

  ksc->writeGlobalSettings();
  colorConfig->getColors( colors );
  config->setGroup("kwrite");
  v->writeConfig( config );
  v->doc()->writeConfig( config );
  v->applyColors();
  hlManager->setHlDataList(hlDataList);
  hlManager->setDefaults(defaultStyleList,defaultFont);
  hlPage->saveData();
  config->sync();

  // all docs need to reread config.

  QListIterator<KateDocument> dit (docManager->docList);
  for (; dit.current(); ++dit)
  {
    dit.current()->readConfig( config );
  }

  QListIterator<KateView> it (viewManager->viewList);
  for (; it.current(); ++it)
  {
    v = it.current();
    indentConfig->getData( v );
    selectConfig->getData( v );
    editConfig->getData( v );
  }

  // repeat some calls: kwrite has a bad design.
  config->setGroup("kwrite");
  v->writeConfig( config );
  v->doc()->writeConfig( config );
  hlPage->saveData();
  config->sync();

  return 1;
}
