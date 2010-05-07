/* This file is part of the KDE project
   Copyright (C) 2006 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Anders Lund <anders@alweb.dk>

   Code taken from katefilelist.cpp:
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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kateviewdocumentproxymodel.h"
#include "kateviewdocumentproxymodel.moc"
#include "katefilelist.h"

#include <KColorScheme>
#include <KColorUtils>
#include <KConfigGroup>
#include <KGlobal>
#include <KDebug>

#include <QColor>
#include <QBrush>
#include <QPalette>

KateViewDocumentProxyModel::KateViewDocumentProxyModel(QObject *parent)
  : QAbstractProxyModel(parent)
  , m_selection(new QItemSelectionModel(this, this))
  , m_markOpenedTimer(new QTimer(this))
  , m_rowCountOffset(0)
{
  KConfigGroup config(KGlobal::config(), "FileList");

  KColorScheme colors(QPalette::Active);

  QColor bg = colors.background().color();
  m_editShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::ActiveText).color(), 0.5);
  m_viewShade = KColorUtils::tint(bg, colors.foreground(KColorScheme::VisitedText).color(), 0.5);
  m_editShade = config.readEntry("Edit Shade", m_editShade);
  m_viewShade = config.readEntry("View Shade", m_viewShade);
  m_shadingEnabled = config.readEntry("Shading Enabled", true);

  m_sortRole = config.readEntry("SortRole", (int)KateDocManager::OpeningOrderRole);

  m_markOpenedTimer->setSingleShot(true);
  connect(m_markOpenedTimer, SIGNAL(timeout()), this, SLOT(slotMarkOpenedTimer()));
  /*    connect(m_selection,SIGNAL(selectionChanged ( const QItemSelection &, const QItemSelection &)),this,SLOT(slotSelectionChanged ( const QItemSelection &, const QItemSelection &)));*/
}

QStringList KateViewDocumentProxyModel::mimeTypes() const
{
  QStringList types;
  types << "application/x-kateviewdocumentproxymodel";
  return types;
}

QMimeData *KateViewDocumentProxyModel::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (const QModelIndex &index, indexes)
  {
    if (index.isValid())
    {
      kDebug()<<"mimeData:"<<index;
      stream << index.row() << index.column();
    }
  }
  mimeData->setData("application/x-kateviewdocumentproxymodel", encodedData);
  return mimeData;
}

bool KateViewDocumentProxyModel::dropMimeData(const QMimeData *data,
    Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  if ( (row == -1) || (column == -1) )
  {
    // dropping into empty space, let's consider this "at the very bottom".
    row = m_mapToSource.count();
    column = 0;
  }

  if (action == Qt::IgnoreAction)
    return true;

  if (!data->hasFormat("application/x-kateviewdocumentproxymodel"))
    return false;

  if (column > 0)
    return false;

  if (parent.isValid()) return false;

  QByteArray encodedData = data->data("application/x-kateviewdocumentproxymodel");
  QDataStream stream(&encodedData, QIODevice::ReadOnly);
#ifdef __GNUC__
#warning handle single item moves only for now;
#endif
  int sourcerow;
  int sourcecolumn;
  stream >> sourcerow;
  stream >> sourcecolumn;
  kDebug() << sourcerow << "/" << sourcecolumn;

  int insertRowAt = row - ((sourcerow < row) ? 1 : 0);



  beginRemoveRows(parent, sourcerow, sourcerow);

  int sourceModelRow = m_mapToSource[sourcerow];

  for (int i = sourcerow;i < m_mapToSource.count() - 1;i++)
    m_mapToSource[i] = m_mapToSource[i+1];
  m_mapToSource.removeLast();

  for (int i = 0;i < m_mapToSource.count();i++)
  {
    int tmp = m_mapToSource[i];
    m_mapFromSource[tmp] = i;
  }
  m_rowCountOffset--;
  endRemoveRows();

   kDebug()<<sourcerow<<"/////"<<insertRowAt;
// 
    beginInsertRows(parent, insertRowAt, insertRowAt);
    m_mapToSource.insert(insertRowAt, sourceModelRow);

    for (int i = 0;i < m_mapToSource.count();i++)
    {
      int tmp = m_mapToSource[i];
      m_mapFromSource[tmp] = i;
    }
// 
   kDebug()<<"m_mapFromSource"<<m_mapFromSource;
   kDebug()<<"m_mapToSource"<<m_mapToSource;
// 
    m_rowCountOffset++;
    endInsertRows();
    QModelIndex index = createIndex(insertRowAt, 0);
    opened(index);

  m_sortRole = CustomOrderRole;

  return true;
}

Qt::DropActions KateViewDocumentProxyModel::supportedDropActions () const
{
#ifdef __GNUC__
#warning Qt needs Qt::CopyAction, otherwise nothing works
#endif
  return Qt::MoveAction | Qt::CopyAction;
}

Qt::ItemFlags KateViewDocumentProxyModel::flags ( const QModelIndex & index ) const
{
  Qt::ItemFlags f = QAbstractProxyModel::flags(index);
  if (index.isValid()) {
    if (m_sortRole==CustomOrderRole) {
        f = (f & ~(Qt::ItemIsDropEnabled)) | Qt::ItemIsDragEnabled;
    } else {
        f = (f & ~(Qt::ItemIsDropEnabled)) & ~(Qt::ItemIsDragEnabled);
    }
  }
  else
    f = f | Qt::ItemIsDropEnabled; // | Qt::ItemIsDragEnabled;
  return f;
}


KateViewDocumentProxyModel::~KateViewDocumentProxyModel()
{}

class EditViewCount
{
  public:
    EditViewCount(): edit(0), view(0)
    {}
    int edit;
    int view;
};

void KateViewDocumentProxyModel::opened(const QModelIndex &index)
{
  kDebug(13001) << index;
//  if (m_current == index) return;

  m_selection->setCurrentIndex(index, QItemSelectionModel::Clear|QItemSelectionModel::SelectCurrent);

  m_current = index;
  m_markOpenedTimer->start(100);

  // This to protect Multiple call to sort()
  // As this function is getting called on KateFileList clicked event to open and activate doc
  // So if the sender is KateFileList ignore sorting.
  KateFileList* f = qobject_cast<KateFileList*>(sender());
  if(!f)
    sort();
}

void KateViewDocumentProxyModel::slotMarkOpenedTimer()
{
//   kDebug(13001) << "KateViewDocumentProxyModel::slotMarkOpenedTimer: before valid check";
  if (!m_current.isValid()) return;
//   kDebug(13001) << "KateViewDocumentProxyModel::slotMarkOpenedTimer: after valid check";
  QModelIndex index = mapToSource(m_current);
  m_viewHistory.removeAll(index);
  m_viewHistory.prepend(index);

  while (m_viewHistory.count() > 10) m_viewHistory.removeLast();
//   kDebug(13001) << "KateViewDocumentProxyModel::slotMarkOpenedTimer: updateBackgrounds";
  updateBackgrounds();

}

void KateViewDocumentProxyModel::modified(const QModelIndex &proxyIndex)
{
  kDebug(13001) << "KateViewDocumentProxyModel::modified"<<proxyIndex;
// FIXME there is an extremely complex path to calls of this, through KateMainWindow.
  QModelIndex index = mapToSource(proxyIndex);
  m_editHistory.removeAll(index);
  m_editHistory.prepend(index);

  while (m_editHistory.count() > 10) m_editHistory.removeLast();

  updateBackgrounds();
}

void KateViewDocumentProxyModel::updateBackgrounds(bool emitSignals)
{
  if (!m_shadingEnabled) return;
//   kDebug()<<"Shading is enabled"<<endl;
//   kDebug()<<emitSignals;
  QMap <QModelIndex, EditViewCount> helper;
  int i = 1;
  foreach (const QModelIndex &idx, m_viewHistory)
  {
    helper[idx].view = i;
    i++;
  }
  i = 1;
  foreach (const QModelIndex &idx, m_editHistory)
  {
    helper[idx].edit = i;
    i++;
  }
  QMap<QModelIndex, QBrush> oldBrushes = m_brushes;
  m_brushes.clear();
  int hc = m_viewHistory.count();
  int ec = m_editHistory.count();
  for (QMap<QModelIndex, EditViewCount>::iterator it = helper.begin();it != helper.end();++it)
  {
    QColor shade( m_viewShade );
    QColor eshade( m_editShade );
    if (it.value().edit > 0)
    {
      int v = hc - it.value().view;
      int e = ec - it.value().edit + 1;
      e = e * e;
      int n = qMax(v + e, 1);
      shade.setRgb(
        ((shade.red()*v) + (eshade.red()*e)) / n,
        ((shade.green()*v) + (eshade.green()*e)) / n,
        ((shade.blue()*v) + (eshade.blue()*e)) / n
      );
    }

    // blend in the shade color; latest is most colored.
    double t = double(hc - it.value().view + 1) / double(hc);

    m_brushes[it.key()] = QBrush(KColorUtils::mix(QPalette().color(QPalette::Base), shade, t));
//     kdDebug()<<"m_brushes[it.key()]"<<it.key()<<m_brushes[it.key()];
  }
  foreach(const QModelIndex & key, m_brushes.keys())
  {
    oldBrushes.remove(key);
    if (emitSignals) dataChanged(key, key);
  }
  foreach(const QModelIndex & key, oldBrushes.keys())
  {
    if (emitSignals) dataChanged(key, key);
  }
}

QVariant KateViewDocumentProxyModel::data ( const QModelIndex & index, int role ) const
{
  if (role == Qt::BackgroundColorRole)
  {
    //kDebug()<<"BACKGROUNDROLE";
    QBrush br = m_brushes[mapToSource(index)];
//     kDebug()<<"br.style()!=Qt::NoBrush:"<<(br.style() != Qt::NoBrush);
//     kDebug()<<"br"<<br;
//     kDebug()<<index;
    if ((br.style() != Qt::NoBrush) && m_shadingEnabled) {
//       kDebug()<<"returning background brush";
      return br;
    }
  }

  if ( role == CustomOrderRole )
    return index.row();


/*  kDebug()<<index;
  kDebug()<<mapToSource(index);*/

  return sourceModel()->data(mapToSource(index), role);
}

QModelIndex KateViewDocumentProxyModel::mapFromSource ( const QModelIndex & sourceIndex ) const
{
  if (!sourceIndex.isValid()) return QModelIndex();
  return createIndex(m_mapFromSource[sourceIndex.row()], sourceIndex.column());
}

QItemSelection KateViewDocumentProxyModel::mapSelectionFromSource ( const QItemSelection & sourceSelection ) const
{
  return QAbstractProxyModel::mapSelectionFromSource(sourceSelection);
}
QItemSelection KateViewDocumentProxyModel::mapSelectionToSource ( const QItemSelection & proxySelection ) const
{
  kDebug() << "KateViewDocumentProxyModel::mapSelectionToSource";
  return QAbstractProxyModel::mapSelectionToSource(proxySelection);
}

QModelIndex KateViewDocumentProxyModel::mapToSource ( const QModelIndex & proxyIndex ) const
{
  if (!proxyIndex.isValid()) return QModelIndex();
  if (proxyIndex.row()>=m_mapToSource.count()) return QModelIndex();
  return sourceModel()->index(m_mapToSource[proxyIndex.row()], proxyIndex.column(), QModelIndex());
}

int KateViewDocumentProxyModel::columnCount ( const QModelIndex & parent) const
{
  return sourceModel()->columnCount(mapToSource(parent));
}

QModelIndex KateViewDocumentProxyModel::index ( int row, int column, const QModelIndex & parent) const
{
  if ( (row<0) || (column<0) ) return QModelIndex();
  if (parent.isValid()) return QModelIndex();
  if (row >= rowCount(parent)) return QModelIndex();
  return createIndex(row, column);
}

QModelIndex KateViewDocumentProxyModel::parent ( const QModelIndex & index ) const
{
  Q_UNUSED(index)
  return QModelIndex();
  //return mapFromSource(sourceModel()->parent(mapToSource(index)));
}

int KateViewDocumentProxyModel::rowCount ( const QModelIndex & parent) const
{
  if (parent.isValid()) return 0;
  return qMax(0, sourceModel()->rowCount(mapToSource(parent)) + m_rowCountOffset);

}

void KateViewDocumentProxyModel::setSourceModel ( QAbstractItemModel * sourceModel )
{
  QAbstractItemModel *sm = this->sourceModel();
  if (sm)
  {
    disconnect(sm, 0, this, 0);
//     disconnect(sm, SIGNAL(columnsAboutToBeInserted ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsAboutToBeInserted ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsAboutToBeRemoved ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(columnsInserted ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsInserted ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(columnsRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsRemoved ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(dataChanged ( const QModelIndex & , const QModelIndex &  )), this, SLOT(slotDataChanged ( const QModelIndex & , const QModelIndex &  )));
//     disconnect(sm, SIGNAL(headerDataChanged ( Qt::Orientation, int , int  )), this, SLOT(slotHeaderDataChanged ( Qt::Orientation, int , int  )));
//     disconnect(sm, SIGNAL(layoutAboutToBeChanged ()), this, SLOT(slotLayoutAboutToBeChanged ()));
//     disconnect(sm, SIGNAL(layoutChanged ()), this, SLOT(slotLayoutChanged ()));
//     disconnect(sm, SIGNAL(modelAboutToBeReset ()), this, SLOT(slotModelAboutToBeReset ()));
//     disconnect(sm, SIGNAL(modelReset ()), this, SLOT(slotModelReset ()));
//     disconnect(sm, SIGNAL(rowsAboutToBeInserted ( const QModelIndex & , int , int  )), this, SLOT(slotRowsAboutToBeInserted ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotRowsAboutToBeRemoved ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(rowsInserted ( const QModelIndex & , int , int  )), this, SLOT(slotRowsInserted ( const QModelIndex & , int , int  )));
//     disconnect(sm, SIGNAL(rowsRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotRowsRemoved ( const QModelIndex & , int , int  )));
  }
  sm = sourceModel;
  if (sm)
  {
    connect(sm, SIGNAL(columnsAboutToBeInserted ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsAboutToBeInserted ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsAboutToBeRemoved ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(columnsInserted ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsInserted ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(columnsRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotColumnsRemoved ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(dataChanged ( const QModelIndex & , const QModelIndex &  )), this, SLOT(slotDataChanged ( const QModelIndex & , const QModelIndex &  )));
    connect(sm, SIGNAL(headerDataChanged ( Qt::Orientation, int , int  )), this, SLOT(slotHeaderDataChanged ( Qt::Orientation, int , int  )));
    connect(sm, SIGNAL(layoutAboutToBeChanged ()), this, SLOT(slotLayoutAboutToBeChanged ()));
    connect(sm, SIGNAL(layoutChanged ()), this, SLOT(slotLayoutChanged ()));
//     connect(sm, SIGNAL(modelAboutToBeReset ()), this, SLOT(slotModelAboutToBeReset ()));
    connect(sm, SIGNAL(modelReset ()), this, SLOT(slotModelReset ()));
    connect(sm, SIGNAL(rowsAboutToBeInserted ( const QModelIndex & , int , int  )), this, SLOT(slotRowsAboutToBeInserted ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotRowsAboutToBeRemoved ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(rowsInserted ( const QModelIndex & , int , int  )), this, SLOT(slotRowsInserted ( const QModelIndex & , int , int  )));
    connect(sm, SIGNAL(rowsRemoved ( const QModelIndex & , int , int  )), this, SLOT(slotRowsRemoved ( const QModelIndex & , int , int  )));
  }
  m_mapToSource.clear();
  m_mapFromSource.clear();
  if (!sm)
    return;

  int cnt = sm->rowCount();
  for (int i = 0;i < cnt;i++)
  {
    m_mapToSource.append(i);
    m_mapFromSource.append(i);
  }
  QAbstractProxyModel::setSourceModel(sm);

  sort();
}


void KateViewDocumentProxyModel::slotColumnsAboutToBeInserted ( const QModelIndex & parent, int start, int end )
{
  Q_UNUSED(parent)
  beginInsertColumns(QModelIndex(), start, end);
}

void KateViewDocumentProxyModel::slotColumnsAboutToBeRemoved ( const QModelIndex & parent, int start, int end )
{
  beginRemoveColumns(mapFromSource(parent), start, end);
}

void KateViewDocumentProxyModel::slotColumnsInserted ( const QModelIndex & parent, int start, int end )
{
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  endInsertColumns();
}
void KateViewDocumentProxyModel::slotColumnsRemoved ( const QModelIndex & parent, int start, int end )
{
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)
  endRemoveColumns();
}
void KateViewDocumentProxyModel::slotDataChanged ( const QModelIndex & topLeft, const QModelIndex & bottomRight )
{
  dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight));
}
void KateViewDocumentProxyModel::slotHeaderDataChanged ( Qt::Orientation orientation, int first, int last )
{
//  emit headerDataChanged(orientation, first, last);
}
void KateViewDocumentProxyModel::slotLayoutAboutToBeChanged ()
{
  emit layoutAboutToBeChanged();
}

void KateViewDocumentProxyModel::slotLayoutChanged ()
{
  emit layoutChanged();
}

void KateViewDocumentProxyModel::slotModelAboutToBeReset ()
{
  //emit modelAboutToBeReset();
}

void KateViewDocumentProxyModel::slotModelReset ()
{
  emit reset();
}
void KateViewDocumentProxyModel::slotRowsAboutToBeInserted ( const QModelIndex & parent, int start, int end )
{
//   kDebug(13001)<<start<<end;
  //beginInsertRows(mapFromSource(parent),start,end);
  beginInsertRows(mapFromSource(parent), m_mapFromSource.count(), m_mapFromSource.count() - start + end);
  int insertedRange = end - start + 1;
  if (m_current.isValid())
  {
    if (m_current.row() > start)
    {
      m_current = createIndex(m_current.row() + insertedRange, m_current.column());
    }
  }
  updateBackgrounds(false);

}


void KateViewDocumentProxyModel::removeItemFromColoring(int row)
{
//   kDebug(13001)<<row;
  QModelIndex removeit = mapToSource(createIndex(row, 0));
  m_editHistory.removeAll(removeit);

  // adjust all indices below 'row'
  for(int i = 0; i < m_editHistory.count(); ++i) {
    if (m_editHistory[i].row() >= removeit.row())
      m_editHistory[i] = createIndex(m_editHistory[i].row() - 1, 0);
  }

  // adjust all indices below 'row'
  m_viewHistory.removeAll(removeit);
  for(int i = 0; i < m_viewHistory.count(); ++i) {
    if (m_viewHistory[i].row() >= removeit.row())
      m_viewHistory[i] = createIndex(m_viewHistory[i].row() - 1, 0);
  }

  updateBackgrounds(false);
}


void KateViewDocumentProxyModel::slotRowsAboutToBeRemoved ( const QModelIndex & parent, int start, int end )
{
//   kDebug(13001)<<start<<end;
  Q_UNUSED(parent)

  beginRemoveRows(QModelIndex(), m_mapFromSource[start], m_mapFromSource[end]);

  for (int row = end;row >= start;row--)
  {
    int removeRow = m_mapFromSource[row];
    removeItemFromColoring(removeRow);

    m_rowCountOffset--;

    m_mapToSource.removeAt(removeRow);
    for (int i = 0; i < m_mapToSource.count(); i++)
    {
      const int currentRow = m_mapToSource[i];
      if (currentRow > row) {
        m_mapToSource[i] = currentRow - 1;
      }
    }

    foreach (int sr, m_mapToSource) kDebug()<<sr;
    kDebug()<<"**************";

    m_mapFromSource.removeLast();
    for (int i = 0; i < m_mapToSource.count(); i++)
    {
      int tmp = m_mapToSource[i];
      m_mapFromSource[tmp] = i;
    }
    //foreach (int sd, m_mapFromSource) kDebug()<<sd;

    for (int i=0;i<m_mapToSource.size();i++)
	kDebug()<<data(createIndex(i,0),Qt::DisplayRole);
  }
  m_rowCountOffset = 0;
}

void KateViewDocumentProxyModel::slotRowsInserted ( const QModelIndex & parent, int start, int end )
{
//   kDebug(13001)<<start<<end;
  Q_UNUSED(parent)
  int cnt = m_mapFromSource.count();
  for (int i = start;i <= end;i++, cnt++)
  {
    m_mapFromSource.insert(i, cnt);
  }
  cnt = m_mapFromSource.count();
  m_mapToSource.clear();
  for (int i = 0;i < cnt;i++)
  {
    m_mapToSource.append(-1);
  }
  for (int i = 0;i < cnt;i++)
  {
    m_mapToSource[m_mapFromSource[i]] = i;
  }
  endInsertRows();
}
void KateViewDocumentProxyModel::slotRowsRemoved ( const QModelIndex & parent, int start, int end )
{
//   kDebug(13001)<<start<<end;
  Q_UNUSED(parent)
  Q_UNUSED(start)
  Q_UNUSED(end)

  endRemoveRows();


  m_rowCountOffset = 0;
  foreach(const QModelIndex &key, m_brushes.keys())
  {
    dataChanged(key, key);
  }
}

void KateViewDocumentProxyModel::setSortRole( int role )
{
//   kDebug(13001)<<role;
  m_sortRole = role;
  sort();
}

void KateViewDocumentProxyModel::sort()
{
//   kDebug(13001)<<"sorting"<<m_sortRole<<m_mapToSource;

  // we want to maintain selection state
  // ### would it be possible to make the selection be maintained in the source model?
  QModelIndex sourceSelected, sourceCurrent;
  if (m_selection->hasSelection())
    sourceSelected = mapToSource(m_selection->selectedIndexes().first());
  sourceCurrent = mapToSource(m_selection->currentIndex());

  switch (m_sortRole) {
    case KateDocManager::OpeningOrderRole:
    case CustomOrderRole:
    {
      QMap<int,int> sorted;
      foreach( int row, m_mapToSource )
        sorted.insert(data(sourceModel()->index(row, 0), m_sortRole).toInt(), m_mapToSource[row]);
      m_mapToSource = sorted.values();
    }
    break;
    default:
    {
      QMap<QString,int> sorted;
      foreach( int row, m_mapToSource ) {
	QString key=data(sourceModel()->index(row, 0), m_sortRole).toString();
        if (key.isEmpty()) key=QString("kate-internal-untitled:/%1").arg(row);
	sorted.insert(key, m_mapToSource[row]);
      }
      m_mapToSource = sorted.values();
    }
  }

  // also reorder m_mapFromSource
  for (int i = 0;i < m_mapToSource.count();i++)
  {
    int tmp = m_mapToSource[i];
    m_mapFromSource[tmp] = i;
  }

  // select the correct item.
  if (sourceSelected.isValid())
    m_selection->select(mapFromSource(sourceSelected), QItemSelectionModel::ClearAndSelect);
  if (sourceCurrent.isValid())
    m_selection->setCurrentIndex(mapFromSource(sourceCurrent), QItemSelectionModel::SelectCurrent);

  updateBackgrounds(false);
  emit layoutChanged();
}

void KateViewDocumentProxyModel::readSessionConfig( const KConfigBase *config, const QString & name ) {
  KConfigGroup cg( config, name );
  m_sortRole = cg.readEntry( "SortRole", m_sortRole );

  // Make sure the new order is only applied if file list and map are in sync.
  QList<int> mapToSource = cg.readEntry( "Order", m_mapToSource );
  if (mapToSource.count() == m_mapToSource.count()) {
    m_mapToSource = mapToSource;
  }
  sort(); // also reorders m_mapFromSource
}

void KateViewDocumentProxyModel::writeSessionConfig( KConfigBase *config, const QString & name ) {
  KConfigGroup cg( config, name );
  cg.writeEntry( "SortRole", m_sortRole );
  cg.writeEntry( "Order", m_mapToSource );
}

// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
