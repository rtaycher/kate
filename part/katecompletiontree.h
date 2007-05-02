/* This file is part of the KDE libraries
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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

#ifndef KATECOMPLETIONTREE_H
#define KATECOMPLETIONTREE_H

#include <QtGui/QTreeView>

class KateCompletionWidget;
class KateCompletionModel;

class KateCompletionTree : public QTreeView
{
  Q_OBJECT

  public:
    explicit KateCompletionTree(KateCompletionWidget* parent);

    KateCompletionWidget* widget() const;
    KateCompletionModel* kateModel() const;

    void resizeColumns(bool fromResizeEvent = false);
    void expandAll(const QModelIndex& index = QModelIndex());

    // Navigation
    bool nextCompletion();
    bool previousCompletion();
    void pageDown();
    void pageUp();
    void top();
    void bottom();

  protected:
    virtual void scrollContentsBy(int dx, int dy);
    virtual QStyleOptionViewItem viewOptions() const;
};

#endif
