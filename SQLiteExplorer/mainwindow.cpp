#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHBoxLayout>
#include <QColor>
#include <QFileDialog>
#include <QMessageBox>
#include <QModelIndex>
#include <QMimeData>

#include "qsqlitetableview.h"
#include "SQLWindow.h"

#include <QDebug>
#include <QProcess>
#include <qevent.h>
#include "DialogAbout.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_pCurSQLite3DB(NULL)
{
    ui->setupUi(this);

    // Init Application icon
    setWindowIcon(QIcon(":/ui/app.png"));
    setWindowTitle("SQLiteExplorer");

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

    m_pAboutAction = new QAction(QIcon(":/toolicon/ui/info.png"), tr("&About..."), this);
    m_pAboutAction->setStatusTip(tr("About"));
    connect(m_pAboutAction, &QAction::triggered, this, &MainWindow::onAboutActionTriggered);

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

    QMenu *help = menuBar()->addMenu(tr("Help"));
    help->addAction(m_pAboutAction);

    QToolBar *toolBar = addToolBar(tr("&Database"));
    toolBar->addAction(m_pCheckAction);
    toolBar->addAction(m_pVacuumAction);


    // Init QTreeView
    m_pTreeView = new QSQLiteMasterTreeView(this);
    m_pTreeViewModel = new QStandardItemModel(m_pTreeView);
    m_pTreeView->setModel(m_pTreeViewModel);
    m_pTreeView->setHeaderHidden(true); // 隐藏表头
    m_pTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置不可编辑

    m_pTreeView->setAlternatingRowColors(true);
    m_pTreeView->setStyleSheet("QTableView{background-color: rgb(250, 250, 115);"
        "alternate-background-color: rgb(141, 163, 215);}");

    connect(m_pTreeView,SIGNAL(signalCurrentChanged(QModelIndex)),this, SLOT(OnTreeViewClick(const QModelIndex)));

    // Init Database Window
    m_pDatabase = new QTableWidget(this);

    // Init Data Window
    m_pData = new DataWindow(this);
    connect(this, SIGNAL(signalSQLiteQuery(QString)), m_pData, SLOT(onSQLiteQueryReceived(QString)));

    // Init SQL Window
    m_pSQL = new QSQLiteQueryWindow(this);

    // Init Design Window
    m_pDesign = new QTableWidget(this);

    // Init Hex Window
    m_pHexWindow = new HexWindow(this);

    // Init DDL Window
    m_pDDL = new QTextEdit(this);
    m_pDDL->setReadOnly(true);

    // Init Graph Window
    m_pGraph = new GraphWindow(this);

    // Init QTabWidget
    m_pTabWidget = new QTabWidget(this);
    m_pTabWidget->addTab(m_pDatabase, "Database");
    m_pTabWidget->addTab(m_pData, "Data");
    m_pTabWidget->addTab(m_pSQL, "SQL");
    m_pTabWidget->addTab(m_pDesign, "Design");
    m_pTabWidget->addTab(m_pHexWindow, "HexWindow");
    m_pTabWidget->addTab(m_pDDL, "DDL");
    m_pTabWidget->addTab(m_pGraph, "Graph");

    m_pTabWidget->setCurrentIndex(1);

    // Init Splitter
    m_pSplitter = new QSplitter(Qt::Horizontal);
    m_pSplitter->addWidget(m_pTreeView);
    m_pSplitter->addWidget(m_pTabWidget);
    m_pSplitter->setStretchFactor(0, 2);
    m_pSplitter->setStretchFactor(1, 6);

    // Init CentralWidget
    ui->centralWidget->layout()->addWidget(m_pSplitter);

    //onOpenActionTriggered();

    setAcceptDrops(true);
}


MainWindow::~MainWindow()
{
    for(QMap<QString, CSQLite3DB*>::iterator it=m_mapSqlite3DBs.begin(); it!=m_mapSqlite3DBs.end(); it++)
    {
        if(*it)
            delete(*it);
    }

    m_mapSqlite3DBs.clear();

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
    //QString path = "D:\\git-project\\SQLiteExplorer\\MM.sqlite";
    if(path.length() > 0)
    {
        openDatabaseFile(path);
    }
}

void MainWindow::onCloseActionTriggered()
{
    QString path = m_mapSqlite3DBs.key(m_pCurSQLite3DB);
    if (path.size() && m_pCurSQLite3DB)
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

    if(m_mapSqlite3DBs.size() == 0)
    {
        m_pCurSQLite3DB = NULL;
        m_pDatabase->clear();
        m_pData->clear();
        m_pDesign->clear();
        m_pDesign->setRowCount(0);
        m_pHexWindow->clear();
        m_pDDL->clear();
        m_pGraph->clear();
    }
}

void MainWindow::onCheckActionTriggered()
{
    QString path = m_mapSqlite3DBs.key(m_pCurSQLite3DB);
    if (path.size())
    {
        try
        {
            CppSQLite3Table table = m_pCurSQLite3DB->getTable("PRAGMA integrity_check;");
            if(table.numRows() == 1)
            {
                table.setRow(0);
                QString res = QString::fromStdString(table.getStringField(0));
                res = "Integrity check result: " + res;
                QMessageBox::information(this, ("SQLiteExplorer"),res);
            }
        }
        catch(CppSQLite3Exception& e)
        {
            QMessageBox::information(this, tr("SQLiteExplorer"), QString::fromStdString(e.errorMessage()));
        }
    }
}

void MainWindow::onVacuumActionTriggered()
{
    QString path = m_mapSqlite3DBs.key(m_pCurSQLite3DB);
    if (path.size())
    {
        try
        {
            m_pCurSQLite3DB->execDML("VACUUM;");
            QMessageBox::information(this, tr("SQLiteExplorer"), tr("Vacuum completed OK"));
        }
        catch(CppSQLite3Exception& e)
        {
            QMessageBox::information(this, tr("SQLiteExplorer"), QString::fromStdString(e.errorMessage()));
        }
    }
}

void MainWindow::onAboutActionTriggered()
{
    DialogAbout dlg(this);//加this后子窗口会在父窗口上面
    dlg.exec();//显示对话框，程序阻塞
}

#define PathRole    Qt::UserRole+1
#define LevelRole   Qt::UserRole+2
#define TypeRole    Qt::UserRole+3

bool MainWindow::openDatabaseFile(const QString &path)
{
    CSQLite3DB *pSqlite = new CSQLite3DB(path.toStdString());
    m_mapSqlite3DBs[path] = pSqlite;
    m_pCurSQLite3DB = pSqlite;

    QFileInfo fi(path);

    // 顶层
    QStandardItem* root = new QStandardItem(QIcon(":/tableview/ui/db.png"), fi.baseName());
    root->setData(path, PathRole);      // Path
    root->setData(1, LevelRole);        // 1,2,3
    root->setData("db", TypeRole);      // db, table, index, trigger, view, freelist
    m_pTreeViewModel->appendRow(root);

    table_content tb;
    cell_content hdr;
    QString errmsg = QString::fromStdString(pSqlite->ExecuteCmd("select type, name, tbl_name from sqlite_master order by tbl_name, name", tb, hdr));
    cell_content sqlite_master;
    sqlite_master.push_back("table");
    sqlite_master.push_back("sqlite_master");
    sqlite_master.push_back("sqlite_master");
    tb.push_back(sqlite_master);

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
        item->setData(2, LevelRole);
        item->setData("table", TypeRole);

        foreach(QString s, idxList)
        {
            QStandardItem* indexItem = new QStandardItem(QIcon(":/tableview/ui/index.jpg"), s);
            indexItem->setData(3, LevelRole);
            indexItem->setData("index", TypeRole);
            item->appendRow(indexItem);
        }

        foreach(QString s, triggerList)
        {
            QStandardItem* triggerItem = new QStandardItem(QIcon(":/tableview/ui/trigger.png"), s);
            triggerItem->setData(3, LevelRole);
            triggerItem->setData("trigger", TypeRole);
            item->appendRow(triggerItem);
        }

        root->appendRow(item);
    }

    foreach(QString s, listView)
    {
        QStandardItem* viewItem = new QStandardItem(QIcon(":/tableview/ui/view.png"), s);
        viewItem->setData(2, LevelRole);
        viewItem->setData("view", TypeRole);
        root->appendRow(viewItem);
    }

    // 设置自由页
    QStandardItem* freeListItem = new QStandardItem(QIcon(":/tableview/ui/freelist.png"), "freelist");
    freeListItem->setData(2, LevelRole);
    freeListItem->setData("freelist", TypeRole);
    root->appendRow(freeListItem);

    m_pTreeView->expand(root->index());
    return true;
}

void MainWindow::OnTreeViewClick(const QModelIndex& index)
{
    int level = index.data(LevelRole).toInt();
    QString path, name, tableName, type;

    type = index.data(TypeRole).toString();
    if(level == 2)
    {
        path = index.parent().data(PathRole).toString();
        name = index.data().toString();
        tableName = name;
    }
    else if (level == 3)
    {
        path = index.parent().parent().data(PathRole).toString();
        name = index.data().toString();
        tableName = index.parent().data().toString();
    }
    else
    {
        return;
    }

    auto it = m_mapSqlite3DBs.find(path);
    //qDebug() << "OnTreeViewClick tableName =" << tableName << ", PathName =" << path;
    if (it != m_mapSqlite3DBs.end())
    {
        m_pCurSQLite3DB = it.value();

        QString sqls;
        QString sql;
        table_content tb;
        cell_content cc;


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

        if(type != "freelist")
        {
            sql = QString("PRAGMA table_info(%1);").arg(tableName);
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
        }

        // Init Hex Window
        m_pHexWindow->SetTableName(name, tableName, type);
        vector<pair<int, PageType>> pages = m_pCurSQLite3DB->GetAllPageIdsAndType(name.toStdString());
        vector<PageUsageInfo> infos = m_pCurSQLite3DB->GetPageUsageInfos(type == "freelist");

        if(type != "freelist")
        {
            m_pHexWindow->SetPageNosAndType(pages);

            if(type == "table")
                sql = QString("SELECT * FROM SQLITE_MASTER WHERE tbl_name='%1'").arg(tableName);
            else
                sql = QString("SELECT * FROM SQLITE_MASTER WHERE name='%1'").arg(name);
            // Init DDL Window
            tb.clear();
            cc.clear();
            m_pCurSQLite3DB->ExecuteCmd(sql.toStdString(), tb, cc);
            while(!tb.empty())
            {
                cell_content cc = tb.front();
                tb.pop_front();
                if(cc[4].size() > 0)
                    sqls += QString::fromStdString(cc[4]);
                else
                    sqls += QString::fromStdString("--" + cc[1]);

                sqls += "\n\n\n";
            }
            m_pDDL->setText(sqls);
        }
        else
        {
            vector<pair<int, PageType>> pages;
            foreach (PageUsageInfo info, infos)
            {
                pages.push_back(make_pair(info.pgno, info.type));
            }

            std::sort(pages.begin(), pages.end());
            m_pHexWindow->SetPageNosAndType(pages);
        }

        // Init Data Window
        if(type != "freelist")
        {
            QString getAllData = "SELECT * FROM " + tableName;
            emit signalSQLiteQuery(getAllData);
        }

        // Init Graph Window
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

            // ncell为页面cell数量
            if(info.ncell>0)
            {
                int ncell = info.ncell;
                // 内部页有一个RightChild，所以ncell+1
                if(info.type == PAGE_TYPE_INDEX_INTERIOR || info.type == PAGE_TYPE_TABLE_INTERIOR)
                    ncell += 1;
                scell = QString("%1").arg(ncell);
            }

            if(info.type == PAGE_TYPE_OVERFLOW)
            {
                // PageNo是从1开始计数
                // 此处cell_idx从0开始计数
                content.push_back(QString("%1[color=\"%2\", label=\"%1 overflow %3 from cell %4\"];")
                                  .arg(info.pgno)
                                  .arg(color)
                                  .arg(info.overflow_page_idx)
                                  .arg(info.overflow_cell_idx));
            }
            else
            {
                content.push_back(QString("%1[color=\"%2\", label=\"%1 ncells:%3\"];").arg(info.pgno).arg(color).arg(scell));
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
        // dot: 渲染的图具有明确的方向性
        // neato:渲染的图缺乏方向性
        // twopi:渲染的图采用放射性布局
        // circo:渲染的图采用环型布局
        // fdp:渲染的图缺乏方向性
        // sfdp:渲染大型的图，图片缺乏方向性
        // patchwork: 主要用于树哈希图（tree map）
        QString program = "graphviz2.38/dot";
        QStringList arguments;
        arguments << "-Tplain" << "tmp.dot" << "-o" << "tmp.plain";
        QProcess *myProcess = new QProcess(this);
        connect(myProcess, SIGNAL(finished(int)), this, SLOT(onProcessFinished(int)));
        myProcess->start(program, arguments);


        arguments.clear();
        arguments << "-Tpng" << "tmp.dot" << "-o" << "tmp.png";
        QProcess *myProcess2 = new QProcess(this);
        connect(myProcess2, SIGNAL(finished(int)), this, SLOT(onProcessFinished(int)));
        myProcess2->start(program, arguments);

//        plain ,
//        plain-ext
//        The plain and plain-ext formats produce output using a simple, line-based language. The latter format differs in that, on edges,
//                it provides port names on head and tail nodes when applicable.
//        There are four types of statements.

//         graph scale width height
//         node name x y width height label style shape color fillcolor
//         edge tail head n x1 y1 .. xn yn [label xl yl] style color
//         stop
//        graph
//        The width and height values give the width and height of the drawing. The lower left corner of the drawing is at the origin.
//                The scale value indicates how the drawing should be scaled if a size attribute was given and the drawing needs to be
//                scaled to conform to that size. If no scaling is necessary, it will be set to 1.0. Note that all graph, node and edge
//                coordinates and lengths are given unscaled.
//        node
//        The name value is the name of the node, and x and y give the node's position. The width and height are the width and height
//                of the node. The label, style, shape, color and fillcolor give the node's label, style, shape, color and fillcolor,
//                respectively, using attribute default values where necessary. If the node does not have a style attribute, "solid" is used.
//        edge
//        The tail and head values give the names of the head and tail nodes. In plain-ext format, the head or tail name will be appended
//                with a colon and a portname if the edge connects to the node at a port. n is the number of control points defining the
//                B-spline forming the edge. This is followed by 2*n numbers giving the x and y coordinates of the control points in order
//                from tail to head. If the edge has a label, this comes next followed by the x and y coordinates of the label's position.
//                The edge description is completed by the edge's style and color. As with nodes, if a style is not defined, "solid" is used.
//        Note: The control points given in an edge statement define the body of the edge. In particular, if the edge has an arrowhead to
//              the head or tail node, there will be a gap between the last or first control points and the boundary of the associated node.
//              There are at least 3 possible ways of handling this gap:

//        Arrange that the input graph uses dir=none, arrowhead=none, or arrowtail=none for all edges. In this case, the terminating control
//              points will always touch the node.
//        Consider the line segment joining the control point and the center of the node, and determine the point where the segment intersects
//              the node's boundary. Then use the control point and the intersection point as the main axis of an arrowhead. The problem with
//              this approach is that, if the edge has a port, the edge will not be pointing to the center of the node. In this case, rather
//              than use the control point and center point, one can use the control point and its tangent.
//        Arrange that the input graph uses headclip=false or tailclip=false. In this case, the edge will terminate at the node's center rather
//              than its boundary. If arrowheads are used, there will still be a gap, but normally this will occur within the node. The application
//              will still need to clip the spline to the node boundary. Also, as with the previous item, if the edge points to a node port, this
//              technique will fail.
//        The output consists of one graph line, a sequence of node lines, one per node, a sequence of edge lines, one per edge, and a final stop
//              line. All units are in inches, represented by a floating point number.
//        Note that the plain formats provide minimal information, really giving not much more than node positions and sizes, and edge spline control
//              points. These formats are usually most useful to applications wanting just this geometric information, and willing to fill in all of
//              the graphical details. The only real advantages to these formats is their terseness and their ease of parsing. In general, the dot
//              and xdot are preferable in terms of the quantity of information provided.
    }
}

void MainWindow::onProcessFinished(int ret)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if(process)
    {
        QStringList arg = process->arguments();
        m_pGraph->SetPath(arg[arg.size()-1]);

        delete process;
    }
}
