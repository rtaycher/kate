/***************************************************************************
                          kateconfigdialog.cpp
                          Configuration dialog for Kate
                             -------------------
    begin                : Wed Jan 3 2001
    copyright            : (C) 2001 by Christoph Cullmann, 2001, 2002 by Anders Lund
    email                : cullmann@kde.org anders@alweb.dk
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
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qhbox.h>
#include <qinputdialog.h>
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

  // General page
  path << i18n("Kate") << i18n("General");
  QFrame* frGeneral = addPage(path, i18n("General Options"), BarIcon("misc", KIcon::SizeSmall));

  QVBoxLayout *lo = new QVBoxLayout( frGeneral );
  lo->setSpacing(KDialog::spacingHint());
  lo->setAutoAdd( true );
  config->setGroup("General");

  // sdi or mdi?
  QButtonGroup *bgMode = new QButtonGroup( 1, Qt::Horizontal, i18n("Application Mode"), frGeneral );
  bgMode->setRadioButtonExclusive( true );
  rb_modeMDI = new QRadioButton( i18n("Kate &MDI"), bgMode );
  rb_modeSDI = new QRadioButton( i18n("Kate &SDI"), bgMode );
  if ( config->readBoolEntry( "sdi", false ) )
    rb_modeSDI->setChecked( true );
  else
    rb_modeMDI->setChecked( true );
  QWhatsThis::add( bgMode, i18n(
        "<qt>Choose which interface you like best.<p><strong>Kate MDI</strong> (default):"
        "<br>All documents are kept within one main window, and you must choose the document "
        "to edit from the \"Document\" menu, or from the File List."
        "<p><strong>Kate SDI</strong>:<br>A Single Document Interface opens only one document "
        "in each window. The File List/File Selector will have it's own window by default. "
        "You can <code>ALT-TAB</code> your way to the desired document."
        "<p><strong>Note:</strong> You need to restart Kate for this setting to take effect.</qt>") );

  // GROUP with the one below: "At Startup"
  QButtonGroup *bgStartup = new QButtonGroup( 1, Qt::Horizontal, i18n("At Startup"), frGeneral );
  // reopen files
  cb_reopenFiles = new QCheckBox( bgStartup );
  cb_reopenFiles->setText(i18n("Reopen &files"));
  //config->setGroup("General");
  cb_reopenFiles->setChecked( config->readBoolEntry("reopen at startup", true) );
  QWhatsThis::add(cb_reopenFiles, i18n(
        "If this is enabled Kate will attempt to reopen files that were open when you closed "
        "last time. Cursor position will be recovered if possible. Non-existent files will "
        "not be opened."));

  //config->setGroup("General");
  // restore view  config
  cb_restoreVC = new QCheckBox( bgStartup );
  cb_restoreVC->setText(i18n("Restore &view configuration"));
  cb_restoreVC->setChecked( config->readBoolEntry("restore views", false) );
  QWhatsThis::add(cb_restoreVC, i18n(
        "Check this if you want all your views and frames restored each time you open Kate"));

  // How instances should be handled
  cb_singleInstance = new QCheckBox(frGeneral);
  cb_singleInstance->setText(i18n("Allow only one &instance of Kate"));
  config->setGroup("startup");
  cb_singleInstance->setChecked(config->readBoolEntry("singleinstance",true));
  QWhatsThis::add( cb_singleInstance, i18n(
        "If this is checked, you can only start one instance of Kate. If you try, the current "
        "instance will get the focus, and open any files you requested opened.") );
  
  // show full path in title
  config->setGroup("General");
  cb_fullPath = new QCheckBox( i18n("Show full &path in title"), frGeneral);
  cb_fullPath->setChecked( config->readBoolEntry("Show Full Path in Title", false ) );

  // opaque resize of view splitters
  cb_opaqueResize = new QCheckBox( frGeneral );
  cb_opaqueResize->setText(i18n("&Show content when resizing views"));
  cb_opaqueResize->setChecked( viewManager->useOpaqueResize );
  QWhatsThis::add( cb_opaqueResize, i18n(
        "If this is disabled, resizing views will display a <i>rubberband</i> to show the "
        "new sizes untill you release the mouse button.") );

  // sync the konsole ?
  cb_syncKonsole = new QCheckBox(frGeneral);
  cb_syncKonsole->setText(i18n("Sync &terminal emulator with active document"));
  cb_syncKonsole->setChecked(parent->syncKonsole);
  QWhatsThis::add( cb_syncKonsole, i18n(
        "If this is checked, the built in Konsole will <code>cd</code> to the directory "
        "of the active document when started and whenever the active document changes, "
        "if the document is a local file.") );

  // number of recent files
  QHBox *hbNrf = new QHBox( frGeneral );
  QLabel *lNrf = new QLabel( i18n("&Number of recent files"), hbNrf );
  sb_numRecentFiles = new QSpinBox( 0, 1000, 1, hbNrf );
  sb_numRecentFiles->setValue( mainWindow->fileOpenRecent->maxItems() );
  lNrf->setBuddy( sb_numRecentFiles );
  QString youwouldnotbelieveit ( i18n(
        "<qt>Sets the number of recent files remembered by Kate.<p><strong>NOTE: </strong>"
        "If you set this lower than the current value, the list will be truncated and "
        "some items forgotten.</qt>") );
  QWhatsThis::add( lNrf, youwouldnotbelieveit );
  QWhatsThis::add( sb_numRecentFiles, youwouldnotbelieveit );

  // FIXME - TrollTech? BORKED!!!!!!!! gets added to TOP :(((((((((((((((((
  //lo->addItem( new QSpacerItem( 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding ) );
  QWidget *qLayoutSeemsBroken = new QWidget( frGeneral, "a working spacer" );
  lo->setStretchFactor( qLayoutSeemsBroken, 100 );

  // END General page
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
  config->writeEntry("sdi",/*cb_sdi->isChecked()*/rb_modeSDI->isChecked() );
  config->setGroup("General");
  config->writeEntry("reopen at startup", cb_reopenFiles->isChecked());

  mainWindow->syncKonsole = cb_syncKonsole->isChecked();

  //config->setGroup("General");
  config->writeEntry("restore views", cb_restoreVC->isChecked());

  config->writeEntry( "Number of recent files", sb_numRecentFiles->value() );
  mainWindow->fileOpenRecent->setMaxItems( sb_numRecentFiles->value() );


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

  viewManager->setShowFullPath( cb_fullPath->isChecked() ); // hm, stored 2 places :(
  config->writeEntry( "Show Full Path in Title", cb_fullPath->isChecked() );
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
