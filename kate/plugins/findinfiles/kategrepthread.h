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

#ifndef _GREPTHREAD_H_
#define _GREPTHREAD_H_

#include <QThread>
#include <QRegExp>
#include <QList>
#include <QStringList>

class KateResultView;

class KateGrepThread : public QThread
{
    Q_OBJECT

  public:
    KateGrepThread (KateResultView* parent);
    ~KateGrepThread ();

  public:
    void startSearch(const QList<QRegExp> &pattern,
                     const QString &dir,
                     const QStringList &fileWildcards,
                     bool recursive,
                     bool followDirSymlinks,
                     bool includeHiddenFiles);

  public Q_SLOTS:
    void cancel ()
    {
      m_cancel = true;
    }

  protected:
    void run();

  private:
    void grepInFile (const QString &fileName, const QString &baseName);

  Q_SIGNALS:
    void foundMatch (const QString &filename, const QString &relname, const QList<int> &lines, const QList<int> &columns, const QString &basename, const QStringList &lineContent);

  private:
    volatile bool m_cancel;
    QStringList m_workQueue;
    bool m_recursive;
    bool m_followDirSymlinks;
    bool m_includeHiddenFiles;
    QStringList m_fileWildcards;
    QList<QRegExp> m_searchPattern;
    QString m_dir;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
