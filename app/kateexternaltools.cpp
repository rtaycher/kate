/*
   This file is part of the Kate text editor of the KDE project.

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

   ---
   Copyright (C) 2004, Anders Lund <anders@alweb.dk>
*/
// TODO
// Icons
// Direct shortcut setting

#include "kateexternaltools.h"
#include "kateexternaltools.moc"
#include "katedocmanager.h"

#include "katemainwindow.h"

#include <kate/view.h>
#include <kate/document.h>

#include <klistbox.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmacroexpander.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <krun.h>
#include <kicondialog.h>

#include <qbitmap.h>
#include <qfile.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlistbox.h>
#include <qmap.h>
#include <qregexp.h>
#include <qwhatsthis.h>

#include <stdlib.h>
#include <unistd.h>

#include <kdebug.h>

//BEGIN KateExternalTool
KateExternalTool::KateExternalTool( const QString &name,
                      const QString &command,
                      const QString &icon,
                      const QString &tryexec,
                      const QStringList &mimetypes,
                      const QString &acname )
  : name ( name ),
    command ( command ),
    icon ( icon ),
    tryexec ( tryexec ),
    mimetypes ( mimetypes ),
    acname ( acname )
{
  //if ( ! tryexec.isEmpty() )
    hasexec = checkExec();
}

bool KateExternalTool::checkExec()
{
  // if tryexec is empty, it is the first word of command
  if ( tryexec.isEmpty() )
    tryexec = command.section( " ", 0, 0, QString::SectionSkipEmpty );

  // NOTE this code is modified taken from kdesktopfile.cpp, from KDesktopFile::tryExec()
  if (!tryexec.isEmpty()) {
    if (tryexec[0] == '/') {
      if (::access(QFile::encodeName(tryexec), R_OK | X_OK))
	return false;

      m_exec = tryexec;
    } else {
      // !!! Sergey A. Sukiyazov <corwin@micom.don.ru> !!!
      // Environment PATH may contain filenames in 8bit locale cpecified
      // encoding (Like a filenames).
      QStringList dirs = QStringList::split(':', QFile::decodeName(::getenv("PATH")));
      QStringList::Iterator it(dirs.begin());
      bool match = false;
      for (; it != dirs.end(); ++it)
      {
	QString fName = *it + "/" + tryexec;
	if (::access(QFile::encodeName(fName), R_OK | X_OK) == 0)
	{
	  match = true;
          m_exec = fName;
	  break;
	}
      }
      // didn't match at all
      if (!match)
        return false;
    }
    return true;
  }
  return false;
}

bool KateExternalTool::valid( QString mt ) const
{
  return mimetypes.isEmpty() || mimetypes.contains( mt );
}
//END KateExternalTool

//BEGIN KateExternalToolAction
KateExternalToolAction::KateExternalToolAction( QObject *parent,
             const char *name, KateExternalTool *t)
  : KAction( parent, name ),
    tool ( t )
{
  setText( t->name );
  if ( ! t->icon.isEmpty() )
    setIconSet( SmallIconSet( t->icon ) );

  connect( this ,SIGNAL(activated()), this, SLOT(slotRun()) );
}

void KateExternalToolAction::slotRun()
{
  // expand the macros in command if any,
  // and construct a command with an absolute path
  QString cmd = tool->command;

  KateMainWindow *mw = ((KateExternalToolsMenuAction*)parent()->parent())->mainwindow;
  Kate::View *view = mw->viewManager()->activeView();

  QRegExp re("[^%]%(\\w+)"); // never at the start of the string
  int pos = 0;
  QMap<QString, QString> m;
  while( (pos = re.search( cmd, pos )) > -1 )
  {
    if ( re.cap(1) == "URL" )
      m.insert( "URL", mw->activeDocumentUrl().url());
    else if ( re.cap(1) == "directory" ) // directory of current doc
      m.insert( "directory", mw->activeDocumentUrl().directory() );
    else if ( re.cap(1) == "filename" )
      m.insert( "filename", mw->activeDocumentUrl().filename());
    else if ( re.cap(1) == "line" ) // cursor line of current doc
      m.insert( "line", QString::number( view->cursorLine() ));
    else if ( re.cap(1) == "col" ) // cursor col of current doc
      m.insert( "col", QString::number( view->cursorColumn() ));
    else if ( re.cap(1) == "selection" ) // selection of current doc if any
      m.insert( "selection", view->getDoc()->selection());
    else if ( re.cap(1) == "text" ) // text of current doc
      m.insert( "text", view->getDoc()->text());

    pos += re.matchedLength();
  }

  cmd.replace( QRegExp("[^%]%URLs"), "%%URLs" );

  cmd = KMacroExpander::expandMacrosShellQuote( cmd, m );

  if ( cmd.contains("%URLs") )
  {
    QStringList l;
    for( Kate::Document *doc = mw->m_docManager->firstDocument(); doc; doc = mw->m_docManager->nextDocument() )
    {
      if ( ! doc->url().isEmpty() )
        l << doc->url().url();
    }
    QMap<QString, QStringList> m1;
    m1.insert( "URLs", l );

    cmd = KMacroExpander::expandMacrosShellQuote( cmd, m1 );
  }

  kdDebug()<<"=== Running command: "<<cmd<<endl;
  KRun::runCommand( cmd, tool->tryexec, tool->icon );
}
//END KateExternalToolAction

//BEGIN KateExternalToolsMenuAction
KateExternalToolsMenuAction::KateExternalToolsMenuAction( const QString &text,
                                               QObject *parent,
                                               const char* name,
                                               KateMainWindow *mw )
    : KActionMenu( text, parent, name ),
      mainwindow( mw )
{

  m_actionCollection = new KActionCollection( this );

//   connect(mw->m_docManager,SIGNAL(documentChanged()),this,SLOT(slotDocumentChanged()));

  reload();
}

void KateExternalToolsMenuAction::reload()
{
  m_actionCollection->clear();

  popupMenu()->clear();

  // load all the tools, and create a action for each of them
  KConfig *config = new KConfig("externaltools", false, false, "appdata");
  config->setGroup("Global");
  QStringList tools = config->readListEntry("tools");

//   QRegExp re_nw("\\W");

  for( QStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
    {
      popupMenu()->insertSeparator();
      // a separator
      continue;
    }

    config->setGroup( *it );

    KateExternalTool *t = new KateExternalTool(
        config->readEntry( "name", "" ),
        config->readEntry( "command", ""),
        config->readEntry( "icon", ""),
        config->readEntry( "executable", ""),
        config->readListEntry( "mimetypes" ),
        config->readEntry( "acname", "" ) );

    if ( t->hasexec )
      insert( new KateExternalToolAction( m_actionCollection, t->acname.ascii(), t ) );
  }

  m_actionCollection->readShortcutSettings( "Shortcuts", config );
}

void KateExternalToolsMenuAction::slotDocumentChanged()
{
//   // enable/disable to match current mime type
//   QString mt = mainwindow->m_docManager->activeDocument()->mimeType();
//
//   KActionPtrList actions = m_actionCollection->actions();
//   for (KActionPtrList::iterator it = actions.begin(); it != actions.end(); ++it )
//   {
//     if ( (KateExternalToolAction *a = dynamic_cast<KateExternalToolAction*>(*it)) )
//     {
//       a->setEnabled a->tool->mimetypes.contain( mt );
//     }
//   }
}
//END KateExternalToolsMenuAction

//BEGIN ToolItem
/**
 * This is a QListBoxItem, that has a KateExternalTool. The text is the Name
 * of the tool.
 */
class ToolItem : public QListBoxPixmap
{
  public:
    ToolItem( QListBox *lb, const QPixmap &icon, KateExternalTool *tool )
        : QListBoxPixmap( lb, icon, tool->name ),
          tool ( tool )
    {;}

    ~ToolItem() {};

    KateExternalTool *tool;
};
//END ToolItem

//BEGIN KateExternalToolServiceEditor
KateExternalToolServiceEditor::KateExternalToolServiceEditor( KateExternalTool *tool,
                                QWidget *parent, const char *name )
    : KDialogBase( parent, name, true, i18n("Edit external tool"), KDialogBase::Ok|KDialogBase::Cancel ),
      tool( tool )
{
    // create a entry for each property
    // fill in the values from the service if available
  QWidget *w = new QWidget( this );
  setMainWidget( w );
  QGridLayout *lo = new QGridLayout( w );
  lo->setSpacing( KDialogBase::spacingHint() );

  QLabel *l;

  leName = new QLineEdit( w );
  lo->addWidget( leName, 1, 2 );
  l = new QLabel( leName, i18n("&Name:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 1, 1 );
  if ( tool ) leName->setText( tool->name );
  QWhatsThis::add( leName, i18n(
      "The name will be displayed in the 'Tools->External' menu") );

  leCommand = new QLineEdit( w );
  lo->addWidget( leCommand, 2, 2 );
  l = new QLabel( leCommand, i18n("Co&mmand:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 2, 1 );
  if ( tool ) leCommand->setText( tool->command );
  QWhatsThis::add( leCommand, i18n(
      "<p>The command to execute to invoke the tool. The following macros "
      "will be expanded:</p>"
      "<ul><li><code>%URL</code> - the URL of the current document."
      "<li><code>%URLs</code> - a list of the URLs of all open documents."
      "<li><code>%directory</code> - the URL of the direcotry containing "
      "the current document."
      "<li><code>filename</code> - the filename of the current document."
      "<li><code>%line</code> - the current line of the text cursor in the "
      "current view."
      "<li><code>%column</code> - the column of the text cursor in the "
      "current view."
      "<li><code>%selection</code> - the selected text in the current view."
      "<li><code>%text</code> - the text of the current document.</ul>" ) );

  btnIcon = new KIconButton( w );
  btnIcon->setIconSize( KIcon::SizeMedium );
  lo->addMultiCellWidget( btnIcon, 1, 2, 3, 3 );
  if ( tool && !tool->icon.isEmpty() )
    btnIcon->setIcon( tool->icon );


  leExecutable = new QLineEdit( w );
  lo->addWidget( leExecutable, 3, 2 );
  l = new QLabel( leExecutable, i18n("&Executable:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 3, 1 );
  if ( tool ) leExecutable->setText( tool->tryexec );
  QWhatsThis::add( leExecutable, i18n(
      "The executable used by the command. This is used to check if a tool "
      "should be displayed. If not set, the first word of <em>command</em> "
      "will be used.") );

  leMimetypes = new QLineEdit( w );
  lo->addWidget( leMimetypes, 4, 2 );
  l = new QLabel( leMimetypes, i18n("&Mime Types:"), w );
  l->setAlignment( l->alignment()|Qt::AlignRight );
  lo->addWidget( l, 4, 1 );
  if ( tool ) leMimetypes->setText( tool->mimetypes.join("; ") );
  QWhatsThis::add( leMimetypes, i18n(
      "A semicolon separated list of mime types, for which this tool should "
      "be available. If this is left empty, the tool is allways available."
      "To choose from known mimetypes, press the button on the right.") );
}

void KateExternalToolServiceEditor::slotOk()
{
  if ( leName->text().isEmpty() ||
       leCommand->text().isEmpty() )
  {
    KMessageBox::information( this, i18n("You must specify at least a name and a command") );
    return;
  }

  KDialogBase::slotOk();
}
//END KateExternalToolServiceEditor

//BEGIN KateExternalToolsConfigWidget
KateExternalToolsConfigWidget::KateExternalToolsConfigWidget( QWidget *parent, const char* name )
  : Kate::ConfigPage( parent, name )
{
  QGridLayout *lo = new QGridLayout( this, 5, 5, 0, KDialog::spacingHint() );

  lbTools = new KListBox( this );
  lo->addMultiCellWidget( lbTools, 1, 4, 0, 3 );
  connect( lbTools, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()) );

  btnNew = new QPushButton( i18n("&New..."), this );
  lo->addWidget( btnNew, 5, 0 );
  connect( btnNew, SIGNAL(clicked()), this, SLOT(slotNew()) );

  btnRemove = new QPushButton( i18n("&Remove"), this );
  lo->addWidget( btnRemove, 5, 1 );
  connect( btnRemove, SIGNAL(clicked()), this, SLOT(slotRemove()) );

  btnEdit = new QPushButton( i18n("&Edit..."), this );
  lo->addWidget( btnEdit, 5, 2 );
  connect( btnEdit, SIGNAL(clicked()), this, SLOT(slotEdit()) );

  QPushButton *b = new QPushButton( i18n("Insert &Separator"), this );
  lo->addWidget( b, 5, 3 );
  connect( b, SIGNAL(clicked()), this, SLOT(slotInsertSeparator()) );

  btnMoveUp = new QPushButton( SmallIconSet("up"), "", this );
  lo->addWidget( btnMoveUp, 2, 4 );
  connect( btnMoveUp, SIGNAL(clicked()), this, SLOT(slotMoveUp()) );

  btnMoveDwn = new QPushButton( SmallIconSet("down"), "", this );
  lo->addWidget( btnMoveDwn, 3, 4 );
  connect( btnMoveDwn, SIGNAL(clicked()), this, SLOT(slotMoveDown()) );

  lo->setRowStretch( 1, 1 );
  lo->setRowStretch( 4, 1 );
  lo->setColStretch( 0, 1 );
  lo->setColStretch( 1, 1 );
  lo->setColStretch( 2, 1 );


  QWhatsThis::add( lbTools, i18n(
      "This list shows all the configured tools, represented by their menu text.") );



  reload();
  slotSelectionChanged();
}

void KateExternalToolsConfigWidget::reload()
{
  //m_tools.clear();
  lbTools->clear();

  // load the files from a KConfig
  KConfig *config = new KConfig("externaltools", false, false, "appdata");
  config->setGroup( "Global" );
  QStringList tools = config->readListEntry("tools");

  for( QStringList::Iterator it = tools.begin(); it != tools.end(); ++it )
  {
    if ( *it == "---" )
    {
      new QListBoxText( lbTools, "---" );
    }
    else
    {
      config->setGroup( *it );

      KateExternalTool *t = new KateExternalTool(
          config->readEntry( "name", "" ),
          config->readEntry( "command", ""),
          config->readEntry( "icon", ""),
          config->readEntry( "executable", ""),
          config->readListEntry( "mimetypes" ),
          config->readEntry( "acname" ) );

      if ( t->hasexec ) // we only show tools that are also in the menu.
        new ToolItem( lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t );
    }
  }


}

QPixmap KateExternalToolsConfigWidget::blankIcon()
{
  QPixmap pm( KIcon::SizeSmall, KIcon::SizeSmall );
  pm.fill();
  pm.setMask( pm.createHeuristicMask() );
  return pm;
}

void KateExternalToolsConfigWidget::apply()
{
  KConfig *config = new KConfig("externaltools", false, false, "appdata");
  // save a new list
  // save each item
  QStringList tools;
  for ( uint i = 0; i < lbTools->count(); i++ )
  {
    if ( lbTools->text( i ) == "---" )
    {
      tools << "---";
      continue;
    }
    KateExternalTool *t = ((ToolItem*)lbTools->item( i ))->tool;
    kdDebug()<<"adding tool: "<<t->name<<endl;
    tools << t->acname;

    config->setGroup( t->acname );
    config->writeEntry( "name", t->name );
    config->writeEntry( "command", t->command );
    config->writeEntry( "icon", t->icon );
    config->writeEntry( "executable", t->tryexec );
    config->writeEntry( "mimetypes", t->mimetypes );
    config->writeEntry( "acname", t->acname );
  }

  config->setGroup("Global");
  config->writeEntry( "tools", tools );

  config->sync();
}

void KateExternalToolsConfigWidget::slotSelectionChanged()
{
  // update button state
  bool hs =  lbTools->selectedItem() != 0;
  btnEdit->setEnabled( hs && static_cast<ToolItem*>(lbTools->selectedItem()) );
  btnRemove->setEnabled( hs );
  btnMoveUp->setEnabled( lbTools->currentItem() > 0 );
  btnMoveDwn->setEnabled( lbTools->currentItem() < (int)lbTools->count()-1 );
}

void KateExternalToolsConfigWidget::slotNew()
{
  // display a editor, and if it is OK'd, create a new tool and
  // create a listbox item for it
  KateExternalToolServiceEditor editor( 0, this );

  if ( editor.exec() )
  {
    KateExternalTool *t = new KateExternalTool(
      editor.leName->text(),
      editor.leCommand->text(),
      editor.btnIcon->icon(),
      editor.leExecutable->text(),
      QStringList::split( QRegExp("\\s*;\\s*"), editor.leMimetypes->text() ) );

    // This is sticky, it does not change again, so that shortcuts sticks
    // TODO check for dups
    t->acname = "externaltool_" + QString(t->name).replace( QRegExp("\\W+"), "" );

    new ToolItem ( lbTools, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t );

    slotChanged();
  }
}

void KateExternalToolsConfigWidget::slotRemove()
{
  // just remove the current listbox item
  if ( lbTools->currentItem() > -1 ) {
    lbTools->removeItem( lbTools->currentItem() );
    slotChanged();
  }
}

void KateExternalToolsConfigWidget::slotEdit()
{
  // show the item in an editor
  KateExternalTool *t = ((ToolItem*)lbTools->selectedItem())->tool;
  KateExternalToolServiceEditor editor( t, this);
  if ( editor.exec() /*== KDialogBase::Ok*/ )
  {

    bool iconChanged = ( editor.btnIcon->icon() != t->icon );

    t->name = editor.leName->text();
    t->command = editor.leCommand->text();
    t->icon = editor.btnIcon->icon();
    t->tryexec = editor.leExecutable->text();
    t->mimetypes = QStringList::split( QRegExp("\\s*;\\s*"), editor.leMimetypes->text() );

    //if the icon has changed, I have to renew the listbox item :S
    if ( iconChanged )
    {
      int idx = lbTools->index( lbTools->selectedItem() );
      lbTools->removeItem( idx );
      lbTools->insertItem( new ToolItem( 0, t->icon.isEmpty() ? blankIcon() : SmallIcon( t->icon ), t ), idx );
    }

    slotChanged();
  }
}

void KateExternalToolsConfigWidget::slotInsertSeparator()
{
  lbTools->insertItem( "---", lbTools->currentItem()+1 );
}

void KateExternalToolsConfigWidget::slotMoveUp()
{
  // move the current item in the listbox upwards if possible
  QListBoxItem *item = lbTools->selectedItem();
  if ( ! item ) return;

  int idx = lbTools->index( item );

  if ( idx < 1 ) return;

  if ( dynamic_cast<ToolItem*>(item) )
  {
    KateExternalTool *tool = ((ToolItem*)item)->tool;
    lbTools->removeItem( idx );
    lbTools->insertItem( new ToolItem( 0, tool->icon.isEmpty() ? blankIcon() : SmallIcon( tool->icon ), tool ), idx-1 );
  }
  else // a separator!
  {
    lbTools->removeItem( idx );
    lbTools->insertItem( new QListBoxText( 0, "---" ), idx-1 );
  }

  lbTools->setCurrentItem( idx - 1 );
  slotSelectionChanged();
}

void KateExternalToolsConfigWidget::slotMoveDown()
{
  // move the current item in the listbox downwards if possible
  QListBoxItem *item = lbTools->selectedItem();
  if ( ! item ) return;

  uint idx = lbTools->index( item );

  if ( idx > lbTools->count()-1 ) return;

  if ( dynamic_cast<ToolItem*>(item) )
  {
    KateExternalTool *tool = ((ToolItem*)item)->tool;
    lbTools->removeItem( idx );
    lbTools->insertItem( new ToolItem( 0, tool->icon.isEmpty() ? blankIcon() : SmallIcon( tool->icon ), tool ), idx+1 );
  }
  else // a separator!
  {
    lbTools->removeItem( idx );
    lbTools->insertItem( new QListBoxText( 0, "---" ), idx+1 );
  }

  lbTools->setCurrentItem( idx+1 );
  slotSelectionChanged();
}
//END KateExternalToolsConfigWidget
