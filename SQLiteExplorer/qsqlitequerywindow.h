#ifndef QSQLITEQUERYWINDOW_H
#define QSQLITEQUERYWINDOW_H

#include <QWidget>
#include <QSplitter>

namespace Ui {
class QSQLiteQueryWindow;
}

class QSQLiteTableView;

class QSQLiteQueryWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QSQLiteQueryWindow(QWidget *parent = 0);
    ~QSQLiteQueryWindow();

signals:
    void signalSQLiteQuery(const QString& sql);

private slots:
    void onExecuteBtnClicked();

private:
    Ui::QSQLiteQueryWindow *ui;

    QSQLiteTableView* m_pTableView;

    // QSplitter
    QSplitter* m_pSplitter;
};

#endif // QSQLITEQUERYWINDOW_H
