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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateconfigdialog.h"
#include "kateconfigdialog.moc"

#include "katemainwindow.h"

#include "katedocmanager.h"
#include "katepluginmanager.h"
#include "kateconfigplugindialogpage.h"
#include "kateviewmanager.h"
#include "kateapp.h"
//#include "katefileselector.h"
#include "katefilelist.h"
#include "kateexternaltools.h"
#include <kvbox.h>
#include <ktexteditor/configpage.h>

#include <kinstance.h>
#include <kdebug.h>
#include <kdialogbase.h>
#include <kglobalaccel.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klistbox.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kwin.h>
#include <kseparator.h>

#include <q3buttongroup.h>
#include <QCheckBox>
#include <QLabel>
#include <QLayout>
#include <kpushbutton.h>
#include <qradiobutton.h>
#include <QSpinBox>
#include <kvbox.h>
#include <QComboBox>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QFrame>
#include <ktexteditor/editorchooser.h>

KateConfigDialog::KateConfigDialog ( KateMainWindow *parent, KTextEditor::View *view )
 : KDialogBase ( KDialogBase::TreeList,
                 i18n("Configure"),
                 KDialogBase::Ok | KDialogBase::Apply|KDialogBase::Cancel | KDialogBase::Help,
                 KDialogBase::Ok,
                 parent,
                 "configdialog" )
{
  KConfig *config = KGlobal::config();

  actionButton( KDialogBase::Apply)->setEnabled( false );

  mainWindow = parent;

  setMinimumSize(600,400);

  v = view;

  QStringList path;

  setShowIconsInTreeList(true);

  path.clear();
  path << i18n("Application");
  setFolderIcon (path, SmallIcon("kate", K3Icon::SizeSmall));

  path.clear();

  //BEGIN General page
  path << i18n("Application") << i18n("General");
  QFrame* frGeneral = addPage(path, i18n("General Options"), BarIcon("gohome", K3Icon::SizeSmall));

  QVBoxLayout *lo = new QVBoxLayout( frGeneral );
  lo->setSpacing(KDialog::spacingHint());
  config->setGroup("General");

  // GROUP with the one below: "Appearance"
  Q3ButtonGroup *bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("&Appearance"), frGeneral );
  lo->addWidget( bgStartup );

  // show full path in title
  config->setGroup("General");
  cb_fullPath = new QCheckBox( i18n("&Show full path in title"), bgStartup);
  cb_fullPath->setChecked( mainWindow->viewManager()->getShowFullPath() );
  cb_fullPath->setWhatsThis(i18n("If this option is checked, the full document path will be shown in the window caption."));
  connect( cb_fullPath, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );


  // GROUP with the one below: "Behavior"
  bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("&Behavior"), frGeneral );
  lo->addWidget( bgStartup );

  // sync the konsole ?
  cb_syncKonsole = new QCheckBox(bgStartup);
  cb_syncKonsole->setText(i18n("Sync &terminal emulator with active document"));
  cb_syncKonsole->setChecked(parent->syncKonsole);
  cb_syncKonsole->setWhatsThis( i18n(
        "If this is checked, the built in Konsole will <code>cd</code> to the directory "
        "of the active document when started and whenever the active document changes, "
        "if the document is a local file.") );
  connect( cb_syncKonsole, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // modified files notification
  cb_modNotifications = new QCheckBox(
      i18n("Wa&rn about files modified by foreign processes"), bgStartup );
  cb_modNotifications->setChecked( parent->modNotification );
  cb_modNotifications->setWhatsThis( i18n(
      "If enabled, when Kate receives focus you will be asked what to do with "
      "files that have been modified on the hard disk. If not enabled, you will "
      "be asked what to do with a file that has been modified on the hard disk only "
      "when that file gains focus inside Kate.") );
  connect( cb_modNotifications, SIGNAL( toggled( bool ) ),
           this, SLOT( slotChanged() ) );

  // GROUP with the one below: "Meta-informations"
  bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Meta-Information"), frGeneral );
  lo->addWidget( bgStartup );

  // save meta infos
  cb_saveMetaInfos = new QCheckBox( bgStartup );
  cb_saveMetaInfos->setText(i18n("Keep &meta-information past sessions"));
  cb_saveMetaInfos->setChecked(KateDocManager::self()->getSaveMetaInfos());
  cb_saveMetaInfos->setWhatsThis( i18n(
        "Check this if you want document configuration like for example "
        "bookmarks to be saved past editor sessions. The configuration will be "
        "restored if the document has not changed when reopened."));
  connect( cb_saveMetaInfos, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  // meta infos days
  KHBox *hbDmf = new KHBox( bgStartup );
  hbDmf->setEnabled(KateDocManager::self()->getSaveMetaInfos());
  QLabel *lDmf = new QLabel( i18n("&Delete unused meta-information after:"), hbDmf );
  sb_daysMetaInfos = new QSpinBox( hbDmf );
  sb_daysMetaInfos->setMaximum( 180 );
  sb_daysMetaInfos->setSpecialValueText(i18n("(never)"));
  sb_daysMetaInfos->setSuffix(i18n(" day(s)"));
  sb_daysMetaInfos->setValue( KateDocManager::self()->getDaysMetaInfos() );
  lDmf->setBuddy( sb_daysMetaInfos );
  connect( cb_saveMetaInfos, SIGNAL( toggled( bool ) ), hbDmf, SLOT( setEnabled( bool ) ) );
  connect( sb_daysMetaInfos, SIGNAL( valueChanged ( int ) ), this, SLOT( slotChanged() ) );


  // editor component
  m_editorChooser=new KTextEditor::EditorChooser(frGeneral);
  m_editorChooser->readAppSetting();
  connect(m_editorChooser,SIGNAL(changed()),this,SLOT(slotChanged()));
  lo->addWidget(m_editorChooser);
  lo->addStretch(1); // :-] works correct without autoadd

  //END General page

  path.clear();

  //BEGIN Session page
  path << i18n("Application") << i18n("Sessions");
  QFrame* frSessions = addPage(path, i18n("Session Management"), BarIcon("history", K3Icon::SizeSmall));

  lo = new QVBoxLayout( frSessions );
  lo->setSpacing(KDialog::spacingHint());

  // GROUP with the one below: "Startup"
  bgStartup = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Elements of Sessions"), frSessions );
  lo->addWidget( bgStartup );

  // restore view  config
  cb_restoreVC = new QCheckBox( bgStartup );
  cb_restoreVC->setText(i18n("Include &window configuration"));
  config->setGroup("General");
  cb_restoreVC->setChecked( config->readEntry("Restore Window Configuration", QVariant(true)).toBool() );
  cb_restoreVC->setWhatsThis( i18n(
        "Check this if you want all your views and frames restored each time you open Kate"));
  connect( cb_restoreVC, SIGNAL( toggled( bool ) ), this, SLOT( slotChanged() ) );

  QRadioButton *rb1, *rb2, *rb3;

  sessions_start = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Behavior on Application Startup"), frSessions );
  lo->addWidget(sessions_start);

  sessions_start->setRadioButtonExclusive( true );
  sessions_start->insert( rb1=new QRadioButton( i18n("&Start new session"), sessions_start ), 0 );
  sessions_start->insert( rb2=new QRadioButton( i18n("&Load last-used session"), sessions_start ), 1 );
  sessions_start->insert( rb3=new QRadioButton( i18n("&Manually choose a session"), sessions_start ), 2 );

  config->setGroup("General");
  QString sesStart (config->readEntry ("Startup Session", "manual"));
  if (sesStart == "new")
    sessions_start->setButton (0);
  else if (sesStart == "last")
    sessions_start->setButton (1);
  else
    sessions_start->setButton (2);

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  sessions_exit = new Q3ButtonGroup( 1, Qt::Horizontal, i18n("Behavior on Application Exit or Session Switch"), frSessions );
  lo->addWidget(sessions_exit);

  sessions_exit->setRadioButtonExclusive( true );
  sessions_exit->insert( rb1=new QRadioButton( i18n("&Do not save session"), sessions_exit ), 0 );
  sessions_exit->insert( rb2=new QRadioButton( i18n("&Save session"), sessions_exit ), 1 );
  sessions_exit->insert( rb3=new QRadioButton( i18n("&Ask user"), sessions_exit ), 2 );

  config->setGroup("General");
  QString sesExit (config->readEntry ("Session Exit", "save"));
  if (sesExit == "discard")
    sessions_exit->setButton (0);
  else if (sesExit == "save")
    sessions_exit->setButton (1);
  else
    sessions_exit->setButton (2);

  connect(rb1, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb2, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));
  connect(rb3, SIGNAL(toggled(bool)), this, SLOT(slotChanged()));

  lo->addStretch(1); // :-] works correct without autoadd
  //END Session page

  path.clear();

  // file selector page
#if 0
  path << i18n("Application") << i18n("File Selector");

  KVBox *page = addVBoxPage( path, i18n("File Selector Settings"),
                              BarIcon("fileopen", K3Icon::SizeSmall) );
  fileSelConfigPage = new KFSConfigPage( page, "file selector config page",
                                         mainWindow->fileselector );
  connect( fileSelConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
  path.clear();
#endif
  KVBox *page;
  path << i18n("Application") << i18n("Document List");
  page = addVBoxPage( path, i18n("Document List Settings"),
  BarIcon("view_text", K3Icon::SizeSmall) );
  filelistConfigPage = new KFLConfigPage( page,
					  mainWindow->filelist );
  connect( filelistConfigPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
  path.clear();

  path << i18n("Application") << i18n("Plugins");
  /*QVBox **/page=addVBoxPage(path,i18n("Plugin Manager"),
                          BarIcon("connect_established",K3Icon::SizeSmall));
  KateConfigPluginPage *configPluginPage = new KateConfigPluginPage(page, this);
  connect( configPluginPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );

  // Tools->External Tools menu
  path.clear();
  path << i18n("Application") << i18n("External Tools");
  page = addVBoxPage( path, i18n("External Tools"),
      BarIcon("configure", K3Icon::SizeSmall) );
  configExternalToolsPage = new KateExternalToolsConfigWidget(page, "external tools config page");
  connect( configExternalToolsPage, SIGNAL(changed()), this, SLOT(slotChanged()) );

  // editor widgets from kwrite/kwdialog
  path.clear();
  path << i18n("Editor");
  setFolderIcon (path, SmallIcon("edit", K3Icon::SizeSmall));

  for (int i = 0; i < KateDocManager::self()->editor()->configPages (); ++i)
  {
    path.clear();
    path << i18n("Editor") << KateDocManager::self()->editor()->configPageName (i);
    /*QVBox **/page = addVBoxPage(path, KateDocManager::self()->editor()->configPageFullName (i),
                              KateDocManager::self()->editor()->configPagePixmap(i, K3Icon::SizeSmall) );

    KTextEditor::ConfigPage *cPage = KateDocManager::self()->editor()->configPage(i, page);
    connect( cPage, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    editorPages.append (cPage);
  }

  KatePluginList &pluginList (KatePluginManager::self()->pluginList());
  foreach (const KatePluginInfo &plugin,pluginList)
  {
    if  ( plugin.load
          && Kate::pluginConfigPageInterface(plugin.plugin) )
      addPluginPage (plugin.plugin);
  }

  enableButtonSeparator(true);
  dataChanged = false;
  unfoldTreeList ();
}

KateConfigDialog::~KateConfigDialog()
{
}

void KateConfigDialog::addPluginPage (Kate::Plugin *plugin)
{
  if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (uint i=0; i<Kate::pluginConfigPageInterface(plugin)->configPages(); i++)
  {
    QStringList path;
    path.clear();
    path << i18n("Application")<<i18n("Plugins") << Kate::pluginConfigPageInterface(plugin)->configPageName(i);
    KVBox *page=addVBoxPage(path, Kate::pluginConfigPageInterface(plugin)->configPageFullName(i), Kate::pluginConfigPageInterface(plugin)->configPagePixmap(i, K3Icon::SizeSmall));

    PluginPageListItem *info=new PluginPageListItem;
    info->plugin = plugin;
    info->page = Kate::pluginConfigPageInterface(plugin)->configPage (i, page);
    connect( info->page, SIGNAL( changed() ), this, SLOT( slotChanged() ) );
    pluginPages.append(info);
  }
}

void KateConfigDialog::removePluginPage (Kate::Plugin *plugin)
{
   if (!Kate::pluginConfigPageInterface(plugin))
    return;

  for (int i=0; i<pluginPages.count(); i++)
  {
    if  ( pluginPages[i]->plugin == plugin )
    {
      QWidget *w = pluginPages[i]->page->parentWidget();
      delete pluginPages[i]->page;
      delete w;
      pluginPages.removeAt(i);
      i--;
    }
  }
}

void KateConfigDialog::slotOk()
{
  slotApply();
  accept();
}

void KateConfigDialog::slotApply()
{
  KConfig *config = KGlobal::config();

  // if data changed apply the kate app stuff
  if( dataChanged )
  {
    config->setGroup("General");

    config->writeEntry("Restore Window Configuration", cb_restoreVC->isChecked());

    int bu = sessions_start->id (sessions_start->selected());

    if (bu == 0)
      config->writeEntry ("Startup Session", "new");
    else if (bu == 1)
      config->writeEntry ("Startup Session", "last");
    else
      config->writeEntry ("Startup Session", "manual");

    bu = sessions_exit->id (sessions_exit->selected());

    if (bu == 0)
      config->writeEntry ("Session Exit", "discard");
    else if (bu == 1)
      config->writeEntry ("Session Exit", "save");
    else
      config->writeEntry ("Session Exit", "ask");


    m_editorChooser->writeAppSetting();

    config->writeEntry("Save Meta Infos", cb_saveMetaInfos->isChecked());
    KateDocManager::self()->setSaveMetaInfos(cb_saveMetaInfos->isChecked());

    config->writeEntry("Days Meta Infos", sb_daysMetaInfos->value() );
    KateDocManager::self()->setDaysMetaInfos(sb_daysMetaInfos->value());

    config->writeEntry("Modified Notification", cb_modNotifications->isChecked());
    mainWindow->modNotification = cb_modNotifications->isChecked();

    mainWindow->syncKonsole = cb_syncKonsole->isChecked();

    //fileSelConfigPage->apply();

    filelistConfigPage->apply();

    configExternalToolsPage->apply();
    KateExternalToolsCommand::self()->reload();
    for (int i=0; i < KateApp::self()->mainWindows(); i++)
    {
      KateMainWindow *win = KateApp::self()->mainWindow (i);
      win->externalTools->reload();
    }
    //mainWindow->externalTools->reload();

    mainWindow->viewManager()->setShowFullPath( cb_fullPath->isChecked() ); // hm, stored 2 places :(

    mainWindow->saveOptions ();

    // save plugin config !!
    KateApp::self()->pluginManager()->storeGeneralConfig (KGlobal::config());
  }

  //
  // editor config ! (the apply() methode will check the changed state internally)
  //
  for (int i=0; i<editorPages.count(); i++)
  {
    editorPages.at(i)->apply();
  }

  // write back the editor config
  KateDocManager::self()->editor()->writeConfig(config);

  //
  // plugins config ! (the apply() methode SHOULD check the changed state internally)
  //
  for (int i=0; i<pluginPages.count(); i++)
  {
    pluginPages[i]->page->apply();
  }

  config->sync();

  dataChanged = false;
  actionButton( KDialogBase::Apply)->setEnabled( false );
}

void KateConfigDialog::slotChanged()
{
  dataChanged = true;
  actionButton( KDialogBase::Apply)->setEnabled( true );
}
