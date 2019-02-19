#ifndef DATAWINDOW_H
#define DATAWINDOW_H

#include <QWidget>


namespace Ui {
class DataWindow;
}
class MainWindow;
class DataWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DataWindow(QWidget *parent = 0);
    ~DataWindow();

    void clear();

public slots:
    void onSQLiteQueryReceived(const QString& sql);
    void onDataLoaded(const QString& msg);
private:
    MainWindow* m_pParent;
    Ui::DataWindow *ui;
};

#endif // DATAWINDOW_H
