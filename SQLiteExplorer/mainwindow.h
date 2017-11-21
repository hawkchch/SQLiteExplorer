#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QTableView>

#include <QTabWidget>
#include <QTextEdit>

#include <QHexEdit/qhexedit.h>
#include <QHexWindow.h>
#include <QSplitter>
#include <QAction>
#include <QStandardItemModel>
#include <QModelIndex>

#include "SQLite3DB.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    CSQLite3DB* GetCurSQLite3DB()
    {
        return m_pCurSQLite3DB;
    }

private:
    Ui::MainWindow *ui;

private Q_SLOTS:
    void open();
    void OnTreeViewClick(const QModelIndex& index);

private:

private:
    // Menu and Tool
    QAction* m_pOpenAction;


    // QTreeView at left
    QTreeView* m_pTreeView;
    QStandardItemModel* m_pTreeViewModel;

    // QTabWidget with two sub widget
    QTabWidget* m_pTabWidget;

    QTableView* m_pTableView;
    QHexWindow* m_pHexWindow;
    QWidget*    m_pDatabase;
    QWidget*    m_pSQL;
    QWidget*    m_pData;
    QWidget*    m_pDesign;
    QTextEdit*  m_pDDL;

    // QSplitter
    QSplitter* m_pSplitter;

    // sqlite3tools
    QMap<QString, CSQLite3DB*> m_mapSqlite3DBs;
    CSQLite3DB* m_pCurSQLite3DB;
};

#endif // MAINWINDOW_H
