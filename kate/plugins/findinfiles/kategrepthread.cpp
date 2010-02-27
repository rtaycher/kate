/* This file is part of the KDE project
   Copyright (C) 2007 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2008 Eduardo Robles Elvira <edulix@gmail.com>
   Copyright (C) 2008 Dominik Haumann <dhaumann kde org>

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

#include "kategrepthread.h"
#include "kategrepthread.moc"
#include "kateresultview.h"

#include <kdebug.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

KateGrepThread::KateGrepThread(KateResultView* parent)
    : QThread (parent)
    , m_cancel (false)
    , m_recursive (false)
    , m_followDirSymlinks (false)
{
}

KateGrepThread::~KateGrepThread ()
{}

void KateGrepThread::startSearch(const QList<QRegExp> &pattern,
                                 const QString &dir,
                                 const QStringList &fileWildcards,
                                 bool recursive,
                                 bool followDirSymlinks)
{
  m_cancel = false;

  m_recursive = recursive;
  m_followDirSymlinks = followDirSymlinks;
  m_fileWildcards = fileWildcards;
  m_searchPattern = pattern;

  m_workQueue << dir;
  QDir baseDir(dir);
  m_dir = baseDir.absolutePath() + QDir::separator();

  start();
}

void KateGrepThread::run ()
{
  // do the real work
  while (!m_cancel && !m_workQueue.isEmpty())
  {
    QDir currentDir (m_workQueue.takeFirst());

    // if not readable, skip it
    if (!currentDir.isReadable ())
      continue;

    // only add subdirs to worklist if we should do recursive search
    if (m_recursive)
    {
      // append all dirs to the workqueue
      QDir::Filters dirFilter = QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable;
      if (!m_followDirSymlinks)
        dirFilter |= QDir::NoSymLinks;
      
      QFileInfoList currentSubDirs = currentDir.entryInfoList (dirFilter);

      // append them to the workqueue, if readable
      for (int i = 0; i < currentSubDirs.size(); ++i)
        m_workQueue << currentSubDirs.at(i).absoluteFilePath ();
    }

    // work with all files in this dir..., use wildcards for them...
    QFileInfoList currentFiles = currentDir.entryInfoList (m_fileWildcards, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

    // iterate over all files
    for (int i = 0; !m_cancel && i < currentFiles.size(); ++i)
      grepInFile (currentFiles.at(i).absoluteFilePath (), currentFiles.at(i).fileName());
  }
}

void KateGrepThread::grepInFile (const QString &fileName, const QString &baseName)
{
  QFile file (fileName);

  // can't read file, return
  if (!file.open(QFile::ReadOnly))
    return;

  // try to extract data
  QTextStream stream (&file);

  // matches, lines, columns
  QList<int> linesArray, columns;
  QStringList lineContent;

  QStringList lines;
  int lineNumber = 0;
  while (!m_cancel)
  {
    // enough lines gathered, try to match them...
    if (lines.size() == m_searchPattern.size())
    {
      int firstColumn = -1;
      for (int i = 0; i < m_searchPattern.size(); ++i)
      {
        int column = m_searchPattern.at(i).indexIn (lines.at(i));
        if (column == -1)
        {
          firstColumn = -1; // reset firstColumn
          break;
        }
        else if (i == 0) // remember first column
          firstColumn = column;
      }

      // found match...
      if (firstColumn != -1)
      {
        linesArray.append (lineNumber);
        columns.append (firstColumn);

        // cut too long lines....
        lineContent.append ((lines.at(0).length() > 512) ? lines.at(0).left(512) : lines.at(0));
      }

      // remove first line...
      lines.pop_front ();
      ++lineNumber;
    }

    QString line = stream.readLine();
    if (line.isNull())
      break;
    lines.append (line);
  }

  if (!linesArray.isEmpty())
  {
    QString relName = fileName;
    if (relName.startsWith(m_dir))
      relName.remove(0, m_dir.size());

    emit foundMatch (fileName, relName, linesArray, columns, baseName, lineContent);
  }
}

// kate: space-indent on; indent-width 2; replace-tabs on;
