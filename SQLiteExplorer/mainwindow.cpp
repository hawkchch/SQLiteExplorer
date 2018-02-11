#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHBoxLayout>
#include <QColor>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>
#include <QMimeData>

#include "qsqlitetableview.h"
#include "qsqlitequerywindow.h"

#include <QDebug>
#include <QProcess>
#include <qevent.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Init Application icon
    setWindowIcon(QIcon(":/ui/app.png"));

    // Init Action
    m_pOpenAction = new QAction(QIcon(":/ui/0.png"), tr("&Open..."), this);
    m_pOpenAction->setShortcuts(QKeySequence::Open);
    m_pOpenAction->setStatusTip(tr("Open Database"));
    connect(m_pOpenAction, &QAction::triggered, this, &MainWindow::onOpenActionTriggered);

    m_pCloseAction = new QAction(QIcon(":/ui/11.png"), tr("&Close..."), this);
    m_pCloseAction->setShortcuts(QKeySequence::Close);
    m_pCloseAction->setStatusTip(tr("Close Database"));
    connect(m_pCloseAction, &QAction::triggered, this, &MainWindow::onCloseActionTriggered);

    m_pCheckAction = new QAction(QIcon(":/ui/2.png"), tr("Check..."), this);
    m_pCheckAction->setStatusTip(tr("Check Database Integrity"));
    connect(m_pCheckAction, &QAction::triggered, this, &MainWindow::onCheckActionTriggered);

    m_pVacuumAction = new QAction(QIcon(":/ui/6.png"), tr("&Vacuum..."), this);
    m_pVacuumAction->setStatusTip(tr("Vacuum Database"));
    connect(m_pVacuumAction, &QAction::triggered, this, &MainWindow::onVacuumActionTriggered);

    // Init File Menu And Tool
    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_pOpenAction);
    file->addAction(m_pCloseAction);

    ui->mainToolBar->addAction(m_pOpenAction);
    ui->mainToolBar->addAction(m_pCloseAction);

    // Init Database Menu And Tool
    QMenu *tool = menuBar()->addMenu(tr("&Database"));
    tool->addAction(m_pCheckAction);
    tool->addAction(m_pVacuumAction);

    QToolBar *toolBar = addToolBar(tr("&Database"));
    toolBar->addAction(m_pCheckAction);
    toolBar->addAction(m_pVacuumAction);


    // Init QTreeView
    m_pTreeView = new QTreeView(this);
    m_pTreeViewModel = new QStandardItemModel(m_pTreeView);
    m_pTreeView->setModel(m_pTreeViewModel);
    m_pTreeView->setHeaderHidden(true); // 隐藏表头
    m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置不可编辑

    m_pTreeView->setAlternatingRowColors(true);
    m_pTreeView->setStyleSheet("QTableView{background-color: rgb(250, 250, 115);"
        "alternate-background-color: rgb(141, 163, 215);}");

    connect(m_pTreeView,SIGNAL(clicked(const QModelIndex)),this, SLOT(OnTreeViewClick(const QModelIndex)));

    // Init Database Window
    m_pDatabase = new QTableWidget(this);

    // Init Data Window
    m_pData = new QSQLiteTableView(this);
    connect(this, SIGNAL(signalSQLiteQuery(QString)), m_pData, SLOT(onSQLiteQueryReceived(QString)));

    // Init SQL Window
    m_pSQL = new QSQLiteQueryWindow(this);

    // Init Design Window
    m_pDesign = new QTableWidget(this);

    // Init Hex Window
    m_pHexWindow = new QHexWindow(this);

    // Init DDL Window
    m_pDDL = new QTextEdit(this);
    m_pDDL->setReadOnly(true);

    // Init Graph Window
    m_pGraph = new QLabel(this);
    QScrollArea* sc = new QScrollArea(this);
    sc->setWidgetResizable(true);
    sc->setWidget(m_pGraph);

    // Init QTabWidget
    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pDatabase, "Database");
    m_pTabWidget->addTab(m_pData, "Data");
    m_pTabWidget->addTab(m_pSQL, "SQL");
    m_pTabWidget->addTab(m_pDesign, "Design");
    m_pTabWidget->addTab(m_pHexWindow, "HexWindow");
    m_pTabWidget->addTab(m_pDDL, "DDL");
    m_pTabWidget->addTab(sc, "Graph");

    // Init Splitter
    m_pSplitter = new QSplitter(Qt::Horizontal);
    m_pSplitter->addWidget(m_pTreeView);
    m_pSplitter->addWidget(m_pTabWidget);
    m_pSplitter->setStretchFactor(0, 2);
    m_pSplitter->setStretchFactor(1, 6);

    // Init CentralWidget
    ui->centralWidget->layout()->addWidget(m_pSplitter);

    // onOpenActionTriggered();

    setAcceptDrops(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    //如果类型是jpg或者png才能接受拖动。
    //这里的compare字符串比较函数，相等的时候返回0，所以要取反
    /*
    if(!event->mimeData()->urls()[0].fileName().right(3).compare("jpg") ||!event->mimeData()->urls()[0].fileName().right(3).compare("png"))
        event->acceptProposedAction();
    else
        event->ignore();//否则不接受鼠标事件
    */

    e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if(urls.isEmpty())
        return ;

    foreach (QUrl u, urls) {
        openDatabaseFile(u.toLocalFile());
    }
}

void MainWindow::onOpenActionTriggered()
{
    QFileDialog::Options options;
    //options |= QFileDialog::DontUseNativeDialog;
    QString selectedFilter;
    //Sqlite Files(*.db *.sqlite)
    QString path = QFileDialog::getOpenFileName(this, tr("Open Sqlite Database file"), "", tr("*.*"), &selectedFilter, options);
    if(path.length() > 0)
    {
        openDatabaseFile(path);
    }
}

void MainWindow::onCloseActionTriggered()
{
    if(m_mapSqlite3DBs.size() == 0)
    {
        return;
    }

    QString path = m_mapSqlite3DBs.key(m_pCurSQLite3DB);
    if (path.size())
    {
        delete m_pCurSQLite3DB;
        m_mapSqlite3DBs.remove(path);

        int rowCount = m_pTreeViewModel->rowCount();
        for(int i=0; i<rowCount; i++)
        {
            QModelIndex idx = m_pTreeViewModel->index(i, 0);
            QString idxPath = idx.data(Qt::UserRole+1).toString();
            if(idxPath == path)
            {
                m_pTreeViewModel->removeRow(i);
                break;
            }
        }
    }
}

void MainWindow::onCheckActionTriggered()
{
    QString path = m_mapSqlite3DBs.key(m_pCurSQLite3DB);
    if (path.size())
    {
        table_content tb;
        cell_content header;
        QString errmsg = QString::fromStdString(m_pCurSQLite3DB->ExecuteCmd("PRAGMA integrity_check;", tb, header));
        if(!errmsg.isEmpty())
        {
            QMessageBox::information(this, tr("SQLiteExplorer"), errmsg);
        }
        else if(tb.size())
        {
            cell_content& cc = tb.front();
            if(cc.size())
            {
                QString res = QString::fromStdString(cc[0]);
                res = "Integrity check result: " + res;
                QMessageBox::information(this, ("SQLiteExplorer"),res);
            }
        }
    }
}

void MainWindow::onVacuumActionTriggered()
{
    QString path = m_mapSqlite3DBs.key(m_pCurSQLite3DB);
    if (path.size())
    {
        table_content tb;
        cell_content header;
        QString errmsg = QString::fromStdString(m_pCurSQLite3DB->ExecuteCmd("VACUUM;", tb, header));
        if(errmsg.isEmpty())
        {
            QMessageBox::information(this, tr("SQLiteExplorer"), tr("Vacuum completed OK"));
        }
        else
        {
            QMessageBox::information(this, tr("SQLiteExplorer"), errmsg);
        }
    }
}

bool MainWindow::openDatabaseFile(const QString &path)
{
    CSQLite3DB *pSqlite = new CSQLite3DB(path.toStdString());
    m_mapSqlite3DBs[path] = pSqlite;
    m_pCurSQLite3DB = pSqlite;

    QFileInfo fi(path);

    QStandardItem* root = new QStandardItem(QIcon(":/tableview/ui/db.png"), fi.baseName());
    root->setData(path, Qt::UserRole+1);

    m_pTreeViewModel->appendRow(root);


    table_content tb;
    cell_content hdr;
    QString errmsg = QString::fromStdString(pSqlite->ExecuteCmd("select type, name, tbl_name from sqlite_master order by tbl_name, name", tb, hdr));

    if (errmsg.size() > 0)
    {
        QMessageBox::information(this, tr("SQLiteExplorer"), errmsg);
        return false;
    }

    QMap<QString, QStringList> mapIndex;
    QMap<QString, QStringList> mapTrigger;
    QStringList listView;

    while(!tb.empty())
    {
        cell_content cell = tb.front();
        tb.pop_front();

        const string& type = cell[0];
        QString name = QString::fromStdString(cell[1]);
        QString tbl_name = QString::fromStdString(cell[2]);
        if (type == "table")
        {
            mapIndex[tbl_name];
            mapTrigger[tbl_name];
        }
        else if (type == "index")
        {
            mapIndex[tbl_name].push_back(name);
        }
        else if (type == "trigger")
        {
            mapTrigger[tbl_name].push_back(name);
        }
        else if (type == "view")
        {
            listView.push_back(name);
        }
    }


    for(auto it=mapIndex.begin(); it!=mapIndex.end(); ++it)
    {
        QString tblname = it.key();
        const QStringList& idxList = it.value();
        const QStringList& triggerList = mapTrigger[tblname];
        QStandardItem* item = new QStandardItem(QIcon(":/tableview/ui/table.png"), tblname);

        foreach(QString s, idxList)
        {
            item->appendRow(new QStandardItem(QIcon(":/tableview/ui/index.jpg"), s));
        }

        foreach(QString s, triggerList)
        {
            item->appendRow(new QStandardItem(QIcon(":/tableview/ui/trigger.png"), s));
        }

        root->appendRow(item);
    }

    m_pTreeView->expand(root->index());
    return true;
}

void MainWindow::OnTreeViewClick(const QModelIndex& index)
{
    QString tableName = index.data().toString();
    const QModelIndex& parent = index.parent();
    QString path = parent.data(Qt::UserRole+1).toString();
    auto it = m_mapSqlite3DBs.find(path);
    if (it != m_mapSqlite3DBs.end())
    {
        CSQLite3DB* pSqlite = it.value();
        m_pCurSQLite3DB = pSqlite;

        QString sqls;
        QString sql;

        // Init Database Window
        m_pDatabase->clear();
        QStringList header;
        header << "Name" << "Value";
        m_pDatabase->setColumnCount(header.size());

        m_pDatabase->setHorizontalHeaderLabels(header);
        map<string, string> vals = m_pCurSQLite3DB->GetDatabaseInfo();
        m_pDatabase->setRowCount(vals.size());
        size_t i=0;
        for(auto it=vals.begin(); it!=vals.end(); ++it, ++i)
        {
            QTableWidgetItem *name=new QTableWidgetItem();//创建一个Item
            name->setText(QString::fromStdString(it->first));//设置内容
            m_pDatabase->setItem(i,0,name);//把这个Item加到第一行第二列中

            name=new QTableWidgetItem();//创建一个Item
            name->setText(QString::fromStdString(it->second));//设置内容
            m_pDatabase->setItem(i,1,name);//把这个Item加到第一行第二列中
        }

        // Init Design Window
        m_pDesign->clear();
        sql = QString("PRAGMA table_info(%1);").arg(tableName);

        table_content tb;
        cell_content cc;

        string err = m_pCurSQLite3DB->ExecuteCmd(sql.toStdString(), tb, cc);

        m_pDesign->setColumnCount(cc.size());
        header.clear();

        for(auto it=cc.begin(); it!=cc.end(); ++it)
        {
            header.push_back(QString::fromStdString(*it));
        }

        m_pDesign->setHorizontalHeaderLabels(header);
        m_pDesign->setRowCount(tb.size());
        for(size_t i=0; i<tb.size(); ++i)
        {
            const cell_content& cell = tb[i];
            for(size_t j=0; j<cell.size(); ++j)
            {
                QTableWidgetItem *name=new QTableWidgetItem();//创建一个Item
                name->setText(QString::fromStdString(cell[j]));//设置内容
                m_pDesign->setItem(i,j,name);//把这个Item加到第一行第二列中
            }
        }
        tb.clear();
        cc.clear();


        // Init Hex Window
        vector<int> pageids;
        pageids = pSqlite->GetAllLeafPageIds(tableName.toStdString());
        m_pHexWindow->SetPageNos(pageids);
        m_pHexWindow->SetTableName(tableName);

        sql = QString("SELECT * FROM SQLITE_MASTER WHERE tbl_name='%1'").arg(tableName);

        pSqlite->ExecuteCmd(sql.toStdString(), tb, cc);
        while(!tb.empty())
        {
            cell_content cc = tb.front();
            tb.pop_front();
            sqls += QString::fromStdString(cc[4]);
            sqls += "\n\n\n";
        }

        m_pDDL->setText(sqls);


        QString getAllData = "SELECT * FROM " + tableName;
        emit signalSQLiteQuery(getAllData);

        vector<PageUsageInfo> infos = m_pCurSQLite3DB->GetPageUsageInfos("");
        QString content = "digraph g { node [shape = record,style=\"filled\"];rankdir=LR;";

        for(auto it=infos.begin(); it!=infos.end(); ++it)
        {
            PageUsageInfo& info = *it;
            QString color;
            QString scell;
            switch(info.type)
            {
            case PAGE_TYPE_OVERFLOW:
                color = "#FEE3BA";
                break;
            case PAGE_TYPE_INDEX_INTERIOR:
            case PAGE_TYPE_TABLE_INTERIOR:
                color = "#E1C4C4";
                break;
            case PAGE_TYPE_INDEX_LEAF:
            case PAGE_TYPE_TABLE_LEAF:
                color = "#62C544";
                break;
            default:
                break;
            }

            if(info.ncell>0)
            {
                int ncell = info.ncell;
                if(info.type == PAGE_TYPE_INDEX_INTERIOR || info.type == PAGE_TYPE_TABLE_INTERIOR)
                    ncell += 1;

                scell = QString(" ncell %1").arg(ncell);
            }

            if(info.type == PAGE_TYPE_OVERFLOW)
            {
                content.push_back(QString("%1[color=\"%2\", label=\"%1 overflow %3 from cell %4\"];")
                                  .arg(info.pgno)
                                  .arg(color)
                                  .arg(info.overflow_page_idx)
                                  .arg(info.overflow_cell_idx));
            }
            else
            {
                content.push_back(QString("%1[color=\"%2\", label=\"%1 %3\"];").arg(info.pgno).arg(color).arg(scell));
            }

        }

        for(auto it=infos.begin(); it!=infos.end(); ++it)
        {
            PageUsageInfo& info = *it;
            if(info.parent != 0)
            {
                content.push_back(QString("%1 -> %2;").arg(info.parent).arg(info.pgno));
                /*
                qDebug() << "type:" << info.type
                         << " parent:" << info.parent
                         << " pgno:" << info.pgno
                         << " desc:" << info.desc.c_str();
                */
            }
        }

        content.push_back("}");

        //qDebug() << content;
        QFile f("tmp.dot");
        if(!f.open(QIODevice::ReadWrite | QIODevice::Text|QIODevice::Truncate)) {
            qDebug() << "Can't open the file!";
        }

        f.write(content.toStdString().c_str());
        f.close();

        QString program = "dot";
        QStringList arguments;
        arguments << "-Tjpg" << "tmp.dot" << "-o" << "tmp.jpg";
        QProcess *myProcess = new QProcess(this);
        connect(myProcess, SIGNAL(finished(int)), this, SLOT(onProcessFinished(int)));
        myProcess->start(program, arguments);
    }
}

void MainWindow::onProcessFinished(int ret)
{
    QString path = QString("tmp.jpg");
    QPixmap px;
    px.load(path);
    m_pGraph->setPixmap(px);

}
