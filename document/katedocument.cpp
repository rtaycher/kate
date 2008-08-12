/* This file is part of the KDE libraries
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>

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
   Boston, MA 02111-13020, USA.
*/

//BEGIN includes
#include "katedocument.h"
#include "katedocument.moc"
#include "katekeyinterceptorfunctor.h"
#include "kateglobal.h"
#include "katedialogs.h"
#include "katehighlight.h"
#include "kateview.h"
#include "kateautoindent.h"
#include "katetextline.h"
#include "katedocumenthelpers.h"
#include "kateprinter.h"
#include "katesmartcursor.h"
#include "katerenderer.h"
#include <ktexteditor/attribute.h>
#include "kateconfig.h"
#include "katemodemanager.h"
#include "kateschema.h"
#include "katetemplatehandler.h"
#include "katesmartmanager.h"
#include <ktexteditor/plugin.h>
#include <ktexteditor/loadsavefiltercheckplugin.h>
#include "kateedit.h"
#include "katebuffer.h"
#include "kateundo.h"
#include "katepartpluginmanager.h"

#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>
#include <kio/kfileitem.h>

#include <kparts/event.h>

#include <klocale.h>
#include <kglobal.h>
#include <kapplication.h>
#include <kmenu.h>
#include <kconfig.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstandardaction.h>
#include <kxmlguifactory.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <klibloader.h>
#include <kdirwatch.h>
#include <kencodingfiledialog.h>
#include <ktemporaryfile.h>
#include <kcodecs.h>
#include <kstandarddirs.h>

#include <services/kservicetypetrader.h>

#include <QtDBus/QtDBus>
#include <QtCore/QTimer>
#include <QtCore/QFile>
#include <QtGui/QClipboard>
#include <QtCore/QTextStream>
#include <QtCore/QTextCodec>
#include <QtCore/QMap>
#include <QtCore/QMutex>
//END  includes



// Turn debug messages on/off here
// #define FAST_DEBUG_ENABLE

#ifdef FAST_DEBUG_ENABLE
# define FAST_DEBUG(x) (kDebug( 13020 ) << x)
#else
# define FAST_DEBUG(x)
#endif



//BEGIN PRIVATE CLASSES
class KatePartPluginItem
{
  public:
    KTextEditor::Plugin *plugin;
};
//END PRIVATE CLASSES

static int dummy = 0;


class KateDocument::LoadSaveFilterCheckPlugins
{
  public:
    LoadSaveFilterCheckPlugins() {
      KService::List traderList = KServiceTypeTrader::self()->query("KTextEditor/LoadSaveFilterCheckPlugin");

      foreach(const KService::Ptr &ptr, traderList)
      {
        QString libname;
        libname=ptr->library();
        libname=libname.right(libname.length()-12); //ktexteditor_ == 12
        m_plugins[libname]=0;//new KatePythonEncodingCheck();
      }

    }
    ~LoadSaveFilterCheckPlugins() {
      if ( m_plugins.count()==0) return;
      QHashIterator<QString,KTextEditor::LoadSaveFilterCheckPlugin*>i(m_plugins);
        while (i.hasNext())
          i.next();
          delete i.value();
      m_plugins.clear();
    }
    bool preSavePostDialogFilterCheck(const QString& pluginName,KateDocument *document,QWidget *parentWidget) {
      KTextEditor::LoadSaveFilterCheckPlugin *plug=getPlugin(pluginName);
      if (!plug) {
        if (KMessageBox::warningContinueCancel (parentWidget
        , i18n ("The filter/check plugin '%1' could not be found, still continue saving of %2", pluginName,document->url().pathOrUrl())
        , i18n ("Saving problems")
        , KGuiItem(i18n("Save Nevertheless"))
        , KStandardGuiItem::cancel()) != KMessageBox::Continue)
          return false;
        else
          return true;
      }
      return plug->preSavePostDialogFilterCheck(document);
    }
    void postLoadFilter(const QString& pluginName,KateDocument *document) {
      KTextEditor::LoadSaveFilterCheckPlugin *plug=getPlugin(pluginName);
      if (!plug) return;
      plug->postLoadFilter(document);
    }
    bool postSaveFilterCheck(const QString& pluginName,KateDocument *document,bool created) {
      KTextEditor::LoadSaveFilterCheckPlugin *plug=getPlugin(pluginName);
      if (!plug) return false;
      return plug->postSaveFilterCheck(document,created);
    }
  private:
    KTextEditor::LoadSaveFilterCheckPlugin *getPlugin(const QString & pluginName)
    {
      if (!m_plugins.contains(pluginName)) return 0;
      if (!m_plugins[pluginName]) {
        m_plugins[pluginName]=KLibLoader::createInstance<KTextEditor::LoadSaveFilterCheckPlugin>( "ktexteditor_"+pluginName);
      }
      return m_plugins[pluginName];
    }
    QHash <QString,KTextEditor::LoadSaveFilterCheckPlugin*> m_plugins;
};

//BEGIN d'tor, c'tor
//
// KateDocument Constructor
//
KateDocument::KateDocument ( bool bSingleViewMode, bool bBrowserView,
                             bool bReadOnly, QWidget *parentWidget,
                             QObject *parent)
: KTextEditor::Document (parent),
  m_activeView(0L),
  m_undoDontMerge(false),
  m_undoIgnoreCancel(false),
  lastUndoGroupWhenSaved( 0 ),
  lastRedoGroupWhenSaved( 0 ),
  docWasSavedWhenUndoWasEmpty( true ),
  docWasSavedWhenRedoWasEmpty( true ),
  m_annotationModel( 0 ),
  m_indenter(this),
  m_modOnHd (false),
  m_modOnHdReason (OnDiskUnmodified),
  s_fileChangedDialogsActivated (false),
  m_tabInterceptor(0)
{
  setComponentData ( KateGlobal::self()->componentData () );

  m_undoComplexMerge=false;

  QString pathName ("/Kate/Document/%1");
  pathName = pathName.arg (++dummy);

  // my dbus object
  QDBusConnection::sessionBus().registerObject (pathName, this);

  // register doc at factory
  KateGlobal::self()->registerDocument(this);

  m_reloading = false;

  m_editHistory = new KateEditHistory(this);
  m_smartManager = new KateSmartManager(this);
  m_buffer = new KateBuffer(this);

  // init the config object, be careful not to use it
  // until the initial readConfig() call is done
  m_config = new KateDocumentConfig(this);

  // init some more vars !
  setActiveView(0L);

  hlSetByUser = false;
  m_fileTypeSetByUser = false;

  editSessionNumber = 0;
  editIsRunning = false;
  m_editCurrentUndo = 0L;
  editWithUndo = false;

  m_docNameNumber = 0;
  m_docName = "need init";

  m_bSingleViewMode = bSingleViewMode;
  m_bBrowserView = bBrowserView;
  m_bReadOnly = bReadOnly;

  setEditableMarks( markType01 );

  m_undoMergeTimer = new QTimer(this);
  m_undoMergeTimer->setSingleShot(true);
  connect(m_undoMergeTimer, SIGNAL(timeout()), SLOT(undoCancel()));

  clearMarks ();
  clearUndo ();
  clearRedo ();
  setModified (false);
  docWasSavedWhenUndoWasEmpty = true;

  // normal hl
  m_buffer->setHighlight (0);

  m_blockRemoveTrailingSpaces = false;
  m_extension = new KateBrowserExtension( this );

  // important, fill in the config into the indenter we use...
  m_indenter.updateConfig ();

  // some nice signals from the buffer
  connect(m_buffer, SIGNAL(tagLines(int,int)), this, SLOT(tagLines(int,int)));
  connect(m_buffer, SIGNAL(codeFoldingUpdated()),this,SIGNAL(codeFoldingUpdated()));

  // if the user changes the highlight with the dialog, notify the doc
  connect(KateHlManager::self(),SIGNAL(changed()),SLOT(internalHlChanged()));

  // signals for mod on hd
  connect( KateGlobal::self()->dirWatch(), SIGNAL(dirty (const QString &)),
           this, SLOT(slotModOnHdDirty (const QString &)) );

  connect( KateGlobal::self()->dirWatch(), SIGNAL(created (const QString &)),
           this, SLOT(slotModOnHdCreated (const QString &)) );

  connect( KateGlobal::self()->dirWatch(), SIGNAL(deleted (const QString &)),
           this, SLOT(slotModOnHdDeleted (const QString &)) );

  // update doc name
  setDocName ("");

  // if single view mode, like in the konqui embedding, create a default view ;)
  // be lazy, only create it now, if any parentWidget is given, otherwise widget()
  // will create it on demand...
  if ( m_bSingleViewMode && parentWidget )
  {
    KTextEditor::View *view = (KTextEditor::View*)createView( parentWidget );
    insertChildClient( view );
    view->show();
    setWidget( view );
  }

  connect(this,SIGNAL(sigQueryClose(bool *, bool*)),this,SLOT(slotQueryClose_save(bool *, bool*)));

  m_isasking = 0;

  // register document in plugins
  KatePartPluginManager::self()->addDocument(this);
}

//
// KateDocument Destructor
//
KateDocument::~KateDocument()
{
  // Tell the world that we're about to close (== destruct)
  // Apps must receive this in a direct signal-slot connection, and prevent
  // any further use of interfaces once they return.
  emit aboutToClose(this);

  // remove file from dirwatch
  deactivateDirWatch ();

  // thanks for offering, KPart, but we're already self-destructing
  setAutoDeleteWidget(false);
  setAutoDeletePart(false);

  // clean up remaining views
  while (!m_views.isEmpty()) {
    delete m_views.takeFirst();
  }

  delete m_editCurrentUndo;

  // cleanup the undo items, very important, truee :/
  qDeleteAll(undoItems);
  undoItems.clear();

  // de-register from plugin
  KatePartPluginManager::self()->removeDocument(this);

  // cu marks
  for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
    delete i.value();
  m_marks.clear();

  delete m_config;
  KateGlobal::self()->deregisterDocument (this);
}
//END

// on-demand view creation
QWidget *KateDocument::widget()
{
  // no singleViewMode -> no widget()...
  if (!singleViewMode())
    return 0;

  // does a widget exist already? use it!
  if (KTextEditor::Document::widget())
    return KTextEditor::Document::widget();

  // create and return one...
  KTextEditor::View *view = (KTextEditor::View*)createView(0);
  insertChildClient( view );
  setWidget( view );
  return view;
}

//BEGIN KTextEditor::Document stuff

KTextEditor::View *KateDocument::createView( QWidget *parent )
{
  KateView* newView = new KateView( this, parent);
  connect(newView, SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(undoCancel()));
  if ( s_fileChangedDialogsActivated )
    connect( newView, SIGNAL(focusIn( KTextEditor::View * )), this, SLOT(slotModifiedOnDisk()) );

  emit viewCreated (this, newView);

  return newView;
}

const QList<KTextEditor::View*> &KateDocument::views () const
{
  return m_textEditViews;
}

KTextEditor::Editor *KateDocument::editor ()
{
  return KateGlobal::self();
}

//BEGIN KTextEditor::EditInterface stuff

QString KateDocument::text() const
{
  QString s;

  for (int i = 0; i < m_buffer->count(); i++)
  {
    KateTextLine::Ptr textLine = m_buffer->plainLine(i);

    if (textLine)
    {
      s.append (textLine->string());

      if ((i+1) < m_buffer->count())
        s.append(QChar::fromAscii('\n'));
    }
  }

  return s;
}

QString KateDocument::text( const KTextEditor::Range& range, bool blockwise ) const
{
  if (!range.isValid()) {
    kWarning() << k_funcinfo << "Text requested for invalid range" << range;
    return QString();
  }

  if ( blockwise && (range.start().column() > range.end().column()) )
    return QString ();

  QString s;

  if (range.start().line() == range.end().line())
  {
    if (range.start().column() > range.end().column())
      return QString ();

    KateTextLine::Ptr textLine = m_buffer->plainLine(range.start().line());

    if ( !textLine )
      return QString ();

    return textLine->string(range.start().column(), range.end().column()-range.start().column());
  }
  else
  {

    for (int i = range.start().line(); (i <= range.end().line()) && (i < m_buffer->count()); ++i)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(i);

      if ( !blockwise )
      {
        if (i == range.start().line())
          s.append (textLine->string(range.start().column(), textLine->length()-range.start().column()));
        else if (i == range.end().line())
          s.append (textLine->string(0, range.end().column()));
        else
          s.append (textLine->string());
      }
      else
      {
        s.append( textLine->string( range.start().column(), range.end().column()-range.start().column()));
      }

      if ( i < range.end().line() )
        s.append(QChar::fromAscii('\n'));
    }
  }

  return s;
}

QChar KateDocument::character( const KTextEditor::Cursor & position ) const
{
  KateTextLine::Ptr textLine = m_buffer->plainLine(position.line());

  if ( !textLine )
    return QChar();

  if (position.column() >= 0 && position.column() < textLine->string().length())
    return textLine->string().at(position.column());

  return QChar();
}

QStringList KateDocument::textLines( const KTextEditor::Range & range, bool blockwise ) const
{
  QStringList ret;

  if (!range.isValid()) {
    kWarning() << k_funcinfo << "Text requested for invalid range" << range;
    return ret;
  }

  if ( blockwise && (range.start().column() > range.end().column()) )
    return ret;

  if (range.start().line() == range.end().line())
  {
    Q_ASSERT(range.start() <= range.end());

    KateTextLine::Ptr textLine = m_buffer->plainLine(range.start().line());

    if ( !textLine )
      return ret;

    ret << textLine->string(range.start().column(), range.end().column() - range.start().column());
  }
  else
  {
    for (int i = range.start().line(); (i <= range.end().line()) && (i < m_buffer->count()); ++i)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(i);

      if ( !blockwise )
      {
        if (i == range.start().line())
          ret << textLine->string(range.start().column(), textLine->length() - range.start().column());
        else if (i == range.end().line())
          ret << textLine->string(0, range.end().column());
        else
          ret << textLine->string();
      }
      else
      {
        ret << textLine->string(range.start().column(), range.end().column() - range.start().column());
      }
    }
  }

  return ret;
}

QString KateDocument::line( int line ) const
{
  KateTextLine::Ptr l = m_buffer->plainLine(line);

  if (!l)
    return QString();

  return l->string();
}

bool KateDocument::setText(const QString &s)
{
  if (!isReadWrite())
    return false;

  QList<KTextEditor::Mark> msave;

  foreach (KTextEditor::Mark* mark, m_marks)
    msave.append(*mark);

  editStart ();

  // delete the text
  clear();

  // insert the new text
  insertText (KTextEditor::Cursor(), s);

  editEnd ();

  foreach (const KTextEditor::Mark& mark, msave)
    setMark (mark.line, mark.type);

  return true;
}

bool KateDocument::setText( const QStringList & text )
{
  if (!isReadWrite())
    return false;

  QList<KTextEditor::Mark> msave;

  foreach (KTextEditor::Mark* mark, m_marks)
    msave.append(*mark);

  editStart ();

  // delete the text
  clear();

  // insert the new text
  insertText (KTextEditor::Cursor::start(), text);

  editEnd ();

  foreach (const KTextEditor::Mark& mark, msave)
    setMark (mark.line, mark.type);

  return true;
}

bool KateDocument::clear()
{
  if (!isReadWrite())
    return false;

  foreach (KateView *view, m_views) {
    view->clear();
    view->tagAll();
    view->update();
  }

  clearMarks ();

  return removeText (KTextEditor::Range(KTextEditor::Cursor(), KTextEditor::Cursor(lastLine()+1, 0)));
}

bool KateDocument::insertText( const KTextEditor::Cursor& position, const QString& text, bool block )
{
  if (!isReadWrite())
    return false;

  if (text.isEmpty())
    return true;

  editStart();

  int currentLine = position.line();
  int currentLineStart = 0;
  int totalLength = text.length();
  int insertColumn = position.column();

  if (position.line() > lines())
  {
    int line = lines();
    while (line != position.line() + totalLength + 1)
    {
      editInsertLine(line,"");
      line++;
    }
  }

  bool replacetabs = ( config()->configFlags() & KateDocumentConfig::cfReplaceTabsDyn );
  int tabWidth = config()->tabWidth();

  static const QChar newLineChar('\n');
  static const QChar tabChar('\t');
  static const QChar spaceChar(' ');

  int insertColumnExpanded = insertColumn;
  KateTextLine::Ptr l = m_buffer->line( currentLine );
  if (l)
    insertColumnExpanded = l->toVirtualColumn( insertColumn, tabWidth );

  int pos = 0;
  for (; pos < totalLength; pos++)
  {
    const QChar& ch = text.at(pos);

    if (ch == newLineChar)
    {
      // Only perform the text insert if there is text to insert
      if (currentLineStart < pos)
        editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart));

      if ( !block )
      {
        editWrapLine(currentLine, insertColumn + pos - currentLineStart);
        insertColumn = 0;
      }
      else
      {
        if ( currentLine == lastLine() )
          editWrapLine(currentLine , insertColumn + pos - currentLineStart);
        insertColumn = position.column(); // tab expansion might change this
      }

      currentLine++;
      currentLineStart = pos + 1;
      l = m_buffer->line( currentLine );
      if (l)
        insertColumnExpanded = l->toVirtualColumn( insertColumn, tabWidth );
    }
    else
    {
      if ( replacetabs && ch == tabChar )
      {
        int spacesRequired = tabWidth - ( (insertColumnExpanded + pos - currentLineStart) % tabWidth );
        editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart) + QString(spacesRequired, spaceChar));

        insertColumn += pos - currentLineStart + spacesRequired;
        currentLineStart = pos + 1;
        l = m_buffer->line( currentLine );
        if (l)
          insertColumnExpanded = l->toVirtualColumn( insertColumn, tabWidth );
      }
    }
  }

  // Only perform the text insert if there is text to insert
  if (currentLineStart < pos)
    editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart));

  editEnd();
  return true;
}

bool KateDocument::insertText( const KTextEditor::Cursor & position, const QStringList & textLines, bool block )
{
  if (!isReadWrite())
    return false;

  if (textLines.isEmpty() || (textLines.count() == 1 && textLines.first().isEmpty()))
    return true;

  // FIXME - huh, contradicted below
  if (position.line() > lines())
    return false;

  editStart();

  if (position.line() > lines())
    editInsertLine(position.line(),"");

  int currentLine = position.line();
  int currentLineStart = 0;
  int insertColumn = position.column();

  bool replacetabs = ( config()->configFlags() & KateDocumentConfig::cfReplaceTabsDyn );
  int tabWidth = config()->tabWidth();

  static const QChar newLineChar('\n');
  static const QChar tabChar('\t');
  static const QChar spaceChar(' ');

  int insertColumnExpanded = insertColumn;
  KateTextLine::Ptr l = m_buffer->line( currentLine );
  if (l)
    insertColumnExpanded = l->toVirtualColumn( insertColumn, tabWidth );

  foreach (const QString &text, textLines)
  {
    int pos = 0;
    for (; pos < text.length(); pos++)
    {
      const QChar& ch = text.at(pos);

      if (ch == newLineChar)
      {
        // Only perform the text insert if there is text to insert
        if (currentLineStart < pos)
          editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart));

        if ( !block )
        {
          editWrapLine(currentLine, pos + insertColumn);
          insertColumn = 0;
        }
        else
        {
          if ( currentLine == lastLine() )
            editWrapLine(currentLine , insertColumn + pos - currentLineStart);
          insertColumn = position.column(); // tab expansion might change this
        }

        currentLine++;
        currentLineStart = pos + 1;
        l = m_buffer->line( currentLine );
        if (l)
          insertColumnExpanded = l->toVirtualColumn( insertColumn, tabWidth );
      }
      else
      {
        if ( replacetabs && ch == tabChar )
        {
          int spacesRequired = tabWidth - ( (insertColumnExpanded + pos - currentLineStart) % tabWidth );
          editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart) + QString(spacesRequired, spaceChar));

          insertColumn += pos - currentLineStart + spacesRequired;
          l = m_buffer->line( currentLine );
          if (l)
            insertColumnExpanded = l->toVirtualColumn( insertColumn, tabWidth );
          currentLineStart = pos + 1;
        }
      }
    }

    // Only perform the text insert if there is text to insert
    if (currentLineStart < pos - currentLineStart)
      editInsertText(currentLine, insertColumn, text.mid(currentLineStart, pos - currentLineStart));
  }

  editEnd();
  return true;
}


bool KateDocument::removeText ( const KTextEditor::Range &_range, bool block )
{
  KTextEditor::Range range = _range;

  if (!isReadWrite())
    return false;

  if ( block && (range.start().column() > range.end().column()) )
    return false;

  // Should now be impossible to trigger with the new Range class
  Q_ASSERT( range.start().line() <= range.end().line() );

  if ( range.start().line() > lastLine() )
    return false;

  if (!block)
    emit aboutToRemoveText(range);

  editStart();

  if ( !block )
  {
    if ( range.end().line() > lastLine() )
    {
      range.end().setPosition(lastLine()+1, 0);
    }

    if (range.onSingleLine())
    {
      editRemoveText(range.start().line(), range.start().column(), range.columnWidth());
    }
    else if (range.start().line() + 1 == range.end().line())
    {
      if ( (m_buffer->plainLine(range.start().line())->length() - range.start().column()) > 0 )
        editRemoveText(range.start().line(), range.start().column(), m_buffer->plainLine(range.start().line())->length() - range.start().column());

      if (range.end().column())
        editRemoveText (range.start().line() + 1, 0, range.end().column());

      editUnWrapLine (range.start().line());
    }
    else
    {
      for (int line = range.end().line(); line >= range.start().line(); --line)
      {
        if ((line > range.start().line()) && (line < range.end().line())) {
          editRemoveLine(line);

        } else if (line == range.end().line()) {
          if ( range.end().line() <= lastLine() )
            editRemoveText(line, 0, range.end().column());

        } else {
          if ( (m_buffer->plainLine(line)->length() - range.start().column()) > 0 )
            editRemoveText(line, range.start().column(), m_buffer->plainLine(line)->length() - range.start().column());

          editUnWrapLine(range.start().line());
        }

        if ( line == 0 )
          break;
      }
    }
  } // if ( ! block )
  else
  {
    int startLine = qMax(0, range.start().line());
    for (int line = qMin(range.end().line(), lastLine()); line >= startLine; --line)
      editRemoveText(line, range.start().column(), range.end().column() - range.start().column());
  }

  editEnd ();
  emit textRemoved();
  return true;
}

bool KateDocument::insertLine( int l, const QString &str )
{
  if (!isReadWrite())
    return false;

  if (l < 0 || l > lines())
    return false;

  return editInsertLine (l, str);
}

bool KateDocument::insertLines( int line, const QStringList & text )
{
  if (!isReadWrite())
    return false;

  if (line < 0 || line > lines())
    return false;

  bool success = true;
  foreach (const QString &string, text)
    // FIXME assumes no \n in each string
    success &= editInsertLine (line++, string);

  return success;
}

bool KateDocument::removeLine( int line )
{
  if (!isReadWrite())
    return false;

  if (line < 0 || line > lastLine())
    return false;

  return editRemoveLine (line);
}

int KateDocument::totalCharacters() const
{
  int l = 0;

  for (int i = 0; i < m_buffer->count(); ++i)
  {
    KateTextLine::Ptr line = m_buffer->plainLine(i);

    if (line)
      l += line->length();
  }

  return l;
}

int KateDocument::lines() const
{
  return m_buffer->count();
}

int KateDocument::numVisLines() const
{
  return m_buffer->countVisible ();
}

int KateDocument::lineLength ( int line ) const
{
  if (line < 0 || line > lastLine())
    return -1;

  KateTextLine::Ptr l = m_buffer->plainLine(line);

  if (!l)
    return -1;

  return l->length();
}
//END

//BEGIN KTextEditor::EditInterface internal stuff
//
// Starts an edit session with (or without) undo, update of view disabled during session
//
void KateDocument::editStart (bool withUndo, Kate::EditSource editSource)
{
  editSessionNumber++;

  if (editSource == Kate::NoEditSource)
    m_editSources.push(m_editSources.isEmpty() ? Kate::UserInputEdit : m_editSources.top());
  else
    m_editSources.push(editSource);

  if (editSessionNumber > 1)
    return;

  // Unlocked in editEnd
  smartMutex()->lock();

  editIsRunning = true;
  editWithUndo = withUndo;

  if (editWithUndo)
    undoStart();
  else
    undoCancel();

  foreach(KateView *view,m_views)
  {
    view->editStart ();
  }

  m_buffer->editStart ();
}

void KateDocument::undoStart()
{
  if (m_editCurrentUndo || (m_activeView && activeKateView()->imComposeEvent())) return;

  // new current undo item
  m_editCurrentUndo = new KateUndoGroup(this);
  if (m_activeView) {
    m_editCurrentUndo->setUndoCursor(m_activeView->cursorPosition());
    m_editCurrentUndo->setUndoSelection(m_activeView->selectionRange());
  }
}

void KateDocument::undoEnd()
{
  if (m_activeView && activeKateView()->imComposeEvent())
    return;

  if (m_editCurrentUndo)
  {
    bool changedUndo = false;

    if (m_activeView) {
        m_editCurrentUndo->setRedoCursor(m_activeView->cursorPosition());
        m_editCurrentUndo->setRedoSelection(m_activeView->selectionRange());
    }

    if (m_editCurrentUndo->isEmpty())
      delete m_editCurrentUndo;
    else if (!m_undoDontMerge && !undoItems.isEmpty() && undoItems.last() && undoItems.last()->merge(m_editCurrentUndo,m_undoComplexMerge))
      delete m_editCurrentUndo;
    else
    {
      undoItems.append(m_editCurrentUndo);
      changedUndo = true;
    }

    m_undoDontMerge = false;
    m_undoIgnoreCancel = true;

    m_editCurrentUndo = 0L;

    // (Re)Start the single-shot timer to cancel the undo merge
    // the user has 5 seconds to input more data, or undo merging gets canceled for the current undo item.
    m_undoMergeTimer->start(5000);

    if (changedUndo)
      emit undoChanged();
  }
}

void KateDocument::undoCancel()
{
  // Don't worry about this when an edit is in progress
  if (editIsRunning)
    return;

  if (m_undoIgnoreCancel) {
    m_undoIgnoreCancel = false;
    return;
  }

  m_undoDontMerge = true;

  Q_ASSERT(!m_editCurrentUndo);

  // As you can see by the above assert, neither of these should really be required
  delete m_editCurrentUndo;
  m_editCurrentUndo = 0L;
}

void KateDocument::undoSafePoint() {
  Q_ASSERT(m_editCurrentUndo);
  if (!m_editCurrentUndo) return;
  m_editCurrentUndo->safePoint();
}

//
// End edit session and update Views
//
void KateDocument::editEnd ()
{
  if (editSessionNumber == 0)
    return;

  // wrap the new/changed text, if something really changed!
  if (m_buffer->editChanged() && (editSessionNumber == 1))
    if (editWithUndo && config()->wordWrap())
      wrapText (m_buffer->editTagStart(), m_buffer->editTagEnd());

  editSessionNumber--;

  m_editSources.pop();

  if (editSessionNumber > 0)
    return;

  // end buffer edit, will trigger hl update
  // this will cause some possible adjustment of tagline start/end
  m_buffer->editEnd ();

  if (editWithUndo)
    undoEnd();

  // edit end for all views !!!!!!!!!
  foreach(KateView *view, m_views)
    view->editEnd (m_buffer->editTagStart(), m_buffer->editTagEnd(), m_buffer->editTagFrom());

  // Was locked in editStart
  smartMutex()->unlock();

  if (m_buffer->editChanged())
  {
    setModified(true);
    emit textChanged (this);
  }

  editIsRunning = false;
}

void KateDocument::pushEditState ()
{
  editStateStack.push(editSessionNumber);
}

void KateDocument::popEditState ()
{
  if (editStateStack.isEmpty()) return;

  int count = editStateStack.pop() - editSessionNumber;
  while (count < 0) { ++count; editEnd(); }
  while (count > 0) { --count; editStart(); }
}

bool KateDocument::wrapText(int startLine, int endLine)
{
  if (startLine < 0 || endLine < 0)
    return false;

  if (!isReadWrite())
    return false;

  int col = config()->wordWrapAt();

  if (col == 0)
    return false;

  editStart ();

  for (int line = startLine; (line <= endLine) && (line < lines()); line++)
  {
    KateTextLine::Ptr l = m_buffer->line(line);

    if (!l)
      return false;

    kDebug (13020) << "try wrap line: " << line;

    if (l->virtualLength(m_buffer->tabWidth()) > col)
    {
      KateTextLine::Ptr nextl = m_buffer->line(line+1);

      kDebug (13020) << "do wrap line: " << line;

      int eolPosition = l->length()-1;

      // take tabs into account here, too
      int x = 0;
      const QString & t = l->string();
      int z2 = 0;
      for ( ; z2 < l->length(); z2++)
      {
        static const QChar tabChar('\t');
        if (t.at(z2) == tabChar)
          x += m_buffer->tabWidth() - (x % m_buffer->tabWidth());
        else
          x++;

        if (x > col)
          break;
      }

      int searchStart = qMin (z2, l->length()-1);

      // If where we are wrapping is an end of line and is a space we don't
      // want to wrap there
      if (searchStart == eolPosition && t.at(searchStart).isSpace())
        searchStart--;

      // Scan backwards looking for a place to break the line
      // We are not interested in breaking at the first char
      // of the line (if it is a space), but we are at the second
      // anders: if we can't find a space, try breaking on a word
      // boundary, using KateHighlight::canBreakAt().
      // This could be a priority (setting) in the hl/filetype/document
      int z = 0;
      int nw = 0; // alternative position, a non word character
      for (z=searchStart; z > 0; z--)
      {
        if (t.at(z).isSpace()) break;
        if ( ! nw && highlight()->canBreakAt( t.at(z) , l->attribute(z) ) )
        nw = z;
      }

      bool removeTrailingSpace = false;
      if (z > 0)
      {
        // So why don't we just remove the trailing space right away?
        // Well, the (view's) cursor may be directly in front of that space
        // (user typing text before the last word on the line), and if that
        // happens, the cursor would be moved to the next line, which is not
        // what we want (bug #106261)
        z++;
        removeTrailingSpace = true;
      }
      else
      {
        // There was no space to break at so break at a nonword character if
        // found, or at the wrapcolumn ( that needs be configurable )
        // Don't try and add any white space for the break
        if ( nw && nw < col ) nw++; // break on the right side of the character
        z = nw ? nw : col;
      }

      if (nextl && !nextl->isAutoWrapped())
      {
        editWrapLine (line, z, true);
        editMarkLineAutoWrapped (line+1, true);

        endLine++;
      }
      else
      {
        if (nextl && (nextl->length() > 0) && !nextl->at(0).isSpace() && ((l->length() < 1) || !l->at(l->length()-1).isSpace()))
          editInsertText (line+1, 0, QString (" "));

        bool newLineAdded = false;
        editWrapLine (line, z, false, &newLineAdded);

        editMarkLineAutoWrapped (line+1, true);

        endLine++;
      }

      if (removeTrailingSpace) {
        // cu space
        editRemoveText (line, z - 1, 1);
      }
    }
  }

  editEnd ();

  return true;
}

void KateDocument::editAddUndo (int type, uint line, uint col, uint len, const QString &text)
{
  if (editIsRunning && editWithUndo && m_editCurrentUndo) {
    m_editCurrentUndo->addItem(static_cast<KateUndoGroup::UndoType>(type), line, col, len, text);

    // Clear redo buffer
    if (redoItems.count()) {
      qDeleteAll(redoItems);
      redoItems.clear();
    }
  }
}

bool KateDocument::editInsertText ( int line, int col, const QString &s, Kate::EditSource editSource )
{
  if (line < 0 || col < 0)
    return false;

  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

  editStart (true, editSource);

  editAddUndo (KateUndoGroup::editInsertText, line, col, s.length(), s);

  l->insertText (col, s);

  m_buffer->changeLine(line);

  history()->doEdit( new KateEditInfo(this, m_editSources.top(), KTextEditor::Range(line, col, line, col), QStringList(), KTextEditor::Range(line, col, line, col + s.length()), QStringList(s)) );
  emit KTextEditor::Document::textInserted(this, KTextEditor::Range(line, col, line, col + s.length()));

  editEnd();

  return true;
}

bool KateDocument::editRemoveText ( int line, int col, int len, Kate::EditSource editSource )
{
  if (line < 0 || col < 0 || len < 0)
    return false;

  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);
  QString oldText = l->string().mid(col, len);

  if (!l)
    return false;

  editStart (true, editSource);

  editAddUndo (KateUndoGroup::editRemoveText, line, col, len, l->string().mid(col, len));

  l->removeText (col, len);
  removeTrailingSpace( line );

  m_buffer->changeLine(line);

  history()->doEdit( new KateEditInfo(this, m_editSources.top(), KTextEditor::Range(line, col, line, col + len), QStringList(l->string().mid(col, len)), KTextEditor::Range(line, col, line, col), QStringList()) );
  emit KTextEditor::Document::textRemoved(this, KTextEditor::Range(line, col, line, col + len));
  emit textRemoved( oldText );

  editEnd ();

  return true;
}

bool KateDocument::editMarkLineAutoWrapped ( int line, bool autowrapped )
{
  if (line < 0)
    return false;

  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

  editStart ();

  editAddUndo (KateUndoGroup::editMarkLineAutoWrapped, line, autowrapped ? 1 : 0, 0, QString());

  l->setAutoWrapped (autowrapped);

  m_buffer->changeLine(line);

  editEnd ();

  return true;
}

bool KateDocument::editWrapLine ( int line, int col, bool newLine, bool *newLineAdded)
{
  if (line < 0 || col < 0)
    return false;

  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);

  if (!l)
    return false;

  editStart ();

  KateTextLine::Ptr nextLine = m_buffer->line(line+1);

  int pos = l->length() - col;

  if (pos < 0)
    pos = 0;

  editAddUndo (KateUndoGroup::editWrapLine, line, col, pos, (!nextLine || newLine) ? "1" : "0");

  if (!nextLine || newLine)
  {
    KateTextLine::Ptr textLine(new KateTextLine());

    textLine->insertText (0, l->string().mid(col, pos));
    l->truncate(col);

    m_buffer->insertLine (line+1, textLine);
    m_buffer->changeLine(line);

    QList<KTextEditor::Mark*> list;
    for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
    {
      if( i.value()->line >= line )
      {
        if ((col == 0) || (i.value()->line > line))
          list.append( i.value() );
      }
    }

    for( int i=0; i < list.size(); ++i )
    {
      KTextEditor::Mark* mark = m_marks.take( list[i]->line );
      mark->line++;
      m_marks.insert( mark->line, mark );
    }

    if( !list.isEmpty() )
      emit marksChanged( this );

    // yes, we added a new line !
    if (newLineAdded)
      (*newLineAdded) = true;

    history()->doEdit( new KateEditInfo(this, m_editSources.top(), KTextEditor::Range(line, col, line, col), QStringList(), KTextEditor::Range(line, col, line+1, 0), QStringList()) );
  }
  else
  {
    nextLine->insertText (0, l->string().mid(col, pos));
    l->truncate(col);

    m_buffer->changeLine(line);
    m_buffer->changeLine(line+1);

    // no, no new line added !
    if (newLineAdded)
      (*newLineAdded) = false;

    history()->doEdit( new KateEditInfo(this, m_editSources.top(), KTextEditor::Range(line, col, line+1, 0), QStringList(), KTextEditor::Range(line, col, line+1, pos), QStringList()) );
  }

  emit KTextEditor::Document::textInserted(this, KTextEditor::Range(line, col, line+1, pos));

  editEnd ();

  return true;
}

bool KateDocument::editUnWrapLine ( int line, bool removeLine, int length )
{
  if (line < 0 || length < 0)
    return false;

  if (!isReadWrite())
    return false;

  KateTextLine::Ptr l = m_buffer->line(line);
  KateTextLine::Ptr nextLine = m_buffer->line(line+1);

  if (!l || !nextLine)
    return false;

  editStart ();

  int col = l->length ();

  editAddUndo (KateUndoGroup::editUnWrapLine, line, col, length, removeLine ? "1" : "0");

  if (removeLine)
  {
    l->insertText (col, nextLine->string());

    m_buffer->changeLine(line);
    m_buffer->removeLine(line+1);
  }
  else
  {
    l->insertText (col, nextLine->string().left((nextLine->length() < length) ? nextLine->length() : length));
    nextLine->removeText (0, (nextLine->length() < length) ? nextLine->length() : length);

    m_buffer->changeLine(line);
    m_buffer->changeLine(line+1);
  }

  QList<KTextEditor::Mark*> list;
  for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
  {
    if( i.value()->line >= line+1 )
      list.append( i.value() );

    if ( i.value()->line == line+1 )
    {
      KTextEditor::Mark* mark = m_marks.take( line );

      if (mark)
      {
        i.value()->type |= mark->type;
      }
    }
  }

  for( int i=0; i < list.size(); ++i )
    {
    KTextEditor::Mark* mark = m_marks.take( list[i]->line );
    mark->line--;
    m_marks.insert( mark->line, mark );
  }

  if( !list.isEmpty() )
    emit marksChanged( this );

  history()->doEdit( new KateEditInfo(this, m_editSources.top(), KTextEditor::Range(line, col, line+1, 0), QStringList(QString()), KTextEditor::Range(line, col, line, col), QStringList()) );
  emit KTextEditor::Document::textRemoved(this, KTextEditor::Range(line, col, line+1, 0));

  editEnd ();

  return true;
}

bool KateDocument::editInsertLine ( int line, const QString &s, Kate::EditSource editSource )
{
  if (line < 0)
    return false;

  if (!isReadWrite())
    return false;

  if ( line > lines() )
    return false;

  editStart (true, editSource);

  editAddUndo (KateUndoGroup::editInsertLine, line, 0, s.length(), s);

  removeTrailingSpace( line ); // old line

  KateTextLine::Ptr tl(new KateTextLine());
  tl->insertText (0, s);
  m_buffer->insertLine(line, tl);
  m_buffer->changeLine(line);

  removeTrailingSpace( line ); // new line

  QList<KTextEditor::Mark*> list;
  for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
  {
    if( i.value()->line >= line )
      list.append( i.value() );
  }

  for( int i=0; i < list.size(); ++i )
  {
    KTextEditor::Mark* mark = m_marks.take( list[i]->line );
    mark->line++;
    m_marks.insert( mark->line, mark );
  }

  if( !list.isEmpty() )
    emit marksChanged( this );

  KTextEditor::Range rangeInserted(line, 0, line, tl->length());

  if (line) {
    KateTextLine::Ptr prevLine = plainKateTextLine(line - 1);
    rangeInserted.start().setPosition(line - 1, prevLine->length());
  } else {
    rangeInserted.end().setPosition(line + 1, 0);
  }

  history()->doEdit( new KateEditInfo(this, m_editSources.top(), KTextEditor::Range(rangeInserted.start(), rangeInserted.start()), QStringList(), rangeInserted, QStringList(s)) );
  emit KTextEditor::Document::textInserted(this, rangeInserted);

  editEnd ();

  return true;
}

bool KateDocument::editRemoveLine ( int line, Kate::EditSource editSource )
{
  if (line < 0)
    return false;

  if (!isReadWrite())
    return false;

  if ( line > lastLine() )
    return false;

  if ( lines() == 1 )
    return editRemoveText (0, 0, m_buffer->line(0)->length());

  editStart (true, editSource);

  QString oldText = this->line(line);

  editAddUndo (KateUndoGroup::editRemoveLine, line, 0, lineLength(line), this->line(line));

  KTextEditor::Range rangeRemoved(line, 0, line, oldText.length());

  if (line < lastLine()) {
    rangeRemoved.end().setPosition(line + 1, 0);
  } else if (line) {
    KateTextLine::Ptr prevLine = plainKateTextLine(line - 1);
    rangeRemoved.start().setPosition(line - 1, prevLine->length());
  }

  m_buffer->removeLine(line);

  KTextEditor::Mark* rmark = 0;
  QList<KTextEditor::Mark*> list;
  for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
  {
    if ( (i.value()->line > line) )
      list.append( i.value() );
    else if ( (i.value()->line == line) )
      rmark = i.value();
  }

  if (rmark)
    delete (m_marks.take (rmark->line));

  for( int i=0; i < list.size(); ++i )
  {
    KTextEditor::Mark* mark = m_marks.take( list[i]->line );
    mark->line--;
    m_marks.insert( mark->line, mark );
  }

  if( !list.isEmpty() )
    emit marksChanged( this );

  history()->doEdit( new KateEditInfo(this, m_editSources.top(), rangeRemoved, QStringList(QString(oldText)), KTextEditor::Range(rangeRemoved.start(), rangeRemoved.start()), QStringList()) );
  emit KTextEditor::Document::textRemoved(this, rangeRemoved);
  emit textRemoved( oldText + '\n');

  editEnd();

  return true;
}
//END

//BEGIN KTextEditor::UndoInterface stuff

uint KateDocument::undoCount () const
{
  return undoItems.count ();
}

uint KateDocument::redoCount () const
{
  return redoItems.count ();
}

void KateDocument::undo()
{
  if ((undoItems.count() > 0) && undoItems.last())
  {
    //clearSelection ();

    undoItems.last()->undo();
    redoItems.append (undoItems.last());
    undoItems.removeLast ();
    updateModified();

    emit undoChanged ();
  }
}

void KateDocument::redo()
{
  if ((redoItems.count() > 0) && redoItems.last())
  {
    //clearSelection ();

    redoItems.last()->redo();
    undoItems.append (redoItems.last());
    redoItems.removeLast ();
    updateModified();

    emit undoChanged ();
  }
}

void KateDocument::updateModified()
{
  /*
  How this works:

    After noticing that there where to many scenarios to take into
    consideration when using 'if's to toggle the "Modified" flag
    I came up with this baby, flexible and repetitive calls are
    minimal.

    A numeric unique pattern is generated by toggleing a set of bits,
    each bit symbolizes a different state in the Undo Redo structure.

      undoItems.isEmpty() != null          BIT 1
      redoItems.isEmpty() != null          BIT 2
      docWasSavedWhenUndoWasEmpty == true  BIT 3
      docWasSavedWhenRedoWasEmpty == true  BIT 4
      lastUndoGroupWhenSavedIsLastUndo     BIT 5
      lastUndoGroupWhenSavedIsLastRedo     BIT 6
      lastRedoGroupWhenSavedIsLastUndo     BIT 7
      lastRedoGroupWhenSavedIsLastRedo     BIT 8

    If you find a new pattern, please add it to the patterns array
  */

  unsigned char currentPattern = 0;
  const unsigned char patterns[] = {5,16,24,26,88,90,93,133,144,149,165};
  const unsigned char patternCount = sizeof(patterns);
  KateUndoGroup* undoLast = 0;
  KateUndoGroup* redoLast = 0;

  if (undoItems.isEmpty())
  {
    currentPattern |= 1;
  }
  else
  {
    undoLast = undoItems.last();
  }

  if (redoItems.isEmpty())
  {
    currentPattern |= 2;
  }
  else
  {
    redoLast = redoItems.last();
  }

  if (docWasSavedWhenUndoWasEmpty) currentPattern |= 4;
  if (docWasSavedWhenRedoWasEmpty) currentPattern |= 8;
  if (lastUndoGroupWhenSaved == undoLast) currentPattern |= 16;
  if (lastUndoGroupWhenSaved == redoLast) currentPattern |= 32;
  if (lastRedoGroupWhenSaved == undoLast) currentPattern |= 64;
  if (lastRedoGroupWhenSaved == redoLast) currentPattern |= 128;

  // This will print out the pattern information

  kDebug(13020) << "Pattern:" << static_cast<unsigned int>(currentPattern);

  for (uint patternIndex = 0; patternIndex < patternCount; ++patternIndex)
  {
    if ( currentPattern == patterns[patternIndex] )
    {
      setModified( false );
      kDebug(13020) << "setting modified to false!";
      break;
    }
  }
}

void KateDocument::clearUndo()
{
  qDeleteAll(undoItems);
  undoItems.clear ();

  lastUndoGroupWhenSaved = 0;
  docWasSavedWhenUndoWasEmpty = false;

  emit undoChanged ();
}

void KateDocument::clearRedo()
{
  qDeleteAll(redoItems);
  redoItems.clear ();

  lastRedoGroupWhenSaved = 0;
  docWasSavedWhenRedoWasEmpty = false;

  emit undoChanged ();
}
//END

//BEGIN KTextEditor::SearchInterface stuff

KTextEditor::Range KateDocument::searchText (const KTextEditor::Range & inputRange, const QString &text, bool casesensitive, bool backwards)
{
  FAST_DEBUG("KateDocument::searchText( " << inputRange.start().line() << ", "
    << inputRange.start().column() << ", " << text << ", " << backwards << " )");
  if (text.isEmpty() || !inputRange.isValid() || (inputRange.start() == inputRange.end()))
  {
    return KTextEditor::Range::invalid();
  }

  // split multi-line needle into single lines
  const QString sep("\n");
  const QStringList needleLines = text.split(sep);
  const int numNeedleLines = needleLines.count();
  FAST_DEBUG("searchText | needle has " << numNeedleLines << " lines");

  if (numNeedleLines > 1)
  {
    // multi-line plaintext search (both forwards or backwards)
    const int lastLine = inputRange.end().line();

    const int forMin   = inputRange.start().line(); // first line in range
    const int forMax   = lastLine + 1 - numNeedleLines; // last line in range
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;

    const int minLeft  = inputRange.start().column();
    const uint maxRight = inputRange.end().column(); // first not included

    // init hay line ring buffer
    int hayLinesZeroIndex = 0;
    QVector<KateTextLine::Ptr> hayLinesWindow;
    for (int i = 0; i < numNeedleLines; i++) {
      KateTextLine::Ptr textLine = m_buffer->plainLine((backwards ? forMax : forMin) + i);

      if (!textLine)
        return KTextEditor::Range::invalid();

      hayLinesWindow.append (textLine);
      FAST_DEBUG("searchText | hayLinesWindow[" << i << "] = \"" << hayLinesWindow[i]->string() << "\"");
    }

    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      // try to match all lines
      uint startCol = 0; // init value not important
      for (int k = 0; k < numNeedleLines; k++)
      {
        // which lines to compare
        const QString & needleLine = needleLines[k];
        KateTextLine::Ptr & hayLine = hayLinesWindow[(k + hayLinesZeroIndex) % numNeedleLines];
        FAST_DEBUG("searchText | hayLine = \"" << hayLine->string() << "\"");

        // position specific comparison (first, middle, last)
        if (k == 0) {
          // first line
          if (needleLine.length() == 0) // if needle starts with a newline
          {
            startCol = hayLine->length();
          }
          else
          {
            uint myMatchLen;
            const uint colOffset = (j > forMin) ? 0 : minLeft;
            const bool matches = hayLine->searchText(colOffset, hayLine->length(),needleLine, &startCol,
              &myMatchLen, casesensitive, false);
            if (!matches || (startCol + myMatchLen != static_cast<uint>(hayLine->length()))) {
              FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": no");
              break;
            }
            FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": yes");
          }
        } else if (k == numNeedleLines - 1) {
          // last line
          uint foundAt, myMatchLen;
          const bool matches = hayLine->searchText(0,hayLine->length(), needleLine, &foundAt, &myMatchLen, casesensitive, false);
          if (matches && (foundAt == 0) && !((k == lastLine)
              && (static_cast<uint>(foundAt + myMatchLen) > maxRight))) // full match!
          {
            FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": yes");
            return KTextEditor::Range(j, startCol, j + k, needleLine.length());
          }
          FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": no");
        } else {
          // mid lines
          uint foundAt, myMatchLen;
          const bool matches = hayLine->searchText(0, hayLine->length(),needleLine, &foundAt, &myMatchLen, casesensitive, false);
          if (!matches || (foundAt != 0) || (myMatchLen != static_cast<uint>(needleLine.length()))) {
            FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": no");
            break;
          }
          FAST_DEBUG("searchText | [" << j << " + " << k << "] line " << j + k << ": yes");
        }
      }

      // get a fresh line into the ring buffer
      if ((backwards && (j > forMin)) || (!backwards && (j < forMax)))
      {
        if (backwards)
        {
          hayLinesZeroIndex = (hayLinesZeroIndex + numNeedleLines - 1) % numNeedleLines;

          KateTextLine::Ptr textLine = m_buffer->plainLine(j - 1);

          if (!textLine)
            return KTextEditor::Range::invalid();

          hayLinesWindow[hayLinesZeroIndex] = textLine;

          FAST_DEBUG("searchText | filling slot " << hayLinesZeroIndex << " with line "
            << j - 1 << ": " << hayLinesWindow[hayLinesZeroIndex]->string());
        }
        else
        {
          hayLinesWindow[hayLinesZeroIndex] = m_buffer->plainLine(j + numNeedleLines);
          FAST_DEBUG("searchText | filling slot " << hayLinesZeroIndex << " with line "
            << j + numNeedleLines << ": " << hayLinesWindow[hayLinesZeroIndex]->string());
          hayLinesZeroIndex = (hayLinesZeroIndex + 1) % numNeedleLines;
        }
      }
    }

    // not found
    return KTextEditor::Range::invalid();
  }
  else
  {
    // single-line plaintext search (both forward of backward mode)
    const int minLeft  = inputRange.start().column();
    const uint maxRight = inputRange.end().column(); // first not included
    const int forMin   = inputRange.start().line();
    const int forMax   = inputRange.end().line();
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;
    FAST_DEBUG("searchText | single line " << (backwards ? forMax : forMin) << ".."
      << (backwards ? forMin : forMax));
    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(j);
      if (!textLine)
      {
        FAST_DEBUG("searchText | line " << j << ": no");
        return KTextEditor::Range::invalid();
      }

      const int offset = (j == forMin) ? minLeft : 0;
      const int line_end= (j==forMax) ? maxRight : textLine->length();
      uint foundAt, myMatchLen;
      FAST_DEBUG("searchText | searching in line line: " << j);
      const bool found = textLine->searchText (offset,line_end, text, &foundAt, &myMatchLen, casesensitive, backwards);
      if (found && !((j == forMax) && (static_cast<uint>(foundAt + myMatchLen) > maxRight)))
      {
        FAST_DEBUG("searchText | line " << j << ": yes");
        return KTextEditor::Range(j, foundAt, j, foundAt + myMatchLen);
      }
      else
      {
        FAST_DEBUG("searchText | line " << j << ": no");
      }
    }
  }
  return KTextEditor::Range::invalid();
}



// helper structs for captures re-construction
struct TwoViewCursor {
  int index;
  int openLine;
  int openCol;
  int closeLine;
  int closeCol;
  // note: open/close distinction does not seem needed
  // anymore. i keep it to make a potential way back
  // easier. overhead is minimal.
};

struct IndexPair {
  int openIndex;
  int closeIndex;
};



QVector<KTextEditor::Range> KateDocument::searchRegex(
    const KTextEditor::Range & inputRange,
    QRegExp &regexp,
    bool backwards)
{
  FAST_DEBUG("KateDocument::searchRegex( " << inputRange.start().line() << ", "
    << inputRange.start().column() << ", " << regexp.pattern() << ", " << backwards << " )");
  if (regexp.isEmpty() || !regexp.isValid() || !inputRange.isValid() || (inputRange.start() == inputRange.end()))
  {
    QVector<KTextEditor::Range> result;
    result.append(KTextEditor::Range::invalid());
    return result;
  }


  // detect pattern type (single- or mutli-line)
  bool isMultiLine;
  QString multiLinePattern = regexp.pattern();

  // detect '.' and '\s' and fix them
  const bool dotMatchesNewline = false; // TODO
  const int replacements = KateDocument::repairPattern(multiLinePattern, isMultiLine);
  if (dotMatchesNewline && (replacements > 0))
  {
    isMultiLine = true;
  }

  const int firstLineIndex = inputRange.start().line();
  const int minColStart = inputRange.start().column();
//  const int maxColEnd = inputRange.end().column();
  if (isMultiLine)
  {
    // multi-line regex search (both forward and backward mode)
    QString wholeDocument;
    const int inputLineCount = inputRange.end().line() - inputRange.start().line() + 1;
    FAST_DEBUG("multi line search (lines " << firstLineIndex << ".." << firstLineIndex + inputLineCount - 1 << ")");

    // nothing to do...
    if (firstLineIndex >= m_buffer->lines())
    {
      QVector<KTextEditor::Range> result;
      result.append(KTextEditor::Range::invalid());
      return result;
    }

    QVector<int> lineLens (inputLineCount);

    // first line
    KateTextLine::Ptr firstLine = m_buffer->plainLine(firstLineIndex);
    if (!firstLine)
    {
      QVector<KTextEditor::Range> result;
      result.append(KTextEditor::Range::invalid());
      return result;
    }

    QString firstLineText = firstLine->string();
    const int firstLineLen = firstLineText.length() - minColStart;
    wholeDocument.append(firstLineText.right(firstLineLen));
    lineLens[0] = firstLineLen;
    FAST_DEBUG("  line" << 0 << "has length" << lineLens[0]);

    // second line and after
    const QString sep("\n");
    for (int i = 1; i < inputLineCount; i++)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(firstLineIndex + i);
      if (!textLine)
      {
        QVector<KTextEditor::Range> result;
        result.append(KTextEditor::Range::invalid());
        return result;
      }

      QString text = textLine->string();
      lineLens[i] = text.length();
      wholeDocument.append(sep);
      wholeDocument.append(text);
      FAST_DEBUG("  line" << i << "has length" << lineLens[i]);
    }

    // apply modified pattern
    regexp.setPattern(multiLinePattern);
    const int pos = backwards
        ? KateDocument::fixedLastIndexIn(regexp, wholeDocument, -1, QRegExp::CaretAtZero)
        : regexp.indexIn(wholeDocument, 0, QRegExp::CaretAtZero);
    if (pos == -1)
    {
      // no match
      FAST_DEBUG("not found");
      {
        QVector<KTextEditor::Range> result;
        result.append(KTextEditor::Range::invalid());
        return result;
      }
    }

#ifdef FAST_DEBUG_ENABLE
    const int matchLen = regexp.matchedLength();
    FAST_DEBUG("found at relative pos " << pos << ", length " << matchLen);
#endif

    // save opening and closing indices and build a map.
    // the correct values will be written into it later.
    QMap<int, TwoViewCursor *> indicesToCursors;
    const int numCaptures = regexp.numCaptures();
    QVector<IndexPair> indexPairs(1 + numCaptures);
    for (int z = 0; z <= numCaptures; z++)
    {
      const int openIndex = regexp.pos(z);
      IndexPair & pair = indexPairs[z];
      if (openIndex == -1)
      {
        // empty capture gives invalid
        pair.openIndex = -1;
        pair.closeIndex = -1;
        FAST_DEBUG("capture []");
      }
      else
      {
        const int closeIndex = openIndex + regexp.cap(z).length();
        pair.openIndex = openIndex;
        pair.closeIndex = closeIndex;
        FAST_DEBUG("capture [" << pair.openIndex << ".." << pair.closeIndex << "]");

        // each key no more than once
        if (!indicesToCursors.contains(openIndex))
        {
          TwoViewCursor * twoViewCursor = new TwoViewCursor;
          twoViewCursor->index = openIndex;
          indicesToCursors.insert(openIndex, twoViewCursor);
          FAST_DEBUG("  border index added: " << openIndex);
        }
        if (!indicesToCursors.contains(closeIndex))
        {
          TwoViewCursor * twoViewCursor = new TwoViewCursor;
          twoViewCursor->index = closeIndex;
          indicesToCursors.insert(closeIndex, twoViewCursor);
          FAST_DEBUG("  border index added: " << closeIndex);
        }
      }
    }

    // find out where they belong
    int curRelLine = 0;
    int curRelCol = 0;
    int curRelIndex = 0;
    QMap<int, TwoViewCursor *>::const_iterator iter = indicesToCursors.constBegin();
    while (iter != indicesToCursors.end())
    {
      // forward to index, save line/col
      const int index = (*iter)->index;
      FAST_DEBUG("resolving position" << index);
      TwoViewCursor & twoViewCursor = *(*iter);
      while (curRelIndex <= index)
      {
        FAST_DEBUG("walk pos (" << curRelLine << "," << curRelCol << ") = "
            << curRelIndex << "relative, steps more to go" << index - curRelIndex);
        const int curRelLineLen = lineLens[curRelLine];
        const int curLineRemainder = curRelLineLen - curRelCol;
        const int lineFeedIndex = curRelIndex + curLineRemainder;
        if (index <= lineFeedIndex) {
            if (index == lineFeedIndex) {
                // on this line _on_ line feed
                FAST_DEBUG("  on line feed");
                const int absLine = curRelLine + firstLineIndex;
                twoViewCursor.openLine
                    = twoViewCursor.closeLine
                    = absLine;
                twoViewCursor.openCol
                    = twoViewCursor.closeCol
                    = ((curRelLine == 0) ? minColStart : 0) + curRelLineLen;

                // advance to next line
                const int advance = (index - curRelIndex) + 1;
                curRelLine++;
                curRelCol = 0;
                curRelIndex += advance;
            } else { // index < lineFeedIndex
                // on this line _before_ line feed
                FAST_DEBUG("  before line feed");
                const int diff = (index - curRelIndex);
                const int absLine = curRelLine + firstLineIndex;
                const int absCol = ((curRelLine == 0) ? minColStart : 0) + curRelCol + diff;
                twoViewCursor.openLine
                    = twoViewCursor.closeLine
                    = absLine;
                twoViewCursor.openCol
                    = twoViewCursor.closeCol
                    = absCol;

                // advance on same line
                const int advance = diff + 1;
                curRelCol += advance;
                curRelIndex += advance;
            }
            FAST_DEBUG("open(" << twoViewCursor.openLine << "," << twoViewCursor.openCol
                << ")  close(" << twoViewCursor.closeLine << "," << twoViewCursor.closeCol << ")");
        }
        else // if (index > lineFeedIndex)
        {
          // not on this line
          // advance to next line
          FAST_DEBUG("  not on this line");
          const int advance = curLineRemainder + 1;
          curRelLine++;
          curRelCol = 0;
          curRelIndex += advance;
        }
      }

      iter++;
    }

    // build result array
    QVector<KTextEditor::Range> result(1 + numCaptures);
    for (int y = 0; y <= numCaptures; y++)
    {
      IndexPair & pair = indexPairs[y];
      if ((pair.openIndex == -1) || (pair.closeIndex == -1))
      {
        result[y] = KTextEditor::Range::invalid();
      }
      else
      {
        const TwoViewCursor * const openCursors = indicesToCursors[pair.openIndex];
        const TwoViewCursor * const closeCursors = indicesToCursors[pair.closeIndex];
        const int startLine = openCursors->openLine;
        const int startCol = openCursors->openCol;
        const int endLine = closeCursors->closeLine;
        const int endCol = closeCursors->closeCol;
        FAST_DEBUG("range " << y << ": (" << startLine << ", " << startCol << ")..(" << endLine << ", " << endCol << ")");
        result[y] = KTextEditor::Range(startLine, startCol, endLine, endCol);
      }
    }

    // free structs allocated for indicesToCursors
    iter = indicesToCursors.constBegin();
    while (iter != indicesToCursors.end())
    {
      TwoViewCursor * const twoViewCursor = *iter;
      delete twoViewCursor;
      iter++;
    }
    return result;
  }
  else
  {
    // single-line regex search (both forward of backward mode)
    const int minLeft  = inputRange.start().column();
    const uint maxRight = inputRange.end().column(); // first not included
    const int forMin   = inputRange.start().line();
    const int forMax   = inputRange.end().line();
    const int forInit  = backwards ? forMax : forMin;
    const int forInc   = backwards ? -1 : +1;
    FAST_DEBUG("single line " << (backwards ? forMax : forMin) << ".."
      << (backwards ? forMin : forMax));
    for (int j = forInit; (forMin <= j) && (j <= forMax); j += forInc)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(j);
      if (!textLine)
      {
        FAST_DEBUG("searchText | line " << j << ": no");
        QVector<KTextEditor::Range> result;
        result.append(KTextEditor::Range::invalid());
        return result;
      }

        // Find (and don't match ^ in between...)
        const int first = (j == forMin) ? minLeft : 0;
        const int afterLast = (j == forMax) ? maxRight : textLine->length();
        const QString hay = textLine->string();
        bool found = true;
        int foundAt;
        uint myMatchLen;
        if (backwards) {
            const int lineLen = textLine->length();
            const int offset = afterLast - lineLen - 1;
            FAST_DEBUG("lastIndexIn(" << hay << "," << offset << ")");
            foundAt = KateDocument::fixedLastIndexIn(regexp, hay, offset);
            found = (foundAt != -1) && (foundAt >= first);
        } else {
            FAST_DEBUG("indexIn(" << hay << "," << first << ")");
            foundAt = regexp.indexIn(hay, first);
            found = (foundAt != -1);
        }
        myMatchLen = found ? regexp.matchedLength() : 0;

      /*
      TODO do we still need this?

          // A special case which can only occur when searching with a regular expression consisting
          // only of a lookahead (e.g. ^(?=\{) for a function beginning without selecting '{').
          if (myMatchLen == 0 && line == startPosition.line() && foundAt == (uint) col)
          {
            if (col < lineLength(line))
              col++;
            else {
              line++;
              col = 0;
            }
            continue;
          }
      */

      if (found && !((j == forMax) && (static_cast<uint>(foundAt + myMatchLen) > maxRight)))
      {
        FAST_DEBUG("line " << j << ": yes");

        // build result array
        const int numCaptures = regexp.numCaptures();
        QVector<KTextEditor::Range> result(1 + numCaptures);
        result[0] = KTextEditor::Range(j, foundAt, j, foundAt + myMatchLen);
        FAST_DEBUG("result range " << 0 << ": (" << j << ", " << foundAt << ")..(" << j << ", " << foundAt + myMatchLen << ")");
        for (int y = 1; y <= numCaptures; y++)
        {
          const int openIndex = regexp.pos(y);
          if (openIndex == -1)
          {
            result[y] = KTextEditor::Range::invalid();
            FAST_DEBUG("capture []");
          }
          else
          {
            const int closeIndex = openIndex + regexp.cap(y).length();
            FAST_DEBUG("result range " << y << ": (" << j << ", " << openIndex << ")..(" << j << ", " << closeIndex << ")");
            result[y] = KTextEditor::Range(j, openIndex, j, closeIndex);
          }
        }
        return result;
      }
      else
      {
        FAST_DEBUG("searchText | line " << j << ": no");
      }
    }
  }

  QVector<KTextEditor::Range> result;
  result.append(KTextEditor::Range::invalid());
  return result;
}

QWidget * KateDocument::dialogParent()
{
    QWidget *w=widget();

    if(!w)
    {
        w=activeView();

        if(!w)
            w=QApplication::activeWindow();
    }

    return w;
}

QVector<KTextEditor::Range> KateDocument::searchText(
    const KTextEditor::Range & range,
    const QString & pattern,
    const KTextEditor::Search::SearchOptions options)
{
  // TODO
  // * support BlockInputRange
  // * support DotMatchesNewline
  QString workPattern(pattern);

  KTextEditor::Search::SearchOptions finalOptions(options);
  const bool escapeSequences = finalOptions.testFlag(KTextEditor::Search::EscapeSequences);

  // abuse regex for whole word plaintext search
  if (finalOptions.testFlag(KTextEditor::Search::WholeWords))
  {
    // resolve escape sequences like \t
    if (escapeSequences)
    {
      KateDocument::escapePlaintext(workPattern);
    }

    // escape dot and friends
    workPattern = "\\b" + QRegExp::escape(workPattern) + "\\b";

    // regex ON, whole words OFF
    finalOptions |= KTextEditor::Search::Regex;
    finalOptions &= ~KTextEditor::Search::SearchOptions(KTextEditor::Search::WholeWords);
  }

  const bool regexMode = finalOptions.testFlag(KTextEditor::Search::Regex);
  const bool caseSensitive = !finalOptions.testFlag(KTextEditor::Search::CaseInsensitive);
  const bool backwards = finalOptions.testFlag(KTextEditor::Search::Backwards);

  if (regexMode)
  {
    // regex search
    const Qt::CaseSensitivity caseSensitivity =
        caseSensitive
        ? Qt::CaseSensitive
        : Qt::CaseInsensitive;

    QRegExp matcher(workPattern, caseSensitivity);
    if (matcher.isValid())
    {
      // valid pattern
      // run engine
      return searchRegex(range, matcher, backwards);
    }
    else
    {
      // invalid pattern
      QVector<KTextEditor::Range> result;
      result.append(KTextEditor::Range::invalid());
      return result;
    }
  }
  else
  {
    // plaintext search

    // resolve escape sequences like \t
    if (escapeSequences)
    {
      KateDocument::escapePlaintext(workPattern);
    }

    // run engine
    KTextEditor::Range resultRange = searchText(range, workPattern, caseSensitive, backwards);
    QVector<KTextEditor::Range> result;
    result.append(resultRange);
    return result;
  }
}



KTextEditor::Search::SearchOptions KateDocument::supportedSearchOptions() const
{
  KTextEditor::Search::SearchOptions supported(KTextEditor::Search::Default);
  supported |= KTextEditor::Search::Regex;
  supported |= KTextEditor::Search::CaseInsensitive;
  supported |= KTextEditor::Search::Backwards;
// supported |= KTextEditor::Search::BlockInputRange;
  supported |= KTextEditor::Search::EscapeSequences;
  supported |= KTextEditor::Search::WholeWords;
// supported |= KTextEditor::Search::DotMatchesNewline;
  return supported;
}



/*static*/ void KateDocument::escapePlaintext(QString & text, QList<ReplacementPart> * parts,
        bool replacementGoodies) {
  // get input
  const int inputLen = text.length();
  int input = 0; // walker index

  // prepare output
  QString output;
  output.reserve(inputLen + 1);

  while (input < inputLen)
  {
    switch (text[input].unicode())
    {
    case L'\n':
      output.append(text[input]);
      input++;
      break;

    case L'\\':
      if (input + 1 >= inputLen)
      {
        // copy backslash
        output.append(text[input]);
        input++;
        break;
      }

      switch (text[input + 1].unicode())
      {
      case L'0': // "\0000".."\0377"
        if (input + 4 >= inputLen)
        {
          if (parts == NULL)
          {
            // strip backslash ("\0" -> "0")
            output.append(text[input + 1]);
          }
          else
          {
            // handle reference
            ReplacementPart curPart;

            // append text before the reference
            if (!output.isEmpty())
            {
              curPart.type = ReplacementPart::Text;
              curPart.text = output;
              output.clear();
              parts->append(curPart);
              curPart.text.clear();
            }

            // append reference
            curPart.type = ReplacementPart::Reference;
            curPart.index = 0;
            parts->append(curPart);
          }
          input += 2;
        }
        else
        {
          bool stripAndSkip = false;
          const ushort text_2 = text[input + 2].unicode();
          if ((text_2 >= L'0') && (text_2 <= L'3'))
          {
            const ushort text_3 = text[input + 3].unicode();
            if ((text_3 >= L'0') && (text_3 <= L'7'))
            {
              const ushort text_4 = text[input + 4].unicode();
              if ((text_4 >= L'0') && (text_4 <= L'7'))
              {
                int digits[3];
                for (int i = 0; i < 3; i++)
                {
                  digits[i] = 7 - (L'7' - text[input + 2 + i].unicode());
                }
                const int ch = 64 * digits[0] + 8 * digits[1] + digits[2];
                output.append(QChar(ch));
                input += 5;
              }
              else
              {
                stripAndSkip = true;
              }
            }
            else
            {
              stripAndSkip = true;
            }
          }
          else
          {
            stripAndSkip = true;
          }

          if (stripAndSkip)
          {
            if (parts == NULL)
            {
              // strip backslash ("\0" -> "0")
              output.append(text[input + 1]);
            }
            else
            {
              // handle reference
              ReplacementPart curPart;

              // append text before the reference
              if (!output.isEmpty())
              {
                curPart.type = ReplacementPart::Text;
                curPart.text = output;
                output.clear();
                parts->append(curPart);
                curPart.text.clear();
              }

              // append reference
              curPart.type = ReplacementPart::Reference;
              curPart.index = 0;
              parts->append(curPart);
            }
            input += 2;
          }
        }
        break;

      case L'1':
      case L'2':
      case L'3':
      case L'4':
      case L'5':
      case L'6':
      case L'7':
      case L'8':
      case L'9':
        if (parts == NULL)
        {
          // strip backslash ("\?" -> "?")
          output.append(text[input + 1]);
        }
        else
        {
          // handle reference
          ReplacementPart curPart;

          // append text before the reference
          if (!output.isEmpty())
          {
            curPart.type = ReplacementPart::Text;
            curPart.text = output;
            output.clear();
            parts->append(curPart);
            curPart.text.clear();
          }

          // append reference
          curPart.type = ReplacementPart::Reference;
          curPart.index = 9 - (L'9' - text[input + 1].unicode());
          parts->append(curPart);
        }
        input += 2;
        break;

      case L'E': // FALLTHROUGH
      case L'L': // FALLTHROUGH
      case L'U':
        if ((parts == NULL) || !replacementGoodies) {
          // strip backslash ("\?" -> "?")
          output.append(text[input + 1]);
        } else {
          // handle case switcher
          ReplacementPart curPart;

          // append text before case switcher
          if (!output.isEmpty())
          {
            curPart.type = ReplacementPart::Text;
            curPart.text = output;
            output.clear();
            parts->append(curPart);
            curPart.text.clear();
          }

          // append case switcher
          switch (text[input + 1].unicode()) {
          case L'L':
            curPart.type = ReplacementPart::LowerCase;
            break;

          case L'U':
            curPart.type = ReplacementPart::UpperCase;
            break;

          case L'E': // FALLTHROUGH
          default:
            curPart.type = ReplacementPart::KeepCase;

          }
          parts->append(curPart);
        }
        input += 2;
        break;

      case L'#':
        if ((parts == NULL) || !replacementGoodies) {
          // strip backslash ("\?" -> "?")
          output.append(text[input + 1]);
          input += 2;
        } else {
          // handle replacement counter
          ReplacementPart curPart;

          // append text before replacement counter
          if (!output.isEmpty())
          {
            curPart.type = ReplacementPart::Text;
            curPart.text = output;
            output.clear();
            parts->append(curPart);
            curPart.text.clear();
          }

          // eat and count all following hash marks
          // each hash stands for a leading zero: \### will produces 001, 002, ...
          int count = 1;
          while ((input + count + 1 < inputLen) && (text[input + count + 1].unicode() == L'#')) {
            count++;
          }
          curPart.type = ReplacementPart::Counter;
          curPart.index = count; // Each hash stands
          parts->append(curPart);
          input += 1 + count;
        }
        break;

      case L'a':
        output.append(QChar(0x07));
        input += 2;
        break;

      case L'f':
        output.append(QChar(0x0c));
        input += 2;
        break;

      case L'n':
        output.append(QChar(0x0a));
        input += 2;
        break;

      case L'r':
        output.append(QChar(0x0d));
        input += 2;
        break;

      case L't':
        output.append(QChar(0x09));
        input += 2;
        break;

      case L'v':
        output.append(QChar(0x0b));
        input += 2;
        break;

      case L'x': // "\x0000".."\xffff"
        if (input + 5 >= inputLen)
        {
          // strip backslash ("\x" -> "x")
          output.append(text[input + 1]);
          input += 2;
        }
        else
        {
          bool stripAndSkip = false;
          const ushort text_2 = text[input + 2].unicode();
          if (((text_2 >= L'0') && (text_2 <= L'9'))
              || ((text_2 >= L'a') && (text_2 <= L'f'))
              || ((text_2 >= L'A') && (text_2 <= L'F')))
          {
            const ushort text_3 = text[input + 3].unicode();
            if (((text_3 >= L'0') && (text_3 <= L'9'))
                || ((text_3 >= L'a') && (text_3 <= L'f'))
                || ((text_3 >= L'A') && (text_3 <= L'F')))
            {
              const ushort text_4 = text[input + 4].unicode();
              if (((text_4 >= L'0') && (text_4 <= L'9'))
                  || ((text_4 >= L'a') && (text_4 <= L'f'))
                  || ((text_4 >= L'A') && (text_4 <= L'F')))
              {
                const ushort text_5 = text[input + 5].unicode();
                if (((text_5 >= L'0') && (text_5 <= L'9'))
                    || ((text_5 >= L'a') && (text_5 <= L'f'))
                    || ((text_5 >= L'A') && (text_5 <= L'F')))
                {
                  int digits[4];
                  for (int i = 0; i < 4; i++)
                  {
                    const ushort cur = text[input + 2 + i].unicode();
                    if ((cur >= L'0') && (cur <= L'9'))
                    {
                      digits[i] = 9 - (L'9' - cur);
                    }
                    else if ((cur >= L'a') && (cur <= L'f'))
                    {
                      digits[i] = 15 - (L'f' - cur);
                    }
                    else // if ((cur >= L'A') && (cur <= L'F')))
                    {
                      digits[i] = 15 - (L'F' - cur);
                    }
                  }

                  const int ch = 4096 * digits[0] + 256 * digits[1] + 16 * digits[2] + digits[3];
                  output.append(QChar(ch));
                  input += 6;
                }
                else
                {
                  stripAndSkip = true;
                }
              }
              else
              {
                stripAndSkip = true;
              }
            }
            else
            {
              stripAndSkip = true;
            }
          }

          if (stripAndSkip)
          {
            // strip backslash ("\x" -> "x")
            output.append(text[input + 1]);
            input += 2;
          }
        }
        break;

      default:
        // strip backslash ("\?" -> "?")
        output.append(text[input + 1]);
        input += 2;

      }
      break;

    default:
      output.append(text[input]);
      input++;

    }
  }

  if (parts == NULL)
  {
    // overwrite with escaped edition
    text = output;
  }
  else
  {
    // append text after the last reference if any
    if (!output.isEmpty())
    {
      ReplacementPart curPart;
      curPart.type = ReplacementPart::Text;
      curPart.text = output;
      parts->append(curPart);
    }
  }
}



// these things can besides '.' and '\s' make apptern multi-line:
// \n, \x000A, \x????-\x????, \0012, \0???-\0???
// a multi-line pattern must not pass as single-line, the other
// way around will just result in slower searches and is therefore
// not as critical
/*static*/ int KateDocument::repairPattern(QString & pattern, bool & stillMultiLine)
{
  const QString & text = pattern; // read-only input for parsing

  // get input
  const int inputLen = text.length();
  int input = 0; // walker index

  // prepare output
  QString output;
  output.reserve(2 * inputLen + 1); // twice should be enough for the average case

  // parser state
  stillMultiLine = false;
  int replaceCount = 0;
  bool insideClass = false;

  while (input < inputLen)
  {
    if (insideClass)
    {
      // wait for closing, unescaped ']'
      switch (text[input].unicode())
      {
      case L'\\':
        switch (text[input + 1].unicode())
        {
        case L'x':
          if (input + 5 < inputLen)
          {
            // copy "\x????" unmodified
            output.append(text.mid(input, 6));
            input += 6;
          } else {
            // copy "\x" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L'0':
          if (input + 4 < inputLen)
          {
            // copy "\0???" unmodified
            output.append(text.mid(input, 5));
            input += 5;
          } else {
            // copy "\0" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L's':
          // replace "\s" with "[ \t]"
          output.append("[ \\t]");
          input += 2;
          replaceCount++;
          break;

        case L'n':
          stillMultiLine = true;
          // FALLTROUGH

        default:
          // copy "\?" unmodified
          output.append(text.mid(input, 2));
          input += 2;
        }
        break;

      case L']':
        // copy "]" unmodified
        insideClass = false;
        output.append(text[input]);
        input++;
        break;

      default:
        // copy "?" unmodified
        output.append(text[input]);
        input++;

      }
    }
    else
    {
      // search for real dots and \S
      switch (text[input].unicode())
      {
      case L'\\':
        switch (text[input + 1].unicode())
        {
        case L'x':
          if (input + 5 < inputLen)
          {
            // copy "\x????" unmodified
            output.append(text.mid(input, 6));
            input += 6;
          } else {
            // copy "\x" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L'0':
          if (input + 4 < inputLen)
          {
            // copy "\0???" unmodified
            output.append(text.mid(input, 5));
            input += 5;
          } else {
            // copy "\0" unmodified
            output.append(text.mid(input, 2));
            input += 2;
          }
          stillMultiLine = true;
          break;

        case L's':
          // replace "\s" with "[ \t]"
          output.append("[ \\t]");
          input += 2;
          replaceCount++;
          break;

        case L'n':
          stillMultiLine = true;
          // FALLTROUGH

        default:
          // copy "\?" unmodified
          output.append(text.mid(input, 2));
          input += 2;
        }
        break;

      case L'.':
        // replace " with "[^\n]"
        output.append("[^\\n]");
        input++;
        replaceCount++;
        break;

      case L'[':
        // copy "]" unmodified
        insideClass = true;
        output.append(text[input]);
        input++;
        break;

      default:
        // copy "?" unmodified
        output.append(text[input]);
        input++;

      }
    }
  }

  // Overwrite with repaired pattern
  pattern = output;
  return replaceCount;
}



/*static*/ int KateDocument::fixedLastIndexIn(const QRegExp & matcher, const QString & str,
        int offset, QRegExp::CaretMode caretMode) {
    int prevPos = -1;
    int prevLen = 1;
    const int strLen = str.length();
    for (;;) {
        const int pos = matcher.indexIn(str, prevPos + prevLen, caretMode);
        if (pos == -1) {
            // No more matches
            break;
        } else {
            const int len = matcher.matchedLength();
            if (pos > strLen + offset + 1) {
                // Gone too far, match in no way of use
                break;
            }

            if (pos + len > strLen + offset + 1) {
                // Gone too far, check if usable
                if (offset == -1) {
                    // No shrinking possible
                    break;
                }

                const QString str2 = str.mid(0, strLen + offset + 1);
                const int pos2 = matcher.indexIn(str2, pos, caretMode);
                if (pos2 != -1) {
                    // Match usable
                    return pos2;
                } else {
                    // Match NOT usable
                    break;
                }
            }

            // Valid match, but maybe not the last one
            prevPos = pos;
            prevLen = (len == 0) ? 1 : len;
        }
    }

    // Previous match is what we want
    if (prevPos != -1) {
        // Do that very search again
        matcher.indexIn(str, prevPos, caretMode);
        return prevPos;
    } else {
        return -1;
    }
}
//END

//BEGIN KTextEditor::HighlightingInterface stuff
bool KateDocument::setMode (const QString &name)
{
  updateFileType (name);
  return true;
}

QString KateDocument::mode () const
{
  return m_fileType;
}

QStringList KateDocument::modes () const
{
  QStringList m;

  const QList<KateFileType *> &modeList = KateGlobal::self()->modeManager()->list();
  for (int i = 0; i < modeList.size(); ++i)
    m << modeList[i]->name;

  return m;
}

bool KateDocument::setHighlightingMode (const QString &name)
{
  m_buffer->setHighlight (KateHlManager::self()->nameFind(name));

  if (true)
  {
    setDontChangeHlOnSave();
    return true;
  }

  return false;
}

QString KateDocument::highlightingMode () const
{
  return highlight()->name ();
}

QStringList KateDocument::highlightingModes () const
{
  QStringList hls;

  for (int i = 0; i < KateHlManager::self()->highlights(); ++i)
    hls << KateHlManager::self()->hlName (i);

  return hls;
}

QString KateDocument::highlightingModeSection( int index ) const
{
  return KateHlManager::self()->hlSection( index );
}

QString KateDocument::modeSection( int index ) const
{
  return KateGlobal::self()->modeManager()->list()[ index ]->section;
}

void KateDocument::bufferHlChanged ()
{
  // update all views
  makeAttribs(false);

  // deactivate indenter if necessary
  m_indenter.checkRequiredStyle();

  emit highlightingModeChanged(this);
}

void KateDocument::setDontChangeHlOnSave()
{
  hlSetByUser = true;
}
//END

//BEGIN KTextEditor::ConfigInterface stuff
void KateDocument::readSessionConfig(const KConfigGroup &kconfig)
{
  // restore the url
  KUrl url (kconfig.readEntry("URL"));

  // get the encoding
  QString tmpenc=kconfig.readEntry("Encoding");
  if (!tmpenc.isEmpty() && (tmpenc != encoding()))
    setEncoding(tmpenc);

  // open the file if url valid
  if (!url.isEmpty() && url.isValid())
    openUrl (url);
  else completed(); //perhaps this should be emitted at the end of this function

  // restore the filetype
  updateFileType (kconfig.readEntry("Mode", "Normal"));

  // restore the hl stuff
  m_buffer->setHighlight(KateHlManager::self()->nameFind(kconfig.readEntry("Highlighting")));

  // indent mode
  config()->setIndentationMode( kconfig.readEntry("Indentation Mode", config()->indentationMode() ) );

  // Restore Bookmarks
  QList<int> marks = kconfig.readEntry("Bookmarks", QList<int>());
  for( int i = 0; i < marks.count(); i++ )
    addMark( marks[i], KateDocument::markType01 );
}

void KateDocument::writeSessionConfig(KConfigGroup &kconfig)
{
  if ( this->url().isLocalFile() ) {
    const QString path = this->url().path();
    if ( KGlobal::dirs()->relativeLocation( "tmp", path ) != path ) {
      return; // inside tmp resource, do not save
    }
  }
  // save url
  kconfig.writeEntry("URL", this->url().prettyUrl() );

  // save encoding
  kconfig.writeEntry("Encoding",encoding());

  // save file type
  kconfig.writeEntry("Mode", m_fileType);

  // save hl
  kconfig.writeEntry("Highlighting", highlight()->name());

  // indent mode
  kconfig.writeEntry("Indentation Mode", config()->indentationMode() );

  // Save Bookmarks
  QList<int> marks;
  for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
    if (i.value()->type & KTextEditor::MarkInterface::markType01)
     marks << i.value()->line;

  kconfig.writeEntry( "Bookmarks", marks );
}

uint KateDocument::mark( int line )
{
  if( !m_marks.value(line) )
    return 0;

  return m_marks[line]->type;
}

void KateDocument::setMark( int line, uint markType )
{
  clearMark( line );
  addMark( line, markType );
}

void KateDocument::clearMark( int line )
{
  if( line > lastLine() )
    return;

  if( !m_marks.value(line) )
    return;

  KTextEditor::Mark* mark = m_marks.take( line );
  emit markChanged( this, *mark, MarkRemoved );
  emit marksChanged( this );
  delete mark;
  tagLines( line, line );
  repaintViews(true);
}

void KateDocument::addMark( int line, uint markType )
{
  if( line > lastLine())
    return;

  if( markType == 0 )
    return;

  if( m_marks.value(line) ) {
    KTextEditor::Mark* mark = m_marks[line];

    // Remove bits already set
    markType &= ~mark->type;

    if( markType == 0 )
      return;

    // Add bits
    mark->type |= markType;
  } else {
    KTextEditor::Mark *mark = new KTextEditor::Mark;
    mark->line = line;
    mark->type = markType;
    m_marks.insert( line, mark );
  }

  // Emit with a mark having only the types added.
  KTextEditor::Mark temp;
  temp.line = line;
  temp.type = markType;
  emit markChanged( this, temp, MarkAdded );

  emit marksChanged( this );
  tagLines( line, line );
  repaintViews(true);
}

void KateDocument::removeMark( int line, uint markType )
{
  if( line > lastLine() )
    return;

  if( !m_marks.value(line) )
    return;

  KTextEditor::Mark* mark = m_marks[line];

  // Remove bits not set
  markType &= mark->type;

  if( markType == 0 )
    return;

  // Subtract bits
  mark->type &= ~markType;

  // Emit with a mark having only the types removed.
  KTextEditor::Mark temp;
  temp.line = line;
  temp.type = markType;
  emit markChanged( this, temp, MarkRemoved );

  if( mark->type == 0 )
    m_marks.remove( line );

  emit marksChanged( this );
  tagLines( line, line );
  repaintViews(true);
}

const QHash<int, KTextEditor::Mark*> &KateDocument::marks()
{
  return m_marks;
}

void KateDocument::clearMarks()
{
  while (!m_marks.isEmpty())
  {
    QHash<int, KTextEditor::Mark*>::iterator it = m_marks.begin();
    KTextEditor::Mark mark = *it.value();
    delete it.value();
    m_marks.erase (it);

    emit markChanged( this, mark, MarkRemoved );
    tagLines( mark.line, mark.line );
  }

  m_marks.clear();

  emit marksChanged( this );
  repaintViews(true);
}

void KateDocument::setMarkPixmap( MarkInterface::MarkTypes type, const QPixmap& pixmap )
{
  m_markPixmaps.insert( type, pixmap );
}

void KateDocument::setMarkDescription( MarkInterface::MarkTypes type, const QString& description )
{
  m_markDescriptions.insert( type, description );
}

QPixmap KateDocument::markPixmap( MarkInterface::MarkTypes type ) const
{
  return m_markPixmaps.contains(type) ?
         m_markPixmaps[type] : QPixmap();
}

QColor KateDocument::markColor( MarkInterface::MarkTypes type ) const
{
  uint reserved = (0x1 << KTextEditor::MarkInterface::reservedMarkersCount()) - 1;
  if ((uint)type >= (uint)markType01 && (uint)type <= reserved) {
    return KateRendererConfig::global()->lineMarkerColor(type);
  } else {
    return QColor();
  }
}

QString KateDocument::markDescription( MarkInterface::MarkTypes type ) const
{
  return m_markDescriptions.contains(type) ?
         m_markDescriptions[type] : QString();
}

void KateDocument::setEditableMarks( uint markMask )
{
  m_editableMarks = markMask;
}

uint KateDocument::editableMarks() const
{
  return m_editableMarks;
}
//END

//BEGIN KTextEditor::PrintInterface stuff
bool KateDocument::printDialog ()
{
  return KatePrinter::print (this);
}

bool KateDocument::print ()
{
  return KatePrinter::print (this);
}
//END

//BEGIN KTextEditor::DocumentInfoInterface (### unfinished)
QString KateDocument::mimeType()
{
  KMimeType::Ptr result = KMimeType::defaultMimeTypePtr();

  // if the document has a URL, try KMimeType::findByURL
  if ( ! this->url().isEmpty() )
    result = KMimeType::findByUrl( this->url() );

  else if ( this->url().isEmpty() || ! this->url().isLocalFile() )
    result = mimeTypeForContent();

  return result->name();
}

KMimeType::Ptr KateDocument::mimeTypeForContent()
{
  QByteArray buf (1024,'\0');
  uint bufpos = 0;

  for (int i=0; i < lines(); ++i)
  {
    QString line = this->line( i );
    uint len = line.length() + 1;

    if (bufpos + len > 1024)
      len = 1024 - bufpos;

    QString ld (line + QChar::fromAscii('\n'));
    buf.replace(bufpos,len,ld.toLatin1()); //memcpy(buf.data() + bufpos, ld.toLatin1().constData(), len);

    bufpos += len;

    if (bufpos >= 1024)
      break;
  }
  buf.resize( bufpos );

  int accuracy = 0;
  KMimeType::Ptr mt = KMimeType::findByContent(buf, &accuracy);
  return mt ? mt : KMimeType::defaultMimeTypePtr();
}
//END KTextEditor::DocumentInfoInterface


//BEGIN KParts::ReadWrite stuff
bool KateDocument::openFile()
{
  // no open errors until now...
  setOpeningError(false);

  // add new m_file to dirwatch
  activateDirWatch ();

  //
  // mime type magic to get encoding right
  //
  QString mimeType = arguments().mimeType();
  int pos = mimeType.indexOf(';');
  if (pos != -1)
    setEncoding (mimeType.mid(pos+1));

  // do we have success ?
  emit KTextEditor::Document::textRemoved(this, documentRange());
  history()->doEdit( new KateEditInfo(this, Kate::CloseFileEdit, documentRange(), QStringList(), KTextEditor::Range(0,0,0,0), QStringList()) );

  bool success = m_buffer->openFile (localFilePath());

  emit KTextEditor::Document::textInserted(this, documentRange());
  history()->doEdit( new KateEditInfo(this, Kate::OpenFileEdit, KTextEditor::Range(0,0,0,0), QStringList(), documentRange(), QStringList()) );

  //
  // yeah, success
  //
  if (success)
  {
    // update file type
    updateFileType (KateGlobal::self()->modeManager()->fileType (this));

    // read dir config (if possible and wanted)
    readDirConfig ();

    // read vars
    readVariables();

    // update the md5 digest
    createDigest( m_digest );

    if (!m_postLoadFilterChecks.isEmpty())
    {
      LoadSaveFilterCheckPlugins *lscps=loadSaveFilterCheckPlugins();
      foreach(const QString& checkplugin, m_postLoadFilterChecks)
      {
         lscps->postLoadFilter(checkplugin,this);
      }
    }
  }

  // Inform that the text has changed (required as we're not inside the usual editStart/End stuff)
  emit textChanged (this);

  //
  // update views
  //
  foreach (KateView * view, m_views)
  {
    // This is needed here because inserting the text moves the view's start position (it is a SmartCursor)
    view->setCursorPosition(KTextEditor::Cursor());
    view->updateView(true);
  }

  //
  // emit the signal we need for example for kate app
  //
  emit documentUrlChanged (this);

  //
  // set doc name, dummy value as arg, don't need it
  //
  setDocName  (QString());

  //
  // to houston, we are not modified
  //
  if (m_modOnHd)
  {
    m_modOnHd = false;
    m_modOnHdReason = OnDiskUnmodified;
    emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
  }

  //
  // display errors
  //
  QWidget *parentWidget(dialogParent());

  if (!suppressOpeningErrorDialogs())
  {
    if (!success)
      KMessageBox::error (parentWidget, i18n ("The file %1 could not be loaded, as it was not possible to read from it.\n\nCheck if you have read access to this file.", this->url().pathOrUrl()));
  }

  if (!success) {
    setOpeningError(true);
    setOpeningErrorMessage(i18n ("The file %1 could not be loaded, as it was not possible to read from it.\n\nCheck if you have read access to this file.",this->url().pathOrUrl()));
  }

  // warn -> opened binary file!!!!!!!
  if (m_buffer->binary())
  {
    // this file can't be saved again without killing it
    setReadWrite( false );

    if(!suppressOpeningErrorDialogs())
      KMessageBox::information (parentWidget
        , i18n ("The file %1 is a binary, saving it will result in a corrupt file.", this->url().pathOrUrl())
        , i18n ("Binary File Opened")
        , "Binary File Opened Warning");

    setOpeningError(true);
    setOpeningErrorMessage(i18n ("The file %1 is a binary, saving it will result in a corrupt file.", this->url().pathOrUrl()));
  }

  // warn: opened broken utf-8 file...
  // only warn if not already a binary file!
  else if (m_buffer->brokenUTF8())
  {
    // this file can't be saved again without killing it
    setReadWrite( false );

    if (!suppressOpeningErrorDialogs())
      KMessageBox::information (parentWidget
        , i18n ("The file %1 was opened with UTF-8 encoding but contained invalid characters."
                " It is set to read-only mode, as saving might destroy its content."
                " Either reopen the file with the correct encoding chosen or enable the read-write mode again in the menu to be able to edit it.", this->url().pathOrUrl())
        , i18n ("Broken UTF-8 File Opened")
        , "Broken UTF-8 File Opened Warning");
    setOpeningError(true);
    setOpeningErrorMessage(i18n ("The file %1 was opened with UTF-8 encoding but contained invalid characters."
              " It is set to read-only mode, as saving might destroy its content."
              " Either reopen the file with the correct encoding chosen or enable the read-write mode again in the menu to be able to edit it.", this->url().pathOrUrl()));
  }

  //
  // return the success
  //
  return success;
}

bool KateDocument::saveFile()
{
  QWidget *parentWidget(dialogParent());

  //
  // warn -> try to save binary file!!!!!!!
  //
  if (m_buffer->binary() && (KMessageBox::warningContinueCancel (parentWidget
        , i18n ("The file %1 is a binary, saving it will result in a corrupt file.", url().pathOrUrl())
        , i18n ("Trying to Save Binary File")
        , KGuiItem(i18n("Save Nevertheless"))
        , KStandardGuiItem::cancel(), "Binary File Save Warning") != KMessageBox::Continue))
    return false;

  // some warnings, if file was changed by the outside!
  if ( !url().isEmpty() )
  {
    if (s_fileChangedDialogsActivated && m_modOnHd)
    {
      QString str = reasonedMOHString() + "\n\n";

      if (!isModified())
      {
        if (KMessageBox::warningContinueCancel(parentWidget,
               str + i18n("Do you really want to save this unmodified file? You could overwrite changed data in the file on disk."),i18n("Trying to Save Unmodified File"),KGuiItem(i18n("Save Nevertheless"))) != KMessageBox::Continue)
          return false;
      }
      else
      {
        if (KMessageBox::warningContinueCancel(parentWidget,
               str + i18n("Do you really want to save this file? Both your open file and the file on disk were changed. There could be some data lost."),i18n("Possible Data Loss"),KGuiItem(i18n("Save Nevertheless"))) != KMessageBox::Continue)
          return false;
      }
    }
  }

  //
  // can we encode it if we want to save it ?
  //
  if (!m_buffer->canEncode ()
       && (KMessageBox::warningContinueCancel(parentWidget,
           i18n("The selected encoding cannot encode every unicode character in this document. Do you really want to save it? There could be some data lost."),i18n("Possible Data Loss"),KGuiItem(i18n("Save Nevertheless"))) != KMessageBox::Continue))
  {
    return false;
  }

  //
  // try to create backup file..
  //

  // local file or not is here the question
  bool l ( url().isLocalFile() );

  // does the user want any backup, if not, not our problem?
  if ( ( l && config()->backupFlags() & KateDocumentConfig::LocalFiles )
       || ( ! l && config()->backupFlags() & KateDocumentConfig::RemoteFiles ) )
  {
    KUrl u( url() );
    u.setFileName( config()->backupPrefix() + url().fileName() + config()->backupSuffix() );

    kDebug( 13020 ) << "backup src file name: " << url();
    kDebug( 13020 ) << "backup dst file name: " << u;

    // handle the backup...
    bool backupSuccess = false;

    // local file mode, no kio
    if (u.isLocalFile ())
    {
      if (QFile::exists (url().toLocalFile ()))
      {
        // first: check if backupFile is already there, if true, unlink it
        QFile backupFile (u.toLocalFile ());
        if (backupFile.exists()) backupFile.remove ();

        backupSuccess = QFile::copy (url().toLocalFile (), u.toLocalFile ());
      }
      else
        backupSuccess = true;
    }
    else // remote file mode, kio
    {
      QWidget *w = widget ();
      if (!w && !m_views.isEmpty ())
        w = m_views.first();

      // get the right permissions, start with safe default
      mode_t  perms = 0600;
      KIO::UDSEntry fentry;
      if (KIO::NetAccess::stat (url(), fentry, kapp->activeWindow()))
      {
        kDebug( 13020 ) << "stating succesfull: " << url();
        KFileItem item (fentry, url());
        perms = item.permissions();

        // do a evil copy which will overwrite target if possible
        KIO::FileCopyJob *job = KIO::file_copy ( url(), u, -1, KIO::Overwrite );
        backupSuccess = KIO::NetAccess::synchronousRun(job, w);
      }
      else
        backupSuccess = true;
    }

    // backup has failed, ask user how to proceed
    if (!backupSuccess && (KMessageBox::warningContinueCancel (parentWidget
        , i18n ("For file %1 no backup copy could be created before saving."
                " If an error occurs while saving, you might lose the data of this file."
                " A reason could be that the media you write to is full or the directory of the file is read-only for you.", url().pathOrUrl())
        , i18n ("Failed to create backup copy.")
        , KGuiItem(i18n("Try to Save Nevertheless"))
        , KStandardGuiItem::cancel(), "Backup Failed Warning") != KMessageBox::Continue))
    {
      return false;
    }
  }

  // update file type
  updateFileType (KateGlobal::self()->modeManager()->fileType (this));

  if (!m_preSavePostDialogFilterChecks.isEmpty())
  {
    LoadSaveFilterCheckPlugins *lscps=loadSaveFilterCheckPlugins();
    foreach(const QString& checkplugin, m_preSavePostDialogFilterChecks)
    {
       if (lscps->preSavePostDialogFilterCheck(checkplugin,this,parentWidget)==false)
         return false;
    }
  }

  // remember the oldpath...
  QString oldPath = m_dirWatchFile;

  // remove file from dirwatch
  deactivateDirWatch ();

  //
  // try to save
  //
  if (!m_buffer->saveFile (localFilePath()))
  {
    // add m_file again to dirwatch
    activateDirWatch (oldPath);

    KMessageBox::error (parentWidget, i18n ("The document could not be saved, as it was not possible to write to %1.\n\nCheck that you have write access to this file or that enough disk space is available.", this->url().pathOrUrl()));

    return false;
  }

  // update the md5 digest
  createDigest( m_digest );

  // add m_file again to dirwatch
  activateDirWatch ();

  // update file type
//  updateFileType (KateGlobal::self()->modeManager()->fileType (this));

  // read dir config (if possible and wanted)
  if ( url().isLocalFile())
  {
    QFileInfo fo (oldPath), fn (m_dirWatchFile);

    if (fo.path() != fn.path())
      readDirConfig();
  }

  // read our vars
  readVariables();

  //
  // we are not modified
  //
  if (m_modOnHd)
  {
    m_modOnHd = false;
    m_modOnHdReason = OnDiskUnmodified;
    emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
  }

  // update document name...
  setDocName( QString() );

  // url may have changed...
  emit documentUrlChanged (this);


#warning IMPLEMENT ME
#if 0
  if (!m_postSaveFilterChecks.isEmpty())
  {
    LoadSaveFilterCheckPlugins *lscps1=loadSaveFilterCheckPlugins();
    foreach(const QString& checkplugin, m_postSaveFilterChecks)
    {
       if (lscps1->postSFilterCheck(checkplugin,this)==false)
         break;
    }
  }
#endif
  //
  // return success
  //
  return true;
}

void KateDocument::readDirConfig ()
{
  int depth = config()->searchDirConfigDepth ();

  if (this->url().isLocalFile() && (depth > -1))
  {
    QString currentDir = QFileInfo (localFilePath()).absolutePath();

    // only search as deep as specified or not at all ;)
    while (depth > -1)
    {
      //kDebug (13020) << "search for config file in path: " << currentDir;

      // try to open config file in this dir
      QFile f (currentDir + "/.kateconfig");

      if (f.open (QIODevice::ReadOnly))
      {
        QTextStream stream (&f);

        uint linesRead = 0;
        QString line = stream.readLine();
        while ((linesRead < 32) && !line.isNull())
        {
          readVariableLine( line );

          line = stream.readLine();

          linesRead++;
        }

        break;
      }

      QString newDir = QFileInfo (currentDir).absolutePath();

      // bail out on looping (for example reached /)
      if (currentDir == newDir)
        break;

      currentDir = newDir;
      --depth;
    }
  }
}

void KateDocument::activateDirWatch (const QString &useFileName)
{
  QString fileToUse = useFileName;
  if (fileToUse.isEmpty())
    fileToUse = localFilePath();

  // same file as we are monitoring, return
  if (fileToUse == m_dirWatchFile)
    return;

  // remove the old watched file
  deactivateDirWatch ();

  // add new file if needed
  if (url().isLocalFile() && !fileToUse.isEmpty())
  {
    KateGlobal::self()->dirWatch ()->addFile (fileToUse);
    m_dirWatchFile = fileToUse;
  }
}

void KateDocument::deactivateDirWatch ()
{
  if (!m_dirWatchFile.isEmpty())
    KateGlobal::self()->dirWatch ()->removeFile (m_dirWatchFile);

  m_dirWatchFile.clear();
}

bool KateDocument::closeUrl()
{
  //
  // file mod on hd
  //
  if ( !m_reloading && !url().isEmpty() )
  {
    if (s_fileChangedDialogsActivated && m_modOnHd)
    {
      QWidget *parentWidget(dialogParent());

      if (!(KMessageBox::warningContinueCancel(
            parentWidget,
            reasonedMOHString() + "\n\n" + i18n("Do you really want to continue to close this file? Data loss may occur."),
            i18n("Possible Data Loss"), KGuiItem(i18n("Close Nevertheless")), KStandardGuiItem::cancel(),
            QString("kate_close_modonhd_%1").arg( m_modOnHdReason ) ) == KMessageBox::Continue))
        return false;
    }
  }

  //
  // first call the normal kparts implementation
  //
  if (!KParts::ReadWritePart::closeUrl ())
    return false;

  // Tell the world that we're about to go ahead with the close
  if (!m_reloading)
    emit aboutToClose(this);

  // remove file from dirwatch
  deactivateDirWatch ();

  //
  // empty url + fileName
  //
  setUrl(KUrl());
  setLocalFilePath(QString());

  // we are not modified
  if (m_modOnHd)
  {
    m_modOnHd = false;
    m_modOnHdReason = OnDiskUnmodified;
    emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
  }

  emit KTextEditor::Document::textRemoved(this, documentRange());
  history()->doEdit( new KateEditInfo(this, Kate::CloseFileEdit, documentRange(), QStringList(), KTextEditor::Range(0,0,0,0), QStringList()) );

  // clear the buffer
  m_buffer->clear();

  // remove all marks
  clearMarks ();

  // clear undo/redo history
  clearUndo();
  clearRedo();

  // no, we are no longer modified
  setModified(false);

  // we have no longer any hl
  m_buffer->setHighlight(0);

  // update all our views
  foreach (KateView * view, m_views )
  {
    view->clearSelection(); // fix bug #118588
    view->clear();
  }

  // uh, fileName changed
  emit documentUrlChanged (this);

  // update doc name
  setDocName (QString());

  // success
  return true;
}

void KateDocument::setReadWrite( bool rw )
{
  if (isReadWrite() != rw)
  {
    KParts::ReadWritePart::setReadWrite (rw);

    foreach( KateView* view, m_views)
    {
      view->slotUpdate();
      view->slotReadWriteChanged ();
    }
  }
}

void KateDocument::setModified(bool m) {

  if (isModified() != m) {
    KParts::ReadWritePart::setModified (m);

    foreach( KateView* view,m_views)
    {
      view->slotUpdate();
    }

    emit modifiedChanged (this);
  }
  if ( m == false )
  {
    if ( ! undoItems.isEmpty() )
    {
      lastUndoGroupWhenSaved = undoItems.last();
    }

    if ( ! redoItems.isEmpty() )
    {
      lastRedoGroupWhenSaved = redoItems.last();
    }

    docWasSavedWhenUndoWasEmpty = undoItems.isEmpty();
    docWasSavedWhenRedoWasEmpty = redoItems.isEmpty();
  }
}
//END

//BEGIN Kate specific stuff ;)

void KateDocument::makeAttribs(bool needInvalidate)
{
  foreach(KateView *view,m_views)
    view->renderer()->updateAttributes ();

  if (needInvalidate)
    m_buffer->invalidateHighlighting();

  tagAll ();
}

// the attributes of a hl have changed, update
void KateDocument::internalHlChanged()
{
  makeAttribs();
}

void KateDocument::addView(KTextEditor::View *view) {
  if (!view)
    return;

  m_views.append( static_cast<KateView*>(view) );
  m_textEditViews.append( view );

  foreach(KTextEditor::SmartRange* highlight, m_documentHighlights) {
    Q_ASSERT(dynamic_cast<KateView*>(view));
    static_cast<KateView*>(view)->addExternalHighlight(highlight, m_documentDynamicHighlights.contains(highlight));
}

  // apply the view & renderer vars from the file type
  if (!m_fileType.isEmpty())
      readVariableLine(KateGlobal::self()->modeManager()->fileType(m_fileType).varLine, true);

  // apply the view & renderer vars from the file
  readVariables (true);

  setActiveView(view);
}

void KateDocument::removeView(KTextEditor::View *view) {
  if (!view)
    return;

  if (activeView() == view)
    setActiveView(0L);

  m_views.removeAll( (KateView *) view );
  m_textEditViews.removeAll( view  );
}

void KateDocument::setActiveView(KTextEditor::View* view)
{
  if ( m_activeView == view ) return;

  if (m_activeView) {
    disconnect(m_activeView, SIGNAL(selectionChanged(KTextEditor::View*)), this, SIGNAL(activeViewSelectionChanged(KTextEditor::View*)));
  }

  m_activeView = (KateView*)view;

  if (m_activeView) {
    connect(m_activeView, SIGNAL(selectionChanged(KTextEditor::View*)), SIGNAL(activeViewSelectionChanged(KTextEditor::View*)));
  }
}

bool KateDocument::ownedView(KateView *view) {
  // do we own the given view?
  return (m_views.contains(view));
}

uint KateDocument::toVirtualColumn( const KTextEditor::Cursor& cursor )
{
  KateTextLine::Ptr textLine = m_buffer->plainLine(cursor.line());

  if (textLine)
    return textLine->toVirtualColumn(cursor.column(), config()->tabWidth());
  else
    return 0;
}

bool KateDocument::typeChars ( KateView *view, const QString &chars )
{
  // Because we want to access text before starting an edit, lock the smart mutex now
  QMutexLocker l(smartMutex());

  KateTextLine::Ptr textLine = m_buffer->plainLine(view->cursorPosition().line ());

  if (!textLine)
    return false;

  bool bracketInserted = false;
  QString buf;
  QChar c;
  for( int z = 0; z < chars.length(); z++ )
  {
    QChar ch = c = chars[z];

    if (ch.isPrint() || ch == QChar::fromAscii('\t'))
    {
      buf.append (ch);

      if (!bracketInserted && (config()->configFlags() & KateDocumentConfig::cfAutoBrackets))
      {
        QChar end_ch;
        bool complete = true;
        QChar prevChar = textLine->at(view->cursorPosition().column()-1);
        QChar nextChar = textLine->at(view->cursorPosition().column());
        switch(ch.toAscii()) {
          case '(': end_ch = ')'; break;
          case '[': end_ch = ']'; break;
          case '{': end_ch = '}'; break;
          case '\'':end_ch = '\'';break;
          case '"': end_ch = '"'; break;
          default: complete = false;
        }
        if (complete)
        {
          if (view->selection())
          { // there is a selection, enclose the selection
            buf.append (view->selectionText());
            buf.append (end_ch);
            bracketInserted = true;
          }
          else
          { // no selection, check whether we should better refuse to complete
            if ( ( (ch == '\'' || ch == '"') &&
                   (prevChar.isLetterOrNumber() || prevChar == ch) )
              || nextChar.isLetterOrNumber()
              || (nextChar == end_ch && prevChar != ch) )
            {
              kDebug(13020) << "AutoBracket refused before: " << nextChar << "\n";
            }
            else
            {
              buf.append (end_ch);
              bracketInserted = true;
            }
          }
        }
      }
    }
  }

  if (buf.isEmpty())
    return false;

  l.unlock(); //editStart will lock the smart-mutex again, and it must be un-locked within editEnd. So unlock here.

  editStart ();

  if (!view->config()->persistentSelection() && view->selection() )
    view->removeSelectedText();

  KTextEditor::Cursor oldCur (view->cursorPosition());

  if (config()->configFlags()  & KateDocumentConfig::cfOvr)
    removeText(KTextEditor::Range(view->cursorPosition(), qMin(buf.length(), textLine->length() - view->cursorPosition().column())));

  insertText(view->cursorPosition(), buf);
  KTextEditor::Cursor b(view->cursorPosition());
  m_indenter.userTypedChar (view, b, c);

  editEnd ();

  if (bracketInserted)
    view->setCursorPositionInternal (view->cursorPosition() - KTextEditor::Cursor(0,1));

  view->slotTextInserted (view, oldCur, chars);
  return true;
}

void KateDocument::newLine( KateView *v )
{
  editStart();

  if( !v->config()->persistentSelection() && v->selection() )
    v->removeSelectedText();

  // query cursor position
  KTextEditor::Cursor c = v->cursorPosition();

  if (c.line() > (int)lastLine())
    c.setLine(lastLine());

  if (c.line() < 0)
    c.setLine(0);

  uint ln = c.line();

  KateTextLine::Ptr textLine = plainKateTextLine(ln);

  if (c.column() > (int)textLine->length())
    c.setColumn(textLine->length());

  // first: wrap line
  editWrapLine (c.line(), c.column());

  // second: indent the new line, if needed...
  m_indenter.userTypedChar(v, v->cursorPosition(), '\n');

  removeTrailingSpace( ln );

  editEnd();
}

void KateDocument::transpose( const KTextEditor::Cursor& cursor)
{
  KateTextLine::Ptr textLine = m_buffer->plainLine(cursor.line());

  if (!textLine || (textLine->length() < 2))
    return;

  uint col = cursor.column();

  if (col > 0)
    col--;

  if ((textLine->length() - col) < 2)
    return;

  uint line = cursor.line();
  QString s;

  //clever swap code if first character on the line swap right&left
  //otherwise left & right
  s.append (textLine->at(col+1));
  s.append (textLine->at(col));
  //do the swap

  // do it right, never ever manipulate a textline
  editStart ();
  editRemoveText (line, col, 2);
  editInsertText (line, col, s);
  editEnd ();
}

void KateDocument::backspace( KateView *view, const KTextEditor::Cursor& c )
{
  if ( !view->config()->persistentSelection() && view->selection() ) {
    view->removeSelectedText();
    return;
  }

  uint col = qMax( c.column(), 0 );
  uint line = qMax( c.line(), 0 );

  if ((col == 0) && (line == 0))
    return;

  int complement = 0;
  if (col > 0)
  {
    if (config()->configFlags() & KateDocumentConfig::cfAutoBrackets)
    {
      // if inside empty (), {}, [], '', "" delete both
      KateTextLine::Ptr tl = m_buffer->plainLine(line);
      if(!tl) return;
      QChar prevChar = tl->at(col-1);
      QChar nextChar = tl->at(col);

      if ( (prevChar == '"' && nextChar == '"') ||
           (prevChar == '\'' && nextChar == '\'') ||
           (prevChar == '(' && nextChar == ')') ||
           (prevChar == '[' && nextChar == ']') ||
           (prevChar == '{' && nextChar == '}') )
      {
        complement = 1;
      }
    }
    if (!(config()->configFlags() & KateDocumentConfig::cfBackspaceIndents))
    {
      // ordinary backspace
      //c.cursor.col--;
      removeText(KTextEditor::Range(line, col-1, line, col+complement));
    }
    else
    {
      // backspace indents: erase to next indent position
      KateTextLine::Ptr textLine = m_buffer->plainLine(line);

      // don't forget this check!!!! really!!!!
      if (!textLine)
        return;

      int colX = textLine->toVirtualColumn(col, config()->tabWidth());
      int pos = textLine->firstChar();
      if (pos > 0)
        pos = textLine->toVirtualColumn(pos, config()->tabWidth());

      if (pos < 0 || pos >= (int)colX)
      {
        // only spaces on left side of cursor
        indent( view, line, -1);
      }
      else
        removeText(KTextEditor::Range(line, col-1, line, col+complement));
    }
  }
  else
  {
    // col == 0: wrap to previous line
    if (line >= 1)
    {
      KateTextLine::Ptr textLine = m_buffer->plainLine(line-1);

      // don't forget this check!!!! really!!!!
      if (!textLine)
        return;

      if (config()->wordWrap() && textLine->endsWith(QLatin1String(" ")))
      {
        // gg: in hard wordwrap mode, backspace must also eat the trailing space
        removeText (KTextEditor::Range(line-1, textLine->length()-1, line, 0));
      }
      else
        removeText (KTextEditor::Range(line-1, textLine->length(), line, 0));
    }
  }
}

void KateDocument::del( KateView *view, const KTextEditor::Cursor& c )
{
  if ( !view->config()->persistentSelection() && view->selection() ) {
    view->removeSelectedText();
    return;
  }

  if( c.column() < (int) m_buffer->plainLine(c.line())->length())
  {
    removeText(KTextEditor::Range(c, 1));
  }
  else if ( c.line() < lastLine() )
  {
    removeText(KTextEditor::Range(c.line(), c.column(), c.line()+1, 0));
  }
}

void KateDocument::paste ( KateView* view, QClipboard::Mode mode )
{
  QString s = QApplication::clipboard()->text(mode);

  if (s.isEmpty())
    return;

  int lines = s.count (QChar::fromAscii ('\n'));

  m_undoDontMerge = true;

  editStart (true, Kate::CutCopyPasteEdit);

  if (!view->config()->persistentSelection() && view->selection() )
    view->removeSelectedText();

  KTextEditor::Cursor pos = view->cursorPosition();

  blockRemoveTrailingSpaces(true);
  insertText(pos, s, view->blockSelectionMode());
  blockRemoveTrailingSpaces(false);

  for (int i = pos.line(); i < pos.line() + lines; ++i)
    removeTrailingSpace(i);

  editEnd();

  // move cursor right for block select, as the user is moved right internal
  // even in that case, but user expects other behavior in block selection
  // mode !
  if (view->blockSelectionMode())
    view->setCursorPositionInternal(pos + KTextEditor::Cursor(lines, 0));

  if (config()->configFlags() & KateDocumentConfig::cfIndentPastedText)
  {
    KTextEditor::Range range = KTextEditor::Range(KTextEditor::Cursor(pos.line(), 0),
                                                  KTextEditor::Cursor(pos.line() + lines, 0));

    int start = view->selectionRange().start().line();
    const int end = view->selectionRange().end().line();

    editStart();

    blockRemoveTrailingSpaces(true);
    m_indenter.indent(view, range);
    blockRemoveTrailingSpaces(false);

    for (; start <= end; ++start)
      removeTrailingSpace(start);

    editEnd();
  }

  if (!view->blockSelectionMode()) emit charactersSemiInteractivelyInserted (pos, s);
  m_undoDontMerge = true;
}

void KateDocument::indent ( KateView *v, uint line, int change)
{
  // dominik: if there is a selection, iterate afterwards over all lines and
  // remove trailing spaces
  const bool hasSelection = v->selection();
  int start = v->selectionRange().start().line();
  const int end = v->selectionRange().end().line();

  KTextEditor::Range range = hasSelection ? v->selectionRange() : KTextEditor::Range (KTextEditor::Cursor (line,0), KTextEditor::Cursor (line,0));

  editStart();
  blockRemoveTrailingSpaces(true);
  m_indenter.changeIndent(v, range, change);
  blockRemoveTrailingSpaces(false);

  if (hasSelection) {
    for (; start <= end; ++start)
      removeTrailingSpace(start);
  }
  editEnd();
}

void KateDocument::align(KateView *view, uint line)
{
  const bool hasSelection = view->selection();
  KTextEditor::Range range = hasSelection ? view->selectionRange() : KTextEditor::Range (KTextEditor::Cursor (line,0), KTextEditor::Cursor (line,0));

  int start = view->selectionRange().start().line();
  const int end = view->selectionRange().end().line();

  blockRemoveTrailingSpaces(true);
  m_indenter.indent(view,range);
  blockRemoveTrailingSpaces(false);

  for (; start <= end; ++start)
    removeTrailingSpace(start);

  editEnd();
}

/*
  Remove a given string at the beginning
  of the current line.
*/
bool KateDocument::removeStringFromBeginning(int line, const QString &str)
{
  KateTextLine::Ptr textline = m_buffer->plainLine(line);

  KTextEditor::Cursor cursor (line, 0);
  bool there = textline->startsWith(str);

  if (!there)
  {
    cursor.setColumn(textline->firstChar());
    there = textline->matchesAt(cursor.column(), str);
  }

  if (there)
  {
    // Remove some chars
    removeText (KTextEditor::Range(cursor, str.length()));
  }

  return there;
}

/*
  Remove a given string at the end
  of the current line.
*/
bool KateDocument::removeStringFromEnd(int line, const QString &str)
{
  KateTextLine::Ptr textline = m_buffer->plainLine(line);

  KTextEditor::Cursor cursor (line, 0);
  bool there = textline->endsWith(str);

  if (there)
  {
    cursor.setColumn(textline->length() - str.length());
  }
  else
  {
    cursor.setColumn(textline->lastChar() - str.length() + 1);
    there = textline->matchesAt(cursor.column(), str);
  }

  if (there)
  {
    // Remove some chars
    removeText (KTextEditor::Range(cursor, str.length()));
  }

  return there;
}

/*
  Add to the current line a comment line mark at the beginning.
*/
void KateDocument::addStartLineCommentToSingleLine( int line, int attrib )
{
  QString commentLineMark = highlight()->getCommentSingleLineStart(attrib);
  int pos = -1;

  if (highlight()->getCommentSingleLinePosition(attrib) == KateHighlighting::CSLPosColumn0)
  {
    pos = 0;
    commentLineMark += ' ';
  } else {
    const KateTextLine::Ptr l = m_buffer->line(line);
    pos = l->firstChar();
  }

  if (pos >= 0)
    insertText (KTextEditor::Cursor(line, pos), commentLineMark);
}

/*
  Remove from the current line a comment line mark at
  the beginning if there is one.
*/
bool KateDocument::removeStartLineCommentFromSingleLine( int line, int attrib )
{
  const QString shortCommentMark = highlight()->getCommentSingleLineStart( attrib );
  const QString longCommentMark = shortCommentMark + ' ';

  editStart();

  // Try to remove the long comment mark first
  bool removed = (removeStringFromBeginning(line, longCommentMark)
               || removeStringFromBeginning(line, shortCommentMark));

  editEnd();

  return removed;
}

/*
  Add to the current line a start comment mark at the
  beginning and a stop comment mark at the end.
*/
void KateDocument::addStartStopCommentToSingleLine( int line, int attrib )
{
  const QString startCommentMark = highlight()->getCommentStart( attrib ) + ' ';
  const QString stopCommentMark = ' ' + highlight()->getCommentEnd( attrib );

  editStart();

  // Add the start comment mark
  insertText (KTextEditor::Cursor(line, 0), startCommentMark);

  // Go to the end of the line
  const int col = m_buffer->plainLine(line)->length();

  // Add the stop comment mark
  insertText (KTextEditor::Cursor(line, col), stopCommentMark);

  editEnd();
}

/*
  Remove from the current line a start comment mark at
  the beginning and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSingleLine( int line, int attrib )
{
  QString shortStartCommentMark = highlight()->getCommentStart( attrib );
  QString longStartCommentMark = shortStartCommentMark + ' ';
  QString shortStopCommentMark = highlight()->getCommentEnd( attrib );
  QString longStopCommentMark = ' ' + shortStopCommentMark;

  editStart();

#ifdef __GNUC__
#warning "that's a bad idea, can lead to stray endings, FIXME"
#endif
  // Try to remove the long start comment mark first
  bool removedStart = (removeStringFromBeginning(line, longStartCommentMark)
                    || removeStringFromBeginning(line, shortStartCommentMark));

  bool removedStop = false;
  if (removedStart)
  {
    // Try to remove the long stop comment mark first
    removedStop = (removeStringFromEnd(line, longStopCommentMark)
                || removeStringFromEnd(line, shortStopCommentMark));
  }

  editEnd();

  return (removedStart || removedStop);
}

/*
  Add to the current selection a start comment mark at the beginning
  and a stop comment mark at the end.
*/
void KateDocument::addStartStopCommentToSelection( KateView *view, int attrib )
{
  const QString startComment = highlight()->getCommentStart( attrib );
  const QString endComment = highlight()->getCommentEnd( attrib );

  KTextEditor::Range range = view->selectionRange();

  if ((range.end().column() == 0) && (range.end().line() > 0))
    range.end().setPosition(range.end().line() - 1, lineLength(range.end().line() - 1));

  editStart();

  insertText (range.end(), endComment);
  insertText (range.start(), startComment);

  editEnd ();
  // selection automatically updated (KateSmartRange)
}

/*
  Add to the current selection a comment line mark at the beginning of each line.
*/
void KateDocument::addStartLineCommentToSelection( KateView *view, int attrib )
{
  const QString commentLineMark = highlight()->getCommentSingleLineStart( attrib ) + ' ';

  int sl = view->selectionRange().start().line();
  int el = view->selectionRange().end().line();

  // if end of selection is in column 0 in last line, omit the last line
  if ((view->selectionRange().end().column() == 0) && (el > 0))
  {
    el--;
  }

  editStart();

  // For each line of the selection
  for (int z = el; z >= sl; z--) {
    //insertText (z, 0, commentLineMark);
    addStartLineCommentToSingleLine(z, attrib );
  }

  editEnd ();
  // selection automatically updated (KateSmartRange)
}

bool KateDocument::nextNonSpaceCharPos(int &line, int &col)
{
  for(; line < (int)m_buffer->count(); line++) {
    KateTextLine::Ptr textLine = m_buffer->plainLine(line);

    if (!textLine)
      break;

    col = textLine->nextNonSpaceChar(col);
    if(col != -1)
      return true; // Next non-space char found
    col = 0;
  }
  // No non-space char found
  line = -1;
  col = -1;
  return false;
}

bool KateDocument::previousNonSpaceCharPos(int &line, int &col)
{
  while(true)
  {
    KateTextLine::Ptr textLine = m_buffer->plainLine(line);

    if (!textLine)
      break;

    col = textLine->previousNonSpaceChar(col);
    if(col != -1) return true;
    if(line == 0) return false;
    --line;
    col = textLine->length();
  }
  // No non-space char found
  line = -1;
  col = -1;
  return false;
}

/*
  Remove from the selection a start comment mark at
  the beginning and a stop comment mark at the end.
*/
bool KateDocument::removeStartStopCommentFromSelection( KateView *view, int attrib )
{
  const QString startComment = highlight()->getCommentStart( attrib );
  const QString endComment = highlight()->getCommentEnd( attrib );

  int sl = qMax<int> (0, view->selectionRange().start().line());
  int el = qMin<int>  (view->selectionRange().end().line(), lastLine());
  int sc = view->selectionRange().start().column();
  int ec = view->selectionRange().end().column();

  // The selection ends on the char before selectEnd
  if (ec != 0) {
    --ec;
  } else if (el > 0) {
    --el;
    ec = m_buffer->plainLine(el)->length() - 1;
  }

  const int startCommentLen = startComment.length();
  const int endCommentLen = endComment.length();

  // had this been perl or sed: s/^\s*$startComment(.+?)$endComment\s*/$2/

  bool remove = nextNonSpaceCharPos(sl, sc)
      && m_buffer->plainLine(sl)->matchesAt(sc, startComment)
      && previousNonSpaceCharPos(el, ec)
      && ( (ec - endCommentLen + 1) >= 0 )
      && m_buffer->plainLine(el)->matchesAt(ec - endCommentLen + 1, endComment);

  if (remove) {
    editStart();

    removeText (KTextEditor::Range(el, ec - endCommentLen + 1, el, ec + 1));
    removeText (KTextEditor::Range(sl, sc, sl, sc + startCommentLen));

    editEnd ();
    // selection automatically updated (KateSmartRange)
  }

  return remove;
}

bool KateDocument::removeStartStopCommentFromRegion(const KTextEditor::Cursor &start,const KTextEditor::Cursor &end,int attrib)
{
  const QString startComment = highlight()->getCommentStart( attrib );
  const QString endComment = highlight()->getCommentEnd( attrib );
  const int startCommentLen = startComment.length();
  const int endCommentLen = endComment.length();

  const bool remove = m_buffer->plainLine(start.line())->matchesAt(start.column(), startComment)
                   && m_buffer->plainLine(end.line())->matchesAt(end.column() - endCommentLen , endComment);
  if (remove) {
    editStart();
      removeText(KTextEditor::Range(end.line(), end.column() - endCommentLen, end.line(), end.column()));
      removeText(KTextEditor::Range(start, startCommentLen));
    editEnd();
  }
  return remove;
}

/*
  Remove from the beginning of each line of the
  selection a start comment line mark.
*/
bool KateDocument::removeStartLineCommentFromSelection( KateView *view, int attrib )
{
  const QString shortCommentMark = highlight()->getCommentSingleLineStart( attrib );
  const QString longCommentMark = shortCommentMark + ' ';

  int sl = view->selectionRange().start().line();
  int el = view->selectionRange().end().line();

  if ((view->selectionRange().end().column() == 0) && (el > 0))
  {
    el--;
  }

  bool removed = false;

  editStart();

  // For each line of the selection
  for (int z = el; z >= sl; z--)
  {
    // Try to remove the long comment mark first
    removed = (removeStringFromBeginning(z, longCommentMark)
            || removeStringFromBeginning(z, shortCommentMark)
            || removed);
  }

  editEnd();
  // selection automatically updated (KateSmartRange)

  return removed;
}

/*
  Comment or uncomment the selection or the current
  line if there is no selection.
*/
void KateDocument::comment( KateView *v, uint line,uint column, int change)
{
  // We need to check that we can sanely comment the selectino or region.
  // It is if the attribute of the first and last character of the range to
  // comment belongs to the same language definition.
  // for lines with no text, we need the attribute for the lines context.
  bool hassel = v->selection();
  int startAttrib, endAttrib;
  if ( hassel )
  {
    KateTextLine::Ptr ln = kateTextLine( v->selectionRange().start().line() );
    int l = v->selectionRange().start().line(), c = v->selectionRange().start().column();
    startAttrib = nextNonSpaceCharPos( l, c ) ? kateTextLine( l )->attribute( c ) : 0;

    ln = kateTextLine( v->selectionRange().end().line() );
    l = v->selectionRange().end().line(), c = v->selectionRange().end().column();
    endAttrib = previousNonSpaceCharPos( l, c ) ? kateTextLine( l )->attribute( c ) : 0;
  }
  else
  {
    KateTextLine::Ptr ln = kateTextLine( line );
    if ( ln->length() )
    {
      startAttrib = ln->attribute( ln->firstChar() );
      endAttrib = ln->attribute( ln->lastChar() );
    }
    else
    {
      int l = line, c = 0;
      if ( nextNonSpaceCharPos( l, c )  || previousNonSpaceCharPos( l, c ) )
        startAttrib = endAttrib = kateTextLine( l )->attribute( c );
      else
        startAttrib = endAttrib = 0;
    }
  }

  if ( ! highlight()->canComment( startAttrib, endAttrib ) )
  {
    kDebug(13020)<<"canComment( "<<startAttrib<<", "<<endAttrib<<" ) returned false!";
    return;
  }

  bool hasStartLineCommentMark = !(highlight()->getCommentSingleLineStart( startAttrib ).isEmpty());
  bool hasStartStopCommentMark = ( !(highlight()->getCommentStart( startAttrib ).isEmpty())
      && !(highlight()->getCommentEnd( endAttrib ).isEmpty()) );

  bool removed = false;

  if (change > 0) // comment
  {
    if ( !hassel )
    {
      if ( hasStartLineCommentMark )
        addStartLineCommentToSingleLine( line, startAttrib );
      else if ( hasStartStopCommentMark )
        addStartStopCommentToSingleLine( line, startAttrib );
    }
    else
    {
      // anders: prefer single line comment to avoid nesting probs
      // If the selection starts after first char in the first line
      // or ends before the last char of the last line, we may use
      // multiline comment markers.
      // TODO We should try to detect nesting.
      //    - if selection ends at col 0, most likely she wanted that
      // line ignored
      if ( hasStartStopCommentMark &&
           ( !hasStartLineCommentMark || (
           ( v->selectionRange().start().column() > m_buffer->plainLine( v->selectionRange().start().line() )->firstChar() ) ||
           ( v->selectionRange().end().column() < ((int)m_buffer->plainLine( v->selectionRange().end().line() )->length()) )
         ) ) )
        addStartStopCommentToSelection( v, startAttrib );
      else if ( hasStartLineCommentMark )
        addStartLineCommentToSelection( v, startAttrib );
    }
  }
  else // uncomment
  {
    if ( !hassel )
    {
      removed = ( hasStartLineCommentMark
                  && removeStartLineCommentFromSingleLine( line, startAttrib ) )
        || ( hasStartStopCommentMark
             && removeStartStopCommentFromSingleLine( line, startAttrib ) );
      if ((!removed) && foldingTree()) {
        kDebug(13020)<<"easy approach for uncommenting did not work, trying harder (folding tree)";
        int commentRegion=(highlight()->commentRegion(startAttrib));
        if (commentRegion){
           KateCodeFoldingNode *n=foldingTree()->findNodeForPosition(line,column);
           if (n) {
            KTextEditor::Cursor start,end;
            if ((n->nodeType()==(int)commentRegion) && n->getBegin(foldingTree(), &start) && n->getEnd(foldingTree(), &end)) {
                kDebug(13020)<<"Enclosing region found:"<<start.column()<<"/"<<start.line()<<"-"<<end.column()<<"/"<<end.line();
                removeStartStopCommentFromRegion(start,end,startAttrib);
             } else {
                  kDebug(13020)<<"Enclosing region found, but not valid";
                  kDebug(13020)<<"Region found: "<<n->nodeType()<<" region needed: "<<commentRegion;
             }
            //perhaps nested regions should be hadled here too...
          } else kDebug(13020)<<"No enclosing region found";
        } else kDebug(13020)<<"No comment region specified for current hl";
      }
    }
    else
    {
      // anders: this seems like it will work with above changes :)
      removed = ( hasStartLineCommentMark
          && removeStartLineCommentFromSelection( v, startAttrib ) )
        || ( hasStartStopCommentMark
          && removeStartStopCommentFromSelection( v, startAttrib ) );
    }
  }
}

void KateDocument::transform( KateView *v, const KTextEditor::Cursor &c,
                            KateDocument::TextTransform t )
{
  editStart();
  KTextEditor::Cursor cursor = c;

  if ( v->selection() )
  {
    // cache the selection and cursor, so we can be sure to restore.
    KTextEditor::Range selection = v->selectionRange();

    KTextEditor::Range range(selection.start(), 0);
    while ( range.start().line() <= selection.end().line() )
    {
      int start = 0;
      int end = lineLength( range.start().line() );

      if (range.start().line() == selection.start().line() || v->blockSelectionMode())
        start = selection.start().column();

      if (range.start().line() == selection.end().line() || v->blockSelectionMode())
        end = selection.end().column();

      if ( start > end )
      {
        int swapCol = start;
        start = end;
        end = swapCol;
      }
      range.start().setColumn( start );
      range.end().setColumn( end );

      QString s = text( range );
      QString old = s;

      if ( t == Uppercase )
        s = s.toUpper();
      else if ( t == Lowercase )
        s = s.toLower();
      else // Capitalize
      {
        KateTextLine::Ptr l = m_buffer->plainLine( range.start().line() );
        int p ( 0 );
        while( p < s.length() )
        {
          // If bol or the character before is not in a word, up this one:
          // 1. if both start and p is 0, upper char.
          // 2. if blockselect or first line, and p == 0 and start-1 is not in a word, upper
          // 3. if p-1 is not in a word, upper.
          if ( ( ! range.start().column() && ! p ) ||
                   ( ( range.start().line() == selection.start().line() || v->blockSelectionMode() ) &&
                   ! p && ! highlight()->isInWord( l->at( range.start().column() - 1 )) ) ||
                   ( p && ! highlight()->isInWord( s.at( p-1 ) ) )
             )
            s[p] = s.at(p).toUpper();
          p++;
        }
      }

      if ( s != old )
      {
        removeText( range );
        insertText( range.start(), s );
      }

      range.setBothLines(range.start().line() + 1);
    }

    // restore selection
    v->setSelection( selection );

  } else {  // no selection
    QString old = text( KTextEditor::Range(cursor, 1) );
    QString s;
    switch ( t ) {
      case Uppercase:
      s = old.toUpper();
      break;
      case Lowercase:
      s = old.toLower();
      break;
      case Capitalize:
      {
        KateTextLine::Ptr l = m_buffer->plainLine( cursor.line() );
        while ( cursor.column() > 0 && highlight()->isInWord( l->at( cursor.column() - 1 ), l->attribute( cursor.column() - 1 ) ) )
          cursor.setColumn(cursor.column() - 1);
        old = text( KTextEditor::Range(cursor, 1) );
        s = old.toUpper();
      }
      break;
      default:
      break;
    }
    if ( s != old )
    {
      removeText( KTextEditor::Range(cursor, 1) );
      insertText( cursor, s );
    }
  }

  editEnd();

  v->setCursorPosition( c );
}

void KateDocument::joinLines( uint first, uint last )
{
//   if ( first == last ) last += 1;
  editStart();
  int line( first );
  while ( first < last )
  {
    // Normalize the whitespace in the joined lines by making sure there's
    // always exactly one space between the joined lines
    // This cannot be done in editUnwrapLine, because we do NOT want this
    // behavior when deleting from the start of a line, just when explicitly
    // calling the join command
    KateTextLine::Ptr l = m_buffer->line( line );
    KateTextLine::Ptr tl = m_buffer->line( line + 1 );

    if ( !l || !tl )
    {
      editEnd();
      return;
    }

    int pos = tl->firstChar();
    if ( pos >= 0 )
    {
      if (pos != 0)
        editRemoveText( line + 1, 0, pos );
      if ( !( l->length() == 0 || l->at( l->length() - 1 ).isSpace() ) )
        editInsertText( line + 1, 0, " " );
    }
    else
    {
      // Just remove the whitespace and let Kate handle the rest
      editRemoveText( line + 1, 0, tl->length() );
    }

    editUnWrapLine( line );
    first++;
  }
  editEnd();
}

QString KateDocument::getWord( const KTextEditor::Cursor& cursor )
{
  int start, end, len;

  KateTextLine::Ptr textLine = m_buffer->plainLine(cursor.line());
  len = textLine->length();
  start = end = cursor.column();
  if (start > len)        // Probably because of non-wrapping cursor mode.
    return QString("");

  while (start > 0 && highlight()->isInWord(textLine->at(start - 1), textLine->attribute(start - 1))) start--;
  while (end < len && highlight()->isInWord(textLine->at(end), textLine->attribute(end))) end++;
  len = end - start;
  return textLine->string().mid(start, len);
}

void KateDocument::tagLines(int start, int end)
{
  foreach(KateView *view,m_views)
    view->tagLines (start, end, true);
}

void KateDocument::tagLines(KTextEditor::Cursor start, KTextEditor::Cursor end)
{
  // May need to switch start/end cols if in block selection mode
/*  if (blockSelectionMode() && start.column() > end.column()) {
    int sc = start.column();
    start.setColumn(end.column());
    end.setColumn(sc);
  }
*/
  foreach (KateView* view, m_views)
    view->tagLines(start, end, true);
}

void KateDocument::repaintViews(bool paintOnlyDirty)
{
  foreach(KateView *view,m_views)
    view->repaintText(paintOnlyDirty);
}

void KateDocument::tagAll()
{
  foreach(KateView *view,m_views)
  {
    view->tagAll();
    view->updateView (true);
  }
}

inline bool isStartBracket( const QChar& c ) { return c == '{' || c == '[' || c == '('; }
inline bool isEndBracket  ( const QChar& c ) { return c == '}' || c == ']' || c == ')'; }
inline bool isBracket     ( const QChar& c ) { return isStartBracket( c ) || isEndBracket( c ); }

/*
   Bracket matching uses the following algorithm:
   If in overwrite mode, match the bracket currently underneath the cursor.
   Otherwise, if the character to the left is a bracket,
   match it. Otherwise if the character to the right of the cursor is a
   bracket, match it. Otherwise, don't match anything.
*/
void KateDocument::newBracketMark( const KTextEditor::Cursor& cursor, KTextEditor::Range& bm, int maxLines )
{
  bm.start() = cursor;

  if( findMatchingBracket( bm, maxLines ) )
    return;

  bm = KTextEditor::Range::invalid();

 // const int tw = config()->tabWidth();
 // const int indentStart = m_buffer->plainLine(bm.start().line())->indentDepth(tw);
 // const int indentEnd = m_buffer->plainLine(bm.end().line())->indentDepth(tw);
  //bm.setIndentMin(qMin(indentStart, indentEnd));
}

bool KateDocument::findMatchingBracket( KTextEditor::Range& range, int maxLines )
{
  KateTextLine::Ptr textLine = m_buffer->plainLine( range.start().line() );
  if( !textLine )
    return false;

  QChar right = textLine->at( range.start().column() );
  QChar left  = textLine->at( range.start().column() - 1 );
  QChar bracket;

  if ( config()->configFlags() & KateDocumentConfig::cfOvr ) {
    if( isBracket( right ) ) {
      bracket = right;
    } else {
      return false;
    }
  } else if ( isBracket( left ) ) {
    range.start().setColumn(range.start().column() - 1);
    bracket = left;
  } else if ( isBracket( right ) ) {
    bracket = right;
  } else {
    return false;
  }

  QChar opposite;

  switch( bracket.toAscii() ) {
  case '{': opposite = '}'; break;
  case '}': opposite = '{'; break;
  case '[': opposite = ']'; break;
  case ']': opposite = '['; break;
  case '(': opposite = ')'; break;
  case ')': opposite = '('; break;
  default: return false;
  }

  bool forward = isStartBracket( bracket );
  uint nesting = 0;

  int minLine = qMax( range.start().line() - maxLines, 0 );
  int maxLine = qMin( range.start().line() + maxLines, documentEnd().line() );

  range.end() = range.start();
  KateDocCursor cursor(range.start(), this);
  uchar validAttr = cursor.currentAttrib();

  while( cursor.line() >= minLine && cursor.line() <= maxLine ) {

    if( forward )
      cursor.moveForward(1);
    else
      cursor.moveBackward(1);

    if( !cursor.validPosition() )
      return false;

    if( cursor.currentAttrib() == validAttr )
    {
      /* Check for match */
      QChar c = cursor.currentChar();
      if( c == bracket ) {
        nesting++;
      } else if( c == opposite ) {
        if( nesting == 0 ) {
          if( forward )
            range.end() = cursor;
          else
            range.start() = cursor;
          return true;
        }
        nesting--;
      }
    }

    if(cursor == KTextEditor::Cursor(0,0) || cursor >= documentEnd())
      return false;
  }

  return false;
}

void KateDocument::guiActivateEvent( KParts::GUIActivateEvent *ev )
{
  KParts::ReadWritePart::guiActivateEvent( ev );
  //if ( ev->activated() )
  //  emit selectionChanged();
}

void KateDocument::setDocName (QString name )
{
  if ( name == m_docName )
    return;

  if ( !name.isEmpty() )
  {
    // TODO check for similarly named documents
    m_docName = name;
    emit documentNameChanged (this);
    return;
  }

  // if the name is set, and starts with FILENAME, it should not be changed!
  if ( ! url().isEmpty() && m_docName.startsWith( url().fileName() ) ) return;

  int count = -1;

  for (int z=0; z < KateGlobal::self()->kateDocuments().size(); ++z)
  {
    KateDocument *doc = (KateGlobal::self()->kateDocuments())[z];

    if ( (doc != this) && (doc->url().fileName() == url().fileName()) )
      if ( doc->m_docNameNumber > count )
        count = doc->m_docNameNumber;
  }

  m_docNameNumber = count + 1;

  m_docName = url().fileName();

  if (m_docName.isEmpty())
    m_docName = i18n ("Untitled");

  if (m_docNameNumber > 0)
    m_docName = QString(m_docName + " (%1)").arg(m_docNameNumber+1);

  emit documentNameChanged (this);
}

void KateDocument::slotModifiedOnDisk( KTextEditor::View * /*v*/ )
{
  if ( m_isasking < 0 )
  {
    m_isasking = 0;
    return;
  }

  if ( !s_fileChangedDialogsActivated || m_isasking )
    return;

  if (m_modOnHd && !url().isEmpty())
  {
    m_isasking = 1;

    QWidget *parentWidget(dialogParent());

    KateModOnHdPrompt p( this, m_modOnHdReason, reasonedMOHString(), parentWidget );
    switch ( p.exec() )
    {
      case KateModOnHdPrompt::Save:
      {
        m_modOnHd = false;
        KEncodingFileDialog::Result res=KEncodingFileDialog::getSaveUrlAndEncoding(config()->encoding(),
            url().url(),QString(),parentWidget,i18n("Save File"));

        kDebug(13020)<<"got "<<res.URLs.count()<<" URLs";
        if( ! res.URLs.isEmpty() && ! res.URLs.first().isEmpty() && checkOverwrite( res.URLs.first(), parentWidget ) )
        {
          setEncoding( res.encoding );

          if( ! saveAs( res.URLs.first() ) )
          {
            KMessageBox::error( parentWidget, i18n("Save failed") );
            m_modOnHd = true;
          }
          else
            emit modifiedOnDisk( this, false, OnDiskUnmodified );
        }
        else // the save as dialog was canceled, we are still modified on disk
        {
          m_modOnHd = true;
        }

        m_isasking = 0;
        break;
      }

      case KateModOnHdPrompt::Reload:
        m_modOnHd = false;
        emit modifiedOnDisk( this, false, OnDiskUnmodified );
        documentReload();
        m_isasking = 0;
        break;

      case KateModOnHdPrompt::Ignore:
        m_modOnHd = false;
        emit modifiedOnDisk( this, false, OnDiskUnmodified );
        m_isasking = 0;
        break;

      case KateModOnHdPrompt::Overwrite:
        m_modOnHd = false;
        emit modifiedOnDisk( this, false, OnDiskUnmodified );
        m_isasking = 0;
        save();
        break;

      default: // Delay/cancel: ignore next focus event
        m_isasking = -1;
    }
  }
}

void KateDocument::setModifiedOnDisk( ModifiedOnDiskReason reason )
{
  m_modOnHdReason = reason;
  m_modOnHd = (reason != OnDiskUnmodified);
  emit modifiedOnDisk( this, (reason != OnDiskUnmodified), reason );
}

class KateDocumentTmpMark
{
  public:
    QString line;
    KTextEditor::Mark mark;
};

void KateDocument::setModifiedOnDiskWarning (bool on)
{
  s_fileChangedDialogsActivated = on;
}

bool KateDocument::documentReload()
{
  if ( !url().isEmpty() )
  {
    if (m_modOnHd && s_fileChangedDialogsActivated)
    {
      QWidget *parentWidget(dialogParent());

      int i = KMessageBox::warningYesNoCancel
                (parentWidget, reasonedMOHString() + "\n\n" + i18n("What do you want to do?"),
                i18n("File Was Changed on Disk"), KGuiItem(i18n("&Reload File")), KGuiItem(i18n("&Ignore Changes")));

      if ( i != KMessageBox::Yes)
      {
        if (i == KMessageBox::No)
        {
          m_modOnHd = false;
          m_modOnHdReason = OnDiskUnmodified;
          emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
        }

        return false;
      }
    }

    emit aboutToReload(this);

    if (clearOnDocumentReload())
      m_smartManager->clear(false);

    QList<KateDocumentTmpMark> tmp;

    for (QHash<int, KTextEditor::Mark*>::const_iterator i = m_marks.constBegin(); i != m_marks.constEnd(); ++i)
    {
      KateDocumentTmpMark m;

      m.line = line (i.value()->line);
      m.mark = *i.value();

      tmp.append (m);
    }

    QString oldMode = mode ();
    bool byUser = m_fileTypeSetByUser;

    m_storedVariables.clear();

    // save cursor positions for all views
    QVector<KTextEditor::Cursor> cursorPositions;
    cursorPositions.reserve(m_views.size());
    foreach (KateView *v, m_views)
      cursorPositions.append( v->cursorPosition() );

    m_reloading = true;
    KateDocument::openUrl( url() );
    m_reloading = false;

    // restore cursor positions for all views
    QLinkedList<KateView*>::iterator it = m_views.begin();
    for(int i = 0; i < m_views.size(); ++i, ++it)
      (*it)->setCursorPositionInternal( cursorPositions[i], m_config->tabWidth(), false );

    for (int z=0; z < tmp.size(); z++)
    {
      if (z < (int)lines())
      {
        if (line(tmp[z].mark.line) == tmp[z].line)
          setMark (tmp[z].mark.line, tmp[z].mark.type);
      }
    }

    if (byUser)
      setMode (oldMode);

    return true;
  }

  return false;
}

bool KateDocument::documentSave()
{
  if( !url().isValid() || !isReadWrite() )
    return documentSaveAs();

  return save();
}

bool KateDocument::documentSaveAs()
{
  QWidget *parentWidget(dialogParent());

  KEncodingFileDialog::Result res=KEncodingFileDialog::getSaveUrlAndEncoding(config()->encoding(),
                url().url(),QString(),parentWidget,i18n("Save File"));

  if( res.URLs.isEmpty() || !checkOverwrite( res.URLs.first(), parentWidget ) )
    return false;

  setEncoding( res.encoding );

  return saveAs( res.URLs.first() );
}

void KateDocument::setWordWrap (bool on)
{
  config()->setWordWrap (on);
}

bool KateDocument::wordWrap () const
{
  return config()->wordWrap ();
}

void KateDocument::setWordWrapAt (uint col)
{
  config()->setWordWrapAt (col);
}

unsigned int KateDocument::wordWrapAt () const
{
  return config()->wordWrapAt ();
}

void KateDocument::setPageUpDownMovesCursor (bool on)
{
  config()->setPageUpDownMovesCursor (on);
}

bool KateDocument::pageUpDownMovesCursor () const
{
  return config()->pageUpDownMovesCursor ();
}

void KateDocument::dumpRegionTree()
{
  m_buffer->foldingTree()->debugDump();
}
//END

void KateDocument::lineInfo (KateLineInfo *info, unsigned int line)
{
  m_buffer->lineInfo(info,line);
}

KateCodeFoldingTree *KateDocument::foldingTree ()
{
  return m_buffer->foldingTree();
}

bool KateDocument::setEncoding (const QString &e)
{
  return m_config->setEncoding(e);
}

const QString &KateDocument::encoding() const
{
  return m_config->encoding();
}

void KateDocument::setProberTypeForEncodingAutoDetection (KEncodingProber::ProberType proberType)
{
  m_config->setEncodingProberType(proberType);
}

KEncodingProber::ProberType KateDocument::proberTypeForEncodingAutoDetection() const
{
  return m_config->encodingProberType();
}

void KateDocument::updateConfig ()
{
  emit undoChanged ();
  tagAll();

  foreach (KateView * view,m_views)
  {
    view->updateDocumentConfig ();
  }

  // switch indenter if needed and update config....
  m_indenter.setMode (m_config->indentationMode());
  m_indenter.updateConfig();

  m_buffer->setTabWidth (config()->tabWidth());
}

//BEGIN Variable reader
// "local variable" feature by anders, 2003
/* TODO
      add config options (how many lines to read, on/off)
      add interface for plugins/apps to set/get variables
      add view stuff
*/
QRegExp KateDocument::kvLine = QRegExp("kate:(.*)");
QRegExp KateDocument::kvLineWildcard = QRegExp("kate-wildcard\\((.*)\\):(.*)");
QRegExp KateDocument::kvLineMime = QRegExp("kate-mimetype\\((.*)\\):(.*)");
QRegExp KateDocument::kvVar = QRegExp("([\\w\\-]+)\\s+([^;]+)");

void KateDocument::readVariables(bool onlyViewAndRenderer)
{
  if (!onlyViewAndRenderer)
    m_config->configStart();

  // views!
  KateView *v;
  foreach (v,m_views)
  {
    v->config()->configStart();
    v->renderer()->config()->configStart();
  }
  // read a number of lines in the top/bottom of the document
  for (int i=0; i < qMin( 9, lines() ); ++i )
  {
    readVariableLine( line( i ), onlyViewAndRenderer );
  }
  if ( lines() > 10 )
  {
    for ( int i = qMax( 10, lines() - 10); i < lines(); i++ )
    {
      readVariableLine( line( i ), onlyViewAndRenderer );
    }
  }

  if (!onlyViewAndRenderer)
    m_config->configEnd();

  foreach (v,m_views)
  {
    v->config()->configEnd();
    v->renderer()->config()->configEnd();
  }
}

void KateDocument::readVariableLine( QString t, bool onlyViewAndRenderer )
{
  // simple check first, no regex
  // no kate inside, no vars, simple...
  if (!t.contains("kate"))
    return;

  // found vars, if any
  QString s;

  // now, try first the normal ones
  if ( kvLine.indexIn( t ) > -1 )
  {
    s = kvLine.cap(1);

    kDebug (13020) << "normal variable line kate: matched: " << s;
  }
  else if (kvLineWildcard.indexIn( t ) > -1) // regex given
  {
    QStringList wildcards (kvLineWildcard.cap(1).split (';', QString::SkipEmptyParts));
    QString nameOfFile = url().fileName();

    bool found = false;
    for (int i = 0; !found && i < wildcards.size(); ++i)
    {
      QRegExp wildcard (wildcards[i], Qt::CaseSensitive, QRegExp::Wildcard);

      found = wildcard.exactMatch (nameOfFile);
    }

    // nothing usable found...
    if (!found)
      return;

    s = kvLineWildcard.cap(2);

    kDebug (13020) << "guarded variable line kate-wildcard: matched: " << s;
  }
  else if (kvLineMime.indexIn( t ) > -1) // mime-type given
  {
    QStringList types (kvLineMime.cap(1).split (';', QString::SkipEmptyParts));

    // no matching type found
    if (!types.contains (mimeType ()))
      return;

    s = kvLineMime.cap(2);

    kDebug (13020) << "guarded variable line kate-mimetype: matched: " << s;
  }
  else // nothing found
  {
    return;
  }

  QStringList vvl; // view variable names
  vvl << "dynamic-word-wrap" << "dynamic-word-wrap-indicators"
      << "line-numbers" << "icon-border" << "folding-markers"
      << "bookmark-sorting" << "auto-center-lines"
      << "icon-bar-color"
      // renderer
      << "background-color" << "selection-color"
      << "current-line-color" << "bracket-highlight-color"
      << "word-wrap-marker-color"
      << "font" << "font-size" << "scheme";
  int spaceIndent = -1;  // for backward compatibility; see below
  bool replaceTabsSet = false;
  int p( 0 );

  QString  var, val;
  while ( (p = kvVar.indexIn( s, p )) > -1 )
  {
    p += kvVar.matchedLength();
    var = kvVar.cap( 1 );
    val = kvVar.cap( 2 ).trimmed();
    bool state; // store booleans here
    int n; // store ints here

    // only apply view & renderer config stuff
    if (onlyViewAndRenderer)
    {
      if ( vvl.contains( var ) ) // FIXME define above
        setViewVariable( var, val );
    }
    else
    {
      // BOOL  SETTINGS
      if ( var == "word-wrap" && checkBoolValue( val, &state ) )
        setWordWrap( state ); // ??? FIXME CHECK
      // KateConfig::configFlags
      // FIXME should this be optimized to only a few calls? how?
      else if ( var == "backspace-indents" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfBackspaceIndents, state );
      else if ( var == "replace-tabs" && checkBoolValue( val, &state ) )
      {
        m_config->setConfigFlags( KateDocumentConfig::cfReplaceTabsDyn, state );
        replaceTabsSet = true;  // for backward compatibility; see below
      }
      else if ( var == "remove-trailing-space" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfRemoveTrailingDyn, state );
      else if ( var == "wrap-cursor" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfWrapCursor, state );
      else if ( var == "auto-brackets" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfAutoBrackets, state );
      else if ( var == "overwrite-mode" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfOvr, state );
      else if ( var == "keep-extra-spaces" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfKeepExtraSpaces, state );
      else if ( var == "tab-indents" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfTabIndents, state );
      else if ( var == "show-tabs" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfShowTabs, state );
      else if ( var == "show-trailing-spaces" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfShowSpaces, state );
      else if ( var == "space-indent" && checkBoolValue( val, &state ) )
      {
        // this is for backward compatibility; see below
        spaceIndent = state;
      }
      else if ( var == "smart-home" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfSmartHome, state );
      else if ( var == "replace-trailing-space-save" && checkBoolValue( val, &state ) )
        m_config->setConfigFlags( KateDocumentConfig::cfRemoveSpaces, state );

      // INTEGER SETTINGS
      else if ( var == "tab-width" && checkIntValue( val, &n ) )
        m_config->setTabWidth( n );
      else if ( var == "indent-width"  && checkIntValue( val, &n ) )
        m_config->setIndentationWidth( n );
      else if ( var == "indent-mode" )
      {
        m_config->setIndentationMode( val );
      }
      else if ( var == "word-wrap-column" && checkIntValue( val, &n ) && n > 0 ) // uint, but hard word wrap at 0 will be no fun ;)
        m_config->setWordWrapAt( n );

      // STRING SETTINGS
      else if ( var == "eol" || var == "end-of-line" )
      {
        QStringList l;
        l << "unix" << "dos" << "mac";
        if ( (n = l.indexOf( val.toLower() )) != -1 )
          m_config->setEol( n );
      }
      else if ( var == "encoding" )
        m_config->setEncoding( val );
      else if (var == "presave-postdialog")
        setPreSavePostDialogFilterChecks(val.split(','));
      else if (var == "postload")
        setPostLoadFilterChecks(val.split(','));
      else if ( var == "syntax" || var == "hl" )
      {
        setHighlightingMode( val );
      }
      else if ( var == "mode" )
      {
        setMode( val );
      }

      // VIEW SETTINGS
      else if ( vvl.contains( var ) )
        setViewVariable( var, val );
      else
      {
        m_storedVariables.insert( var, val );
        emit variableChanged( this, var, val );
      }
    }
  }

  // Backward compatibility
  // If space-indent was set, but replace-tabs was not set, we assume
  // that the user wants to replace tabulators and set that flag.
  // If both were set, replace-tabs has precedence.
  // At this point spaceIndent is -1 if it was never set,
  // 0 if it was set to off, and 1 if it was set to on.
  // Note that if onlyViewAndRenderer was requested, spaceIndent is -1.
  if ( !replaceTabsSet && spaceIndent >= 0 )
  {
    m_config->setConfigFlags( KateDocumentConfig::cfReplaceTabsDyn, spaceIndent > 0 );
  }
}

void KateDocument::setViewVariable( QString var, QString val )
{
  KateView *v;
  bool state;
  int n;
  QColor c;
  foreach (v,m_views)
  {
    if ( var == "dynamic-word-wrap" && checkBoolValue( val, &state ) )
      v->config()->setDynWordWrap( state );
    else if ( var == "persistent-selection" && checkBoolValue( val, &state ) )
      v->config()->setPersistentSelection( state );
    else if ( var == "block-selection"  && checkBoolValue( val, &state ) )
          v->setBlockSelectionMode( state );
    //else if ( var = "dynamic-word-wrap-indicators" )
    else if ( var == "line-numbers" && checkBoolValue( val, &state ) )
      v->config()->setLineNumbers( state );
    else if (var == "icon-border" && checkBoolValue( val, &state ) )
      v->config()->setIconBar( state );
    else if (var == "folding-markers" && checkBoolValue( val, &state ) )
      v->config()->setFoldingBar( state );
    else if ( var == "auto-center-lines" && checkIntValue( val, &n ) )
      v->config()->setAutoCenterLines( n ); // FIXME uint, > N ??
    else if ( var == "icon-bar-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setIconBarColor( c );
    // RENDERER
    else if ( var == "background-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setBackgroundColor( c );
    else if ( var == "selection-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setSelectionColor( c );
    else if ( var == "current-line-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setHighlightedLineColor( c );
    else if ( var == "bracket-highlight-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setHighlightedBracketColor( c );
    else if ( var == "word-wrap-marker-color" && checkColorValue( val, c ) )
      v->renderer()->config()->setWordWrapMarkerColor( c );
    else if ( var == "font" || ( var == "font-size" && checkIntValue( val, &n ) ) )
    {
      QFont _f( v->renderer()->config()->font() );

      if ( var == "font" )
      {
        _f.setFamily( val );
        _f.setFixedPitch( QFont( val ).fixedPitch() );
      }
      else
        _f.setPointSize( n );

      v->renderer()->config()->setFont( _f );
    }
    else if ( var == "scheme" )
    {
      v->renderer()->config()->setSchema( val );
    }
  }
}

bool KateDocument::checkBoolValue( QString val, bool *result )
{
  val = val.trimmed().toLower();
  QStringList l;
  l << "1" << "on" << "true";
  if ( l.contains( val ) )
  {
    *result = true;
    return true;
  }
  l.clear();
  l << "0" << "off" << "false";
  if ( l.contains( val ) )
  {
    *result = false;
    return true;
  }
  return false;
}

bool KateDocument::checkIntValue( QString val, int *result )
{
  bool ret( false );
  *result = val.toInt( &ret );
  return ret;
}

bool KateDocument::checkColorValue( QString val, QColor &c )
{
  c.setNamedColor( val );
  return c.isValid();
}

// KTextEditor::variable
QString KateDocument::variable( const QString &name ) const
{
  if ( m_storedVariables.contains( name ) )
    return m_storedVariables[ name ];

  return "";
}

//END

void KateDocument::slotModOnHdDirty (const QString &path)
{
  if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != OnDiskModified))
  {
    // compare md5 with the one we have (if we have one)
    if ( ! m_digest.isEmpty() )
    {
      QByteArray tmp;
      if ( createDigest( tmp ) && tmp == m_digest )
        return;
    }

    m_modOnHd = true;
    m_modOnHdReason = OnDiskModified;

    // reenable dialog if not running atm
    if (m_isasking == -1)
      m_isasking = false;

    emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
  }
}

void KateDocument::slotModOnHdCreated (const QString &path)
{
  if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != OnDiskCreated))
  {
    m_modOnHd = true;
    m_modOnHdReason = OnDiskCreated;

    // reenable dialog if not running atm
    if (m_isasking == -1)
      m_isasking = false;

    emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
  }
}

void KateDocument::slotModOnHdDeleted (const QString &path)
{
  if ((path == m_dirWatchFile) && (!m_modOnHd || m_modOnHdReason != OnDiskDeleted))
  {
    m_modOnHd = true;
    m_modOnHdReason = OnDiskDeleted;

    // reenable dialog if not running atm
    if (m_isasking == -1)
      m_isasking = false;

    emit modifiedOnDisk (this, m_modOnHd, m_modOnHdReason);
  }
}

bool KateDocument::createDigest( QByteArray &result )
{
  bool ret = false;
  result = "";
  if ( url().isLocalFile() )
  {
    QFile f ( url().path() );
    if ( f.open( QIODevice::ReadOnly) )
    {
      KMD5 md5;
      ret = md5.update( f );
      md5.hexDigest( result );
      f.close();
      ret = true;
    }
  }
  return ret;
}

QString KateDocument::reasonedMOHString() const
{
  switch( m_modOnHdReason )
  {
    case OnDiskModified:
      return i18n("The file '%1' was modified by another program.",  url().pathOrUrl() );
      break;
    case OnDiskCreated:
      return i18n("The file '%1' was created by another program.",  url().pathOrUrl() );
      break;
    case OnDiskDeleted:
      return i18n("The file '%1' was deleted by another program.",  url().pathOrUrl() );
      break;
    default:
      return QString();
  }
}

void KateDocument::removeTrailingSpace(int line)
{
  // remove trailing spaces from left line if required
  if (m_blockRemoveTrailingSpaces
      || !(config()->configFlags() & KateDocumentConfig::cfRemoveTrailingDyn))
    return;

  KateTextLine::Ptr ln = plainKateTextLine(line);

  if (!ln || ln->length() == 0)
    return;

  if (line == activeView()->cursorPosition().line()
      && activeView()->cursorPosition().column() >= qMax(0, ln->lastChar()))
    return;

  const int p = ln->lastChar() + 1;
  const int l = ln->length() - p;
  if (l > 0) {
    m_blockRemoveTrailingSpaces = true;
    editRemoveText(line, p, l);
    m_blockRemoveTrailingSpaces = false;
  }
}

void KateDocument::updateFileType (const QString &newType, bool user)
{
  if (user || !m_fileTypeSetByUser)
  {
    if (!newType.isEmpty())
    {
          m_fileType = newType;

          m_config->configStart();

          if (!hlSetByUser && !KateGlobal::self()->modeManager()->fileType(newType).hl.isEmpty())
          {
            int hl (KateHlManager::self()->nameFind (KateGlobal::self()->modeManager()->fileType(newType).hl));

            if (hl >= 0)
              m_buffer->setHighlight(hl);
          }


          // views!
          KateView *v;
          foreach (v,m_views)
          {
            v->config()->configStart();
            v->renderer()->config()->configStart();
          }

          readVariableLine( KateGlobal::self()->modeManager()->fileType(newType).varLine );

          m_config->configEnd();
          foreach (v,m_views)
          {
            v->config()->configEnd();
            v->renderer()->config()->configEnd();
          }
    }
  }

  // fixme, make this better...
  emit modeChanged (this);
}

void KateDocument::slotQueryClose_save(bool *handled, bool* abortClosing) {
      *handled=true;
      *abortClosing=true;
      if (this->url().isEmpty())
      {
        QWidget *parentWidget(dialogParent());

        KEncodingFileDialog::Result res=KEncodingFileDialog::getSaveUrlAndEncoding(config()->encoding(),
                QString(),QString(),parentWidget,i18n("Save File"));

        if( res.URLs.isEmpty() || !checkOverwrite( res.URLs.first(), parentWidget ) ) {
                *abortClosing=true;
                return;
        }
        setEncoding( res.encoding );
          saveAs( res.URLs.first() );
        *abortClosing=false;
      }
      else
      {
          save();
          *abortClosing=false;
      }

}

bool KateDocument::checkOverwrite( KUrl u, QWidget *parent )
{
  if( !u.isLocalFile() )
    return true;

  QFileInfo info( u.path() );
  if( !info.exists() )
    return true;

  return KMessageBox::Cancel != KMessageBox::warningContinueCancel( parent,
    i18n( "A file named \"%1\" already exists. "
          "Are you sure you want to overwrite it?" ,  info.fileName() ),
    i18n( "Overwrite File?" ),
    KGuiItem(i18n( "&Overwrite" )) );
}


//BEGIN KTextEditor::TemplateInterface
bool KateDocument::insertTemplateTextImplementation ( const KTextEditor::Cursor &c, const QString &templateString, const QMap<QString,QString> &initialValues, QWidget *) {
      return (new KateTemplateHandler(this,c,templateString,initialValues))->initOk();
}

void KateDocument::testTemplateCode() {
  //qobject_cast<KTextEditor::TemplateInterface*>(activeView())->insertTemplateText(activeView()->cursorPosition(),"for ${index} \\${NOPLACEHOLDER} ${index} ${blah} ${fullname} \\$${Placeholder} \\${${PLACEHOLDER2}}\n next line:${ANOTHERPLACEHOLDER} $${DOLLARBEFOREPLACEHOLDER} {NOTHING} {\n${cursor}\n}",QMap<QString,QString>());
  qobject_cast<KTextEditor::TemplateInterface*>(activeView())->insertTemplateText(activeView()->cursorPosition(),"for ${index} \\${NOPLACEHOLDER} ${index} ${blah} \\$${Placeholder} \\${${PLACEHOLDER2}}\n next line:${ANOTHERPLACEHOLDER} $${DOLLARBEFOREPLACEHOLDER} {NOTHING} {\n${cursor}\n}",QMap<QString,QString>());
}


bool KateDocument::invokeTabInterceptor(int key) {
  if (m_tabInterceptor) return (*m_tabInterceptor)(key);
  return false;
}

bool KateDocument::setTabInterceptor(KateKeyInterceptorFunctor *interceptor) {
  if (m_tabInterceptor) return false;
  m_tabInterceptor=interceptor;
  return true;
}

bool KateDocument::removeTabInterceptor(KateKeyInterceptorFunctor *interceptor) {
  if (m_tabInterceptor!=interceptor) return false;
  m_tabInterceptor=0;
  return true;
}

KateView * KateDocument::activeKateView( ) const
{
  return static_cast<KateView*>(m_activeView);
}

KTextEditor::Cursor KateDocument::documentEnd( ) const
{
  return KTextEditor::Cursor(lastLine(), lineLength(lastLine()));
}
//END KTextEditor::TemplateInterface

//BEGIN KTextEditor::SmartInterface
int KateDocument::currentRevision() const
{
  return m_smartManager->currentRevision();
}

void KateDocument::releaseRevision(int revision) const
{
  m_smartManager->releaseRevision(revision);
}

void KateDocument::useRevision(int revision)
{
  m_smartManager->useRevision(revision);
}

KTextEditor::Cursor KateDocument::translateFromRevision(const KTextEditor::Cursor& cursor, KTextEditor::SmartCursor::InsertBehavior insertBehavior) const
{
  return m_smartManager->translateFromRevision(cursor, insertBehavior);
}

KTextEditor::Range KateDocument::translateFromRevision(const KTextEditor::Range& range, KTextEditor::SmartRange::InsertBehaviors insertBehavior) const
{
  return m_smartManager->translateFromRevision(range, insertBehavior);
}

KTextEditor::SmartCursor* KateDocument::newSmartCursor( const KTextEditor::Cursor & position, KTextEditor::SmartCursor::InsertBehavior insertBehavior )
{
  return m_smartManager->newSmartCursor(position, insertBehavior, false);
}

KTextEditor::SmartRange * KateDocument::newSmartRange( const KTextEditor::Range & range, KTextEditor::SmartRange * parent, KTextEditor::SmartRange::InsertBehaviors insertBehavior )
{
  return m_smartManager->newSmartRange( range, parent, insertBehavior, false );
}

KTextEditor::SmartRange * KateDocument::newSmartRange( KTextEditor::SmartCursor * start, KTextEditor::SmartCursor * end, KTextEditor::SmartRange * parent, KTextEditor::SmartRange::InsertBehaviors insertBehavior )
{
  KateSmartCursor* kstart = dynamic_cast<KateSmartCursor*>(start);
  KateSmartCursor* kend = dynamic_cast<KateSmartCursor*>(end);
  if (!kstart || !kend)
    return 0L;
  if (kstart->range() || kend->range())
    return 0L;
  return m_smartManager->newSmartRange(kstart, kend, parent, insertBehavior, false);
}

void KateDocument::unbindSmartRange( KTextEditor::SmartRange * range )
{
  m_smartManager->unbindSmartRange(range);
}

bool KateDocument::replaceText( const KTextEditor::Range & range, const QString & s, bool block )
{
  // TODO more efficient?
  editStart();
  bool changed = removeText(range, block);
  changed |= insertText(range.start(), s, block);
  editEnd();
  return changed;
}

void KateDocument::addHighlightToDocument( KTextEditor::SmartRange * topRange, bool supportDynamic )
{
  if (m_documentHighlights.contains(topRange))
    return;

  m_documentHighlights.append(topRange);

  // Deal with the range being deleted externally
  topRange->addWatcher(this);

  if (supportDynamic) {
    m_documentDynamicHighlights.append(topRange);
    emit dynamicHighlightAdded(static_cast<KateSmartRange*>(topRange));
  }

  foreach (KateView * view, m_views)
    view->addExternalHighlight(topRange, supportDynamic);
}

void KateDocument::removeHighlightFromDocument( KTextEditor::SmartRange * topRange )
{
  if (!m_documentHighlights.contains(topRange))
    return;

  foreach (KateView * view, m_views)
    view->removeExternalHighlight(topRange);

  m_documentHighlights.removeAll(topRange);
  topRange->removeWatcher(this);

  if (m_documentDynamicHighlights.contains(topRange)) {
    m_documentDynamicHighlights.removeAll(topRange);
    emit dynamicHighlightRemoved(static_cast<KateSmartRange*>(topRange));
  }
}

const QList< KTextEditor::SmartRange * > KateDocument::documentHighlights( ) const
{
  return m_documentHighlights;
}

void KateDocument::addHighlightToView( KTextEditor::View * view, KTextEditor::SmartRange * topRange, bool supportDynamic )
{
  static_cast<KateView*>(view)->addExternalHighlight(topRange, supportDynamic);
}

void KateDocument::removeHighlightFromView( KTextEditor::View * view, KTextEditor::SmartRange * topRange )
{
  static_cast<KateView*>(view)->removeExternalHighlight(topRange);
}

const QList< KTextEditor::SmartRange * > KateDocument::viewHighlights( KTextEditor::View * view ) const
{
  return static_cast<KateView*>(view)->externalHighlights();
}

void KateDocument::addActionsToDocument( KTextEditor::SmartRange * topRange )
{
  if (m_documentActions.contains(topRange))
    return;

  m_documentActions.append(topRange);

  // Deal with the range being deleted externally
  topRange->addWatcher(this);
}

void KateDocument::removeActionsFromDocument( KTextEditor::SmartRange * topRange )
{
  if (!m_documentActions.contains(topRange))
    return;

  m_documentActions.removeAll(topRange);
  topRange->removeWatcher(this);
}

const QList< KTextEditor::SmartRange * > KateDocument::documentActions( ) const
{
  return m_documentActions;
}

void KateDocument::addActionsToView( KTextEditor::View * view, KTextEditor::SmartRange * topRange )
{
  static_cast<KateView*>(view)->addActions(topRange);
}

void KateDocument::removeActionsFromView( KTextEditor::View * view, KTextEditor::SmartRange * topRange )
{
  static_cast<KateView*>(view)->removeActions(topRange);
}

const QList< KTextEditor::SmartRange * > KateDocument::viewActions( KTextEditor::View * view ) const
{
  return static_cast<KateView*>(view)->actions();
}

void KateDocument::attributeDynamic( KTextEditor::Attribute::Ptr )
{
  // TODO link in with cursor + mouse tracking
}

void KateDocument::attributeNotDynamic( KTextEditor::Attribute::Ptr )
{
  // TODO de-link cursor + mouse tracking
}

void KateDocument::clearSmartInterface( )
{
  clearDocumentHighlights();
  foreach (KateView* view, m_views)
    clearViewHighlights(view);

  clearDocumentActions();

  m_smartManager->clear(false);
}

void KateDocument::deleteCursors( )
{
  m_smartManager->deleteCursors(false);
}

void KateDocument::deleteRanges( )
{
  m_smartManager->deleteRanges(false);
}

void KateDocument::clearDocumentHighlights( )
{
  m_documentHighlights.clear();
}

void KateDocument::clearViewHighlights( KTextEditor::View * view )
{
  static_cast<KateView*>(view)->clearExternalHighlights();
}

void KateDocument::clearDocumentActions( )
{
  m_documentActions.clear();
}

void KateDocument::clearViewActions( KTextEditor::View * view )
{
  static_cast<KateView*>(view)->clearActions();
}

void KateDocument::ignoreModifiedOnDiskOnce( )
{
  m_isasking = -1;
}

KateHighlighting * KateDocument::highlight( ) const
{
  return m_buffer->highlight();
}

uint KateDocument::getRealLine( unsigned int virtualLine )
{
  return m_buffer->lineNumber (virtualLine);
}

uint KateDocument::getVirtualLine( unsigned int realLine )
{
  return m_buffer->lineVisibleNumber (realLine);
}

uint KateDocument::visibleLines( )
{
  return m_buffer->countVisible ();
}

KateTextLine::Ptr KateDocument::kateTextLine( uint i )
{
  return m_buffer->line (i);
}

KateTextLine::Ptr KateDocument::plainKateTextLine( uint i )
{
  return m_buffer->plainLine (i);
}

bool KateDocument::undoDontMerge( ) const
{
  return m_undoDontMerge;
}

void KateDocument::setUndoDontMergeComplex(bool dontMerge)
{
  m_undoComplexMerge = dontMerge;
}

bool KateDocument::undoDontMergeComplex( ) const
{
  return m_undoComplexMerge;
}

void KateDocument::setUndoDontMerge(bool dontMerge)
{
  m_undoDontMerge = dontMerge;
}

bool KateDocument::isEditRunning() const
{
  return editIsRunning;
}

void KateDocument::rangeDeleted( KTextEditor::SmartRange * range )
{
  removeHighlightFromDocument(range);
  removeActionsFromDocument(range);
}

//END KTextEditor::SmartInterface

// kate: space-indent on; indent-width 2; replace-tabs on;

bool KateDocument::simpleMode ()
{
  return KateGlobal::self()->simpleMode () && KateGlobal::self()->documentConfig()->allowSimpleMode ();
}

KateDocument::LoadSaveFilterCheckPlugins* KateDocument::loadSaveFilterCheckPlugins()
{
  K_GLOBAL_STATIC(KateDocument::LoadSaveFilterCheckPlugins, s_loadSaveFilterCheckPlugins)
  return s_loadSaveFilterCheckPlugins;
}

//BEGIN KTextEditor::AnnotationInterface
void KateDocument::setAnnotationModel( KTextEditor::AnnotationModel* model )
{
  KTextEditor::AnnotationModel* oldmodel = m_annotationModel;
  m_annotationModel = model;
  emit annotationModelChanged(oldmodel, m_annotationModel);
}

KTextEditor::AnnotationModel* KateDocument::annotationModel() const
{
  return m_annotationModel;
}
//END KTextEditor::AnnotationInterface

//TAKEN FROM kparts.h
bool KateDocument::queryClose()
{
    if ( !isReadWrite() || !isModified() )
        return true;

    QString docName = documentName();

    QWidget *parentWidget=widget();
    if(!parentWidget) parentWidget=QApplication::activeWindow();

    int res = KMessageBox::warningYesNoCancel( parentWidget,
                                               i18n( "The document \"%1\" has been modified.\n"
                                                     "Do you want to save your changes or discard them?" ,  docName ),
                                               i18n( "Close Document" ), KStandardGuiItem::save(), KStandardGuiItem::discard() );

    bool abortClose=false;
    bool handled=false;

    switch(res) {
    case KMessageBox::Yes :
        sigQueryClose(&handled,&abortClose);
        if (!handled)
        {
            if (url().isEmpty())
            {
                KUrl url = KFileDialog::getSaveUrl(KUrl(), QString(), parentWidget);
                if (url.isEmpty())
                    return false;

                saveAs( url );
            }
            else
            {
                save();
            }
        } else if (abortClose) return false;
        return waitSaveComplete();
    case KMessageBox::No :
        return true;
    default : // case KMessageBox::Cancel :
        return false;
    }
}

// Kill our helpers again
#ifdef FAST_DEBUG_ENABLE
# undef FAST_DEBUG_ENABLE
#endif
#undef FAST_DEBUG

// kate: space-indent on; indent-width 2; replace-tabs on;