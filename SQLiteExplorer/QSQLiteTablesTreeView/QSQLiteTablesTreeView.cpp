#include "QSQLiteTablesTreeView.h"

QSQLiteTablesTreeView::QSQLiteTablesTreeView(QWidget *parent)
    : QTreeView(parent)
{

}

void QSQLiteTablesTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    emit signalCurrentChanged(current);
}
