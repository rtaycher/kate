/***************************************************************************
                          katemainwindow.cpp  -  description
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

#include <qabstractlayout.h>
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
#include <kstandarddirs.h>
#include <kwin.h>

KateConfigDialog::KateConfigDialog (KateMainWindow *parent, const char *name)
 : KDialogBase (KDialogBase::TreeList, i18n("Configure Kate"), KDialogBase::Ok|KDialogBase::Apply|KDialogBase::Cancel, KDialogBase::Ok, parent, name)
{
  config = parent->config;
  docManager = parent->docManager;
  viewManager = parent->viewManager;
  pluginManager = parent->pluginManager;
  mainWindow = parent;

  setMinimumSize(600,400);

  v = viewManager->activeView();

  if (!v) return;

  pluginPages.setAutoDelete (false);

  KWin kwin;
  kwin.setIcons(winId(), kapp->icon(), kapp->miniIcon());

  QStringList path;

  setShowIconsInTreeList(true);

  path.clear();
  path << i18n("Kate");
  setFolderIcon (path, SmallIcon("kate", KIcon::SizeSmall));

  path.clear();
  path << i18n("Kate") << i18n("General");
  QFrame* frGeneral = addPage(path, i18n("General Options"), BarIcon("misc", KIcon::SizeSmall));
  QGridLayout* gridFrG = new QGridLayout(frGeneral);
  gridFrG->setSpacing( 6 );
  
  // How instances should be handled
  cb_singleInstance = new QCheckBox(frGeneral);
  cb_singleInstance->setText(i18n("Restrict to single instance"));
  gridFrG->addMultiCellWidget(cb_singleInstance,0,0,0,1);
  config->setGroup("startup");
  cb_singleInstance->setChecked(config->readBoolEntry("singleinstance",true));

  // sdi mode
  cb_sdi = new QCheckBox(frGeneral);
  cb_sdi->setText(i18n("SDI Mode enabled"));
  gridFrG->addMultiCellWidget(cb_sdi,1,1,0,1);
  config->setGroup("startup");
  cb_sdi->setChecked(config->readBoolEntry("sdi",false));

  // opaque resize of view splitters
  cb_opaqueResize = new QCheckBox( frGeneral );
  cb_opaqueResize->setText(i18n("Show &Content when resizing views"));
  gridFrG->addMultiCellWidget( cb_opaqueResize, 2, 2, 0, 1 );
  cb_opaqueResize->setChecked(viewManager->useOpaqueResize);
  QWhatsThis::add(cb_opaqueResize, i18n("If this is disabled, resizing views will display a <i>rubberband</i> to show the new sizes untill you release the mouse button."));

  // reopen files
  cb_reopenFiles = new QCheckBox( frGeneral );
  cb_reopenFiles->setText(i18n("Reopen &Files at startup"));
  gridFrG->addMultiCellWidget( cb_reopenFiles, 3, 3, 0, 1 );
  config->setGroup("open files");
  cb_reopenFiles->setChecked( config->readBoolEntry("reopen at startup", true) );
  QWhatsThis::add(cb_reopenFiles, i18n("If this is enabled Kate will attempt to reopen files that were open when you closed last time. Cursor position will be recovered if possible. Non-existent files will not be opened."));

  // restore view  config
  cb_restoreVC = new QCheckBox( frGeneral );
  cb_restoreVC->setText(i18n("Restore &View Configuration"));
  gridFrG->addMultiCellWidget( cb_restoreVC, 4, 4, 0, 1 );
  config->setGroup("General");
  cb_restoreVC->setChecked( config->readBoolEntry("restore views", false) );
  QWhatsThis::add(cb_restoreVC, i18n("Check this if you want all your views restored each time you open Kate"));

  // sync the konsole ?
  cb_syncKonsole = new QCheckBox(frGeneral);
  cb_syncKonsole->setText(i18n("Sync Konsole with active Document"));
  gridFrG->addMultiCellWidget(cb_syncKonsole,5,5,0,1);
  cb_syncKonsole->setChecked(parent->syncKonsole);

  //QSpacerItem *sp = new QSpacerItem(frGeneral);
  gridFrG->addMultiCell( new QSpacerItem(0,0), 6,6,0,1);
  gridFrG->setRowStretch(6,1);

  config->setGroup("General");

  // editor widgets from kwrite/kwdialog

  path.clear();
  path << i18n("Editor");
  setFolderIcon (path, SmallIcon("edit", KIcon::SizeSmall));

  // color options
  path.clear();
  path << i18n("Editor") << i18n("Colors");
  QVBox *page = addVBoxPage(path, i18n("Colors"),
                              BarIcon("colorize", KIcon::SizeSmall) );
  colorConfigPage = v->getDoc()->colorConfigPage(page);

  // font options
  path.clear();
  path << i18n("Editor") << i18n("Fonts");
  page = addVBoxPage(path, i18n("Fonts Settings"),
                              BarIcon("fonts", KIcon::SizeSmall) );
  fontConfigPage = v->getDoc()->fontConfigPage(page);

  // indent options
  path.clear();
  path << i18n("Editor") << i18n("Indent");
  page=addVBoxPage(path, i18n("Indent Options"),
                       BarIcon("rightjust", KIcon::SizeSmall) );
  indentConfigPage = v->getDoc()->indentConfigPage(page);

  // select options
  path.clear();
  path << i18n("Editor") << i18n("Select");
  page=addVBoxPage(path, i18n("Selection behavior"),
                       BarIcon("misc") );
  selectConfigPage = v->getDoc()->selectConfigPage(page);

  // edit options
  path.clear();
  path << i18n("Editor") << i18n("Edit");
  page=addVBoxPage(path, i18n("Editing Options"),
                       BarIcon("edit", KIcon::SizeSmall ) );
  editConfigPage = v->getDoc()->editConfigPage (page);
  
  path.clear();
  path << i18n("Editor") << i18n("Keyboard");
  page=addVBoxPage(path,i18n("Keyboard configuration"),
                        SmallIcon("edit", KIcon::SizeSmall));
  keysConfigPage = v->getDoc()->keysConfigPage (page);

  // spell checker
  path.clear();
  path << i18n("Editor") << i18n("Spelling");
  page = addVBoxPage( path, i18n("Spell checker behavior"),
                          BarIcon("spellcheck", KIcon::SizeSmall) );
  kSpellConfigPage = v->getDoc()->kSpellConfigPage (page);

  path.clear();
  path << i18n("Editor") << i18n("Highlighting");
  page=addVBoxPage(path,i18n("Highlighting configuration"),
                        SmallIcon("highlighting", KIcon::SizeSmall));
  hlConfigPage = v->getDoc()->hlConfigPage (page);

  path.clear();
  path << i18n("Plugins") << i18n("Manager");
  page=addVBoxPage(path,i18n("Configure plugins"),
                          BarIcon("misc",KIcon::SizeSmall));
  (void)new KateConfigPluginPage(page, this);

  for (uint i=0; i<pluginManager->myPluginList.count(); i++)
  {
    if  ( pluginManager->myPluginList.at(i)->load && pluginManager->myPluginList.at(i)->plugin->hasConfigPage() )
      addPluginPage (pluginManager->myPluginList.at(i)->plugin);
  }
}

KateConfigDialog::~KateConfigDialog()
{
}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin)
{
  if (!plugin->hasConfigPage()) return;

  QStringList path;
  path.clear();
  path << i18n("Plugins") << plugin->configPageName();
  QVBox *page=addVBoxPage(path, plugin->configPageTitle(), plugin->configPageIcon());

  PluginPageListItem *info=new PluginPageListItem;
  info->plugin = plugin;
  info->page = plugin->createConfigPage (page);
  pluginPages.append(info);
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
  if (!plugin->hasConfigPage()) return;

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

int KateConfigDialog::exec()
{
  int n = KDialogBase::exec();
  if (n)
    slotApply();

  return n;
}

void KateConfigDialog::slotApply()
{
  viewManager->setUseOpaqueResize(cb_opaqueResize->isChecked());
  config->setGroup("startup");
  config->writeEntry("singleinstance",cb_singleInstance->isChecked());
  config->writeEntry("sdi",cb_sdi->isChecked());
  config->setGroup("open files");
  config->writeEntry("reopen at startup", cb_reopenFiles->isChecked());

  mainWindow->syncKonsole = cb_syncKonsole->isChecked();

  config->setGroup("General");
  config->writeEntry("restore views", cb_restoreVC->isChecked());

  colorConfigPage->apply();
  fontConfigPage->apply();
  indentConfigPage->apply();
  selectConfigPage->apply();
  editConfigPage->apply();
  keysConfigPage->apply();
  kSpellConfigPage->apply();
  hlConfigPage->apply();

  v->getDoc()->writeConfig();
  v->getDoc()->readConfig();

  config->sync();

  // all docs need to reread config.
  QPtrListIterator<Kate::Document> dit (docManager->docList);
  for (; dit.current(); ++dit)
  {
    dit.current()->readConfig(  );
  }

  for (uint i=0; i<pluginPages.count(); i++)
  {
    pluginPages.at(i)->page->applyConfig();
  }
}
