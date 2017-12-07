#ifndef QSQLITETABLEVIEW_H
#define QSQLITETABLEVIEW_H

#include <QWidget>
#include <QTableView>
#include <QTableWidget>

#include "SQLite3DB.h"
class MainWindow;


class QSQLiteTableView : public QTableWidget
{
    Q_OBJECT
public:
    QSQLiteTableView(QWidget *parent = 0);

public slots:
    void onSQLiteQueryReceived(const QString& sql);
    void sortByColumn(int column);

private:
    MainWindow* m_pParent;
    CSQLite3DB* m_pCurSQLite3DB;
};

#endif // QSQLITETABLEVIEW_H
