#ifndef QSQLITETABLESTREEVIEW_H.h
#define QSQLITETABLESTREEVIEW_H

#include <QWidget>
#include <QTreeView>


class QSQLiteMasterTreeView : public QTreeView
{
    Q_OBJECT

    enum ItemType{
        ItemTypeDatabase = 0,
        ItemTypeFreeList,

        ItemTypeTable,
        ItemTypeIndex,
        ItemTypeTrigger,
        ItemTypeView,

        ItemTypeUnknown
    };

public:
    QSQLiteMasterTreeView(QWidget *parent = 0);

//    void InsertItem(QString dbPath, ItemType type, QString name, QString tableName);
//    void ClearItem(QString dbPath);
//    void EraseItem(QString dbPath, ItemType type, QString name);

signals:

    // type,name,tableName分别是sqlite_master读出的type,name,tbl_name
    // type: table, index, trigger, view
    void signalItemClicked(QString dbPath, ItemType type, QString name, QString tableName);

    void signalCurrentChanged(const QModelIndex &current);

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

};

#endif // QSQLITETABLESTREEVIEW_H
