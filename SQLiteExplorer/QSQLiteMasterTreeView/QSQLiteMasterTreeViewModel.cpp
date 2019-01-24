#include "QSQLiteMasterTreeViewModel.h"

QSQLiteMasterTreeViewModel::QSQLiteMasterTreeViewModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

QVariant QSQLiteMasterTreeViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // FIXME: Implement me!
}

bool QSQLiteMasterTreeViewModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role)
{
    if (value != headerData(section, orientation, role)) {
        // FIXME: Implement me!
        emit headerDataChanged(orientation, section, section);
        return true;
    }
    return false;
}

QModelIndex QSQLiteMasterTreeViewModel::index(int row, int column, const QModelIndex &parent) const
{
    // FIXME: Implement me!
}

QModelIndex QSQLiteMasterTreeViewModel::parent(const QModelIndex &index) const
{
    // FIXME: Implement me!
}

int QSQLiteMasterTreeViewModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
}

int QSQLiteMasterTreeViewModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    // FIXME: Implement me!
}

bool QSQLiteMasterTreeViewModel::hasChildren(const QModelIndex &parent) const
{
    // FIXME: Implement me!
}

bool QSQLiteMasterTreeViewModel::canFetchMore(const QModelIndex &parent) const
{
    // FIXME: Implement me!
    return false;
}

void QSQLiteMasterTreeViewModel::fetchMore(const QModelIndex &parent)
{
    // FIXME: Implement me!
}

QVariant QSQLiteMasterTreeViewModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    // FIXME: Implement me!
    return QVariant();
}

bool QSQLiteMasterTreeViewModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value) {
        // FIXME: Implement me!
        emit dataChanged(index, index, QVector<int>() << role);
        return true;
    }
    return false;
}

Qt::ItemFlags QSQLiteMasterTreeViewModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEditable; // FIXME: Implement me!
}

bool QSQLiteMasterTreeViewModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endInsertRows();
}

bool QSQLiteMasterTreeViewModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endInsertColumns();
}

bool QSQLiteMasterTreeViewModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    // FIXME: Implement me!
    endRemoveRows();
}

bool QSQLiteMasterTreeViewModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);
    // FIXME: Implement me!
    endRemoveColumns();
}
