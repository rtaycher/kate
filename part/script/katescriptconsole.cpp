/* This file is part of the KDE project
   Copyright (C) 2010 Miquel Sabaté <mikisabate@gmail.com>

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


//BEGIN Includes
// Qt
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QDir>
#include <QtCore/QSize>
#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>

// KDE
#include <KStandardDirs>
#include <KLocale>
#include <KDebug>
#include <KIO/NetAccess>
#include <KIO/CopyJob>
#include <KMessageBox>

// Kate
#include <kateglobal.h>
#include "katescriptconsole.h"
//END Includes


//BEGIN KateScriptConsoleEngine
KateScriptConsoleEngine::KateScriptConsoleEngine(KateView * view)
    : m_view (view)
{
  /* Directory settings */
  scriptDir = KStandardDirs::locateLocal("data", "katepart/scriptconsole");
  KGlobal::dirs()->makeDir(scriptDir); // create dir if needed
  QDir dir(scriptDir, "*.js"); // remove .js files if there're existing ones
  for (unsigned int i = 0; i < dir.count(); ++i)
    QFile::remove(fileUrl(dir[i]));

  /* Creating auxiliar .js files: original.js and exec.js */
  QString srcUrl = KGlobal::dirs()->findResource("data", "katepart/script/utils.js");
  KIO::CopyJob *job = KIO::copy(srcUrl, fileUrl("original.js"), KIO::HideProgressInfo);
  if (! KIO::NetAccess::synchronousRun(job, 0))
    kDebug() << "Error: Kate Console Script can't create auxiliar file";
  KIO::CopyJob *ajob = KIO::copy(srcUrl, fileUrl("exec.js"), KIO::HideProgressInfo);
  if (! KIO::NetAccess::synchronousRun(ajob, 0))
    kDebug() << "Error: Kate Console Script can't create auxiliar file";

  /* Obtaining "standard" java-scripted functions */
  m_definedFunctions = KateGlobal::self()->scriptManager()->
                                   commandLineScripts()[0]->commandHeader().functions();
}

KateScriptConsoleEngine::~KateScriptConsoleEngine()
{
  /* Clean directory */
  QDir dir(scriptDir, "*.js");
  for (unsigned int i = 0; i < dir.count(); ++i)
    QFile::remove(fileUrl(dir[i]));
}

QString KateScriptConsoleEngine::fileUrl(const QString & name) const
{
  return scriptDir + '/' + name;
}

const QString & KateScriptConsoleEngine::execute(const QString & text)
{
  static QString msg;
  int ret;

  msg = "";
  executable = "";
  if ((ret = parseScript(text, msg)) != -1) {
    if (!ret) { // It's a command
      KateCommandLineScriptHeader header;
      KateCommandLineScript *script = new KateCommandLineScript(fileUrl("exec.js"), header);
      script->exec(m_view, text, msg);
      delete script;
    } else // It's a function definition
      execFunctions(msg);
  } else
    return msg;
  if (!msg.isEmpty())
    return msg;
  msg = "Success!";
  return msg;
}

int KateScriptConsoleEngine::parseScript(const QString & text, QString & msg)
{
  bool func = false;
  QString dirty;

  /* First of all, is this a valid script ? */
  for (int i = 0; i < text.size(); i++) {
    while (text[i] != 'f') {
      if (text[i] == '{'){
        msg = "Error: There are bad defined functions";
        return -1;
      }
      if (func)
        dirty.append(text[i]);
      if ((i + 6) >= text.size()) {
        if (m_definedFunctions.contains(text))
          return 0;
        else {
          msg = "Error: This is not a valid script";
          return -1;
        }
      }
      ++i;
    }
    if (text[i+1] == 'u' && text[i+2] == 'n' && text[i+3] == 'c'
      && text[i+4] == 't' && text[i+5] == 'i' && text[i+6] == 'o' && text[i+7] == 'n') {
      /* Make sure we have no commands between function definitions */
      foreach (const QChar & c, dirty) {
        if (c != ' ' && c != '\n') {
          msg = "Error: You can't mix line commands with function definitions";
          return -1;
        }
      }
      if (!getFunctionInfo(text, i)) {
        msg = "Error: There are bad defined functions";
        return -1;
      }
      func = true;
    }
  }

  return 1;
}

bool KateScriptConsoleEngine::getFunctionInfo(const QString & text, int & index)
{
  QString funcName;
  QString funcCode = "function ";

  /* First we obtain function name */
  index += 9;
  for (; index < text.size(); ++index) {
    while (text[index] != ' ' && text[index] != '(') {
      if (text[index] == '}' || text[index] == ')')
        return false;
      funcName.append(text[index]);
      funcCode.append(text[index]);
      ++index;
    }
    break;
  }

  /* Next step is to make sure () is in there */
  int catalan = 0;
  while (text[index] != '{') {
    if (text[index] == '(')
      ++catalan;
    else if (text[index] == ')')
      --catalan;
    else if (text[index] == '}') /* really bad ... */
      return false;
    funcCode.append(text[index]);
    index++;
  }
  if (catalan)
    return false;

  /* Finally, watch if the function is closed by {} */
  catalan = 0;
  do {
    if (text[index] == '{')
      ++catalan;
    else if (text[index] == '}')
      --catalan;
    funcCode.append(text[index]);
    ++index;
  } while (catalan && index < text.size());
  if (catalan)
    return false;

  /* We can now ensure basic syntax is ok */
  m_functions[funcName] = funcCode;
  if (executable.isEmpty())
    executable = funcName;
  return true;
}

void KateScriptConsoleEngine::execFunctions(QString & msg)
{
  bool removeExec = false;
  QStringList newFunct;

  /* Check if there're redefinitions */
  foreach (const QString & str, m_functions.keys()) {
    if (m_definedFunctions.contains(str)) {
      QFile::remove(fileUrl(str + ".js"));
      removeExec = true;
    } else {
      m_definedFunctions.append(str);
      newFunct.append(str);
    }
  }

  /*
   * Should we remove exec.js to update it or not ? Anyway, we should
   * update proper auxiliar files.
   */
  if (removeExec) {
    QFile::remove(fileUrl("exec.js"));
    KIO::CopyJob *job = KIO::copy(fileUrl("original.js"), fileUrl("exec.js"), KIO::HideProgressInfo);
    if (! KIO::NetAccess::synchronousRun(job, 0))
      kDebug() << "Error: Kate Console Script can't create auxiliar file";
    QFile destFile(fileUrl("exec.js"));
    if(!destFile.open(QIODevice::Append)) {
      kDebug(13050) << "Error: Cannot open file " << qPrintable(fileUrl("exec.js")) << '\n';
      return;
    }
    QTextStream tsDest(&destFile);
    foreach (const QString & func, m_functions.keys()) {
      QFile file(fileUrl(func + ".js"));
      if(!file.open(QIODevice::Append)) {
        kDebug(13050) << "Error: Cannot open file " << qPrintable(fileUrl(func+".js")) << '\n';
        return;
      }
      QTextStream ts(&file);
      ts << m_functions[func];
      tsDest << m_functions[func];
      file.close();
    }
  } else {
    foreach (const QString & func, newFunct) {
      QFile file(fileUrl(func + ".js"));
      if(!file.open(QIODevice::Append)) {
        kDebug(13050) << "Error: Cannot open file " << qPrintable(fileUrl(func+".js")) << '\n';
        return;
      }
      QTextStream ts(&file);
      ts << m_functions[func];
      QFile dst(fileUrl("exec.js"));
      if(!dst.open(QIODevice::Append)) {
        kDebug(13050) << "Error: Cannot open file " << qPrintable(fileUrl("exec.js")) << '\n';
        return;
      }
      QTextStream tsDest(&dst);
      tsDest << m_functions[func];
      file.close();
      dst.close();
    }
  }

  /* All the ugly stuff is done, now it's time to execute a function :) */
  KateCommandLineScriptHeader header; // dummy header
  KateCommandLineScript *script = new KateCommandLineScript(fileUrl("exec.js"), header);
  script->exec(m_view, executable, msg);
  delete script;
  m_functions.clear();
}
//END KateScriptConsoleEngine


//BEGIN KateScriptConsole
KateScriptConsole::KateScriptConsole(KateView * view, QWidget * parent)
    : KateViewBarWidget (true, parent)
    , m_view (view)
{
  Q_ASSERT(m_view != NULL);

  initialSize = parent->size();
  layout = new QVBoxLayout();
  centralWidget()->setLayout(layout);
  layout->setMargin(0);
  hLayout = new QHBoxLayout;
  m_result = new QLabel(this);
  m_edit = new QTextEdit(this);
  m_execute = new QPushButton(i18n("Execute"), this);
  m_execute->setIcon(KIcon("quickopen"));
  connect(m_execute, SIGNAL(clicked()), this, SLOT(executePressed()));

  layout->addWidget(m_edit);
  hLayout->addWidget(m_result);
  hLayout->addWidget(m_execute, 1, Qt::AlignRight);
  layout->addLayout(hLayout);

  m_engine = new KateScriptConsoleEngine(m_view);
}

void KateScriptConsole::setupLayout()
{
  resize(endSize);
  layout->setMargin(0);
  hLayout = new QHBoxLayout;
  layout->addWidget(m_edit);
  hLayout->addWidget(m_result);
  hLayout->addWidget(m_execute, 1, Qt::AlignRight);
  layout->addLayout(hLayout);
}

KateScriptConsole::~KateScriptConsole()
{
  delete m_engine;
}

void KateScriptConsole::closed()
{
  endSize = this->size();
  layout->removeWidget(m_edit);
  hLayout->removeWidget(m_result);
  hLayout->removeWidget(m_execute);
  delete hLayout;
  resize(initialSize);
}

void KateScriptConsole::switched()
{
  if (this->size() != initialSize)
    closed();
}

void KateScriptConsole::executePressed()
{
  QString text = m_edit->toPlainText();
  QString msg;
  if (!text.isEmpty()) {
    msg = m_engine->execute(text);
    if (msg != "Success!")
      KMessageBox::error(this, msg);
    else
      m_result->setText("<b>" + msg + "</b>");
  } else
    m_result->setText("<b>There's no code to execute</b>");
}
//END KateScriptConsole


#include "katescriptconsole.moc"


// kate: space-indent on; indent-width 2; replace-tabs on;

