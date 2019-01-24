#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QTableView>

#include <QTabWidget>
#include <QTextEdit>

#include <QHexEdit/qhexedit.h>
#include <QSplitter>
#include <QAction>
#include <QStandardItemModel>
#include <QModelIndex>
#include <QLabel>
#include <QColumnView>
#include <QTimeLine>
#include <QGraphicsEllipseItem>
#include <QGraphicsItemAnimation>
#include <QGraphicsScene>
#include <QGraphicsView>
#include "pixitem.h"
#include <QPixmap>
#include <QTimerEvent>

#include "QHexWindow.h"
#include "QSQLiteTablesTreeView.h"
#include "SQLite3DB.h"

namespace Ui {
class MainWindow;
}

class QSQLiteTableView;
class QSQLiteQueryWindow;

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

protected:
    void dragEnterEvent(QDragEnterEvent* e);
    void dropEvent(QDropEvent* e);

private:
    Ui::MainWindow *ui;

signals:
    void signalSQLiteQuery(const QString& sql);

private Q_SLOTS:
    void OnTreeViewClick(const QModelIndex& index);
    void onProcessFinished(int ret);

    void onOpenActionTriggered();
    void onCloseActionTriggered();
    void onCheckActionTriggered();
    void onVacuumActionTriggered();

private:
    bool openDatabaseFile(const QString& path);

private:
    // Menu and Tool
    QAction* m_pOpenAction;
    QAction* m_pCloseAction;
    QAction* m_pCheckAction;
    QAction* m_pVacuumAction;


    // QTreeView at left
    QSQLiteTablesTreeView* m_pTreeView;
    QStandardItemModel* m_pTreeViewModel;

    // QTabWidget with sub widget
    QTabWidget* m_pTabWidget;

    QHexWindow*         m_pHexWindow;
    QTableWidget*       m_pDatabase;
    QSQLiteQueryWindow* m_pSQL;
    QSQLiteTableView*   m_pData;
    QTableWidget*       m_pDesign;
    QTextEdit*          m_pDDL;
    /////////////////////////////////////////////////////////
    /// \brief m_graph
    QGraphicsScene* m_graphicsScene;
    PixItem*        m_graphicsItem;
    QGraphicsView*  m_graphicsView;
    /////////////////////////////////////////////////////////

    // QSplitter
    QSplitter* m_pSplitter;

    // sqlite3tools
    QMap<QString, CSQLite3DB*> m_mapSqlite3DBs;
    CSQLite3DB* m_pCurSQLite3DB;
};

#endif // MAINWINDOW_H
