/* This file is part of the KDE libraries
   Copyright (C) 2003 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>

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

#include "kateview.h"
#include "kateview.moc"

#include "kateviewinternal.h"
#include "katerenderer.h"
#include "katedocument.h"
#include "katefactory.h"
#include "katehighlight.h"
#include "kateviewdialog.h"
#include "kateiconborder.h"
#include "katedialogs.h"
#include "katefiledialog.h"
#include "katetextline.h"
#include "kateexportaction.h"
#include "katecodefoldinghelpers.h"
#include "katecodecompletion.h"
#include "kateviewhighlightaction.h"
#include "katesearch.h"
#include "katebookmarks.h"
#include "katebrowserextension.h"
#include "katesearch.h"
#include "katecmdline.h"
#include "kateconfig.h"

#include <ktexteditor/plugin.h>

#include <kparts/event.h>

#include <kconfig.h>
#include <kurldrag.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kcursor.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kxmlguifactory.h>
#include <kaccel.h>
#include <klibloader.h>

#include <qfont.h>
#include <qfileinfo.h>
#include <qstyle.h>
#include <qevent.h>
#include <qpopupmenu.h>
#include <qlayout.h>

KateView::KateView( KateDocument *doc, QWidget *parent, const char * name )
    : Kate::View( doc, parent, name )
    , m_doc( doc )
    , m_search( new KateSearch( this ) )
    , m_bookmarks( new KateBookmarks( this ) )
    , m_rmbMenu( 0 )
    , m_cmdLine (0)
    , m_cmdLineOn (false)
    , m_active( false )
    , m_hasWrap( false )
    , m_userWantsFoldingMarkersOff( false )
    , m_startingUp (true)
{
  KateFactory::registerView( this );
  m_config = new KateViewConfig (this);

  m_renderer = new KateRenderer(doc, this);

  m_grid = new QGridLayout (this, 3, 3);

  m_grid->setRowStretch ( 0, 10 );
  m_grid->setRowStretch ( 1, 0 );
  m_grid->setColStretch ( 0, 0 );
  m_grid->setColStretch ( 1, 10 );
  m_grid->setColStretch ( 2, 0 );

  m_viewInternal = new KateViewInternal( this, doc );
  m_grid->addWidget (m_viewInternal, 0, 1);

  setClipboardInterfaceDCOPSuffix (viewDCOPSuffix());
  setCodeCompletionInterfaceDCOPSuffix (viewDCOPSuffix());
  setDynWordWrapInterfaceDCOPSuffix (viewDCOPSuffix());
  setPopupMenuInterfaceDCOPSuffix (viewDCOPSuffix());
  setSessionConfigInterfaceDCOPSuffix (viewDCOPSuffix());
  setViewCursorInterfaceDCOPSuffix (viewDCOPSuffix());
  setViewStatusMsgInterfaceDCOPSuffix (viewDCOPSuffix());

  setInstance( KateFactory::instance() );
  doc->addView( this );

  setFocusProxy( m_viewInternal );
  setFocusPolicy( StrongFocus );

  if (!doc->m_bSingleViewMode) {
    setXMLFile( "katepartui.rc" );
  } else {
    if( doc->m_bReadOnly )
      setXMLFile( "katepartreadonlyui.rc" );
    else
      setXMLFile( "katepartui.rc" );
  }

  setupConnections();
  setupActions();
  setupEditActions();
  setupCodeFolding();
  setupCodeCompletion();
  setupViewPlugins();

  // update the enabled state of the undo/redo actions...
  slotNewUndo();

  m_startingUp = false;
  updateConfig ();

  m_viewInternal->show ();

  /*test texthint
  connect(this,SIGNAL(needTextHint(int, int, QString &)),
  this,SLOT(slotNeedTextHint(int, int, QString &)));
  enableTextHints(1000);
  test texthint*/
}

KateView::~KateView()
{
  if (m_doc && !m_doc->m_bSingleViewMode)
    m_doc->removeView( this );

  delete m_viewInternal;
  delete m_codeCompletion;

  delete m_renderer;

  delete m_config;
  KateFactory::deregisterView (this);
}

void KateView::setupConnections()
{
  connect( m_doc, SIGNAL(undoChanged()),
           this, SLOT(slotNewUndo()) );
  connect( m_doc, SIGNAL(hlChanged()),
           this, SLOT(updateFoldingMarkersAction()) );
  connect( m_doc, SIGNAL(canceled(const QString&)),
           this, SLOT(slotSaveCanceled(const QString&)) );
  connect( m_viewInternal, SIGNAL(dropEventPass(QDropEvent*)),
           this,           SIGNAL(dropEventPass(QDropEvent*)) );
  connect(this,SIGNAL(cursorPositionChanged()),this,SLOT(slotStatusMsg()));
  connect(this,SIGNAL(newStatus()),this,SLOT(slotStatusMsg()));
  connect(m_doc, SIGNAL(undoChanged()), this, SLOT(slotStatusMsg()));

  if ( m_doc->m_bBrowserView )
  {
    connect( this, SIGNAL(dropEventPass(QDropEvent*)),
             this, SLOT(slotDropEventPass(QDropEvent*)) );
  }
}

void KateView::setupActions()
{
  KActionCollection *ac = this->actionCollection ();
  KAction *a;

  m_toggleWriteLock = 0;

  m_cut = a=KStdAction::cut(this, SLOT(cut()), ac);
  a->setWhatsThis(i18n("Cut the selected text and move it to the clipboard"));

  m_paste = a=KStdAction::paste(this, SLOT(paste()), ac);
  a->setWhatsThis(i18n("Paste previously copied or cut clipboard contents"));

  m_copy = a=KStdAction::copy(this, SLOT(copy()), ac);
  a->setWhatsThis(i18n( "Use this command to copy the currently selected text to the system clipboard."));


  if (!m_doc->m_bReadOnly)
  {
    KStdAction::spelling( m_doc, SLOT(spellcheck()), ac );

    a=KStdAction::save(this, SLOT(save()), ac);
    a->setWhatsThis(i18n("Save the current document"));

    a=m_editUndo = KStdAction::undo(m_doc, SLOT(undo()), ac);
    a->setWhatsThis(i18n("Revert the most recent editing actions"));

    a=m_editRedo = KStdAction::redo(m_doc, SLOT(redo()), ac);
    a->setWhatsThis(i18n("Revert the most recent undo operation"));

    (new KAction(i18n("Apply Word Wrap"), "", 0, m_doc, SLOT(applyWordWrap()), ac, "tools_apply_wordwrap"))->setWhatsThis(
  i18n("Use this command to wrap all lines of the current document which are longer than the width of the"
    " current view, to fit into this view.<br><br> This is a static word wrap, meaning it is not updated"
    " when the view is resized."));

    // setup Tools menu
    a=new KAction(i18n("&Indent"), "indent", Qt::CTRL+Qt::Key_I, this, SLOT(indent()),
                              ac, "tools_indent");
    a->setWhatsThis(i18n("Use this to indent a selected block of text.<br><br>"
    "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));
    a=new KAction(i18n("&Unindent"), "unindent", Qt::CTRL+Qt::SHIFT+Qt::Key_I, this, SLOT(unIndent()),
                                ac, "tools_unindent");
    a->setWhatsThis(i18n("Use this to unindent a selected block of text."));
    a=new KAction(i18n("&Clean Indentation"), 0, this, SLOT(cleanIndent()),
                                   ac, "tools_cleanIndent");
    a->setWhatsThis(i18n("Use this to clean the indentation of a selected block of text (only tabs/only spaces)<br><br>"
    "You can configure whether tabs should be honored and used or replaced with spaces, in the configuration dialog."));

    a=new KAction(i18n("C&omment"), CTRL+Qt::Key_D, this, SLOT(comment()),
                               ac, "tools_comment");
    a->setWhatsThis(i18n("This command comments out the current line or a selected block of text.<BR><BR>"
    "The characters for single/multiple line comments are defined within the language's highlighting."));

    a=new KAction(i18n("Unco&mment"), CTRL+SHIFT+Qt::Key_D, this, SLOT(uncomment()),
                                 ac, "tools_uncomment");
    a->setWhatsThis(i18n("This command removes comments from the current line or a selected block of text.<BR><BR>"
    "The characters for single/multiple line comments are defined within the language's highlighting."));
    a = m_toggleWriteLock = new KToggleAction(
                i18n("Write &Lock"), 0, 0,
                this, SLOT( toggleWriteLock() ),
                ac, "tools_toggle_write_lock" );
    a->setWhatsThis( i18n("Lock/unlock the document for writing") );
  }
  else
  {
    m_cut->setEnabled (false);
    m_paste->setEnabled (false);
  }

  a=KStdAction::print( m_doc, SLOT(print()), ac );
  a->setWhatsThis(i18n("Print the current document."));

  a=new KAction(i18n("Reloa&d"), "reload", KStdAccel::reload(), this, SLOT(reloadFile()), ac, "file_reload");
  a->setWhatsThis(i18n("Reload the current document from disk."));

  a=KStdAction::saveAs(this, SLOT(saveAs()), ac);
  a->setWhatsThis(i18n("Save the current document to disk, with a name of your choice."));

  a=KStdAction::gotoLine(this, SLOT(gotoLine()), ac);
  a->setWhatsThis(i18n("This command opens a dialog and lets you choose a line that you want the cursor to move to."));

  a=new KAction(i18n("&Configure Editor..."), 0, m_doc, SLOT(configDialog()),ac, "set_confdlg");
  a->setWhatsThis(i18n("Configure various aspects of this editor."));

  m_setHighlight = m_doc->hlActionMenu (i18n("&Highlight Mode"),ac,"set_highlight");
  m_doc->exportActionMenu (i18n("E&xport"),ac,"file_export");

  m_selectAll = a=KStdAction::selectAll(m_doc, SLOT(selectAll()), ac);
  a->setWhatsThis(i18n("Select the entire text of the current document."));

  m_deSelect = a=KStdAction::deselect(m_doc, SLOT(clearSelection()), ac);
  a->setWhatsThis(i18n("If you have selected something within the current document, this will no longer be selected."));

  a=new KAction(i18n("Increase Font Sizes"), "viewmag+", 0, m_viewInternal, SLOT(slotIncFontSizes()), ac, "incFontSizes");
  a->setWhatsThis(i18n("This increases the display font size."));

  a=new KAction(i18n("Decrease Font Sizes"), "viewmag-", 0, m_viewInternal, SLOT(slotDecFontSizes()), ac, "decFontSizes");
  a->setWhatsThis(i18n("This decreases the display font size."));

  a=new KAction(i18n("T&oggle Block Selection"), CTRL + SHIFT + Key_B, m_doc, SLOT(toggleBlockSelectionMode()), ac, "set_verticalSelect");
  a->setWhatsThis(i18n("This command allows switching between the normal (line based) selection mode and the block selection mode."));

  a=new KAction(i18n("Toggle &Insert"), Key_Insert, this, SLOT(toggleInsert()), ac, "set_insert" );
  a->setWhatsThis(i18n("Choose whether you want the text you type to be inserted or to overwrite existing text."));

  KToggleAction *toggleAction;
   a= m_toggleDynWrap = toggleAction = new KToggleAction(
    i18n("&Dynamic Word Wrap"), Key_F10,
    this, SLOT(toggleDynWordWrap()),
    ac, "view_dynamic_word_wrap" );
  a->setWhatsThis(i18n("If this option is checked, the text lines will be wrapped at the view border on the screen."));

  a= m_setDynWrapIndicators = new KSelectAction(i18n("Dynamic Word Wrap Indicators"), 0, ac, "dynamic_word_wrap_indicators");
  a->setWhatsThis(i18n("Choose when the Dynamic Word Wrap Indicators should be displayed"));

  connect(m_setDynWrapIndicators, SIGNAL(activated(int)), this, SLOT(setDynWrapIndicators(int)));
  QStringList list2;
  list2.append("&Off");
  list2.append("Follow &Line Numbers");
  list2.append("&Always On");
  m_setDynWrapIndicators->setItems(list2);

  a= toggleAction=m_toggleFoldingMarkers = new KToggleAction(
    i18n("Show Folding &Markers"), Key_F9,
    this, SLOT(toggleFoldingMarkers()),
    ac, "view_folding_markers" );
  a->setWhatsThis(i18n("You can choose if the codefolding marks should be shown, if codefolding is possible."));

   a= m_toggleIconBar = toggleAction = new KToggleAction(
    i18n("Show &Icon Border"), Key_F6,
    this, SLOT(toggleIconBorder()),
    ac, "view_border");
  a=toggleAction;
  a->setWhatsThis(i18n("Show/hide the icon border.<BR><BR> The icon border shows bookmark symbols, for instance."));

  a= m_toggleLineNumbers = toggleAction = new KToggleAction(
     i18n("Show &Line Numbers"), Key_F11,
     this, SLOT(toggleLineNumbersOn()),
     ac, "view_line_numbers" );
  a->setWhatsThis(i18n("Show/hide the line numbers on the left hand side of the view."));

  a = m_toggleWWMarker = new KToggleAction(
        i18n("Show &Word Wrap Marker"), 0,
        this, SLOT( toggleWWMarker() ),
        ac, "view_word_wrap_marker" );
  a->setWhatsThis( i18n(
        "Show/hide the Word Wrap Marker, a vertical line drawn at the word "
        "wrap column as defined in the editing properties" ));

  a= toggleAction = toggleAction = new KToggleAction(
     i18n("Show C&ommand Line"), 0,
     this, SLOT(toggleCmdLine()),
     ac, "view_cmd_line" );
  a->setWhatsThis(i18n("Show/hide the command line on the bottom of the view."));

  a=m_setEndOfLine = new KSelectAction(i18n("&End of Line"), 0, ac, "set_eol");
  a->setWhatsThis(i18n("Choose which line endings should be used, when you save the document"));

  connect(m_setEndOfLine, SIGNAL(activated(int)), this, SLOT(setEol(int)));
  QStringList list;
  list.append("&UNIX");
  list.append("&Windows/DOS");
  list.append("&Macintosh");
  m_setEndOfLine->setItems(list);

  m_setEncoding = new KSelectAction(i18n("Set &Encoding"), 0, ac, "set_encoding");
  connect(m_setEncoding, SIGNAL(activated(const QString&)), this, SLOT(slotSetEncoding(const QString&)));
  list = KGlobal::charsets()->descriptiveEncodingNames();
  list.prepend( i18n( "Auto" ) );
  m_setEncoding->setItems(list);

  m_search->createActions( ac );
  m_bookmarks->createActions( ac );

  selectionChanged ();

  connect (m_doc, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
}

void KateView::setupEditActions()
{
  m_editActions = new KActionCollection( m_viewInternal );
  KActionCollection* ac = m_editActions;

  new KAction(
    i18n("Move Word Left"),                         CTRL + Key_Left,
    this,SLOT(wordLeft()),
    ac, "word_left" );
  new KAction(
    i18n("Select Character Left"),          SHIFT +        Key_Left,
    this,SLOT(shiftCursorLeft()),
    ac, "select_char_left" );
  new KAction(
    i18n("Select Word Left"),               SHIFT + CTRL + Key_Left,
    this, SLOT(shiftWordLeft()),
    ac, "select_word_left" );

  new KAction(
    i18n("Move Word Right"),                        CTRL + Key_Right,
    this, SLOT(wordRight()),
    ac, "word_right" );
  new KAction(
    i18n("Select Character Right"),         SHIFT        + Key_Right,
    this, SLOT(shiftCursorRight()),
    ac, "select_char_right" );
  new KAction(
    i18n("Select Word Right"),              SHIFT + CTRL + Key_Right,
    this,SLOT(shiftWordRight()),
    ac, "select_word_right" );

  new KAction(
    i18n("Move to Beginning of Line"),                      Key_Home,
    this, SLOT(home()),
    ac, "beginning_of_line" );
  new KAction(
    i18n("Move to Beginning of Document"),           CTRL + Key_Home,
    this, SLOT(top()),
    ac, "beginning_of_document" );
  new KAction(
    i18n("Select to Beginning of Line"),     SHIFT +        Key_Home,
    this, SLOT(shiftHome()),
    ac, "select_beginning_of_line" );
  new KAction(
    i18n("Select to Beginning of Document"), SHIFT + CTRL + Key_Home,
    this, SLOT(shiftTop()),
    ac, "select_beginning_of_document" );

  new KAction(
    i18n("Move to End of Line"),                            Key_End,
    this, SLOT(end()),
    ac, "end_of_line" );
  new KAction(
    i18n("Move to End of Document"),                 CTRL + Key_End,
    this, SLOT(bottom()),
    ac, "end_of_document" );
  new KAction(
    i18n("Select to End of Line"),           SHIFT +        Key_End,
    this, SLOT(shiftEnd()),
    ac, "select_end_of_line" );
  new KAction(
    i18n("Select to End of Document"),       SHIFT + CTRL + Key_End,
    this, SLOT(shiftBottom()),
    ac, "select_end_of_document" );

  new KAction(
    i18n("Select to Previous Line"),                SHIFT + Key_Up,
    this, SLOT(shiftUp()),
    ac, "select_line_up" );
  new KAction(
    i18n("Scroll Line Up"),"",              CTRL +          Key_Up,
    this, SLOT(scrollUp()),
    ac, "scroll_line_up" );

  new KAction(
    i18n("Select to Next Line"),                    SHIFT + Key_Down,
    this, SLOT(shiftDown()),
    ac, "select_line_down" );
  new KAction(
    i18n("Scroll Line Down"),               CTRL +          Key_Down,
    this, SLOT(scrollDown()),
    ac, "scroll_line_down" );

  new KAction(
    i18n("Scroll Page Up"),                                 Key_PageUp,
    this, SLOT(pageUp()),
    ac, "scroll_page_up" );
  new KAction(
    i18n("Select Page Up"),                         SHIFT + Key_PageUp,
    this, SLOT(shiftPageUp()),
    ac, "select_page_up" );
  new KAction(
    i18n("Move to Top of View"),             CTRL +         Key_PageUp,
    this, SLOT(topOfView()),
    ac, "move_top_of_view" );

  new KAction(
    i18n("Scroll Page Down"),                               Key_PageDown,
    this, SLOT(pageDown()),
    ac, "scroll_page_down" );
  new KAction(
    i18n("Select Page Down"),                       SHIFT + Key_PageDown,
    this, SLOT(shiftPageDown()),
    ac, "select_page_down" );
  new KAction(
    i18n("Move to Bottom of View"),          CTRL +         Key_PageDown,
    this, SLOT(bottomOfView()),
    ac, "move_bottom_of_view" );
  new KAction(
    i18n("Move to Matching Bracket"),               CTRL + Key_6,
    this, SLOT(toMatchingBracket()),
    ac, "to_matching_bracket" );
  new KAction(
    i18n("Select to Matching Bracket"),      SHIFT +  CTRL + Key_6,
    this, SLOT(shiftToMatchingBracket()),
    ac, "select_matching_bracket" );

  new KAction(
    i18n("Switch to Command Line"),      Qt::Key_F7,
    this, SLOT(switchToCmdLine()),
    ac, "switch_to_cmd_line" );

  // anders: shortcuts doing any changes should not be created in browserextension
  if ( !m_doc->m_bReadOnly )
  {
    new KAction(
      i18n("Transpose Characters"),           CTRL          + Key_T,
      this, SLOT(transpose()),
      ac, "transpose_char" );
//   new KAction(
//      i18n("Transpose Words"),                CTRL + SHIFT + Key_T,
//      this, SLOT(transposeWord()),
//      ac, "transpose_word" );
//   new KAction(
//      i18n("Transpose Line"),                 CTRL + SHIFT + Key_T, ??? What key combo?
//      this, SLOT(transposeLine()),
//      ac, "transpose_line" );

//   new KAction(
//     i18n("Delete Word"),                    CTRL + Key_K, ??? What key combo?
//     this, SLOT(killWord()),
//     ac, "delete_word" );
    new KAction(
      i18n("Delete Line"),                    CTRL + Key_K,
      this, SLOT(killLine()),
      ac, "delete_line" );

    new KAction(
      i18n("Delete Word Left"),               CTRL + Key_Backspace,
      this, SLOT(deleteWordLeft()),
      ac, "delete_word_left" );

    new KAction(
      i18n("Delete Word Right"),              CTRL + Key_Delete,
      this, SLOT(deleteWordRight()),
      ac, "delete_word_right" );
  }

  connect( this, SIGNAL(gotFocus(Kate::View*)),
           this, SLOT(slotGotFocus()) );
  connect( this, SIGNAL(lostFocus(Kate::View*)),
           this, SLOT(slotLostFocus()) );

  if( hasFocus() )
    slotGotFocus();
  else
    slotLostFocus();

  m_editActions->readShortcutSettings();
}

void KateView::setupCodeFolding()
{
  KActionCollection *ac=this->actionCollection();
  new KAction( i18n("Collapse Toplevel"), CTRL+SHIFT+Key_Minus,
       m_doc->foldingTree(),SLOT(collapseToplevelNodes()),ac,"folding_toplevel");
  new KAction( i18n("Expand Toplevel"), CTRL+SHIFT+Key_Plus,
       this,SLOT(slotExpandToplevel()),ac,"folding_expandtoplevel");
  new KAction( i18n("Collapse One Local Level"), CTRL+Key_Minus,
       this,SLOT(slotCollapseLocal()),ac,"folding_collapselocal");
  new KAction( i18n("Expand One Local Level"), CTRL+Key_Plus,
       this,SLOT(slotExpandLocal()),ac,"folding_expandlocal");

  KAccel* debugAccels = new KAccel(this,this);
  debugAccels->insert("KATE_DUMP_REGION_TREE",i18n("Show the code folding region tree"),"","Ctrl+Shift+Alt+D",m_doc,SLOT(dumpRegionTree()));
  debugAccels->setEnabled(true);
}

void KateView::slotExpandToplevel()
{
  m_doc->foldingTree()->expandToplevelNodes(m_doc->numLines());
}

void KateView::slotCollapseLocal()
{
  int realLine = m_doc->foldingTree()->collapseOne(cursorLine());
  if (realLine != -1)
    // TODO rodda: fix this to only set line and allow internal view to chose column
    setCursorPosition(realLine, cursorColumn());
}

void KateView::slotExpandLocal()
{
  m_doc->foldingTree()->expandOne(cursorLine(), m_doc->numLines());
}

void KateView::setupCodeCompletion()
{
  m_codeCompletion = new KateCodeCompletion(this);
  connect( m_codeCompletion, SIGNAL(completionAborted()),
           this,             SIGNAL(completionAborted()));
  connect( m_codeCompletion, SIGNAL(completionDone()),
           this,             SIGNAL(completionDone()));
  connect( m_codeCompletion, SIGNAL(argHintHidden()),
           this,             SIGNAL(argHintHidden()));
  connect( m_codeCompletion, SIGNAL(completionDone(KTextEditor::CompletionEntry)),
           this,             SIGNAL(completionDone(KTextEditor::CompletionEntry)));
  connect( m_codeCompletion, SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString*)),
           this,             SIGNAL(filterInsertString(KTextEditor::CompletionEntry*,QString*)));
}

void KateView::setupViewPlugins()
{
  m_doc->enableAllPluginsGUI (this);
}

void KateView::slotGotFocus()
{
  m_editActions->accel()->setEnabled( true );
}

void KateView::slotLostFocus()
{
  m_editActions->accel()->setEnabled( false );
}

void KateView::setDynWrapIndicators(int mode)
{
  config()->setDynWordWrapIndicators (mode);
}

void KateView::slotStatusMsg ()
{
  QString ovrstr;
  if (m_doc->isReadWrite())
  {
    if (m_doc->configFlags() & KateDocument::cfOvr)
      ovrstr = i18n(" OVR ");
    else
      ovrstr = i18n(" INS ");
  }
  else
    ovrstr = i18n(" R/O ");

  uint r = cursorLine() + 1;
  uint c = cursorColumn();

  QString s1 = i18n(" Line: %1").arg(KGlobal::locale()->formatNumber(r, 0));
  QString s2 = i18n(" Col: %1").arg(KGlobal::locale()->formatNumber(c, 0));

  QString modstr = m_doc->isModified() ? QString (" * ") : QString ("   ");
  QString blockstr = m_doc->blockSelectionMode() ? i18n(" BLK ") : i18n(" NORM ");

  emit viewStatusMsg (s1 + s2 + " " + ovrstr + blockstr + modstr);
}

void KateView::slotSelectionTypeChanged()
{
  emit newStatus();
}

void KateView::reloadFile()
{
  // save cursor position
  uint cl = cursorLine();
  uint cc = cursorColumn();

  // save bookmarks
  m_doc->reloadFile();

  if (m_doc->numLines() >= cl)
    setCursorPosition( cl, cc );
}

void KateView::slotUpdate()
{
  emit newStatus();
  slotNewUndo();
  if ( m_toggleWriteLock && m_toggleWriteLock->isChecked() == m_doc->isReadWrite() )
  {
    m_toggleWriteLock->setChecked( ! m_doc->isReadWrite() );
  }
}

void KateView::slotNewUndo()
{
  if (m_doc->m_bReadOnly)
    return;

  if ((m_doc->undoCount() > 0) != m_editUndo->isEnabled())
    m_editUndo->setEnabled(m_doc->undoCount() > 0);

  if ((m_doc->redoCount() > 0) != m_editRedo->isEnabled())
    m_editRedo->setEnabled(m_doc->redoCount() > 0);
}

void KateView::slotDropEventPass( QDropEvent * ev )
{
  KURL::List lstDragURLs;
  bool ok = KURLDrag::decode( ev, lstDragURLs );

  KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( doc() );
  if ( ok && ext )
    emit ext->openURLRequest( lstDragURLs.first() );
}

void KateView::updateFoldingMarkersAction()
{
  setFoldingMarkersOn ( m_doc->highlight() && m_doc->highlight()->allowsFolding() && config()->foldingBar() &&
                        !m_userWantsFoldingMarkersOff );

  m_toggleFoldingMarkers->setChecked( foldingMarkersOn() );
  m_toggleFoldingMarkers->setEnabled( m_doc->highlight() && m_doc->highlight()->allowsFolding() );
}

void KateView::customEvent( QCustomEvent *ev )
{
    if ( KParts::GUIActivateEvent::test( ev ) && static_cast<KParts::GUIActivateEvent *>( ev )->activated() )
    {
        installPopup(static_cast<QPopupMenu *>(factory()->container("ktexteditor_popup", this) ) );
        return;
    }

    KTextEditor::View::customEvent( ev );
}

void KateView::contextMenuEvent( QContextMenuEvent *ev )
{
    if ( !m_doc || !m_doc->m_extension  )
        return;

    emit m_doc->m_extension->popupMenu( ev->globalPos(), m_doc->url(),
                                        QString::fromLatin1( "text/plain" ) );
    ev->accept();
}

bool KateView::setCursorPositionInternal( uint line, uint col, uint tabwidth )
{
  if( line > m_doc->lastLine() )
    return false;

  QString line_str = m_doc->textLine( line );

  uint z;
  uint x = 0;
  for (z = 0; z < line_str.length() && z < col; z++) {
    if (line_str[z] == QChar('\t')) x += tabwidth - (x % tabwidth); else x++;
  }

  m_viewInternal->updateCursor( KateTextCursor( line, x ) );

  return true;
}

void KateView::setOverwriteMode( bool b )
{
  if ( isOverwriteMode() && !b )
    m_doc->setConfigFlags( m_doc->_configFlags ^ KateDocument::cfOvr );
  else
    m_doc->setConfigFlags( m_doc->_configFlags | KateDocument::cfOvr );
}

void KateView::toggleInsert()
{
  m_doc->setConfigFlags(m_doc->_configFlags ^ KateDocument::cfOvr);
  emit newStatus();
}

bool KateView::canDiscard()
{
  return m_doc->closeURL();
}

void KateView::flush()
{
  m_doc->closeURL();
}

KateView::saveResult KateView::save()
{
  if( m_doc->url().fileName().isEmpty() || !doc()->isReadWrite() )
    return saveAs();

  // If document is new but has a name, check if saving it would
  // overwrite a file that has been created since the new doc
  // was created:
  if( m_doc->isNewDoc() && !checkOverwrite( m_doc->url() ) )
    return SAVE_CANCEL;

  if( !m_doc->save() ) {
    KMessageBox::sorry(this,
        i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

KateView::saveResult KateView::saveAs()
{
  KateFileDialog dialog(
    m_doc->url().url(),
    doc()->encoding(),
    this,
    i18n("Save File"),
    KFileDialog::Saving );
  dialog.setSelection( m_doc->url().fileName() );
  KateFileDialogData data = dialog.exec();

  if( data.url.isEmpty() || !checkOverwrite( data.url ) )
    return SAVE_CANCEL;

  m_doc->setEncoding( data.encoding );
  if( !m_doc->saveAs( data.url ) ) {
    KMessageBox::sorry(this,
      i18n("The file could not be saved. Please check if you have write permission."));
    return SAVE_ERROR;
  }

  return SAVE_OK;
}

bool KateView::checkOverwrite( KURL u )
{
  if( !u.isLocalFile() )
    return true;

  QFileInfo info( u.path() );
  if( !info.exists() )
    return true;

  return KMessageBox::Cancel != KMessageBox::warningContinueCancel( this,
    i18n( "A file named \"%1\" already exists. "
          "Are you sure you want to overwrite it?" ).arg( info.fileName() ),
    i18n( "Overwrite File?" ),
    i18n( "Overwrite" ) );
}

void KateView::slotSaveCanceled( const QString& error )
{
  if ( !error.isEmpty() ) // happens when cancelling a job
    KMessageBox::error( this, error );
}

void KateView::gotoLine()
{
  GotoLineDialog *dlg;

  dlg = new GotoLineDialog(this, m_viewInternal->getCursor().line() + 1, m_doc->numLines());

  if (dlg->exec() == QDialog::Accepted)
    gotoLineNumber( dlg->getLine() - 1 );

  delete dlg;
}

void KateView::gotoLineNumber( int line )
{
  m_viewInternal->updateCursor( KateTextCursor( line, 0 ) );
}

void KateView::readSessionConfig(KConfig *config)
{
  setCursorPositionReal (config->readNumEntry("CursorLine"), config->readNumEntry("CursorColumn"));
}

void KateView::writeSessionConfig(KConfig *config)
{
  config->writeEntry("CursorLine",m_viewInternal->cursor.line());
  config->writeEntry("CursorColumn",m_viewInternal->cursor.col());
}

void KateView::setEol(int eol)
{
  if (!doc()->isReadWrite())
    return;

  m_doc->eolMode = eol;
  m_doc->setModified(true);
}

void KateView::slotSetEncoding( const QString& descriptiveName )
{
  setEncoding( KGlobal::charsets()->encodingForName( descriptiveName ) );

  m_doc->reloadFile();
  m_viewInternal->tagAll();
  m_viewInternal->updateView (true);
}

void KateView::setIconBorder( bool enable )
{
  config()->setIconBar (enable);
}

void KateView::toggleIconBorder()
{
  config()->setIconBar (!config()->iconBar());
}

void KateView::setLineNumbersOn( bool enable )
{
  config()->setLineNumbers (enable);
}

void KateView::toggleLineNumbersOn()
{
  config()->setLineNumbers (!config()->lineNumbers());
}

void KateView::toggleDynWordWrap()
{
  config()->setDynWordWrap( !config()->dynWordWrap() );
}

void KateView::setDynWordWrap( bool b )
{
  config()->setDynWordWrap( b );
}

void KateView::toggleWWMarker()
{
  //m_renderer->setWordWrapMarker (!m_renderer->wordWrapMarker());
}

void KateView::setFoldingMarkersOn( bool enable )
{
  config()->setFoldingBar ( enable );
}

void KateView::toggleFoldingMarkers()
{
  config()->setFoldingBar ( !config()->foldingBar() );

  // if the user has turned off View/Show Folding Markers,
  // then s/he _really_ doesn't want them to reappear when s/he saves
  // (but probably just for this time else s/he'd change the View Defaults)
  m_userWantsFoldingMarkersOff = !foldingMarkersOn();
}

bool KateView::iconBorder() {
  return m_viewInternal->leftBorder->iconBorderOn();
}

bool KateView::lineNumbersOn() {
  return m_viewInternal->leftBorder->lineNumbersOn();
}

int KateView::dynWrapIndicators() {
  return m_viewInternal->leftBorder->dynWrapIndicators();
}

bool KateView::foldingMarkersOn() {
  return m_viewInternal->leftBorder->foldingMarkersOn();
}

void KateView::setCmdLine ( bool enabled )
{
  if (enabled == m_cmdLineOn)
    return;

  if (enabled)
  {
    if (!m_cmdLine)
    {
      m_cmdLine = new KateCmdLine (this);
      m_grid->addMultiCellWidget (m_cmdLine, 2, 2, 0, 2);
    }

    m_cmdLine->show ();
  }
  else
    m_cmdLine->hide ();

  m_cmdLineOn = enabled;
}

void KateView::toggleCmdLine ()
{
  setCmdLine (!m_cmdLineOn);
}

void KateView::setAutoCenterLines(int viewLines)
{
  m_viewInternal->setAutoCenterLines(viewLines);
}

void KateView::toggleWriteLock()
{
  m_doc->setReadWrite( ! m_doc->isReadWrite() );
}

void KateView::enableTextHints(int timeout)
{
  m_viewInternal->enableTextHints(timeout);
}

void KateView::disableTextHints()
{
  m_viewInternal->disableTextHints();
}

void KateView::slotNeedTextHint(int line, int col, QString &text)
{
  text=QString("test %1 %2").arg(line).arg(col);
}

void KateView::find()
{
  m_search->find();
}

void KateView::replace()
{
  m_search->replace();
}

void KateView::findAgain( bool back )
{
  m_search->findAgain( back );
}

void KateView::selectionChanged ()
{
  if (m_doc->hasSelection())
  {
    m_copy->setEnabled (true);
    m_deSelect->setEnabled (true);
  }
  else
  {
    m_copy->setEnabled (false);
    m_deSelect->setEnabled (false);
  }

  if (m_doc->m_bReadOnly)
    return;

  if (m_doc->hasSelection())
    m_cut->setEnabled (true);
  else
    m_cut->setEnabled (false);
}

void KateView::switchToCmdLine ()
{
  if (!m_cmdLineOn)
    setCmdLine (true);

  m_cmdLine->setFocus ();
}

void KateView::showArgHint( QStringList arg1, const QString& arg2, const QString& arg3 )
{
  m_codeCompletion->showArgHint( arg1, arg2, arg3 );
}

void KateView::showCompletionBox( QValueList<KTextEditor::CompletionEntry> arg1, int offset, bool cs )
{
  m_codeCompletion->showCompletionBox( arg1, offset, cs );
}

KateRenderer *KateView::renderer ()
{
  return m_renderer;
}

void KateView::updateConfig ()
{
  if (m_startingUp)
    return;

  m_editActions->readShortcutSettings();

  // dyn. word wrap & markers
  if (m_hasWrap != config()->dynWordWrap()) {
    m_viewInternal->prepareForDynWrapChange();

    m_hasWrap = config()->dynWordWrap();

    m_viewInternal->dynWrapChanged();

    m_setDynWrapIndicators->setEnabled(config()->dynWordWrap());
    m_toggleDynWrap->setChecked( config()->dynWordWrap() );
  }

  m_viewInternal->leftBorder->setDynWrapIndicators( config()->dynWordWrapIndicators() );
  m_setDynWrapIndicators->setCurrentItem( config()->dynWordWrapIndicators() );

  // line numbers
  m_viewInternal->leftBorder->setLineNumbersOn( config()->lineNumbers() );
  m_toggleLineNumbers->setChecked( config()->lineNumbers() );

  // icon bar
  m_viewInternal->leftBorder->setIconBorderOn( config()->iconBar() );
  m_toggleIconBar->setChecked( config()->iconBar() );

  // folding bar
  bool doit = m_doc->highlight() && m_doc->highlight()->allowsFolding() && config()->foldingBar() && !m_userWantsFoldingMarkersOff;
  m_viewInternal->leftBorder->setFoldingMarkersOn(doit);
  m_toggleFoldingMarkers->setChecked( doit );
  m_toggleFoldingMarkers->setEnabled( m_doc->highlight() && m_doc->highlight()->allowsFolding() );

  // bookmark
  m_bookmarks->setSorting( (KateBookmarks::Sorting) config()->bookmarkSort() );

  //setAutoCenterLines(m_doc->autoCenterLines());
}

void KateView::updateRendererConfig()
{
  if (m_startingUp)
    return;

  m_toggleWWMarker->setChecked( m_renderer->config()->wordWrapMarker()  );

  m_viewInternal->updateView (true);
  m_viewInternal->repaint ();
}
