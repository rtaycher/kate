/***************************************************************************
                          kateviewdialog.cpp  -  description
                             -------------------
    begin                : Mon Feb 5 2001
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

#include <stdio.h>
#include <stdlib.h>

#include <qgrid.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qspinbox.h>
#include <kcombobox.h>
#include <qgroupbox.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qcollection.h>
#include <qpushbutton.h>
#include <qobjectlist.h>
#include <qradiobutton.h>
#include <qwhatsthis.h>
#include <qstringlist.h>
#include <klocale.h>
#include <kcolorbtn.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <qvbox.h>
#include <kfontdialog.h>
#include <kregexpeditorinterface.h>
#include <qdialog.h>
#include <kparts/componentfactory.h>

#include "katedocument.h"
#include "kateviewdialog.h"

SearchDialog::SearchDialog( QWidget *parent, QStringList &searchFor, QStringList &replaceWith, int flags )
  : KDialogBase( parent, 0L, true, i18n( "Find Text" ), Ok | Cancel, Ok )
    , m_replace( 0L ), m_regExpDialog( 0L )
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );

  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );

  m_search = new KComboBox( true, page );
  m_search->insertStringList( searchFor );
  m_search->setMinimumWidth( m_search->sizeHint().width() );
  m_search->lineEdit()->selectAll();
  QLabel *label = new QLabel( m_search, i18n( "&Text To Find:" ), page );
  m_optRegExp = new QCheckBox( i18n( "&Regular Expression" ), page );

  topLayout->addWidget( label );
  topLayout->addWidget( m_search );

  QHBoxLayout* regexpLayout = new QHBoxLayout( topLayout );
  regexpLayout->addWidget( m_optRegExp );

  // Add the Edit button if KRegExp exists.
  m_regExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor" );
  if ( m_regExpDialog ) {
    QPushButton* regexpButton = new QPushButton( i18n("Edit"), page );

    regexpLayout->addWidget( regexpButton );
    regexpLayout->addStretch(1);

    connect( regexpButton, SIGNAL( clicked() ), this, SLOT( slotEditRegExp() ) );
    connect( m_optRegExp, SIGNAL( toggled(bool) ), regexpButton, SLOT( setEnabled(bool) ) );
    regexpButton->setEnabled( false );
  }

  if( flags & KateDocument::sfReplace )
  {
    // make it a replace dialog
    setCaption( i18n( "Replace Text" ) );
    m_replace = new KComboBox( true, page );
    m_replace->insertStringList( replaceWith );
    m_replace->setMinimumWidth( m_search->sizeHint().width() );
    label = new QLabel( m_replace, i18n( "&Replace With:" ), page );
    topLayout->addWidget( label );
    topLayout->addWidget( m_replace );
  }

  QGroupBox *group = new QGroupBox( i18n( "Options" ), page );
  topLayout->addWidget( group, 10 );

  QGridLayout *gbox = new QGridLayout( group, 5, 2, spacingHint() );
  gbox->addRowSpacing( 0, fontMetrics().lineSpacing() );
  gbox->setRowStretch( 4, 10 );

  m_opt1 = new QCheckBox( i18n( "C&ase Sensitive" ), group );
  gbox->addWidget( m_opt1, 1, 0 );

  m_opt2 = new QCheckBox(i18n("&Whole Words Only" ), group );
  gbox->addWidget( m_opt2, 2, 0 );

  m_opt3 = new QCheckBox(i18n("&From Beginning" ), group );
  gbox->addWidget( m_opt3, 3, 0 );

  m_opt4 = new QCheckBox(i18n("Find &Backwards" ), group );
  gbox->addWidget( m_opt4, 1, 1 );

  if( flags & KateDocument::sfReplace )
  {
    m_opt5 = new QCheckBox(i18n("&Selected Text" ), group );
    gbox->addWidget( m_opt5, 2, 1 );
    m_opt6 = new QCheckBox( i18n( "&Prompt On Replace" ), group );
    gbox->addWidget( m_opt6, 3, 1 );
    connect(m_opt5, SIGNAL(stateChanged(int)), this, SLOT(selectedStateChanged(int)));
    connect(m_opt6, SIGNAL(stateChanged(int)), this, SLOT(selectedStateChanged(int)));
    m_opt5->setChecked( flags & KateDocument::sfSelected );
    m_opt6->setChecked( flags & KateDocument::sfPrompt );
  }

  m_opt1->setChecked( flags & KateDocument::sfCaseSensitive );
  m_opt2->setChecked( flags & KateDocument::sfWholeWords );
  m_opt3->setChecked( flags & KateDocument::sfFromBeginning );
  m_optRegExp->setChecked( flags & KateDocument::sfRegularExpression );
  m_opt4->setChecked( flags & KateDocument::sfBackward );

  m_search->setFocus();
}

QString SearchDialog::getSearchFor()
{
  return m_search->currentText();
}

void SearchDialog::selectedStateChanged (int)
{
  if (m_opt6->isChecked())
    m_opt5->setEnabled (false);
  else
    m_opt5->setEnabled (true);

  if (m_opt5->isChecked())
    m_opt6->setEnabled (false);
  else
    m_opt6->setEnabled (true);
}

QString SearchDialog::getReplaceWith()
{
  return m_replace->currentText();
}

int SearchDialog::getFlags()
{
  int flags = 0;

  if( m_opt1->isChecked() ) flags |= KateDocument::sfCaseSensitive;
  if( m_opt2->isChecked() ) flags |= KateDocument::sfWholeWords;
  if( m_opt3->isChecked() ) flags |= KateDocument::sfFromBeginning;
  if( m_opt4->isChecked() ) flags |= KateDocument::sfBackward;
  if( m_optRegExp->isChecked() ) flags |= KateDocument::sfRegularExpression;
  if( m_replace )
  {
    if( m_opt6->isChecked() )
      flags |= KateDocument::sfPrompt;
    else
    {
      if( m_opt5->isChecked() )
        flags |= KateDocument::sfSelected;
    }

    flags |= KateDocument::sfReplace;
  }

  return flags;
}

void SearchDialog::slotOk()
{
  if ( !m_search->currentText().isEmpty() )
  {
    if ( !m_optRegExp->isChecked() )
    {
      accept();
    }
    else
    {
      // Check for a valid regular expression.

      QRegExp regExp( m_search->currentText() );

      if ( regExp.isValid() )
        accept();
    }
  }
}

void SearchDialog::slotEditRegExp()
{
  KRegExpEditorInterface *iface = dynamic_cast<KRegExpEditorInterface *>( m_regExpDialog );
  if (!iface)
    return;

  iface->setRegExp( m_search->currentText() );
  int ok = m_regExpDialog->exec();
  if (ok == QDialog::Accepted) {
    m_search->setCurrentText( iface->regExp() );    
  }
}



void SearchDialog::setSearchText( const QString &searchstr )
  {
   m_search->insertItem( searchstr, 0 );
   m_search->setCurrentItem( 0 );
   m_search->lineEdit()->selectAll();
 }

// this dialog is not modal
ReplacePrompt::ReplacePrompt( QWidget *parent )
  : KDialogBase(parent, 0L, true, i18n( "Replace Text" ),
  User3 | User2 | User1 | Close, User3, true,
  i18n("&All"), i18n("&No"), i18n("&Yes")) {

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  QLabel *label = new QLabel(i18n("Replace this occurence?"),page);
  topLayout->addWidget(label );
}

void ReplacePrompt::slotUser1( void ) { // All
  done(KateView::srAll);
}

void ReplacePrompt::slotUser2( void ) { // No
  done(KateView::srNo);
}

void ReplacePrompt::slotUser3( void ) { // Yes
  accept();
}

void ReplacePrompt::done(int r) {
  setResult(r);
  emit clicked();
}

GotoLineDialog::GotoLineDialog(QWidget *parent, int line, int max)
  : KDialogBase(parent, 0L, true, i18n("Goto Line"), Ok | Cancel, Ok) {

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  e1 = new KIntNumInput(line, page);
  e1->setRange(1, max);
  e1->setEditFocus(true);

  QLabel *label = new QLabel( e1,i18n("&Goto Line:"), page );
  topLayout->addWidget(label);
  topLayout->addWidget(e1);
  topLayout->addSpacing(spacingHint()); // A little bit extra space
  topLayout->addStretch(10);
  e1->setFocus();
}

int GotoLineDialog::getLine() {
  return e1->value();
}

const int IndentConfigTab::flags[] = {KateDocument::cfAutoIndent, KateDocument::cfSpaceIndent,
  KateDocument::cfBackspaceIndents,KateDocument::cfTabIndents, KateDocument::cfKeepIndentProfile, KateDocument::cfKeepExtraSpaces};

IndentConfigTab::IndentConfigTab(QWidget *parent, KateDocument *view)
  : QWidget(parent, 0L)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = view->configFlags();

  opt[0] = new QCheckBox(i18n("&Auto Indent"), this);
  layout->addWidget(opt[0], 0, AlignLeft);
  opt[0]->setChecked(configFlags & flags[0]);

  opt[1] = new QCheckBox(i18n("Indent With &Spaces"), this);
  layout->addWidget(opt[1], 0, AlignLeft);
  opt[1]->setChecked(configFlags & flags[1]);

  opt[2] = new QCheckBox(i18n("&Backspace Key Indents"), this);
  layout->addWidget(opt[2], 0, AlignLeft);
  opt[2]->setChecked(configFlags & flags[2]);

  opt[3] = new QCheckBox(i18n("&Tab Key Indents"), this);
  layout->addWidget(opt[3], 0, AlignLeft);
  opt[3]->setChecked(configFlags & flags[3]);

  opt[4] = new QCheckBox(i18n("Keep Indent &Profile"), this);
  layout->addWidget(opt[4], 0, AlignLeft);
//  opt[4]->setChecked(configFlags & flags[4]);
  opt[4]->setChecked(true);
  opt[4]->hide();

  opt[5] = new QCheckBox(i18n("&Keep Extra Spaces"), this);
  layout->addWidget(opt[5], 0, AlignLeft);
  opt[5]->setChecked(configFlags & flags[5]);

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("When <b>Auto indent</b> is on, KateView will indent new lines to equal the indent on the previous line.<p>If the previous line is blank, the nearest line above with text is used"));
  QWhatsThis::add(opt[1], i18n("Check this if you want to indent with spaces rather than tabs.<br>A Tab will be converted to <u>Tab-width</u> as set in the <b>edit</b> options"));
  QWhatsThis::add(opt[2], i18n("This allows the <b>backspace</b> key to be used to indent."));
  QWhatsThis::add(opt[3], i18n("This allows the <b>tab</b> key to be used to indent."));
  QWhatsThis::add(opt[4], i18n("This retains current indentation settings for future documents."));
  QWhatsThis::add(opt[5], i18n("Indentations of more than the selected number of spaces will not be shortened."));
}

void IndentConfigTab::getData(KateDocument *view) {
  int configFlags, z;

  configFlags = view->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  view->setConfigFlags(configFlags);
}

const int SelectConfigTab::flags[] = {KateDocument::cfPersistent, KateDocument::cfDelOnInput,
  KateDocument::cfMouseAutoCopy, KateDocument::cfSingleSelection, KateDocument::cfXorSelect};

SelectConfigTab::SelectConfigTab(QWidget *parent, KateDocument *view)
  : QWidget(parent, 0L)
{
  QVBoxLayout *layout = new QVBoxLayout(this, 0, KDialog::spacingHint() );
  int configFlags = view->configFlags();

  opt[0] = new QCheckBox(i18n("&Persistent Selections"), this);
  layout->addWidget(opt[0], 0, AlignLeft);
  opt[0]->setChecked(configFlags & flags[0]);

  opt[1] = new QCheckBox(i18n("&Overwrite Selections"), this);
  layout->addWidget(opt[1], 0, AlignLeft);
  opt[1]->setChecked(configFlags & flags[1]);

  opt[2] = new QCheckBox(i18n("Mouse &Autocopy"), this);
  layout->addWidget(opt[2], 0, AlignLeft);
  opt[2]->setChecked(configFlags & flags[2]);

  opt[3] = new QCheckBox(i18n("&X11-like Single Selection"), this);
  layout->addWidget(opt[3], 0, AlignLeft);
  opt[3]->setChecked(configFlags & flags[3]);

  opt[4] = new QCheckBox(i18n("&Toggle Old"), this);
  layout->addWidget(opt[4], 0, AlignLeft);
  opt[4]->setChecked(configFlags & flags[4]);

  layout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("Enabling this prevents key input or cursor movement by way of the arrow keys from causing the elimination of text selection.<p><b>Note:</b> If the Overwrite Selections option is activated then any typed character input or paste operation will replace the selected text."));
  QWhatsThis::add(opt[1], i18n("When this is on, any keyed character input or paste operation will replace the selected text."));
  QWhatsThis::add(opt[2], i18n("When this is on, any text selected with the mouse will be automatically copied to the clipboard."));
  QWhatsThis::add(opt[3], i18n("Not implemented yet."));
  QWhatsThis::add(opt[4], i18n("Not yet implemented."));
}

void SelectConfigTab::getData(KateDocument *view) {
  int configFlags, z;

  configFlags = view->configFlags();
  for (z = 0; z < numFlags; z++) {
    configFlags &= ~flags[z]; // clear flag
    if (opt[z]->isChecked()) configFlags |= flags[z]; // set flag if checked
  }
  view->setConfigFlags(configFlags);
}

const int EditConfigTab::flags[] = {KateDocument::cfWordWrap, KateDocument::cfReplaceTabs, KateDocument::cfRemoveSpaces,
  KateDocument::cfAutoBrackets, KateDocument::cfGroupUndo, KateDocument::cfShowTabs, KateDocument::cfSmartHome,
  KateDocument::cfPageUDMovesCursor, KateDocument::cfWrapCursor};

EditConfigTab::EditConfigTab(QWidget *parent, KateDocument *view)
  : QWidget(parent, 0L) {

  QHBoxLayout *mainLayout;
  QVBoxLayout *cbLayout, *leLayout;
  int configFlags;

  mainLayout = new QHBoxLayout(this, 0, KDialog::spacingHint() );

  // checkboxes
  cbLayout = new QVBoxLayout( mainLayout );
  configFlags = view->configFlags();

  opt[0] = new QCheckBox(i18n("&Word wrap"), this);
  cbLayout->addWidget(opt[0], 0, AlignLeft);
  opt[0]->setChecked(view->wordWrap());

  opt[1] = new QCheckBox(i18n("Replace &tabs with spaces"), this);
  cbLayout->addWidget(opt[1], 0, AlignLeft);
  opt[1]->setChecked(configFlags & flags[1]);

  opt[2] = new QCheckBox(i18n("&Remove trailing spaces"), this);
  cbLayout->addWidget(opt[2], 0, AlignLeft);
  opt[2]->setChecked(configFlags & flags[2]);

  opt[3] = new QCheckBox(i18n("&Auto brackets"), this);
  cbLayout->addWidget(opt[3], 0, AlignLeft);
  opt[3]->setChecked(configFlags & flags[3]);

  opt[4] = new QCheckBox(i18n("Group &undos"), this);
  cbLayout->addWidget(opt[4], 0, AlignLeft);
  opt[4]->setChecked(configFlags & flags[4]);

  opt[5] = new QCheckBox(i18n("&Show tabs"), this);
  cbLayout->addWidget(opt[5], 0, AlignLeft);
  opt[5]->setChecked(configFlags & flags[5]);

  opt[6] = new QCheckBox(i18n("Smart &home"), this);
  cbLayout->addWidget(opt[6], 0, AlignLeft);
  opt[6]->setChecked(configFlags & flags[6]);

  opt[7] = new QCheckBox(i18n("&Page up/down moves cursor"), this);
  cbLayout->addWidget(opt[7], 0, AlignLeft);
  opt[7]->setChecked(configFlags & flags[7]);

  opt[8] = new QCheckBox(i18n("Wrap &cursor"), this);
  cbLayout->addWidget(opt[8], 0, AlignLeft);
  opt[8]->setChecked(configFlags & flags[8]);

  cbLayout->addStretch();

  // edit lines
  leLayout = new QVBoxLayout();
  mainLayout->addLayout(leLayout,10);

  e1 = new KIntNumInput(view->wordWrapAt(), this);
  e1->setRange(20, 200, 1, false);
  e1->setLabel(i18n("Wrap Words At:"));

  e2 = new KIntNumInput(e1, view->tabWidth(), this);
  e2->setRange(1, 16, 1, false);
  e2->setLabel(i18n("Tab/Indent Width:"));

  e3 = new KIntNumInput(e2, view->undoSteps(), this);
  e3->setRange(5, 30000, 1, false);
  e3->setLabel(i18n("Undo steps:"));

  leLayout->addWidget(e1, 0, AlignLeft);
  leLayout->addWidget(e2, 0, AlignLeft);
  leLayout->addWidget(e3, 0, AlignLeft);

  leLayout->addStretch();

  // What is this? help
  QWhatsThis::add(opt[0], i18n("Word wrap is a feature that causes the editor to automatically start a new line of text and move (wrap) the cursor to the beginning of that new line. KateView will automatically start a new line of text when the current line reaches the length specified by the Wrap Words At: option.<p><b>NOTE:<b> Word Wrap will not change existing lines or wrap them for easy reading as in some applications."));
  QWhatsThis::add(e1, i18n("If the Word Wrap option is selected this entry determines the length (in characters) at which the editor will automatically start a new line."));
  QWhatsThis::add(opt[1], i18n("KateView will replace any tabs with the number of spaces indicated in the Tab Width: entry."));
  QWhatsThis::add(e2, i18n("If the Replace Tabs By Spaces option is selected this entry determines the number of spaces with which the editor will automatically replace tabs."));
  QWhatsThis::add(opt[2], i18n("KateView will automatically eliminate extra spaces at the ends of lines of text."));
  QWhatsThis::add(opt[3], i18n("When the user types a left bracket ([,(, or {) KateView automatically enters the right bracket (}, ), or ]) to the right of the cursor."));
  QWhatsThis::add(opt[4], i18n("Checking this will cause sequences of similar actions to be undone at once."));
  QWhatsThis::add(opt[5], i18n("The editor will display a symbol to indicate the presence of a tab in the text."));
  QWhatsThis::add(opt[6], i18n("Not yet implemented."));
  QWhatsThis::add(opt[7], i18n("If this is selected, the insertion cursor will be moved to the first/last line when pressing the page up/down buttons.<p>If not selected, it will remain at it's relative position in the visible text."));
  QWhatsThis::add(e3, i18n("Sets the number of undo/redo steps to record. More steps uses more memory."));
  QWhatsThis::add(opt[8], i18n("When on, moving the insertion cursor using the <b>Left</b> and <b>Right</b> keys will go on to previous/next line at beginning/end of the line, similar to most editors.<p>When off, the insertion cursor cannot be moved left of the line start, but it can be moved off the line end, which can be very handy for programmers."));
}

void EditConfigTab::getData(KateDocument *view)
{
  int configFlags, z;

  configFlags = view->configFlags();
  for (z = 1; z < numFlags; z++) {
    configFlags &= ~flags[z];
    if (opt[z]->isChecked()) configFlags |= flags[z];
  }
  view->setConfigFlags(configFlags);

  view->setWordWrapAt(e1->value());
  view->setWordWrap (opt[0]->isChecked());
  view->setTabWidth(e2->value());
  view->setUndoSteps(e3->value());
}

ColorConfig::ColorConfig( QWidget *parent, char *name )
  : QWidget( parent, name )
{
  QGridLayout *glay = new QGridLayout( this, 6, 2, 0, KDialog::spacingHint());
  glay->setColStretch(1,1);
  glay->setRowStretch(5,1);

  QLabel *label;

  label = new QLabel( i18n("Background:"), this);
  label->setAlignment( AlignRight|AlignVCenter );
  m_back = new KColorButton( this );
  glay->addWidget( label, 0, 0 );
  glay->addWidget( m_back, 0, 1 );

  label = new QLabel( i18n("Selected:"), this);
  label->setAlignment( AlignRight|AlignVCenter );
  m_selected = new KColorButton( this );
  glay->addWidget( label, 2, 0 );
  glay->addWidget( m_selected, 2, 1 );

  // QWhatsThis help
  QWhatsThis::add(m_back, i18n("Sets the background color of the editing area"));
  QWhatsThis::add(m_selected, i18n("Sets the background color of the selection. To set the text color for selected text, use the &quot;<b>Configure Highlighting</b>&quot; dialog."));
}


ColorConfig::~ColorConfig()
{
}

void ColorConfig::setColors(QColor *colors)
{
  m_back->setColor( colors[0] );
  m_selected->setColor( colors[1] );
}

void ColorConfig::getColors(QColor *colors)
{
  colors[0] = m_back->color();
  colors[1] = m_selected->color();
}

FontConfig::FontConfig( QWidget *parent, char *name )
  : QWidget( parent, name )
{
    // sizemanagment
  QGridLayout *grid = new QGridLayout( this, 1, 1 );

  m_fontchooser = new KFontChooser ( this );
  m_fontchooser->enableColumn(KFontChooser::StyleList, false);
  grid->addWidget( m_fontchooser, 0, 0);

  connect (m_fontchooser, SIGNAL (fontSelected( const QFont & )), this, SLOT (slotFontSelected( const QFont & )));
}

FontConfig::~FontConfig()
{
}

void FontConfig::setFont ( const QFont &font )
{
  m_fontchooser->setFont (font);
  myFont = font;
}

void FontConfig::slotFontSelected( const QFont &font )
{
  myFont = font;
}

#include "kateviewdialog.moc"



