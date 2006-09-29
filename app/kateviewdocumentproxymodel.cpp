/* This file is part of the KDE project
   Copyright (C) 2006 Joseph Wenninger <jowenn@kde.org>

****************
Code taken from katefilelist.cpp:
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
******

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

#include <QColor>
#include <QBrush>
#include <QPalette>

#include <kdebug.h>

KateViewDocumentProxyModel::KateViewDocumentProxyModel(QObject *parent):QAbstractProxyModel(parent),m_selection(new QItemSelectionModel(this,this)) {
/*    connect(m_selection,SIGNAL(selectionChanged ( const QItemSelection &, const QItemSelection &)),this,SLOT(slotSelectionChanged ( const QItemSelection &, const QItemSelection &)));*/
}


KateViewDocumentProxyModel::~KateViewDocumentProxyModel() {
}

/*void KateViewDocumentProxyModel::slotSelectionChanged ( const QItemSelection & selected, const QItemSelection & deselected ){
    kDebug()<<"KateViewDocumentProxyModel::slotSelectionChanged"<<endl;
    QModelIndexList list=selected.indexes();
    kDebug()<<"List length:"<<list.count()<<endl;
    foreach(const QModelIndex & item,list) {
        kDebug()<<"KateViewDocumentProxyModel::slotSelectionChanged: there is a selected item"<<endl;
        m_viewHistory.prepend(item);
        emit dataChanged(item,item);
    }
}*/

class EditViewCount {
    public:
        EditViewCount():edit(0),view(0) {}
        int edit;
        int view;
};

void KateViewDocumentProxyModel::opened(const QModelIndex &index) {
    m_current=index;
    kDebug()<<"KateViewDocumentProxyModel::opened"<<endl;;
    m_viewHistory.removeAll(index);
    m_viewHistory.prepend(index);

    while (m_viewHistory.count()>20) m_viewHistory.removeLast();

    updateBackgrounds();
}

void KateViewDocumentProxyModel::modified(const QModelIndex &index) {
    kDebug()<<"KateViewDocumentProxyModel::modified"<<endl;;
    m_editHistory.removeAll(index);
    m_editHistory.prepend(index);

    while (m_editHistory.count()>20) m_editHistory.removeLast();

    updateBackgrounds();
}

void KateViewDocumentProxyModel::updateBackgrounds(bool emitSignals) {
    QMap <QModelIndex,EditViewCount> helper;
    int i=1;
    foreach (const QModelIndex &idx,m_viewHistory) {
        helper[idx].view=i;
        i++;
    }
    i=0;
    foreach (const QModelIndex &idx,m_editHistory) {
        helper[idx].edit=i;
        i++;
    }
    QMap<QModelIndex,QBrush> oldBrushes=m_brushes;
    m_brushes.clear();
    int hc=m_viewHistory.count();
    int ec=m_viewHistory.count();
    for (QMap<QModelIndex,EditViewCount>::iterator it=helper.begin();it!=helper.end();++it) {
        QColor b = QPalette().color(QPalette::Base);
        QColor shade( 51, 204, 255 );
        QColor eshade( 255, 102, 153 );
        if (it.value().edit>0) {
            int v = hc-it.value().view;
            int e = ec-it.value().edit+1;
            e = e*e;
            int n = qMax(v + e, 1);
            shade.setRgb(
                ((shade.red()*v) + (eshade.red()*e))/n,
                ((shade.green()*v) + (eshade.green()*e))/n,
                ((shade.blue()*v) + (eshade.blue()*e))/n
            );
        }
        // blend in the shade color.
        // max transperancy < .5, latest is most colored.
        float t = (0.5/hc)*(hc-it.value().view+1);
        b.setRgb(
            (int)((b.red()*(1-t)) + (shade.red()*t)),
            (int)((b.green()*(1-t)) + (shade.green()*t)),
            (int)((b.blue()*(1-t)) + (shade.blue()*t))
        );
        m_brushes[it.key()]=QBrush(b);
    }
    foreach(const QModelIndex & key,m_brushes.keys()) {
        oldBrushes.remove(key);
        if (emitSignals) dataChanged(key,key);
    }
    foreach(const QModelIndex & key,oldBrushes.keys()) {
        if (emitSignals) dataChanged(key,key);
    }
}

QVariant KateViewDocumentProxyModel::data ( const QModelIndex & index, int role ) const {
    if (role==Qt::BackgroundColorRole) {
        kDebug()<<"BACKGROUNDROLE"<<endl;
        QBrush br=m_brushes[index];
        if (br.style()!=Qt::NoBrush)
            return br;
    }
    return sourceModel()->data(mapToSource(index), role);
}

QModelIndex KateViewDocumentProxyModel::mapFromSource ( const QModelIndex & sourceIndex ) const {
    if (!sourceIndex.isValid()) return QModelIndex();
    return createIndex(sourceIndex.row(),sourceIndex.column());
} 

QItemSelection KateViewDocumentProxyModel::mapSelectionFromSource ( const QItemSelection & sourceSelection ) const {
    return QAbstractProxyModel::mapSelectionFromSource(sourceSelection);
} 
QItemSelection KateViewDocumentProxyModel::mapSelectionToSource ( const QItemSelection & proxySelection ) const {
    kDebug()<<"KateViewDocumentProxyModel::mapSelectionToSource"<<endl;
    return QAbstractProxyModel::mapSelectionToSource(proxySelection);
} 

QModelIndex KateViewDocumentProxyModel::mapToSource ( const QModelIndex & proxyIndex ) const {
    if (!proxyIndex.isValid()) return QModelIndex();
    return sourceModel()->index(proxyIndex.row(),proxyIndex.column(),QModelIndex());
} 

int KateViewDocumentProxyModel::columnCount ( const QModelIndex & parent) const {
    return sourceModel()->columnCount(mapToSource(parent));
}

QModelIndex KateViewDocumentProxyModel::index ( int row, int column, const QModelIndex & parent) const {
    return mapFromSource(sourceModel()->index(row,column,mapToSource(parent)));
}

QModelIndex KateViewDocumentProxyModel::parent ( const QModelIndex & index ) const {
    return QModelIndex();
    //return mapFromSource(sourceModel()->parent(mapToSource(index)));
}

int KateViewDocumentProxyModel::rowCount ( const QModelIndex & parent) const {
    return sourceModel()->rowCount(mapToSource(parent));
}

void KateViewDocumentProxyModel::setSourceModel ( QAbstractItemModel * sourceModel ) {
    QAbstractItemModel *sm=this->sourceModel();
    if (sm) {
        disconnect(sm,SIGNAL(columnsAboutToBeInserted ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsAboutToBeInserted ( const QModelIndex & , int , int  )));
        disconnect(sm,SIGNAL(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        disconnect(sm,SIGNAL(columnsInserted ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsInserted ( const QModelIndex & , int , int  )));
        disconnect(sm,SIGNAL(columnsRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsRemoved ( const QModelIndex & , int , int  )));
        disconnect(sm,SIGNAL(dataChanged ( const QModelIndex & , const QModelIndex &  )),this,SLOT(slotDataChanged ( const QModelIndex & , const QModelIndex &  )));
        disconnect(sm,SIGNAL(headerDataChanged ( Qt::Orientation, int , int  )),this,SLOT(slotHeaderDataChanged ( Qt::Orientation, int , int  )));
        disconnect(sm,SIGNAL(layoutAboutToBeChanged ()),this,SLOT(slotLayoutAboutToBeChanged ()));
        disconnect(sm,SIGNAL(layoutChanged ()),this,SLOT(slotLayoutChanged ()));
        disconnect(sm,SIGNAL(modelAboutToBeReset ()),this,SLOT(slotModelAboutToBeReset ()));
        disconnect(sm,SIGNAL(modelReset ()),this,SLOT(slotModelReset ()));
        disconnect(sm,SIGNAL(rowsAboutToBeInserted ( const QModelIndex & , int , int  )),this,SLOT(slotRowsAboutToBeInserted ( const QModelIndex & , int , int  ))); 
        disconnect(sm,SIGNAL(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotRowsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        disconnect(sm,SIGNAL(rowsInserted ( const QModelIndex & , int , int  )),this,SLOT(slotRowsInserted ( const QModelIndex & , int , int  )));
        disconnect(sm,SIGNAL(rowsRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotRowsRemoved ( const QModelIndex & , int , int  )));
    }
    sm=sourceModel;
    if (sm) {
        connect(sm,SIGNAL(columnsAboutToBeInserted ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsAboutToBeInserted ( const QModelIndex & , int , int  )));
        connect(sm,SIGNAL(columnsAboutToBeRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        connect(sm,SIGNAL(columnsInserted ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsInserted ( const QModelIndex & , int , int  )));
        connect(sm,SIGNAL(columnsRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotColumnsRemoved ( const QModelIndex & , int , int  )));
        connect(sm,SIGNAL(dataChanged ( const QModelIndex & , const QModelIndex &  )),this,SLOT(slotDataChanged ( const QModelIndex & , const QModelIndex &  )));
        connect(sm,SIGNAL(headerDataChanged ( Qt::Orientation, int , int  )),this,SLOT(slotHeaderDataChanged ( Qt::Orientation, int , int  )));
        connect(sm,SIGNAL(layoutAboutToBeChanged ()),this,SLOT(slotLayoutAboutToBeChanged ()));
        connect(sm,SIGNAL(layoutChanged ()),this,SLOT(slotLayoutChanged ()));
        connect(sm,SIGNAL(modelAboutToBeReset ()),this,SLOT(slotModelAboutToBeReset ()));
        connect(sm,SIGNAL(modelReset ()),this,SLOT(slotModelReset ()));
        connect(sm,SIGNAL(rowsAboutToBeInserted ( const QModelIndex & , int , int  )),this,SLOT(slotRowsAboutToBeInserted ( const QModelIndex & , int , int  ))); 
        connect(sm,SIGNAL(rowsAboutToBeRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotRowsAboutToBeRemoved ( const QModelIndex & , int , int  )));
        connect(sm,SIGNAL(rowsInserted ( const QModelIndex & , int , int  )),this,SLOT(slotRowsInserted ( const QModelIndex & , int , int  )));
        connect(sm,SIGNAL(rowsRemoved ( const QModelIndex & , int , int  )),this,SLOT(slotRowsRemoved ( const QModelIndex & , int , int  )));
    }
    QAbstractProxyModel::setSourceModel(sm);
}


void KateViewDocumentProxyModel::slotColumnsAboutToBeInserted ( const QModelIndex & parent, int start, int end ){
    beginInsertColumns(QModelIndex(),start,end);
}
 
void KateViewDocumentProxyModel::KateViewDocumentProxyModel::slotColumnsAboutToBeRemoved ( const QModelIndex & parent, int start, int end ) {
    beginRemoveColumns(mapFromSource(parent),start,end);
}

void KateViewDocumentProxyModel::slotColumnsInserted ( const QModelIndex & parent, int start, int end ) {
    endInsertColumns();
}
void KateViewDocumentProxyModel::slotColumnsRemoved ( const QModelIndex & parent, int start, int end ) {
    endRemoveColumns();
} 
void KateViewDocumentProxyModel::slotDataChanged ( const QModelIndex & topLeft, const QModelIndex & bottomRight ) {
    dataChanged(mapFromSource(topLeft),mapFromSource(bottomRight));
} 
void KateViewDocumentProxyModel::slotHeaderDataChanged ( Qt::Orientation orientation, int first, int last ){
    emit headerDataChanged(orientation,first,last);
} 
void KateViewDocumentProxyModel::slotLayoutAboutToBeChanged () {
    emit layoutAboutToBeChanged();
} 

void KateViewDocumentProxyModel::slotLayoutChanged ()  {
    emit layoutChanged();
}

void KateViewDocumentProxyModel::slotModelAboutToBeReset () {
    //emit modelAboutToBeReset();
} 

void KateViewDocumentProxyModel::slotModelReset () {
    emit reset();
}
void KateViewDocumentProxyModel::slotRowsAboutToBeInserted ( const QModelIndex & parent, int start, int end ) {
    beginInsertRows(mapFromSource(parent),start,end);
    QList<QModelIndex> tmpView;
    QList<QModelIndex> tmpEdit;
    int insertedRange=end-start+1;
    foreach (const QModelIndex &idx, m_viewHistory) {
        if (idx.row()<start) tmpView.append(idx);
        else tmpView.append(createIndex(idx.row()+insertedRange,idx.column()));
    }
    foreach (const QModelIndex &idx, m_editHistory) {
        if (idx.row()<start) tmpEdit.append(idx);
        else if (idx.row()>start) tmpEdit.append(createIndex(idx.row()+insertedRange,idx.column()));
    }
    if (m_current.isValid()) {
        if (m_current.row()>start) {
            m_current=createIndex(m_current.row()+insertedRange,m_current.column());
        }
    }
    m_editHistory=tmpEdit;
    m_viewHistory=tmpView;
    m_brushes.clear();
    updateBackgrounds(false);

} 

void KateViewDocumentProxyModel::slotRowsAboutToBeRemoved ( const QModelIndex & parent, int start, int end ) {
    beginRemoveRows(mapFromSource(parent),start,end);
    QList<QModelIndex> tmpView;
    QList<QModelIndex> tmpEdit;
    int removedRange=end-start+1;
    foreach (const QModelIndex &idx, m_viewHistory) {
        if (idx.row()<start) tmpView.append(idx);
        else if (idx.row()>start)
            tmpView.append(createIndex(idx.row()-removedRange,idx.column()));
    }
    foreach (const QModelIndex &idx, m_editHistory) {
        if (idx.row()<start) tmpEdit.append(idx);
        else if (idx.row()>end) tmpEdit.append(createIndex(idx.row()-removedRange,idx.column()));
    }
    if (m_current.isValid()) {
        if (m_current.row()>start) {
            if (m_current.row()>end) m_current=createIndex(m_current.row()-removedRange,m_current.column());
            else m_current=QModelIndex();
        }
    }
    m_editHistory=tmpEdit;
    m_viewHistory=tmpView;
    m_brushes.clear();
    updateBackgrounds(false);
}
 
void KateViewDocumentProxyModel::slotRowsInserted ( const QModelIndex & parent, int start, int end ){
    endInsertRows();
} 
void KateViewDocumentProxyModel::slotRowsRemoved ( const QModelIndex & parent, int start, int end ) {
    endRemoveRows();
    foreach(const QModelIndex & key,m_brushes.keys()) {
        m_brushes.remove(key);
        dataChanged(key,key);
    }
}
