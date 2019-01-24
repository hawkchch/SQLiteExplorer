#include "QSQLiteMasterTreeView.h"

QSQLiteMasterTreeView::QSQLiteMasterTreeView(QWidget *parent)
    : QTreeView(parent)
{

}

void QSQLiteMasterTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)
    emit signalCurrentChanged(current);
}
