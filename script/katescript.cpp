
/// This file is part of the KDE libraries
/// Copyright (C) 2008 Paul Giannaros <paul@giannaros.org>
///
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Library General Public
/// License version 2 as published by the Free Software Foundation.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Library General Public License for more details.
///
/// You should have received a copy of the GNU Library General Public License
/// along with this library; see the file COPYING.LIB.  If not, write to
/// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
/// Boston, MA 02110-1301, USA.

#include "katescript.h"
#include "kateview.h"
#include "katedocument.h"

#include <QFile>

#include <QScriptEngine>
#include <QScriptValue>

#include <kdebug.h>
#include <klocale.h>

KateScriptDocument::KateScriptDocument ()
 : QObject ()
 , m_document (0)
{
}

KateScriptView::KateScriptView ()
 : QObject ()
 , m_view (0)
{
}

KateScript::KateScript(const QString &url, const KateScriptInformation &information) :
    m_loaded(false), m_url(url), m_information(information), m_engine(0)
  , m_document (0), m_view (0)
{
}

KateScript::~KateScript()
{
  // remove data...
  delete m_engine;
  delete m_document;
  delete m_view;
}

void KateScript::displayBacktrace(const QString &header)
{
  if(!m_engine) {
    kDebug(13050) << "KateScript::displayBacktrace: no engine, cannot display error\n";
  }
  kDebug(13050) << "\033[31m";
  if(!header.isNull())
    kDebug(13050) << qPrintable(header) << '\n';
  kDebug(13050) << qPrintable(m_engine->uncaughtExceptionBacktrace().join("\n"));
  kDebug(13050) << "\033[0m";
}

QScriptValue KateScript::global(const QString &name)
{
  // load the script if necessary
  if(!load())
    return QScriptValue();
  return m_engine->globalObject().property(name);
}

QScriptValue KateScript::function(const QString &name)
{
  QScriptValue value = global(name);
  if(!value.isFunction())
    return QScriptValue();
  return value;
}

bool KateScript::load()
{
  if(m_loaded)
    return m_loadSuccessful;

  m_loaded = true;
  // read the file into memory
  QString filename = QFile::encodeName(m_url);
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    m_errorMessage = i18n("Unable to read file: '%1'", filename);
    kDebug(13051) << m_errorMessage;
    m_loadSuccessful = false;
    return false;
  }
  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  QString source = stream.readAll();
  file.close();
  // evaluate it
  m_engine = new QScriptEngine();
  QScriptValue result = m_engine->evaluate(source, m_url);
  if(m_engine->hasUncaughtException()) {
    displayBacktrace(QString("Error loading %1\n").arg(m_url));
    m_errorMessage = i18n("Error loading indent script %1", filename);
    m_loadSuccessful = false;
    return false;
  }

  // set the view/document objects as necessary
  m_engine->globalObject().setProperty("document", m_engine->newQObject(m_document = new KateScriptDocument ()));
  m_engine->globalObject().setProperty("view", m_engine->newQObject(m_view = new KateScriptView ()));

  // yip yip!
  m_loadSuccessful = true;
  return true;
}

bool KateScript::setView (KateView *view)
{
  if (!load())
    return false;

  // setup the stuff
  m_document->setDocument (view->doc());
  m_view->setView (view);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
