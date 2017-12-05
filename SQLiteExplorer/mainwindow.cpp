#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHBoxLayout>
#include <QColor>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>
#include "qsqlitetableview.h"
#include "qsqlitequerywindow.h"

#include <QDebug>
#include <QProcess>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Init MenuBar
    m_pOpenAction = new QAction(QIcon(":/images/doc-open"), tr("&Open..."), this);
    m_pOpenAction->setShortcuts(QKeySequence::Open);
    m_pOpenAction->setStatusTip(tr("Open an existing file"));
    connect(m_pOpenAction, &QAction::triggered, this, &MainWindow::open);

    QMenu *file = menuBar()->addMenu(tr("&File"));
    file->addAction(m_pOpenAction);

    //QToolBar *toolBar = addToolBar(tr("&File"));
    //toolBar->addAction(m_pOpenAction);

    // Init QTreeView
    m_pTreeView = new QTreeView(this);
    m_pTreeViewModel = new QStandardItemModel(m_pTreeView);
    m_pTreeView->setModel(m_pTreeViewModel);
    m_pTreeView->setHeaderHidden(true); // 隐藏表头
    m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置不可编辑
    connect(m_pTreeView,SIGNAL(clicked(const QModelIndex)),this, SLOT(OnTreeViewClick(const QModelIndex)));

    // Init Data window (Tab 0)
    m_pData = new QSQLiteTableView(this);
    connect(this, SIGNAL(signalSQLiteQuery(QString)), m_pData, SLOT(onSQLiteQueryReceived(QString)));

    // Init SQL Window (Tab 1)
    m_pSQL = new QSQLiteQueryWindow(this);

    // Init QHexEditWindow
    m_pHexWindow = new QHexWindow(this);
    // Init QTextEdit DLL
    m_pDDL = new QTextEdit(this);
    m_pDDL->setReadOnly(true);

    m_pGraph = new QLabel(this);
    QScrollArea* sc = new QScrollArea(this);
    sc->setWidgetResizable(true);
    sc->setWidget(m_pGraph);



    // Init QTabWidget
    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pData, "Data");
    m_pTabWidget->addTab(m_pSQL, "SQL");
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

    open();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::open()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open Sqlite Database file"), ".", tr("Sqlite Files(*.db *.sqlite)"));
    if(path.length() > 0)
    {
        CSQLite3DB *pSqlite = new CSQLite3DB(path.toStdString());
        m_mapSqlite3DBs[path] = pSqlite;
        m_pCurSQLite3DB = pSqlite;

        QFileInfo fi(path);
        QStandardItem* root = new QStandardItem(fi.baseName());
        root->setData(path, Qt::UserRole+1);

        m_pTreeViewModel->appendRow(root);

        vector<string> vs = pSqlite->GetAllTableNames();
        for(auto it=vs.begin(); it!=vs.end(); ++it)
        {
            QStandardItem* item = new QStandardItem(QString::fromStdString(*it));
            root->appendRow(item);
        }

        m_pTreeView->expandAll();
    }
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
        vector<int> pageids;
        pageids = pSqlite->GetAllLeafPageIds(tableName.toStdString());
        m_pHexWindow->SetPageNos(pageids);
        m_pHexWindow->SetTableName(tableName);

        QString sqls;
        QString sql = QString("SELECT * FROM SQLITE_MASTER WHERE tbl_name='%1'").arg(tableName);

        table_content tb;
        cell_content cc;
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
            if(info.type == PAGE_TYPE_OVERFLOW)
            {
                content.push_back(QString("%1[color=\"yellow\", label=\"%1 overflow %2 from cell %3\"];")
                                  .arg(info.pgno)
                                  .arg(info.overflow_page_idx)
                                  .arg(info.overflow_cell_idx));
            }
            else if (info.parent == 0)
            {
                content.push_back(QString("%1[color=\"red\"];").arg(info.pgno));
            }
            else
            {
                content.push_back(QString("%1[color=\"green\"];").arg(info.pgno));
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
