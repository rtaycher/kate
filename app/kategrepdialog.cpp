/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#include "kategrepdialog.h"
#include "katemainwindow.h"

#include <qobject.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qevent.h>
#include <qlistbox.h>
#include <qregexp.h>
#include <qwhatsthis.h>
#include <qcursor.h>

#include <kaccelmanager.h>
#include <kbuttonbox.h>
#include <kfiledialog.h>
#include <kprocess.h>
#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kurlcompletion.h>
#include <kcombobox.h>
#include <klineedit.h>

const char *template_desc[] = {
  "normal",
  "assignment",
  "->MEMBER(",
  "class::MEMBER(",
  "OBJECT->member(",
  0
};

const char *template_str[] = {
  "%s",
  "\\<%s\\>[\t ]*=[^=]",
  "\\->[\\t ]*\\<%s\\>[\\t ]*(",
  "[a-z0-9_$]\\+[\\t ]*::[\\t ]*\\<%s\\>[\\t ]*(",
  "\\<%s\\>[\\t ]*\\->[\\t ]*[a-z0-9_$]\\+[\\t ]*(",
  0
};


GrepTool::GrepTool(const QString &dirname, KateMainWindow *parent, const char *name)
  : QWidget(parent, name/*, false*/), childproc(0)
{
  setCaption(i18n("Find in Files"));
  config = KGlobal::config();
  config->setGroup("GrepTool");
  lastSearchItems = config->readListEntry("LastSearchItems");
  lastSearchPaths = config->readListEntry("LastSearchPaths");

  QGridLayout *layout = new QGridLayout(this, 6, 3, 4, 4);
  layout->setColStretch(0, 10);
  layout->addColSpacing(1, 10);
  layout->setColStretch(1, 0);
  layout->setColStretch(2, 1);
  layout->setRowStretch(1, 0);
  layout->setRowStretch(2, 10);
  layout->setRowStretch(4, 0);

  QGridLayout *input_layout = new QGridLayout(4, 2, 4);
  layout->addLayout(input_layout, 0, 0);
  input_layout->setColStretch(0, 0);
  input_layout->setColStretch(1, 20);

  QLabel *pattern_label = new QLabel(i18n("Pattern:"), this);
  pattern_label->setFixedSize(pattern_label->sizeHint());
  input_layout->addWidget(pattern_label, 0, 0, AlignRight | AlignVCenter);

  pattern_combo = new QComboBox(true, this);
  pattern_combo->insertStringList(lastSearchItems);
  pattern_combo->setEditText(QString::null);
  pattern_combo->setInsertionPolicy(QComboBox::NoInsertion);
  pattern_label->setBuddy(pattern_combo);
  pattern_combo->setFocus();
  pattern_combo->setMinimumSize(pattern_combo->sizeHint());
  input_layout->addWidget(pattern_combo, 0, 1);

  QLabel *template_label = new QLabel(i18n("Template:"), this);
  template_label->setFixedSize(template_label->sizeHint());
  input_layout->addWidget(template_label, 1, 0, AlignRight | AlignVCenter);

  QBoxLayout *template_layout = new QHBoxLayout(4);
  input_layout->addLayout(template_layout, 1, 1);

  template_edit = new QLineEdit(this);
  template_label->setBuddy(template_edit);
  template_edit->setText(template_str[0]);
  template_edit->setMinimumSize(template_edit->sizeHint());
  template_layout->addWidget(template_edit);

  QComboBox *template_combo = new QComboBox(false, this);
  template_combo->insertStrList(template_desc);
  template_combo->adjustSize();
  template_combo->setFixedSize(template_combo->size());
  template_layout->addWidget(template_combo);

  QLabel *files_label = new QLabel(i18n("Files:"), this);
  files_label->setFixedSize(files_label->sizeHint());
  input_layout->addWidget(files_label, 2, 0, AlignRight | AlignVCenter);

  files_combo = new QComboBox(true, this);
  files_label->setBuddy(files_combo->focusProxy());
  files_combo->setMinimumSize(files_combo->sizeHint());
  files_combo->insertItem("*.h,*.hxx,*.cpp,*.cc,*.C,*.cxx,*.idl,*.c");
  files_combo->insertItem("*.cpp,*.cc,*.C,*.cxx,*.c");
  files_combo->insertItem("*.h,*.hxx,*.idl");
  files_combo->insertItem("*");
  input_layout->addWidget(files_combo, 2, 1);

  QLabel *dir_label = new QLabel(i18n("Directory:"), this);
  dir_label->setFixedSize(dir_label->sizeHint());
  input_layout->addWidget(dir_label, 3, 0, AlignRight | AlignVCenter);

  QBoxLayout *dir_layout = new QHBoxLayout(3);
  input_layout->addLayout(dir_layout, 3, 1);

  dir_combo = new KURLRequester( new KComboBox(true, this), this, "dir combo" );
  dir_combo->completionObject()->setMode(KURLCompletion::DirCompletion);
  dir_combo->comboBox()->insertStringList(lastSearchPaths);
  dir_combo->setMode( KFile::Directory|KFile::LocalOnly );
  dir_layout->addWidget(dir_combo);
  dir_label->setBuddy(dir_combo);

  recursive_box = new QCheckBox(i18n("Recursive"), this);
  recursive_box->setMinimumWidth(recursive_box->sizeHint().width());
  recursive_box->setChecked(true);
  dir_layout->addSpacing(10);
  dir_layout->addWidget(recursive_box);

  KButtonBox *actionbox = new KButtonBox(this, Qt::Vertical);
  layout->addWidget(actionbox, 0, 2);
  actionbox->addStretch();
  search_button = actionbox->addButton(i18n("Search"));
  search_button->setDefault(true);
  clear_button = actionbox->addButton(i18n("Clear"));
  actionbox->addStretch();
  actionbox->layout();

  resultbox = new QListBox(this);
  QFontMetrics rb_fm(resultbox->fontMetrics());
  layout->addMultiCellWidget(resultbox, 2, 2, 0, 2);

  layout->activate();

  KAcceleratorManager::manage( this );

  QWhatsThis::add(pattern_combo,
    i18n("Enter the regular expression you want to search for here.<br>"
     "Possible meta characters are:<br>"
     "<b>.</b> - Matches any character<br>"
     "<b>^</b> - Matches the beginning of a line<br>"
     "<b>$</b> - Matches the end of a line<br>"
     "<b>\\\\\\&lt;</b> - Matches the beginning of a word<br>"
     "<b>\\\\\\&gt;</b> - Matches the end of a word<br>"
     "<br>"
     "The following repetition operators exist:<br>"
     "<b>?</b> - The preceding item is matched at most once<br>"
     "<b>*</b> - The preceding item is matched zero or more times<br>"
     "<b>+</b> - The preceding item is matched one or more times<br>"
     "<b>{<i>n</i>}</b> - The preceding item is matched exactly <i>n</i> times<br>"
     "<b>{<i>n</i>,}</b> - The preceding item is matched <i>n</i> or more times<br>"
     "<b>{,<i>n</i>}</b> - The preceding item is matched at most <i>n</i> times<br>"
     "<b>{<i>n</i>,<i>m</i>}</b> - The preceding item is matched at least <i>n</i>,<br>"
     "   but at most <i>m</i> times.<br>"
     "<br>"
     "Furthermore, backreferences to bracketed subexpressions are<br>"
     "available via the notation \\\\<i>n</i>."
     ));
  QWhatsThis::add(files_combo,
    i18n("Enter the file name pattern of the files to search here.\n"
     "You may give several patterns separated by commas"));
  QWhatsThis::add(template_edit,
    i18n("You can choose a template for the pattern from the combo box\n"
     "and edit it here. The string %s in the template is replaced\n"
     "by the pattern input field, resulting in the regular expression\n"
     "to search for."));
  QWhatsThis::add(dir_combo,
    i18n("Enter the directory which contains the files you want to search in."));
  QWhatsThis::add(recursive_box,
    i18n("Check this box to search in all subdirectories."));
  QWhatsThis::add(resultbox,
    i18n("The results of the grep run are listed here. Select a\n"
     "filename/line number combination and press Enter or doubleclick\n"
     "on the item to show the respective line in the editor."));

  // event filter, do something relevant for RETURN
  pattern_combo->installEventFilter( this );
  template_edit->installEventFilter( this );
  pattern_combo->installEventFilter( this );
  files_combo->installEventFilter( this );
  dir_combo->comboBox()->installEventFilter( this );

  connect( template_combo, SIGNAL(activated(int)),
           SLOT(templateActivated(int)) );
  connect( resultbox, SIGNAL(selected(const QString&)),
           SLOT(itemSelected(const QString&)) );
  connect( search_button, SIGNAL(clicked()),
           SLOT(slotSearch()) );
  connect( clear_button, SIGNAL(clicked()),
           SLOT(slotClear()) );
  connect( pattern_combo->lineEdit(), SIGNAL(textChanged ( const QString & )),
           SLOT( patternTextChanged( const QString & )));

  patternTextChanged( pattern_combo->lineEdit()->text());
}


GrepTool::~GrepTool()
{
  delete childproc;
}

void GrepTool::patternTextChanged( const QString & _text)
{
  search_button->setEnabled( !_text.isEmpty() );
}

void GrepTool::templateActivated(int index)
{
  template_edit->setText(template_str[index]);
}

void GrepTool::itemSelected(const QString& item)
{
  int pos;
  QString filename, linenumber;

  QString str = item;
  if ( (pos = str.find(':')) != -1)
  {
    filename = str.left(pos);
    str = str.right(str.length()-1-pos);
    if ( (pos = str.find(':')) != -1)
    {
      linenumber = str.left(pos);
      emit itemSelected(filename,linenumber.toInt()-1);
    }
  }
}

void GrepTool::processOutput()
{
  int pos;
  while ( (pos = buf.find('\n')) != -1)
  {
    QString item = buf.left(pos);
    if (!item.isEmpty())
      resultbox->insertItem(item);
    buf = buf.right(buf.length()-pos-1);
  }
}

void GrepTool::slotSearch()
{
  if (pattern_combo->currentText().isEmpty())
  return;

//   search_button->setEnabled(false);
  if ( childproc && childproc->isRunning() )
  {
    childproc->kill();
    return;
  }

  QString files;
  QString files_temp = files_combo->currentText();
  if (files_temp.right(1) != ",")
  files_temp = files_temp + ",";

  QStringList tokens = QStringList::split ( ",", files_temp, FALSE );
  QStringList::Iterator it = tokens.begin();
  if (it != tokens.end())
  files = " '"+(*it++)+"'" ;

  for ( ; it != tokens.end(); it++ )
  files = files + " -o -name " + "'"+(*it)+ "'";

  QString pattern = template_edit->text();
  pattern.replace("%s", pattern_combo->currentText());
  pattern.replace("'", "'\\''");

  QString filepattern = "`find ";
  filepattern += KProcess::quote(dir_combo->url());
  if (!recursive_box->isChecked())
  filepattern += " -maxdepth 1";
  filepattern += " \\( -name ";
  filepattern += files;
  filepattern += " \\) -print";
  filepattern += "`";

  childproc = new KShellProcess();
  *childproc << "grep";
  *childproc << "-n";
  *childproc << (QString("-e ") + KProcess::quote(pattern));
  *childproc << filepattern;
   *childproc << "/dev/null";

  connect( childproc, SIGNAL(processExited(KProcess *)),
           SLOT(childExited()) );
  connect( childproc, SIGNAL(receivedStdout(KProcess *, char *, int)),
           SLOT(receivedOutput(KProcess *, char *, int)) );
  connect( childproc, SIGNAL(receivedStderr(KProcess *, char *, int)),
           SLOT(receivedErrOutput(KProcess *, char *, int)) );

  // actually it should be checked whether the process was started successfully
  resultbox->setCursor( QCursor(Qt::WaitCursor) );
  clear_button->setEnabled( false );
  search_button->setText( i18n("Cancel") );
  childproc->start(KProcess::NotifyOnExit, KProcess::AllOutput);
}

void GrepTool::slotSearchFor(const QString &pattern){
  slotClear();
  pattern_combo->setEditText(pattern);
  slotSearch();
}

void GrepTool::finish()
{
  search_button->setEnabled( !pattern_combo->lineEdit()->text().isEmpty() );

  buf += '\n';
  processOutput();
  delete childproc;
  childproc = 0;

  config->setGroup("GrepTool");

  if (lastSearchItems.contains(pattern_combo->currentText()) == 0)
  {
    pattern_combo->insertItem(pattern_combo->currentText(), 0);
    lastSearchItems.prepend(pattern_combo->currentText());
    if (lastSearchItems.count() > 10) {
      lastSearchItems.remove(lastSearchItems.fromLast());
      pattern_combo->removeItem(pattern_combo->count() - 1);
    }
    config->writeEntry("LastSearchItems", lastSearchItems);
  }

  if (lastSearchPaths.contains(dir_combo->url()) == 0)
  {
    dir_combo->comboBox()->insertItem(dir_combo->url(), 0);
    lastSearchPaths.prepend(dir_combo->url());
    if (lastSearchPaths.count() > 10)
    {
      lastSearchPaths.remove(lastSearchPaths.fromLast());
      dir_combo->comboBox()->removeItem(dir_combo->comboBox()->count() - 1);
    }
    config->writeEntry("LastSearchPaths", lastSearchPaths);
  }
}

void GrepTool::slotCancel()
{
  finish();
}

void GrepTool::childExited()
{
//   int status = childproc->exitStatus();
  resultbox->unsetCursor();
  clear_button->setEnabled( true );
  search_button->setText( i18n("Search") );

  if ( ! errbuf.isEmpty() )
  {
    KMessageBox::information( parentWidget(), i18n("<strong>Error:</strong><p>") + errbuf, i18n("Grep tool error") );
    errbuf.truncate(0);
  }
  else
    finish();
}

void GrepTool::receivedOutput(KProcess */*proc*/, char *buffer, int buflen)
{
  buf += QCString(buffer, buflen+1);
  processOutput();
}

void GrepTool::receivedErrOutput(KProcess */*proc*/, char *buffer, int buflen)
{
  errbuf += QCString( buffer, buflen + 1 );
}

void GrepTool::slotClear()
{
  finish();
  resultbox->clear();
}

void  GrepTool::setDirName(const QString &dir){
  dir_combo->setURL(dir);
}

bool GrepTool::eventFilter( QObject *o, QEvent *e )
{
  if ( e->type() == QEvent::KeyPress && (
       ((QKeyEvent*)e)->key() == Qt::Key_Return ||
       ((QKeyEvent*)e)->key() == Qt::Key_Enter ) )
  {
    if ( pattern_combo->currentText().isEmpty() )
      pattern_combo->setFocus();
    else if ( template_edit->text().isEmpty() )
      template_edit->setFocus();
    else if ( files_combo->currentText().isEmpty() )
      files_combo->setFocus();
    else if ( dir_combo->url().isEmpty() )
      dir_combo->setFocus();

    else
      slotSearch();

    return true;
  }

  return QWidget::eventFilter( o, e );
}

#include "kategrepdialog.moc"
