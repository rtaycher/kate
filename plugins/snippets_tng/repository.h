/* This file is part of the KDE project
 * Copyright (C) 2009 Joseph Wenninger <jowenn@kde.org>
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef __JOWENN_SNIPPETS_REPOSITORY_H__
#define __JOWENN_SNIPPETS_REPOSITORY_H__

#include "kwidgetitemdelegate.h"
#include <QAbstractListModel>
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/document.h>

class KConfigBase;

namespace JoWenn {
  class KateSnippetRepositoryEntry;
  class KateSnippetCompletionEntry;  
  
  class KateSnippetRepositoryItemDelegate: public KWidgetItemDelegate
  {
      Q_OBJECT
    public:
      explicit KateSnippetRepositoryItemDelegate(QAbstractItemView *itemView, QObject * parent = 0);
      virtual ~KateSnippetRepositoryItemDelegate();

      virtual QList<QWidget*> createItemWidgets() const;

      virtual void updateItemWidgets(const QList<QWidget*> widgets,
        const QStyleOptionViewItem &option,
        const QPersistentModelIndex &index) const;
        
      virtual void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
      virtual QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const;
    public Q_SLOTS:
      void enabledChanged(int state);
      void editEntry();
      void deleteEntry();      
  };
  
  
  class KateSnippetRepositoryModel: public QAbstractListModel
  {
      Q_OBJECT
    public:
      explicit KateSnippetRepositoryModel(QObject *parent=0);
      virtual ~KateSnippetRepositoryModel();
      enum Roles {
        NameRole = Qt::UserRole,
        FilenameRole,
        FiletypeRole,
        AuthorsRole,
        LicenseRole,
        SystemFileRole,
        EnabledRole,
        DeleteNowRole,
        EditNowRole
      };
      
      virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
      virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
      virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
      void addEntry(const QString& name, const QString& filename, const QString& filetype, const QString& authors, const QString& license, bool systemFile, bool enabled);
      KTextEditor::CodeCompletionModel2* completionModel(const QString &filetype);
      void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
      void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

    Q_SIGNALS:
      void typeChanged(const QString& fileType);
    public Q_SLOTS:
      void newEntry();
    private:
      QList<KateSnippetRepositoryEntry> m_entries;
  };

#if 0  
    class KateSnippetRepositoryModelAdaptor: public QDBusAbstractAdaptor
    {
        Q_OBJECT
        Q_CLASSINFO("D-Bus Interface", "org.kde.Kate.Plugin.SnippetsTNG.Repository")
      public:
        KateSnippetRepositoryModelAdaptor(KateSnippetRepositoryModel *repository);
        virtual ~KateSnippetRepositoryModelAdaptor();
      public Q_SLOTS:
        void updateFileLocation(const QString& oldPath, const QString& newPath);
      private:
        KateSnippetRepositoryModel* m_repository;
    };
#endif      
  
}
#endif
// kate: space-indent on; indent-width 2; replace-tabs on;