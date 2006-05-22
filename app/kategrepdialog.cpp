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
#include "katemainwindow.h"

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
#include <kbuttonbox.h>
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

const char *strTemplate[] = {
  "%s",
  "\\<%s\\>[\t ]*=[^=]",
  "\\->[\\t ]*\\<%s\\>[\\t ]*(",
  "[a-z0-9_$]\\+[\\t ]*::[\\t ]*\\<%s\\>[\\t ]*(",
  "\\<%s\\>[\\t ]*\\->[\\t ]*[a-z0-9_$]\\+[\\t ]*(",
  0
};


GrepTool::GrepTool(QWidget *parent)
  : QWidget(parent), childproc(0)
{
  setWindowTitle(i18n("Find in Files"));
  config = KGlobal::config();
  config->setGroup("GrepTool");
  lastSearchItems = config->readEntry("LastSearchItems",QStringList());
  lastSearchPaths = config->readEntry("LastSearchPaths",QStringList());
  lastSearchFiles = config->readEntry("LastSearchFiles",QStringList());

  if( lastSearchFiles.isEmpty() )
  {
    // if there are no entries, most probably the first Kate start.
    // Initialize with default values.
    lastSearchFiles << "*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c"
                    << "*.cpp,*.cc,*.C,*.cxx,*.c"
                    << "*.h,*.hxx,*.idl"
                    << "*";
  }

  QGridLayout *layout = new QGridLayout(this);
  QGridLayout *loInput = new QGridLayout();
  QHBoxLayout *loPattern = new QHBoxLayout();
  QHBoxLayout *loTemplate = new QHBoxLayout();
  QHBoxLayout *loDir = new QHBoxLayout();

  layout->setMargin(0);
  layout->addLayout(loInput, 0, 0);
  loInput->addLayout(loPattern, 0, 1);
  loInput->addLayout(loTemplate, 1, 1);
  loInput->addLayout(loDir, 3, 1);

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

  cbRegex = new QCheckBox( i18n("Regular expression"), this );
  cbRegex->setMinimumWidth( cbRegex->sizeHint().width() );
  cbRegex->setChecked( config->readEntry( "Regex", QVariant(true )).toBool() );

  loInput->addWidget(lPattern, 0, 0, Qt::AlignRight | Qt::AlignVCenter);
  loPattern->addWidget(cmbPattern);
  loPattern->addWidget(cbCasesensitive);
  loPattern->addWidget(cbRegex);
  loPattern->setStretchFactor(cmbPattern, 1);

  // fill template layout
  QLabel *lTemplate = new QLabel(i18n("Template:"), this);
  lTemplate->setFixedSize(lTemplate->sizeHint());

  leTemplate = new KLineEdit(this);
  lTemplate->setBuddy(leTemplate);
  leTemplate->setText(strTemplate[0]);
  leTemplate->setMinimumSize(leTemplate->sizeHint());

  QStringList template_desc;
  template_desc << "normal"
                << "assignment"
                << "->MEMBER("
                << "class::MEMBER("
                << "OBJECT->member(";

  KComboBox *cmbTemplate = new KComboBox(false, this);
  cmbTemplate->insertItems(0, template_desc);
  cmbTemplate->adjustSize();
  cmbTemplate->setFixedSize(cmbTemplate->size());

  loInput->addWidget(lTemplate, 1, 0, Qt::AlignRight | Qt::AlignVCenter);
  loTemplate->addWidget(leTemplate);
  loTemplate->addWidget(cmbTemplate);
  loTemplate->setStretchFactor(cmbTemplate, 1);

  // files row
  QLabel *lFiles = new QLabel(i18n("Files:"), this);

  cmbFiles = new KComboBox(this);
  cmbFiles->setEditable(true);
  lFiles->setBuddy(cmbFiles->focusProxy());
  cmbFiles->setMinimumSize(cmbFiles->sizeHint());
  cmbFiles->setInsertPolicy(QComboBox::NoInsert);
  cmbFiles->setDuplicatesEnabled(false);
  cmbFiles->insertItems(0, lastSearchFiles);

  loInput->addWidget(lFiles, 2, 0, Qt::AlignRight | Qt::AlignVCenter);
  loInput->addWidget(cmbFiles, 2, 1);

  // fill dir layout
  QLabel *lDir = new QLabel(i18n("Folder:"), this);

  KComboBox* cmbUrl = new KComboBox(true, this);
  cmbUrl->setMinimumWidth(80); // make sure that 800x600 resolution works
  cmbUrl->setDuplicatesEnabled(false);
  cmbDir = new KUrlRequester( cmbUrl, this);
  cmbDir->completionObject()->setMode(KUrlCompletion::DirCompletion);
  cmbDir->comboBox()->insertItems(0, lastSearchPaths);
  cmbDir->setMode( KFile::Directory|KFile::LocalOnly );
  lDir->setBuddy(cmbDir);

  cbRecursive = new QCheckBox(i18n("Recursive"), this);
  cbRecursive->setMinimumWidth(cbRecursive->sizeHint().width());
  cbRecursive->setChecked(config->readEntry("Recursive", QVariant(true)).toBool());

  loInput->addWidget(lDir, 3, 0, Qt::AlignRight | Qt::AlignVCenter);
  loDir->addWidget(cmbDir, 1);
  loDir->addWidget(cbRecursive);
  loDir->setStretchFactor(cmbDir, 1);


  // buttons find and clear
  KButtonBox *actionbox = new KButtonBox(this, Qt::Vertical);
  layout->addWidget(actionbox, 0, 1);
  actionbox->addStretch();
  btnSearch = static_cast<KPushButton*>(actionbox->addButton(KStdGuiItem::find()));
  btnSearch->setDefault(true);
  btnClear = static_cast<KPushButton*>(actionbox->addButton( KStdGuiItem::clear() ));
  actionbox->addStretch();

  lbResult = new QTreeWidget(this);
  QStringList headers;
  headers << i18n("File") << i18n("Line") << i18n("Text");
  lbResult->setHeaderLabels(headers);
  lbResult->setIndentation(0);
  layout->addWidget(lbResult, 1, 0, 1, 2);

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
  lTemplate->setWhatsThis(    i18n("You can choose a template for the pattern from the combo box\n"
     "and edit it here. The string %s in the template is replaced\n"
     "by the pattern input field, resulting in the regular expression\n"
     "to search for."));
  lDir->setWhatsThis(    i18n("Enter the folder which contains the files in which you want to search."));
  cbRecursive->setWhatsThis(    i18n("Check this box to search in all subfolders."));
  cbCasesensitive->setWhatsThis(    i18n("If this option is enabled (the default), the search will be case sensitive."));
  cbRegex->setWhatsThis(i18n(
      "<p>If this is enabled, your pattern will be passed unmodified to "
      "<em>grep(1)</em>. Otherwise, all characters that are not letters will be "
      "escaped using a backslash character to prevent grep from interpreting "
      "them as part of the expression.") );
  lbResult->setWhatsThis(    i18n("The results of the grep run are listed here. Select a\n"
     "filename/line number combination and press Enter or doubleclick\n"
     "on the item to show the respective line in the editor."));

  // event filter, do something relevant for RETURN
  cmbPattern->installEventFilter( this );
  leTemplate->installEventFilter( this );
  cmbPattern->installEventFilter( this );
  cmbFiles->installEventFilter( this );
  cmbDir->comboBox()->installEventFilter( this );

  connect( cmbTemplate, SIGNAL(activated(int)),
           SLOT(templateActivated(int)) );
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


GrepTool::~GrepTool()
{
  delete childproc;
}

void GrepTool::patternTextChanged( const QString & _text)
{
  btnSearch->setEnabled( !_text.isEmpty() );
}

void GrepTool::templateActivated(int index)
{
  leTemplate->setText(strTemplate[index]);
}

void GrepTool::itemSelected(QTreeWidgetItem *item, int column)
{
  Q_UNUSED(column);
  const QString filename = item->data(0, Qt::UserRole).toString();
  const int linenumber = item->data(1, Qt::UserRole).toInt();

  emit itemSelected(filename, linenumber);
}

void GrepTool::processOutput()
{
  int pos;
  while ( (pos = buf.indexOf('\n')) != -1)
  {
    QString line = buf.left(pos);
    int doubledot;
    if ((doubledot = line.indexOf(':')) != -1)
    {
      const QString filename = line.left(doubledot);
      line = line.mid(doubledot+1);
      if ((doubledot = line.indexOf(':')) != -1)
      {
        const QString linenumber = line.left(doubledot);
        line = line.mid(doubledot+1);
        QTreeWidgetItem* item = new QTreeWidgetItem(lbResult);
        item->setText(0, filename);
        item->setText(1, linenumber);
        item->setText(2, line.trimmed());
        item->setData(0, Qt::UserRole, m_workingDir + QDir::separator() + filename);
        item->setData(1, Qt::UserRole, linenumber.toInt() - 1);
      }
    }
    buf = buf.mid(pos+1);
  }
  kapp->processEvents();
}

void GrepTool::slotSearch()
{
  if ( cmbPattern->currentText().isEmpty() )
  {
    cmbPattern->setFocus();
    return;
  }

  if ( cmbDir->url().isEmpty() || ! QDir(cmbDir->url()).exists() )
  {
    cmbDir->setFocus();
    KMessageBox::information( this, i18n(
        "You must enter an existing local folder in the 'Folder' entry."),
        i18n("Invalid Folder"), "Kate grep tool: invalid folder" );
    return;
  }

  if ( ! leTemplate->text().contains("%s") )
  {
    leTemplate->setFocus();
    return;
  }

  if ( childproc && childproc->isRunning() )
  {
    childproc->kill();
    return;
  }

  slotClear ();

  m_workingDir = cmbDir->url();

  QString s = cmbPattern->currentText();
  if ( ! cbRegex->isChecked() )
    s.replace( QRegExp( "([^\\w'()<>])" ), "\\\\1" );
  QString pattern = leTemplate->text();
  pattern.replace( "%s", s );

  childproc = new KProcess();
  childproc->setWorkingDirectory( m_workingDir );
  *childproc << "find" << ".";
  if (!cbRecursive->isChecked())
    *childproc << "-maxdepth" << "1";
  if (!cmbFiles->currentText().isEmpty() )
  {
    QStringList files = cmbFiles->currentText().split( ",", QString::SkipEmptyParts );
    *childproc << "(";
    bool first = true;
    for ( QStringList::Iterator it = files.begin(); it != files.end(); ++it )
    {
      if (!first)
        *childproc << "-o";
      *childproc << "-name" << (*it);
      first = false;
    }
    *childproc << ")";
  }
  *childproc << "-exec" << "grep";
  if (!cbCasesensitive->isChecked())
    *childproc << "-i";
  *childproc << "-n" << "-e" << pattern << "{}";
  *childproc << "/dev/null"; //trick to have grep always display the filename
  *childproc << ";";

  connect( childproc, SIGNAL(processExited(KProcess *)),
           SLOT(childExited()) );
  connect( childproc, SIGNAL(receivedStdout(KProcess *, char *, int)),
           SLOT(receivedOutput(KProcess *, char *, int)) );
  connect( childproc, SIGNAL(receivedStderr(KProcess *, char *, int)),
           SLOT(receivedErrOutput(KProcess *, char *, int)) );

  // actually it should be checked whether the process was started successfully
  lbResult->setCursor( QCursor(Qt::WaitCursor) );
  btnClear->setEnabled( false );
  btnSearch->setGuiItem( KStdGuiItem::cancel() );
  childproc->start(KProcess::NotifyOnExit, KProcess::AllOutput);
}

void GrepTool::slotSearchFor(const QString &pattern)
{
  slotClear();
  cmbPattern->setEditText(pattern);
  slotSearch();
}

void GrepTool::finish()
{
  btnSearch->setEnabled( !cmbPattern->lineEdit()->text().isEmpty() );

  buf += '\n';
  processOutput();
  delete childproc;
  childproc = 0;

  config->setGroup("GrepTool");

  QString cmbText = cmbPattern->currentText();
  bool itemsRemoved = lastSearchItems.removeAll(cmbText) > 0;
  lastSearchItems.prepend(cmbText);
  if (itemsRemoved)
  {
    cmbPattern->removeItem(cmbPattern->findText(cmbText));
  }
  cmbPattern->insertItem(0, cmbText);
  cmbPattern->setCurrentIndex(0);
  if (lastSearchItems.count() > 10) {
    lastSearchItems.pop_back();
    cmbPattern->removeItem(cmbPattern->count() - 1);
  }
  config->writeEntry("LastSearchItems", lastSearchItems);


  cmbText = cmbDir->url();
  itemsRemoved = lastSearchPaths.removeAll(cmbText) > 0;
  lastSearchPaths.prepend(cmbText);
  if (itemsRemoved)
  {
    cmbDir->comboBox()->removeItem(cmbDir->comboBox()->findText(cmbText));
  }
  cmbDir->comboBox()->insertItem(0, cmbText);
  cmbDir->comboBox()->setCurrentIndex(0);
  if (lastSearchPaths.count() > 10)
  {
    lastSearchPaths.pop_back();
    cmbDir->comboBox()->removeItem(cmbDir->comboBox()->count() - 1);
  }
  config->writeEntry("LastSearchPaths", lastSearchPaths);


  cmbText = cmbFiles->currentText();
  // remove and prepend, so that the mose recently used item is on top
  itemsRemoved = lastSearchFiles.removeAll(cmbText) > 0;
  lastSearchFiles.prepend(cmbText);
  if (itemsRemoved) // combo box already contained item -> remove it first
  {
    cmbFiles->removeItem(cmbFiles->findText(cmbText));
  }
  cmbFiles->insertItem(0, cmbText);
  cmbFiles->setCurrentIndex(0);
  if (lastSearchFiles.count() > 10) {
    lastSearchFiles.pop_back();
    cmbFiles->removeItem(cmbFiles->count() - 1);
  }
  config->writeEntry("LastSearchFiles", lastSearchFiles);

  config->writeEntry("Recursive", cbRecursive->isChecked());
  config->writeEntry("CaseSensitive", cbCasesensitive->isChecked());
  config->writeEntry("Regex", cbRegex->isChecked());
}

void GrepTool::slotCancel()
{
  finish();
}

void GrepTool::childExited()
{
//   int status = childproc->exitStatus();
  lbResult->unsetCursor();
  btnClear->setEnabled( true );
  btnSearch->setGuiItem( KStdGuiItem::find() );

  if ( ! errbuf.isEmpty() )
  {
    KMessageBox::information( parentWidget(), i18n("<strong>Error:</strong><p>") + errbuf, i18n("Grep Tool Error") );
    errbuf.truncate(0);
  }
  else
    finish();
}

void GrepTool::receivedOutput(KProcess */*proc*/, char *buffer, int buflen)
{
  buf += QByteArray(buffer, buflen+1);
  processOutput();
}

void GrepTool::receivedErrOutput(KProcess */*proc*/, char *buffer, int buflen)
{
  errbuf += QByteArray( buffer, buflen + 1 );
}

void GrepTool::slotClear()
{
  finish();
  lbResult->clear();
}

void GrepTool::updateDirName(const QString &dir)
{
  if (m_lastUpdatedDir != dir)
  {
    setDirName (dir);
    m_lastUpdatedDir = dir;
  }
}

void GrepTool::setDirName(const QString &dir){
  cmbDir->setUrl(dir);
}

bool GrepTool::eventFilter( QObject *o, QEvent *e )
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

#include "kategrepdialog.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
