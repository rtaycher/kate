/* This file is part of the KDE libraries
   Copyright (C) 2001-2005 Christoph Cullmann <cullmann@kde.org>

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

#include "kateglobal.h"
#include "kateglobal.moc"

#include "katedocument.h"
#include "kateview.h"
#include "katerenderer.h"
#include "katecmds.h"
#include "katefiletype.h"
#include "kateschema.h"
#include "katesearch.h"
#include "kateconfig.h"
#ifndef Q_WS_WIN //todo
#include "katejscript.h"
#endif
#include "katecmd.h"
#include "katecmdactionmanager.h"
#include "katebuffer.h"

#include <kvmallocator.h>
#include <klocale.h>
#include <kservicetypetrader.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kpagedialog.h>
#include <kpagewidgetmodel.h>
#include <kwin.h>
#include <kiconloader.h>

#include <QVBoxLayout>

KateGlobal *KateGlobal::s_self = 0;

int KateGlobal::s_ref = 0;

KateGlobal::KateGlobal ()
 : KTextEditor::Editor (0)
 , m_aboutData ("katepart", I18N_NOOP("Kate Part"), KATEPART_VERSION,
             I18N_NOOP( "Embeddable editor component" ), KAboutData::License_LGPL_V2,
             I18N_NOOP( "(c) 2000-2005 The Kate Authors" ), 0, "http://kate.kde.org")
 , m_instance (&m_aboutData)
 , m_plugins (KServiceTypeTrader::self()->query("KTextEditor/Plugin"))
{
  // set s_self
  s_self = this;

  //
  // fill about data
  //
  m_aboutData.addAuthor ("Christoph Cullmann", I18N_NOOP("Maintainer"), "cullmann@kde.org", "http://www.babylon2k.de");
  m_aboutData.addAuthor ("Anders Lund", I18N_NOOP("Core Developer"), "anders@alweb.dk", "http://www.alweb.dk");
  m_aboutData.addAuthor ("Joseph Wenninger", I18N_NOOP("Core Developer"), "jowenn@kde.org","http://stud3.tuwien.ac.at/~e9925371");
  m_aboutData.addAuthor ("Hamish Rodda",I18N_NOOP("Core Developer"), "rodda@kde.org");
  m_aboutData.addAuthor ("Waldo Bastian", I18N_NOOP( "The cool buffersystem" ), "bastian@kde.org" );
  m_aboutData.addAuthor ("Charles Samuels", I18N_NOOP("The Editing Commands"), "charles@kde.org");
  m_aboutData.addAuthor ("Matt Newell", I18N_NOOP("Testing, ..."), "newellm@proaxis.com");
  m_aboutData.addAuthor ("Michael Bartl", I18N_NOOP("Former Core Developer"), "michael.bartl1@chello.at");
  m_aboutData.addAuthor ("Michael McCallum", I18N_NOOP("Core Developer"), "gholam@xtra.co.nz");
  m_aboutData.addAuthor ("Jochen Wilhemly", I18N_NOOP( "KWrite Author" ), "digisnap@cs.tu-berlin.de" );
  m_aboutData.addAuthor ("Michael Koch",I18N_NOOP("KWrite port to KParts"), "koch@kde.org");
  m_aboutData.addAuthor ("Christian Gebauer", 0, "gebauer@kde.org" );
  m_aboutData.addAuthor ("Simon Hausmann", 0, "hausmann@kde.org" );
  m_aboutData.addAuthor ("Glen Parker",I18N_NOOP("KWrite Undo History, Kspell integration"), "glenebob@nwlink.com");
  m_aboutData.addAuthor ("Scott Manson",I18N_NOOP("KWrite XML Syntax highlighting support"), "sdmanson@alltel.net");
  m_aboutData.addAuthor ("John Firebaugh",I18N_NOOP("Patches and more"), "jfirebaugh@kde.org");
  m_aboutData.addAuthor ("Dominik Haumann", I18N_NOOP("Developer & Highlight wizard"), "dhdev@gmx.de");
  m_aboutData.addAuthor ("Andreas Kling", I18N_NOOP("Developer"), "kling@impul.se");

  m_aboutData.addCredit ("Matteo Merli",I18N_NOOP("Highlighting for RPM Spec-Files, Perl, Diff and more"), "merlim@libero.it");
  m_aboutData.addCredit ("Rocky Scaletta",I18N_NOOP("Highlighting for VHDL"), "rocky@purdue.edu");
  m_aboutData.addCredit ("Yury Lebedev",I18N_NOOP("Highlighting for SQL"),"");
  m_aboutData.addCredit ("Chris Ross",I18N_NOOP("Highlighting for Ferite"),"");
  m_aboutData.addCredit ("Nick Roux",I18N_NOOP("Highlighting for ILERPG"),"");
  m_aboutData.addCredit ("Carsten Niehaus", I18N_NOOP("Highlighting for LaTeX"),"");
  m_aboutData.addCredit ("Per Wigren", I18N_NOOP("Highlighting for Makefiles, Python"),"");
  m_aboutData.addCredit ("Jan Fritz", I18N_NOOP("Highlighting for Python"),"");
  m_aboutData.addCredit ("Daniel Naber","","");
  m_aboutData.addCredit ("Roland Pabel",I18N_NOOP("Highlighting for Scheme"),"");
  m_aboutData.addCredit ("Cristi Dumitrescu",I18N_NOOP("PHP Keyword/Datatype list"),"");
  m_aboutData.addCredit ("Carsten Pfeiffer", I18N_NOOP("Very nice help"), "");
  m_aboutData.addCredit (I18N_NOOP("All people who have contributed and I have forgotten to mention"),"","");

  m_aboutData.setTranslator(I18N_NOOP2("NAME OF TRANSLATORS","Your names"), I18N_NOOP2("EMAIL OF TRANSLATORS","Your emails"));

  //
  // dir watch
  //
  m_dirWatch = new KDirWatch ();

  //
  // command manager
  //
  m_cmdManager = new KateCmd ();

  //
  // command action manager
  //
  m_cmdBindingManager = new KateCmdBindingManager();

  //
  // hl manager
  //
  m_hlManager = new KateHlManager ();

  //
  // filetype man
  //
  m_fileTypeManager = new KateFileTypeManager ();

  //
  // schema man
  //
  m_schemaManager = new KateSchemaManager ();

  // config objects
  m_documentConfig = new KateDocumentConfig ();
  m_viewConfig = new KateViewConfig ();
  m_rendererConfig = new KateRendererConfig ();

  // vm allocator
  m_vm = new KVMAllocator ();

  // create script man (search scripts) + register commands
  m_jscriptManager = new KateJScriptManager ();
  KateCmd::self()->registerCommand (m_jscriptManager);
  m_indentScriptManager = new KateIndentJScriptManager();

  //
  // init the cmds
  //
  m_cmds.push_back (new KateCommands::CoreCommands());
  m_cmds.push_back (new KateCommands::SedReplace ());
  m_cmds.push_back (new KateCommands::Character ());
  m_cmds.push_back (new KateCommands::Date ());
  m_cmds.push_back (new SearchCommand());

  for ( QList<KTextEditor::Command *>::iterator it = m_cmds.begin(); it != m_cmds.end(); ++it )
    m_cmdManager->registerCommand (*it);
}

KateGlobal::~KateGlobal()
{
  delete m_documentConfig;
  delete m_viewConfig;
  delete m_rendererConfig;

  delete m_fileTypeManager;
  delete m_schemaManager;

  delete m_dirWatch;

  delete m_vm;

  // you too
  qDeleteAll (m_cmds);

  // cu manager
  delete m_indentScriptManager;
  delete m_jscriptManager;

  delete m_hlManager;

  delete m_cmdBindingManager;

  delete m_cmdManager;

  s_self = 0;
}

KTextEditor::Document *KateGlobal::createDocument ( QObject *parent )
{
  KateDocument *doc = new KateDocument (false, false, false, 0, parent);

  emit documentCreated (this, doc);

  return doc;
}

const QList<KTextEditor::Document*> &KateGlobal::documents ()
{
  return m_docs;
}

//BEGIN KTextEditor::Editor config stuff
void KateGlobal::readConfig(KConfig *config)
{
  if( !config )
    config = KGlobal::config();

  config->setGroup("Kate Document Defaults");
  KateDocumentConfig::global()->readConfig (config);

  config->setGroup("Kate View Defaults");
  KateViewConfig::global()->readConfig (config);

  config->setGroup("Kate Renderer Defaults");
  KateRendererConfig::global()->readConfig (config);

  config->setGroup("Kate Command Bindings");
  cmdBindingManager()->readConfig(config);
}

void KateGlobal::writeConfig(KConfig *config)
{
  if( !config )
    config = KGlobal::config();

  config->setGroup("Kate Document Defaults");
  KateDocumentConfig::global()->writeConfig (config);

  config->setGroup("Kate View Defaults");
  KateViewConfig::global()->writeConfig (config);

  config->setGroup("Kate Renderer Defaults");
  KateRendererConfig::global()->writeConfig (config);

  config->setGroup("Kate Command Bindings");
  cmdBindingManager()->writeConfig(config);

  config->sync();
}
//END KTextEditor::Editor config stuff

bool KateGlobal::configDialogSupported () const
{
  return true;
}

void KateGlobal::configDialog(QWidget *parent)
{
  KPageDialog *kd = new KPageDialog(parent);
  kd->setCaption( i18n("Configure") );
  kd->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Help );
  kd->setFaceType( KPageDialog::List );

  QList<KTextEditor::ConfigPage*> editorPages;

  for (int i = 0; i < configPages (); ++i)
  {
    const QString name = configPageName (i);

    QFrame *page = new QFrame();

    KPageWidgetItem *item = kd->addPage( page, name );
    item->setHeader( configPageFullName (i) );
    //item->setIcon( KIcon( configPageIcon(i, K3Icon::SizeMedium) ) );

    QVBoxLayout *topLayout = new QVBoxLayout( page );
    topLayout->setMargin( 0 );
    topLayout->setSpacing( KDialog::spacingHint() );

    KTextEditor::ConfigPage *cp = configPage(i, page);
    topLayout->addWidget( cp);
    editorPages.append (cp);
  }

  if (kd->exec())
  {
    KateDocumentConfig::global()->configStart ();
    KateViewConfig::global()->configStart ();
    KateRendererConfig::global()->configStart ();

    for (int i=0; i < editorPages.count(); ++i)
    {
      editorPages.at(i)->apply();
    }

    KateDocumentConfig::global()->configEnd ();
    KateViewConfig::global()->configEnd ();
    KateRendererConfig::global()->configEnd ();

    writeConfig ();
  }

  delete kd;
}

int KateGlobal::configPages () const
{
  return 12;
}

KTextEditor::ConfigPage *KateGlobal::configPage (int number, QWidget *parent)
{
  switch( number )
  {
    case 0:
      return new KateViewDefaultsConfig (parent);

    case 1:
      return new KateSchemaConfigPage (parent, 0);

    case 2:
      return new KateSelectConfigTab (parent);

    case 3:
      return new KateEditConfigTab (parent);

    case 4:
      return new KateIndentConfigTab (parent);

    case 5:
      return new KateSaveConfigTab (parent);

    case 6:
      return new KateHlConfigPage (parent, 0);

    case 7:
      return new KateFileTypeConfigTab (parent);

    case 8:
      return new KateEditKeyConfiguration (parent, 0);

    case 9:
      return new KatePartPluginConfigPage (parent);

    case 10:
      return new KateScriptConfigPage (parent);

    case 11:
      return new KateCmdBindingConfigPage (parent);

    default:
      return 0;
  }

  return 0;
}

QString KateGlobal::configPageName (int number) const
{
  switch( number )
  {
    case 0:
      return i18n ("Appearance");

    case 1:
      return i18n ("Fonts & Colors");

    case 2:
      return i18n ("Cursor & Selection");

    case 3:
      return i18n ("Editing");

    case 4:
      return i18n ("Indentation");

    case 5:
      return i18n("Open/Save");

    case 6:
      return i18n ("Highlighting");

    case 7:
      return i18n("Filetypes");

    case 8:
      return i18n ("Shortcuts");

    case 9:
      return i18n ("Plugins");

    case 10:
      return i18n("Scripts");

    case 11:
      return i18n("Commands");

    default:
      return QString ("");
  }

  return QString ("");
}

QString KateGlobal::configPageFullName (int number) const
{
  switch( number )
  {
    case 0:
      return i18n("Appearance");

    case 1:
      return i18n ("Font & Color Schemas");

    case 2:
      return i18n ("Cursor & Selection Behavior");

    case 3:
      return i18n ("Editing Options");

    case 4:
      return i18n ("Indentation Rules");

    case 5:
      return i18n("File Opening & Saving");

    case 6:
      return i18n ("Highlighting Rules");

    case 7:
      return i18n("Filetype Specific Settings");

    case 8:
      return i18n ("Shortcuts Configuration");

    case 9:
      return i18n ("Plugin Manager");

    case 10:
      return i18n ("Script Manager");

    case 11:
      return i18n ("Command Menu Bindings");

    default:
      return QString ("");
  }

  return QString ("");
}

KIcon KateGlobal::configPageIcon (int number) const
{
  switch( number )
  {
    case 0:
      return KIcon("view_text");

    case 1:
      return KIcon("colorize");

    case 2:
        return KIcon("frame_edit");

    case 3:
      return KIcon("edit");

    case 4:
      return KIcon("rightjust");

    case 5:
      return KIcon("filesave");

    case 6:
      return KIcon("source");

    case 7:
      return KIcon("edit");

    case 8:
      return KIcon("key_enter");

    case 9:
      return KIcon("connect_established");

    case 10:
      return KIcon("edit");

    case 11:
      return KIcon("menu");

    default:
      return KIcon("edit");
  }

  return KIcon("edit");
}

KateGlobal *KateGlobal::self ()
{
  if (!s_self) {
    new KateGlobal ();
  }

  return s_self;
}

void KateGlobal::registerDocument ( KateDocument *doc )
{
  KateGlobal::incRef ();
  m_documents.append( doc );
  m_docs.append (doc);
}

void KateGlobal::deregisterDocument ( KateDocument *doc )
{
  m_docs.removeAll (doc);
  m_documents.removeAll( doc );
  KateGlobal::decRef ();
}

void KateGlobal::registerView ( KateView *view )
{
  KateGlobal::incRef ();
  m_views.append( view );
}

void KateGlobal::deregisterView ( KateView *view )
{
  m_views.removeAll( view );
  KateGlobal::decRef ();
}

//BEGIN command interface
bool KateGlobal::registerCommand (KTextEditor::Command *cmd)
{return m_cmdManager->registerCommand(cmd);}

bool KateGlobal::unregisterCommand (KTextEditor::Command *cmd)
{return m_cmdManager->unregisterCommand(cmd);}

KTextEditor::Command *KateGlobal::queryCommand (const QString &cmd) const
{return m_cmdManager->queryCommand(cmd);}

QList<KTextEditor::Command*> KateGlobal::commands() const
{return m_cmdManager->commands();}

QStringList KateGlobal::commandList() const
{return m_cmdManager->commandList();}
//END command interface


// kate: space-indent on; indent-width 2; replace-tabs on;
