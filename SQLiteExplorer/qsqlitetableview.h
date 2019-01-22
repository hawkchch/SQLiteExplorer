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

signals:
    void dataLoaded(const QString& msg);

public slots:
    void onSQLiteQueryReceived(const QString& sql);
    void onValueChanged(int value);

private:
    MainWindow* m_pParent;
    CSQLite3DB* m_pCurSQLite3DB;
    CppSQLite3Query m_curQuery;
    int m_rowThresh;
};

#endif // QSQLITETABLEVIEW_H
