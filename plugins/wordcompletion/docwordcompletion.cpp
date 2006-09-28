/*
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

    ---
    file: docwordcompletion.cpp

    KTextEditor plugin to autocompletion with document words.
    Copyright Anders Lund <anders.lund@lund.tdcadsl.dk>, 2003

    The following completion methods are supported:
    * Completion with bigger matching words in
      either direction (backward/forward).
    * NOT YET Pop up a list of all bigger matching words in document

*/

// remove when QVBoxLayout::setAutoAdd is ported
#define QT3_SUPPORT
#define QT3_SUPPORT_WARNINGS
#ifdef __GNUC__
#warning TODO: remove QT3_SUPPORT
#endif

//BEGIN includes
#include "docwordcompletion.h"

#include <ktexteditor/document.h>
#include <ktexteditor/variableinterface.h>

#include <kconfig.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kaction.h>
#include <knotification.h>
#include <kparts/part.h>
#include <kiconloader.h>
#include <kpagedialog.h>
#include <kpagewidgetmodel.h>
#include <ktoggleaction.h>

#include <qregexp.h>
#include <qstring.h>
#include <q3dict.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <Q3PtrList>

#include <kvbox.h>
#include <qcheckbox.h>

// #include <kdebug.h>
//END

//BEGIN DocWordCompletionPlugin
K_EXPORT_COMPONENT_FACTORY( ktexteditor_docwordcompletion, KGenericFactory<DocWordCompletionPlugin>( "ktexteditor_docwordcompletion" ) )
DocWordCompletionPlugin::DocWordCompletionPlugin( QObject *parent,
                            const QStringList& /*args*/ )
	: KTextEditor::Plugin ( parent )
{
  readConfig();
}

void DocWordCompletionPlugin::configDialog (QWidget *parent)
{
 // If we have only one page, we use a simple dialog, else an icon list type
  KPageDialog::FaceType ft = configPages() > 1 ? KPageDialog::List :     // still untested
                                                 KPageDialog::Plain;

  KPageDialog *kd = new KPageDialog ( parent );
  kd->setFaceType( ft );
  kd->setCaption( i18n("Configure") );
  kd->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Help );
  kd->setDefaultButton( KDialog::Ok );

  QList<KTextEditor::ConfigPage*> editorPages;

  for (uint i = 0; i < configPages (); i++)
  {
    QWidget *page = new QWidget(0);

    KPageWidgetItem *item = new KPageWidgetItem( page, configPageName( i ) );
    item->setHeader( configPageFullName( i ) );
// FIXME: set the icon here      item->setIcon();

    kd->addPage( item );

    editorPages.append( configPage( i, page, "" ) );
  }

  if (kd->exec())
  {

    for( int i=0; i<editorPages.count(); i++ )
    {
      editorPages.at( i )->apply();
    }
  }

  delete kd;
}

void DocWordCompletionPlugin::readConfig()
{
  KConfigGroup cg(KGlobal::config(), "DocWordCompletion Plugin" );
  m_treshold = cg.readEntry( "treshold", 3 );
  m_autopopup = cg.readEntry( "autopopup", true );
}

void DocWordCompletionPlugin::writeConfig()
{
  KConfigGroup cg(KGlobal::config(), "DocWordCompletion Plugin" );
  cg.writeEntry("autopopup", m_autopopup );
  cg.writeEntry("treshold", m_treshold );
}

void DocWordCompletionPlugin::addView(KTextEditor::View *view)
{
  DocWordCompletionPluginView *nview = new DocWordCompletionPluginView (m_treshold, m_autopopup, view, "Document word completion");
  m_views.append (nview);
}

void DocWordCompletionPlugin::removeView(KTextEditor::View *view)
{
  for (int z=0; z < m_views.count(); z++)
    if (m_views.at(z)->parentClient() == view)
    {
       DocWordCompletionPluginView *nview = m_views.at(z);
       m_views.removeAll (nview);
       delete nview;
    }
}

KTextEditor::ConfigPage* DocWordCompletionPlugin::configPage( uint, QWidget *parent, const char *name )
{
  return new DocWordCompletionConfigPage( this, parent, name );
}

QString DocWordCompletionPlugin::configPageName( uint ) const
{
  return i18n("Word Completion Plugin");
}

QString DocWordCompletionPlugin::configPageFullName( uint ) const
{
  return i18n("Configure the Word Completion Plugin");
}

// FIXME provide sucn a icon
       QPixmap DocWordCompletionPlugin::configPagePixmap( uint, int size ) const
{
  return UserIcon( "kte_wordcompletion", size );
}
//END

//BEGIN DocWordCompletionPluginView
struct DocWordCompletionPluginViewPrivate
{
  uint line, col;       // start position of last match (where to search from)
  uint cline, ccol;     // cursor position
  uint lilen;           // length of last insertion
  QString last;         // last word we were trying to match
  QString lastIns;      // latest applied completion
  QRegExp re;           // hrm
  KToggleAction *autopopup; // for accessing state
  uint treshold;         // the required length of a word before popping up the completion list automatically
};

DocWordCompletionPluginView::DocWordCompletionPluginView( uint treshold, bool autopopup, KTextEditor::View *view, const char *name )
  : QObject( view ),
    KXMLGUIClient( view ),
    KTextEditor::CompletionProvider(),
    m_view( view ),
    d( new DocWordCompletionPluginViewPrivate )
{
  setObjectName( name );

  d->treshold = treshold;
  view->insertChildClient( this );
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(view);
  if (cci) {cci->registerCompletionProvider(this); kDebug()<<"*******Completion provider registered"<<endl; }
  else kDebug()<<"****** No code completion interface available for view"<<endl;
  setInstance( KGenericFactory<DocWordCompletionPlugin>::instance() );

  KAction *action = new KAction( i18n("Reuse Word Above"), actionCollection(), "doccomplete_bw" );
  action->setShortcut( Qt::CTRL+Qt::Key_8 );
  connect( action, SIGNAL( triggered() ), this, SLOT(completeBackwards()) );

  action = new KAction( i18n("Reuse Word Below"), actionCollection(), "doccomplete_fw" );
  action->setShortcut( Qt::CTRL+Qt::Key_9 );
  connect( action, SIGNAL( triggered() ), this, SLOT(completeForwards()) );

  action = new KAction( i18n("Pop Up Completion List"), actionCollection(), "doccomplete_pu" );
  connect( action, SIGNAL( triggered() ), this, SLOT(popupCompletionList()) );

  action = new KAction( i18n("Shell Completion"), actionCollection(), "doccomplete_sh" );
  connect( action, SIGNAL( triggered() ), this, SLOT(shellComplete()) );

  d->autopopup = new KToggleAction( i18n("Automatic Completion Popup"), actionCollection(), "enable_autopopup" );
  connect( d->autopopup, SIGNAL( triggered() ), this, SLOT(toggleAutoPopup()) );

  d->autopopup->setChecked( autopopup );
  toggleAutoPopup();

  setXMLFile("docwordcompletionui.rc");

  KTextEditor::VariableInterface *vi = qobject_cast<KTextEditor::VariableInterface *>( view->document() );
  if ( vi )
  {
    QString e = vi->variable("wordcompletion-autopopup");
    if ( ! e.isEmpty() )
      d->autopopup->setEnabled( e == "true" );

    connect( view->document(), SIGNAL(variableChanged(const QString &, const QString &)),
             this, SLOT(slotVariableChanged(const QString &, const QString &)) );
  }
}

DocWordCompletionPluginView::~DocWordCompletionPluginView()
{
  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(m_view);
  if (cci) cci->unregisterCompletionProvider(this);

  delete d;
  d=0;
}

void DocWordCompletionPluginView::settreshold( uint t )
{
  d->treshold = t;
}

void DocWordCompletionPluginView::completeBackwards()
{
  complete( false );
}

void DocWordCompletionPluginView::completeForwards()
{
  complete();
}

// Pop up the editors completion list if applicable
void DocWordCompletionPluginView::popupCompletionList( QString w )
{
  if ( w.isEmpty() )
    w = word();
  if ( w.isEmpty() )
    return;

  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>( m_view );
  Q_UNUSED( cci );
#ifdef __GNUC__
  #warning cci->showCompletionBox( allMatches( w ), w.length() );
#endif
}

void DocWordCompletionPluginView::toggleAutoPopup()
{
  if ( d->autopopup->isChecked() ) {
    if ( ! connect( m_view, SIGNAL(textInserted ( KTextEditor::View *, const KTextEditor::Cursor &, const QString & )),
         this, SLOT(autoPopupCompletionList()) ))
    {
      connect( m_view->document(), SIGNAL(textChanged(KTextEditor::View *)), this, SLOT(autoPopupCompletionList()) );
    }
  } else {
    disconnect( m_view->document(), SIGNAL(textChanged(KTextEditor::View *)), this, SLOT(autoPopupCompletionList()) );
    disconnect( m_view, SIGNAL(textInserted( KTextEditor::View *, const KTextEditor::Cursor &, const QString &)),
                this, SLOT(autoPopupCompletionList()) );

  }
}

const KTextEditor::CompletionData DocWordCompletionPluginView::completionData(KTextEditor::View*,enum KTextEditor::CompletionType comptype, const
	KTextEditor::Cursor&, const QString&,const KTextEditor::Cursor& pos , const QString& line)
{
  kDebug()<<"Should we provide a completion list?"<<endl;
  if ((!d->autopopup->isChecked()) && (comptype==KTextEditor::CompletionAsYouType)) return KTextEditor::CompletionData::Null();
  QString w=word(pos.column(),line);
  kDebug()<<"Checking word length"<<endl;
  if (w.length() >= (int)d->treshold) {
    {  //showCompletionBox( allMatches( w ), w.length() );
      kDebug()<<"About to return a completion list"<<endl;
      KTextEditor::Cursor newCursor=KTextEditor::Cursor(pos.line(),pos.column()-w.length());
      kDebug()<<"newCursor"<<newCursor.line()<<"/"<<newCursor.column()<<" m_oldCursor"<<m_oldCursor.line()<<"/"<<m_oldCursor.column()<<endl;
      kDebug()<<"m_oldWord:"<<m_oldWord<<" w:"<<w<<endl;
      kDebug()<<"m_completionData.isValid()"<<m_completionData.isValid()<<endl;
      if ( ((!m_oldWord.isEmpty()) && (w.indexOf(m_oldWord)==0)) && m_completionData.isValid() //perhaps there should be some kind of invalid cursor
        && (m_oldCursor==newCursor))
      return m_completionData;
      m_oldWord=w;
      m_oldCursor=newCursor;
      m_completionData=KTextEditor::CompletionData(allMatches(w),newCursor,true);
      return m_completionData;
    }
  } else {
    m_completionData=KTextEditor::CompletionData::Null();
    return m_completionData;
  }
}

// for autopopup FIXME - don't pop up if reuse word is inserting
void DocWordCompletionPluginView::autoPopupCompletionList()
{
  if ( ! m_view->hasFocus() ) return;
  QString w = word();
  if ( w.length() >= (int)d->treshold )
  {
      popupCompletionList( w );
  }
}

// Contributed by <brain@hdsnet.hu>
void DocWordCompletionPluginView::shellComplete()
{
#ifdef __GNUC__
#warning reimplement me
#endif
#if 0
    // find the word we are typing
  KTextEditor::Cursor pos = m_view->cursorPosition();

  QString wrd = word();
  if (wrd.isEmpty())
    return;

  Q3ValueList < KTextEditor::CompletionEntry > matches = allMatches(wrd);
  if (matches.size() == 0)
    return;
  QString partial = findLongestUnique(matches);
  if (partial.length() == wrd.length())
  {
    KTextEditor::CodeCompletionInterface * cci = codeCompletionInterface(m_view);
    cci->showCompletionBox(matches, wrd.length());
  }
  else
  {
    partial.remove(0, wrd.length());
    m_view->document()->insertText(pos, partial);
  }
#endif
}

// Do one completion, searching in the desired direction,
// if possible
void DocWordCompletionPluginView::complete( bool fw )
{
  // find the word we are typing
  int cline, ccol;
  m_view->cursorPosition().position ( cline, ccol );
  QString wrd = word();
  if ( wrd.isEmpty() ) return;

  /* IF the current line is equal to the previous line
     AND the position - the length of the last inserted string
          is equal to the old position
     AND the lastinsertedlength last characters of the word is
          equal to the last inserted string
          */
  if ( cline == (int)d-> cline &&
          ccol - d->lilen == d->ccol &&
          wrd.endsWith( d->lastIns ) )
  {
    // this is a repeted activation
    ccol = d->ccol;
    wrd = d->last;
  }
  else
  {
    d->cline = cline;
    d->ccol = ccol;
    d->last = wrd;
    d->lastIns.clear();
    d->line = d->cline;
    d->col = d->ccol - wrd.length();
    d->lilen = 0;
  }

  d->re.setPattern( "\\b" + wrd + "(\\w+)" );
  int inc = fw ? 1 : -1;
  int pos ( 0 );
  QString ln = m_view->document()->line( d->line );

  if ( ! fw )
    ln = ln.mid( 0, d->col );

  while ( true )
  {
    pos = fw ?
      d->re.indexIn( ln, d->col ) :
      d->re.lastIndexIn( ln, d->col );

    if ( pos > -1 ) // we matched a word
    {
      QString m = d->re.cap( 1 );
      if ( m != d->lastIns )
      {
        // we got good a match! replace text and return.
        if ( d->lilen )
          m_view->document()->removeText( KTextEditor::Range(d->cline, d->ccol, d->cline, d->ccol + d->lilen) );
        m_view->document()->insertText( KTextEditor::Cursor (d->cline, d->ccol), m );

        d->lastIns = m;
        d->lilen = m.length();
        d->col = pos; // for next try

        if ( fw )
          d->col += m.length();

        return;
      }

      // equal to last one, continue
      else
      {
        d->col = pos; // for next try
        if ( fw )
          d->col += m.length();
        else // FIXME figure out if all of that is really necessary
        {
          if ( pos == 0 )
          {
            if ( d->line > 0 )
            {
              d->line += inc;
              ln = m_view->document()->line( d->line );
              d->col = ln.length();
            }
            else
            {
              KNotification::beep();
              return;
            }
          }
          else
            d->col--;
        }
      }
    }

    else  // no match
    {
      if ( ! fw && d->line == 0)
      {
        KNotification::beep();
        return;
      }
      else if ( fw && d->line >= (uint)m_view->document()->lines() )
      {
        KNotification::beep();
        return;
      }

      d->line += inc;
      if ( fw )
        d->col++;

      ln = m_view->document()->line( d->line );
      d->col = fw ? 0 : ln.length();
    }
  } // while true
}

// Contributed by <brain@hdsnet.hu>
QString DocWordCompletionPluginView::findLongestUnique(const Q3ValueList < KTextEditor::CompletionItem > &matches)
{
  QString partial = matches.front().text();
  Q3ValueList < KTextEditor::CompletionItem >::const_iterator i = matches.begin();
  for (++i; i != matches.end(); ++i)
  {
    if (!(*i).text().startsWith(partial))
    {
      while(partial.length() > 0)
      {
        partial.remove(partial.length() - 1, 1);
        if ((*i).text().startsWith(partial))
        {
          break;
        }
      }
      if (partial.length() == 0)
        return QString();
    }
  }

  return partial;
}

// Return the string to complete (the letters behind the cursor)
QString DocWordCompletionPluginView::word(int col, const QString& line)
{
  //KTextEditor::Cursor end = m_view->cursorPosition();

  if ( ! col) return QString(); // no word

  //KTextEditor::Cursor start (end.line(), 0);

  d->re.setPattern( "\\b(\\w+)$" );
  if ( d->re.lastIndexIn(line.left(col)
        //m_view->document()->text( start, end )
        ) < 0 )
    return QString(); // no word
  return d->re.cap( 1 );
}

// Return the string to complete (the letters behind the cursor)
QString DocWordCompletionPluginView::word()
{
  KTextEditor::Cursor end = m_view->cursorPosition();

  if ( ! end.column()) return QString(); // no word

  KTextEditor::Cursor start (end.line(), 0);

  d->re.setPattern( "\\b(\\w+)$" );
  if ( d->re.lastIndexIn(
        m_view->document()->text( KTextEditor::Range(start, end) )
        ) < 0 )
    return QString(); // no word
  return d->re.cap( 1 );
}


// Scan throughout the entire document for possible completions,
// ignoring any dublets
QList<KTextEditor::CompletionItem> DocWordCompletionPluginView::allMatches( const QString &word )
{
  QList<KTextEditor::CompletionItem> l;
  int i( 0 );
  int pos( 0 );
  d->re.setPattern( "\\b("+word+"\\w+)" );
  QString s, m;
  Q3Dict<int> seen; // maybe slow with > 17 matches
  int sawit(1);    // to ref for the dict

  while( i < m_view->document()->lines() )
  {
    s = m_view->document()->line( i );
    pos = 0;
    while ( pos >= 0 )
    {
      pos = d->re.indexIn( s, pos );
      if ( pos >= 0 )
      {
        m = d->re.cap( 1 );
        if ( ! seen[ m ] ) {
          seen.insert( m, &sawit );
          l.append( KTextEditor::CompletionItem(m));
        }
        pos += d->re.matchedLength();
      }
    }
    i++;
  }
  return l;
}

void DocWordCompletionPluginView::slotVariableChanged( const QString &var, const QString &val )
{
  if ( var == "wordcompletion-autopopup" )
    d->autopopup->setEnabled( val == "true" );
  else if ( var == "wordcompletion-treshold" )
    d->treshold = val.toInt();
}
//END

//BEGIN DocWordCompletionConfigPage
DocWordCompletionConfigPage::DocWordCompletionConfigPage( DocWordCompletionPlugin *completion, QWidget *parent, const char * )
  : KTextEditor::ConfigPage( parent )
  , m_completion( completion )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  lo->setSpacing( KDialog::spacingHint() );

  cbAutoPopup = new QCheckBox( i18n("Automatically &show completion list"), this );
  lo->addWidget( cbAutoPopup );

  KHBox *hb = new KHBox( this );
  hb->setSpacing( KDialog::spacingHint() );
  lo->addWidget( hb );
  QLabel *l = new QLabel( i18nc(
      "Translators: This is the first part of two strings which will comprise the "
      "sentence 'Show completions when a word is at least N characters'. The first "
      "part is on the right side of the N, which is represented by a spinbox "
      "widget, followed by the second part: 'characters long'. Characters is a "
      "ingeger number between and including 1 and 30. Feel free to leave the "
      "second part of the sentence blank if it suits your language better. ",
      "Show completions &when a word is at least"), hb );
  sbAutoPopup = new QSpinBox( hb );
  sbAutoPopup->setRange( 1, 30 );
  sbAutoPopup->setSingleStep( 1 );
  l->setBuddy( sbAutoPopup );
  lSbRight = new QLabel( i18nc(
      "This is the second part of two strings that will comprise teh sentence "
      "'Show completions when a word is at least N characters'",
      "characters long."), hb );

  cbAutoPopup->setWhatsThis(i18n(
      "Enable the automatic completion list popup as default. The popup can "
      "be disabled on a view basis from the 'Tools' menu.") );
  sbAutoPopup->setWhatsThis(i18n(
      "Define the length a word should have before the completion list "
      "is displayed.") );

  cbAutoPopup->setChecked( m_completion->autoPopupEnabled() );
  sbAutoPopup->setValue( m_completion->treshold() );

  lo->addStretch();
}

void DocWordCompletionConfigPage::apply()
{
  m_completion->setAutoPopupEnabled( cbAutoPopup->isChecked() );
  m_completion->setTreshold( sbAutoPopup->value() );
  m_completion->writeConfig();
}

void DocWordCompletionConfigPage::reset()
{
  cbAutoPopup->setChecked( m_completion->autoPopupEnabled() );
  sbAutoPopup->setValue( m_completion->treshold() );
}

void DocWordCompletionConfigPage::defaults()
{
  cbAutoPopup->setChecked( true );
  sbAutoPopup->setValue( 3 );
}

//END DocWordCompletionConfigPage

#include "docwordcompletion.moc"
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
