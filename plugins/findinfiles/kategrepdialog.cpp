/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001, 2004 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "kategrepdialog.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>

#include <QObject>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QEvent>
#include <QRegExp>
#include <QCursor>
//Added by qt3to4:
#include <QByteArray>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <kapplication.h>
#include <kacceleratormanager.h>
#include <kdialogbuttonbox.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <kurlrequester.h>
#include <kurlcompletion.h>
#include <kcombobox.h>
#include <klineedit.h>

KateGrepDialog::KateGrepDialog(QWidget *parent, Kate::MainWindow *mw)
  : QWidget(parent), m_mw (mw), m_grepThread (0)
{
  setWindowTitle(i18n("Find in Files"));
  config = KGlobal::config();
  config->setGroup("KateGrepDialog");
  lastSearchItems = config->readEntry("LastSearchItems",QStringList());
  lastSearchPaths = config->readEntry("LastSearchPaths",QStringList());
  lastSearchFiles = config->readEntry("LastSearchFiles",QStringList());

  if( lastSearchFiles.isEmpty() )
  {
    // if there are no entries, most probably the first Kate start.
    // Initialize with default values.
    lastSearchFiles << "*"
                    << "*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c"
                    << "*.cpp,*.cc,*.C,*.cxx,*.c"
                    << "*.h,*.hxx,*.idl";
  }

  // toplevel, vbox
  QVBoxLayout *topLayout = new QVBoxLayout(this);

  // hbox inside, inputfields + buttons
  QHBoxLayout *layout = new QHBoxLayout();
  topLayout->addLayout (layout);

  // inputfields
  QGridLayout *loInput = new QGridLayout();
  layout->addLayout(loInput);
  loInput->setColumnStretch (1, 100);

  // fill pattern layout
  QLabel *lPattern = new QLabel(i18n("Pattern:"), this);

  cmbPattern = new KComboBox(this);
  cmbPattern->setEditable(true);
  cmbPattern->setDuplicatesEnabled(false);
  cmbPattern->insertItems(0, lastSearchItems);
  cmbPattern->setEditText(QString());
  cmbPattern->setInsertPolicy(QComboBox::NoInsert);
  lPattern->setBuddy(cmbPattern);
  cmbPattern->setFocus();
  cmbPattern->setMinimumSize(cmbPattern->sizeHint());

  cbCasesensitive = new QCheckBox(i18n("Case sensitive"), this);
  cbCasesensitive->setMinimumWidth(cbCasesensitive->sizeHint().width());
  cbCasesensitive->setChecked(config->readEntry("CaseSensitive", QVariant(true)).toBool());

  loInput->addWidget(lPattern, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
  loInput->addWidget(cmbPattern, 0, 1);
  loInput->addWidget(cbCasesensitive, 0, 2);

  // fill dir layout
  QLabel *lDir = new QLabel(i18n("Folder:"), this);
  loInput->addWidget(lDir, 1, 0, Qt::AlignRight | Qt::AlignVCenter);

  KComboBox* cmbUrl = new KComboBox(true, this);
  cmbUrl->setDuplicatesEnabled(false);
  cmbDir = new KUrlRequester( cmbUrl, this);
  cmbDir->completionObject()->setMode(KUrlCompletion::DirCompletion);
  cmbDir->comboBox()->insertItems(0, lastSearchPaths);
  cmbDir->setMode( KFile::Directory|KFile::LocalOnly );
  lDir->setBuddy(cmbDir);

  cbRecursive = new QCheckBox(i18n("Recursive"), this);
  cbRecursive->setChecked(config->readEntry("Recursive", QVariant(true)).toBool());

  loInput->addWidget(cmbDir, 1, 1);
  loInput->addWidget(cbRecursive, 1, 2);

  // files row
  QLabel *lFiles = new QLabel(i18n("Files:"), this);
  loInput->addWidget(lFiles, 2, 0, Qt::AlignRight | Qt::AlignVCenter);

  cmbFiles = new KComboBox(this);
  loInput->addWidget(cmbFiles, 2, 1);
  cmbFiles->setEditable(true);
  lFiles->setBuddy(cmbFiles->focusProxy());
  cmbFiles->setInsertPolicy(QComboBox::NoInsert);
  cmbFiles->setDuplicatesEnabled(false);
  cmbFiles->insertItems(0, lastSearchFiles);

  // buttons find and clear
  KDialogButtonBox *actionbox = new KDialogButtonBox(this, Qt::Vertical);
  btnSearch = actionbox->addButton(KStandardGuiItem::find(), QDialogButtonBox::ActionRole );
  btnSearch->setDefault(true);
  btnClear = actionbox->addButton( KStandardGuiItem::clear() , QDialogButtonBox::ActionRole );
  layout->addWidget(actionbox);

  // result view, list all matches....
  lbResult = new QTreeWidget(this);
  QStringList headers;
  headers << i18n("File") << i18n("Line") << i18n("Text");
  lbResult->setHeaderLabels(headers);
  lbResult->setIndentation(0);
  topLayout->addWidget(lbResult, 10);

  // auto-accels
  KAcceleratorManager::manage( this );

  lPattern->setWhatsThis(    i18n("<p>Enter the expression you want to search for here."
     "<p>If 'regular expression' is unchecked, any non-space letters in your "
     "expression will be escaped with a backslash character."
     "<p>Possible meta characters are:<br>"
     "<b>.</b> - Matches any character<br>"
     "<b>^</b> - Matches the beginning of a line<br>"
     "<b>$</b> - Matches the end of a line<br>"
     "<b>\\&lt;</b> - Matches the beginning of a word<br>"
     "<b>\\&gt;</b> - Matches the end of a word"
     "<p>The following repetition operators exist:<br>"
     "<b>?</b> - The preceding item is matched at most once<br>"
     "<b>*</b> - The preceding item is matched zero or more times<br>"
     "<b>+</b> - The preceding item is matched one or more times<br>"
     "<b>{<i>n</i>}</b> - The preceding item is matched exactly <i>n</i> times<br>"
     "<b>{<i>n</i>,}</b> - The preceding item is matched <i>n</i> or more times<br>"
     "<b>{,<i>n</i>}</b> - The preceding item is matched at most <i>n</i> times<br>"
     "<b>{<i>n</i>,<i>m</i>}</b> - The preceding item is matched at least <i>n</i>, "
     "but at most <i>m</i> times."
     "<p>Furthermore, backreferences to bracketed subexpressions are available "
     "via the notation <code>\\#</code>."
     "<p>See the grep(1) documentation for the full documentation."
     ));
  lFiles->setWhatsThis(    i18n("Enter the file name pattern of the files to search here.\n"
     "You may give several patterns separated by commas."));
  lDir->setWhatsThis(    i18n("Enter the folder which contains the files in which you want to search."));
  cbRecursive->setWhatsThis(    i18n("Check this box to search in all subfolders."));
  cbCasesensitive->setWhatsThis(    i18n("If this option is enabled (the default), the search will be case sensitive."));
  lbResult->setWhatsThis(    i18n("The results of the grep run are listed here. Select a\n"
     "filename/line number combination and press Enter or doubleclick\n"
     "on the item to show the respective line in the editor."));

  // event filter, do something relevant for RETURN
  cmbPattern->installEventFilter( this );
  cmbFiles->installEventFilter( this );
  cmbDir->comboBox()->installEventFilter( this );

  connect( lbResult, SIGNAL(itemActivated(QTreeWidgetItem *, int)),
           SLOT(itemSelected(QTreeWidgetItem *, int)) );
  connect( btnSearch, SIGNAL(clicked()),
           SLOT(slotSearch()) );
  connect( btnClear, SIGNAL(clicked()),
           SLOT(slotClear()) );
  connect( cmbPattern->lineEdit(), SIGNAL(textChanged ( const QString & )),
           SLOT( patternTextChanged( const QString & )));

  patternTextChanged( cmbPattern->lineEdit()->text());
}


KateGrepDialog::~KateGrepDialog()
{
  killThread ();
}

void KateGrepDialog::killThread ()
{
    if (m_grepThread)
    {
        m_grepThread->cancel();
        m_grepThread->wait ();
        delete m_grepThread;
        m_grepThread = 0;
    }
}

void KateGrepDialog::patternTextChanged( const QString & _text)
{
  btnSearch->setEnabled( !_text.isEmpty() );
}

void KateGrepDialog::itemSelected(QTreeWidgetItem *item, int)
{
  // get stuff
  const QString filename = item->data(0, Qt::UserRole).toString();
  const int linenumber = item->data(1, Qt::UserRole).toInt();
  const int column = item->data(2, Qt::UserRole).toInt();

  // open file (if needed, otherwise, this will activate only the right view...)
  KUrl fileURL;
  fileURL.setPath( filename );
  m_mw->openUrl( fileURL );

  // any view active?
  if ( !m_mw->activeView() )
    return;

  // do it ;)
  m_mw->activeView()->setCursorPosition( KTextEditor::Cursor (linenumber, column) );
}

void KateGrepDialog::slotSearch()
{
  // already running, cancel...
  if (m_grepThread)
  {
    killThread ();
    return;
  }

  // no pattern set...
  if ( cmbPattern->currentText().isEmpty() )
  {
    cmbPattern->setFocus();
    return;
  }

  // dir does not exist...
  if ( cmbDir->url().isEmpty() || ! QDir(cmbDir->url().toLocalFile ()).exists() )
  {
    cmbDir->setFocus();
    KMessageBox::information( this, i18n(
        "You must enter an existing local folder in the 'Folder' entry."),
        i18n("Invalid Folder"), "Kate grep tool: invalid folder" );
    return;
  }

  // switch the button + cursor...
  lbResult->setCursor( QCursor(Qt::WaitCursor) );
  btnClear->setEnabled( false );
  btnSearch->setGuiItem( KStandardGuiItem::cancel() );

  // clear the listview
  slotClear ();

  //
  // init the grep thread
  //

  // wildcards
  QStringList wildcards = cmbFiles->currentText().split(QRegExp("[,;]"), QString::SkipEmptyParts);

  // regexps
  QRegExp reg (cmbPattern->currentText(), cbCasesensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
  QList<QRegExp> liste;
  liste << reg;

  // create the thread object
  m_grepThread = new KateGrepThread (this, cmbDir->url().toLocalFile (), cbRecursive->isChecked(), wildcards, liste);

  // connect feedback signals
  connect (m_grepThread, SIGNAL(finished()), this, SLOT(searchFinished()));
  connect (m_grepThread, SIGNAL(foundMatch (const QString &, int, int, const QString &, const QString &)),
           this, SLOT(searchMatchFound(const QString &, int, int, const QString &, const QString &)));

  // grep
  m_grepThread->start();
}

void KateGrepDialog::searchFinished ()
{
  lbResult->unsetCursor();
  btnClear->setEnabled( true );
  btnSearch->setGuiItem( KStandardGuiItem::find() );
}

void KateGrepDialog::searchMatchFound(const QString &filename, int line, int column, const QString &basename, const QString &lineContent)
{
  QTreeWidgetItem* item = new QTreeWidgetItem(lbResult);
  item->setText(0, basename);
  item->setText(1, QString::number (line+1));
  item->setText(2, lineContent.trimmed());
  item->setData(0, Qt::UserRole, filename);
  item->setData(1, Qt::UserRole, line);
  item->setData(2, Qt::UserRole, column);
}

void KateGrepDialog::slotClear()
{
  lbResult->clear();
}

bool KateGrepDialog::eventFilter( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::KeyPress && (
       ((QKeyEvent*)e)->key() == Qt::Key_Return ||
       ((QKeyEvent*)e)->key() == Qt::Key_Enter ) )
  {
    slotSearch();
    return true;
  }

  return QWidget::eventFilter( o, e );
}


void KateGrepDialog::showEvent(QShowEvent* event)
{
  if (event->spontaneous())
    return;

  // sync url with active view
  KUrl url = m_mw->activeView()->document()->url();
  if (url.isLocalFile())
    cmbDir->setUrl(url.directory());
}


#if 0

void KateMainWindow::slotKateGrepDialogItemSelected(const QString &filename, int linenumber)
{

}
#endif

#include "kategrepdialog.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
