#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHBoxLayout>
#include <QColor>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>

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

    // Init QTableView (Tab 0)
    m_pTableView = new QTableView(this);
    // Init QHexEditWindow (Tab 1)
    m_pHexWindow = new QHexWindow(this);
    // Init QTextEdit DLL
    m_pDDL = new QTextEdit(this);

    // Init QTabWidget
    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pTableView, "Table");
    m_pTabWidget->addTab(m_pHexWindow, "HexWindow");
    m_pTabWidget->addTab(m_pDDL, "DDL");

    m_pTabWidget->setTabEnabled(1, true);

    // Init Splitter
    m_pSplitter = new QSplitter(Qt::Horizontal);
    m_pSplitter->addWidget(m_pTreeView);
    m_pSplitter->addWidget(m_pTabWidget);
    m_pSplitter->setStretchFactor(1, 1);

    // Init CentralWidget
    //ui->centralWidget->setLayout(new QHBoxLayout());
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
    //QString path = "MM.sqlite";
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
        vector<int> pageids;
        pageids = pSqlite->GetAllLeafPageIds(tableName.toStdString());
        m_pHexWindow->SetPageIds(pageids);
    }
}
