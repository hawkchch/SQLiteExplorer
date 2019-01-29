#ifndef QSQLITEQUERYWINDOW_H
#define QSQLITEQUERYWINDOW_H

#include <QWidget>
#include <QSplitter>

namespace Ui {
class QSQLiteQueryWindow;
}

class QSQLiteTableView;
class MainWindow;

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
    void onExplainBtnClicked();
    void onDataLoaded(const QString& msg);
private:
    Ui::QSQLiteQueryWindow *ui;

    QSQLiteTableView* m_pTableView;

    // QSplitter
    QSplitter* m_pSplitter;

    MainWindow* m_pParent;
};

#endif // QSQLITEQUERYWINDOW_H
