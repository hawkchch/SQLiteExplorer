#include "qhexwindow.h"
#include "ui_qhexwindow.h"

#include "mainwindow.h"
#include <QDebug>
#include <QTimer>

#include <set>
using std::set;

QHexWindow::QHexWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QHexWindow)
{
    ui->setupUi(this);

    // Init Splitter
    m_pSplitter = new QSplitter(Qt::Vertical);
    m_pSplitter->setStretchFactor(1, 1);

    // Init splitter two sub widget
    m_pHexEdit = new QHexEdit(this);
    m_pHexEdit->setReadOnly(true);
    connect(m_pHexEdit, SIGNAL(currentAddressChanged(qint64)), this, SLOT(onCurrentAddressChanged(qint64)));
    m_pTableWdiget = new QTableWidget(this);
    m_pSplitter->addWidget(m_pHexEdit);
    m_pSplitter->addWidget(m_pTableWdiget);
    m_pTableWdiget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pTableWdiget->setSelectionMode(QAbstractItemView::SingleSelection);

    setStyleSheet("QTableWidget::item:selected { background-color: rgb(143, 221, 119) }");

    // Init splitter two sub widget stretch factor
    m_pSplitter->setStretchFactor(0, 5);
    m_pSplitter->setStretchFactor(1, 1);

    // Init layout
    ui->widget_2->setLayout(new QHBoxLayout);
    ui->widget_2->layout()->setMargin(0);
    ui->widget_2->layout()->addWidget(m_pSplitter);

    MainWindow* pMainWindow = qobject_cast<MainWindow*>(parentWidget());
    if (pMainWindow)
    {
        m_pParent = pMainWindow;
    }

    connect(ui->comboBoxPageType, SIGNAL(currentIndexChanged(QString)), this, SLOT(onPageTypeChanged(QString)));
    connect(ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onComboxChanged(QString)));

    connect(ui->pushButtonPrev, SIGNAL(clicked(bool)), this, SLOT(onPrevBtnClicked()));
    connect(ui->pushButtonNext, SIGNAL(clicked(bool)), this, SLOT(onNextBtnClicked()));
    connect(ui->pushButtonFirst, SIGNAL(clicked(bool)), this, SLOT(onFirstBtnClicked()));
    connect(ui->pushButtonLast, SIGNAL(clicked(bool)), this, SLOT(onLastBtnClicked()));

    m_pageTypeName[PAGE_TYPE_UNKNOWN] = "Unknown";
    m_pageTypeName[PAGE_TYPE_INDEX_INTERIOR] = "IndexInterior";
    m_pageTypeName[PAGE_TYPE_TABLE_INTERIOR] = "TableInteror";
    m_pageTypeName[PAGE_TYPE_INDEX_LEAF] = "IndexLeaf";
    m_pageTypeName[PAGE_TYPE_TABLE_LEAF] = "TableLeaf";
    m_pageTypeName[PAGE_TYPE_OVERFLOW] = "Overflow";
    m_pageTypeName[PAGE_TYPE_FREELIST_TRUNK] = "FreelistTrunk";
    m_pageTypeName[PAGE_TYPE_FREELIST_LEAF] = "FreelistLeaf";
    m_pageTypeName[PAGE_TYPE_PTR_MAP] = "PtrMap";
}

QHexWindow::~QHexWindow()
{
    delete ui;
}

void QHexWindow::SetPageNos(const vector<int> &pgnos)
{
    if (m_pParent)
    {
        m_pCurSQLite3DB = m_pParent->GetCurSQLite3DB();
    }
    else
    {
        return;
    }

    ui->comboBox->clear();
    QStringList ids;
    for(auto it=pgnos.begin(); it!=pgnos.end(); ++it)
    {
        ids.push_back(QString("%1").arg(*it));
    }
    // 清空TableWidget中内容
    m_pTableWdiget->clear();
    m_pTableWdiget->setColumnCount(0);

    ui->comboBox->addItems(ids);
    ui->comboBox->setCurrentIndex(0);
}

void QHexWindow::SetPageNosAndType(const vector<pair<int, PageType> > &pgs)
{
    if (m_pParent)
    {
        m_pCurSQLite3DB = m_pParent->GetCurSQLite3DB();
    }
    else
    {
        return;
    }

    ui->comboBox->clear();
    ui->comboBoxPageType->clear();
    m_pageNoAndTypes.clear();

    set<PageType> pageTypes;
    for(auto it=pgs.begin(); it!=pgs.end(); ++it)
    {
        m_pageNoAndTypes.push_back(QString("%1/%2").arg(it->first).arg(m_pageTypeName[it->second]));
        pageTypes.insert(it->second);
    }
    // 清空TableWidget中内容
    m_pTableWdiget->clear();
    m_pTableWdiget->setColumnCount(0);

    ui->comboBox->addItems(m_pageNoAndTypes);
    ui->comboBox->setCurrentIndex(0);

    QStringList pts;
    pts.push_back("AllPageType");
    for(auto it=pageTypes.begin(); it!=pageTypes.end(); it++)
    {
        pts.push_back(m_pageTypeName[*it]);
    }
    ui->comboBoxPageType->addItems(pts);
}

void QHexWindow::SetTableName(const QString &name, const QString &tableName, const QString &type)
{
    if (m_pParent)
    {
        m_pCurSQLite3DB = m_pParent->GetCurSQLite3DB();
    }
    else
    {
        return;
    }
    m_tableHeaders.clear();
    m_curTableName = tableName;
    m_curName = name;
    vector<string> headers;
    if(type == "table")
    {
        m_pCurSQLite3DB->GetColumnNames(tableName.toStdString(), headers);

        QString tableNames;
        for(int i=0; i<headers.size(); i++)
        {
            tableNames += QString::fromStdString(headers[i]) + ",";
        }
        qDebug() << "Table Headers =" << tableNames;
    }
    else if (type == "index")
    {
        m_pCurSQLite3DB->GetIndexNames(name.toStdString(), tableName.toStdString(), headers);
        QString idxNames;
        for(int i=0; i<headers.size(); i++)
        {
            idxNames += QString::fromStdString(headers[i]) + ",";
        }
        qDebug() << "Index Headers =" << idxNames;
    }

    for(auto it=headers.begin(); it!=headers.end(); ++it)
    {
        m_tableHeaders.push_back(QString::fromStdString(*it));
    }
}

void QHexWindow::onPageIdSelect(int pgno, PageType type)
{
    bool decode = (
        type == PAGE_TYPE_INDEX_INTERIOR ||type == PAGE_TYPE_TABLE_INTERIOR ||
        type == PAGE_TYPE_INDEX_LEAF || type == PAGE_TYPE_TABLE_LEAF);
    string raw = m_pCurSQLite3DB->LoadPage(pgno, decode);

    QByteArray ba = QByteArray::fromStdString(raw);
    QHexDocument* document = m_pHexEdit->document();

    if(document == NULL)
    {
        document = QHexDocument::fromMemory(ba);
        m_pHexEdit->setDocument(document);
    }
    else
    {
        document->replace(0, ba);
    }

    document->clearHighlighting();
    document->clearComments();
    document->clearMetadata();

    document->setBaseAddress((pgno-1)*m_pCurSQLite3DB->GetPageSize());

    // Bulk metadata management (paints only one time)
    document->beginMetadata();
    if(decode)
    {
        ContentArea& pageHeaderArea = m_pCurSQLite3DB->m_pSqlite3Page->m_pageHeaderArea;
        ContentArea& cellidxArea = m_pCurSQLite3DB->m_pSqlite3Page->m_cellIndexArea;
        vector<ContentArea> payloadArea = m_pCurSQLite3DB->m_pSqlite3Page->m_payloadArea;  // payload区域
        m_payloadArea = payloadArea;
        ContentArea& unusedArea = m_pCurSQLite3DB->m_pSqlite3Page->m_unusedArea;            // 未使用区域

        document->highlightBackRange(pageHeaderArea.m_startAddr, pageHeaderArea.m_len, QColor(0x6A, 0x88, 0x82));
        document->highlightBackRange(cellidxArea.m_startAddr, cellidxArea.m_len, QColor(0xE9, 0xFD, 0xF2));
        document->highlightBackRange(unusedArea.m_startAddr, unusedArea.m_len, QColor(0xFE, 0xE3, 0xBA));

        QColor p[3];
        p[0].setRgb(0xC9, 0xFB, 0xB9);
        p[1].setRgb(0x8F, 0xDD, 0x77);
        p[2].setRgb(0x62, 0xC5, 0x44);

        sort(payloadArea.begin(), payloadArea.end(), [](const ContentArea& l, const ContentArea& r){
            return l.m_startAddr < r.m_startAddr;
        });

        for(size_t i=0; i<payloadArea.size(); ++i)
        {
            ContentArea& ca = payloadArea[i];
            document->highlightBackRange(ca.m_startAddr, ca.m_len, p[i%3]);
        }
    }
    else
    {
        document->highlightBackRange(0, m_pCurSQLite3DB->GetPageSize(), QColor(Qt::white));
    }
    document->endMetadata();
    vector<string> pkFiledName;
    vector<string> pkType;
    vector<int> pkIdx;
    bool withoutRowid = false;
    if(decode)
    {
        m_pCurSQLite3DB->GetTablePrimaryKey(m_curTableName.toStdString(), pkFiledName, pkType, pkIdx, withoutRowid);
    }

    // 将该页中所有数据填充到m_pTableWdiget中
    if(type == PAGE_TYPE_TABLE_INTERIOR)
    {
        m_pTableWdiget->setColumnCount(2);
        QString pkName = "Rowid";
        if(pkFiledName.size() == 1) pkName = QString::fromStdString(pkFiledName[0]);

        QStringList tableHeaders;
        tableHeaders << "LeftChildPageNo" << pkName;
        m_pTableWdiget->setHorizontalHeaderLabels(tableHeaders);
        m_pTableWdiget->setRowCount(m_payloadArea.size());

        for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
        {
            int idx = it - m_payloadArea.begin();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);

            int leftChild = m_pCurSQLite3DB->m_pSqlite3Payload->GetLeftChild();
            i64 rowid = m_pCurSQLite3DB->m_pSqlite3Payload->GetRowid();

            QTableWidgetItem *name=new QTableWidgetItem();
            name->setText(QString("%1").arg(leftChild));
            m_pTableWdiget->setItem(idx,0,name);

            name=new QTableWidgetItem();
            name->setText(QString("%1").arg(rowid));
            m_pTableWdiget->setItem(idx,1,name);
        }
    }
    else if(type == PAGE_TYPE_TABLE_LEAF)
    {
        m_pTableWdiget->setColumnCount(m_tableHeaders.size());
        m_pTableWdiget->setHorizontalHeaderLabels(m_tableHeaders);
        m_pTableWdiget->setRowCount(m_payloadArea.size());

        for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
        {
            int idx = it - m_payloadArea.begin();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);

            //qDebug() << "m_pCurSQLite3DB->DecodeCell(pgno, idx, vars) [" << pgno << "," << idx << "," << vars.size() << "]";

            i64 rowid = m_pCurSQLite3DB->m_pSqlite3Payload->GetRowid();
            for(size_t i=0; i<vars.size(); i++)
            {
                SQLite3Variant& var = vars[i];
                QTableWidgetItem *name=new QTableWidgetItem();//创建一个Item
                QString val;
                switch (var.type) {
                case SQLITE_TYPE_INTEGER:
                    val = QString("%1").arg(var.iVal);
                    break;
                case SQLITE_TYPE_FLOAT:
                    val = QString("%1").arg(var.lfVal);
                    break;
                case SQLITE_TYPE_TEXT:
                    val = QString::fromStdString(var.text);
                    break;
                case SQLITE_TYPE_NULL:
                    val = "(null)";
                    break;
                case SQLITE_TYPE_BLOB:
                    val = QString::fromStdString(var.text);
                    break;
                default:
                    break;
                }

                if(pkIdx.size() == 1 && pkIdx[0] == i && var.type == SQLITE_TYPE_NULL && StrUpper(pkType[0]) == "INTEGER")
                {
                    val = QString("%1").arg(rowid);
                }

                name->setText(val);//设置内容
                m_pTableWdiget->setItem(idx,i,name);//把这个Item加到第一行第二列中
            }
        }
    }
    else if(type == PAGE_TYPE_INDEX_INTERIOR)
    {
        QStringList headers = m_tableHeaders;
        QString pkName = "Rowid";
        if(pkFiledName.size() == 1) pkName = QString::fromStdString(pkFiledName[0]);
        // 当主键和索引列名称一样时，取pkName为Rowid
        if(headers.size() == 1)
        {
            if(headers[0] == pkName) pkName = "Rowid";
        }
        else if(headers.empty()) pkName = "Rowid";

        // 如果建表时，使用了WITHOUT ROWID，对应表类型会是Index Interior/Index Leaf
        // 这时就不需要把主键名称放到后面
        if(withoutRowid && m_curTableName == m_curName) pkName = "";

        bool setHeaders = false;
        if(headers.size() > 0)
        {
            headers.push_front("LeftChildPageNo");
            if(pkName.size() > 0) headers.push_back(pkName);
            m_pTableWdiget->setColumnCount(headers.size());
            m_pTableWdiget->setHorizontalHeaderLabels(headers);
            setHeaders = true;

            qDebug() << headers.size();
        }

        m_pTableWdiget->setRowCount(m_payloadArea.size());

        for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
        {
            int idx = it - m_payloadArea.begin();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);

            //qDebug() << "m_pCurSQLite3DB->DecodeCell(pgno, idx, vars) [" << pgno << "," << idx << "," << vars.size() << "]";

            if(!setHeaders)
            {
                headers.push_back("LeftChildPageNo");
                for(size_t i=0; i<vars.size()-1; i++)
                {
                    headers.push_back(QString("%1").arg(i));
                }
                if(pkName.size() > 0) headers.push_back(pkName);
                m_pTableWdiget->setColumnCount(headers.size());
                m_pTableWdiget->setHorizontalHeaderLabels(headers);
                setHeaders = true;
            }

            int leftChild = m_pCurSQLite3DB->m_pSqlite3Payload->GetLeftChild();
            QTableWidgetItem *leftChildItem = new QTableWidgetItem();
            leftChildItem->setText(QString("%1").arg(leftChild));//设置内容
            m_pTableWdiget->setItem(idx,0,leftChildItem);

            for(size_t i=0; i<vars.size(); i++)
            {
                SQLite3Variant& var = vars[i];
                QTableWidgetItem *name=new QTableWidgetItem();//创建一个Item
                QString val;
                switch (var.type) {
                case SQLITE_TYPE_INTEGER:
                    val = QString("%1").arg(var.iVal);
                    break;
                case SQLITE_TYPE_FLOAT:
                    val = QString("%1").arg(var.lfVal);
                    break;
                case SQLITE_TYPE_TEXT:
                    val = QString::fromStdString(var.text);
                    break;
                case SQLITE_TYPE_NULL:
                    val = "(null)";
                    break;
                case SQLITE_TYPE_BLOB:
                    val = QString::fromStdString(var.text);
                    break;
                default:
                    break;
                }

                name->setText(val);//设置内容
                m_pTableWdiget->setItem(idx,i+1,name);//把这个Item加到第一行第二列中
            }
        }
    }
    else if(type == PAGE_TYPE_INDEX_LEAF)
    {
        QStringList headers = m_tableHeaders;

        // 对应主键名称
        QString pkName = "Rowid";
        if(pkFiledName.size() == 1) pkName = QString::fromStdString(pkFiledName[0]);
        // 当主键和索引列名称一样时，取pkName为Rowid
        if(headers.size() == 1)
        {
            if(headers[0] == pkName) pkName = "Rowid";
        }
        else if(headers.empty()) pkName = "Rowid";

        // 如果建表时，使用了WITHOUT ROWID，对应表类型会是Index Interior/Index Leaf
        // 这时就不需要把主键名称放到后面
        if(withoutRowid && m_curTableName == m_curName) pkName = "";

        bool setHeaders = false;
        if(headers.size() > 0)
        {
            if(pkName.size() > 0) headers.push_back(pkName);
            m_pTableWdiget->setColumnCount(headers.size());
            m_pTableWdiget->setHorizontalHeaderLabels(headers);
            setHeaders = true;
        }

        m_pTableWdiget->setRowCount(m_payloadArea.size());

        for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
        {
            int idx = it - m_payloadArea.begin();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);

            if(!setHeaders)
            {
                for(size_t i=0; i<vars.size()-1; i++)
                {
                    headers.push_back(QString("%1").arg(i));
                }
                if(pkName.size() > 0) headers.push_back(pkName);
                m_pTableWdiget->setColumnCount(headers.size());
                m_pTableWdiget->setHorizontalHeaderLabels(headers);
                setHeaders = true;
            }

            //qDebug() << "m_pCurSQLite3DB->DecodeCell(pgno, idx, vars) [" << pgno << "," << idx << "," << vars.size() << "]";
            for(size_t i=0; i<vars.size(); i++)
            {
                SQLite3Variant& var = vars[i];
                QTableWidgetItem *name=new QTableWidgetItem();//创建一个Item
                QString val;
                switch (var.type) {
                case SQLITE_TYPE_INTEGER:
                    val = QString("%1").arg(var.iVal);
                    break;
                case SQLITE_TYPE_FLOAT:
                    val = QString("%1").arg(var.lfVal);
                    break;
                case SQLITE_TYPE_TEXT:
                    val = QString::fromStdString(var.text);
                    break;
                case SQLITE_TYPE_NULL:
                    val = "(null)";
                    break;
                case SQLITE_TYPE_BLOB:
                    val = QString::fromStdString(var.text);
                    break;
                default:
                    break;
                }

                name->setText(val);//设置内容
                m_pTableWdiget->setItem(idx,i,name);//把这个Item加到第一行第二列中
            }
        }
    }
    else if(type == PAGE_TYPE_FREELIST_TRUNK)
    {
        ContentArea sNextTrunkPageNo;
        int nNextTrunkPageNo;
        ContentArea sLeafPageCounts;
        int nLeafPageCounts;
        vector<ContentArea> sLeafPageNos;
        vector<int> nLeafPageNos;
        ContentArea sUnused;
        m_pCurSQLite3DB->DecodeFreeListTrunkPage(pgno, sNextTrunkPageNo, nNextTrunkPageNo,
                                                 sLeafPageCounts,nLeafPageCounts,sLeafPageNos,nLeafPageNos,sUnused);

        document->beginMetadata();
        document->highlightBackRange(sNextTrunkPageNo.m_startAddr, sNextTrunkPageNo.m_len, QColor(0x6A, 0x88, 0x82));
        document->highlightBackRange(sLeafPageCounts.m_startAddr, sLeafPageCounts.m_len, QColor(0xE9, 0xFD, 0xF2));
        document->highlightBackRange(sUnused.m_startAddr, sUnused.m_len, QColor(0xFE, 0xE3, 0xBA));

        QColor p[3];
        p[0].setRgb(0xC9, 0xFB, 0xB9);
        p[1].setRgb(0x8F, 0xDD, 0x77);
        p[2].setRgb(0x62, 0xC5, 0x44);

        for(size_t i=0; i<sLeafPageNos.size(); ++i)
        {
            ContentArea& ca = sLeafPageNos[i];
            document->highlightBackRange(ca.m_startAddr, ca.m_len, p[i%3]);
        }
        document->endMetadata();

        m_pTableWdiget->setRowCount(nLeafPageNos.size());
        m_pTableWdiget->setColumnCount(1);

        QStringList headers;
        headers << "FreelistLeafPageNo";
        m_pTableWdiget->setHorizontalHeaderLabels(headers);

        for(size_t i=0; i<nLeafPageNos.size(); ++i)
        {
            QTableWidgetItem *name=new QTableWidgetItem();
            name->setText(QString("%1").arg(nLeafPageNos[i]));
            m_pTableWdiget->setItem(i,0,name);
        }

        m_payloadArea = sLeafPageNos;
    }
    else
    {
        m_pTableWdiget->setColumnCount(0);
        m_pTableWdiget->setRowCount(0);
    }

    setPushBtnStats();
}

void QHexWindow::onPageTypeChanged(const QString &pageType)
{
    if(pageType == "AllPageType")
    {
        ui->comboBox->clear();
        ui->comboBox->addItems(m_pageNoAndTypes);
    }
    else
    {
        QStringList list;
        foreach (QString pageNoAndType, m_pageNoAndTypes)
        {
            if(pageNoAndType.contains(pageType))
            {
                list.push_back(pageNoAndType);
            }
        }

        ui->comboBox->clear();
        ui->comboBox->addItems(list);
    }
}

void QHexWindow::onComboxChanged(const QString &item)
{
    QStringList list = item.split('/');
    if(list.size() > 0)
    {
        int pgno = list[0].toInt();
        if(pgno > 0)
        {
            onPageIdSelect(pgno, m_pageTypeName.key(list[1]));
        }
    }
}

void QHexWindow::onPrevBtnClicked()
{
    int curIdx = ui->comboBox->currentIndex();
    if(curIdx > 0)
    {
        curIdx -= 1;
        ui->comboBox->setCurrentIndex(curIdx);
    }
}

void QHexWindow::onNextBtnClicked()
{
    int curIdx = ui->comboBox->currentIndex();
    int count = ui->comboBox->count();
    if(curIdx+1 < count)
    {
        curIdx += 1;
        ui->comboBox->setCurrentIndex(curIdx);
    }
}

void QHexWindow::onFirstBtnClicked()
{
    int count = ui->comboBox->count();
    if(count > 0)
    {
        ui->comboBox->setCurrentIndex(0);
    }
}

void QHexWindow::onLastBtnClicked()
{
    int count = ui->comboBox->count();
    if(count > 0)
    {
        ui->comboBox->setCurrentIndex(count - 1);
    }
}

void QHexWindow::onCurrentAddressChanged(qint64 address)
{
    for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
    {
        i64 start = it->m_startAddr;
        i64 end = start + it->m_len;
        if(address >= start && address <= end)
        {
            int row = it - m_payloadArea.begin();
            m_pTableWdiget->selectRow(row);
            m_pTableWdiget->showRow(row);
            break;
        }
    }
}

void QHexWindow::setPushBtnStats()
{
    int curIdx = ui->comboBox->currentIndex();
    int count = ui->comboBox->count();

    if(curIdx == 0)
    {
        ui->pushButtonFirst->setEnabled(false);
        ui->pushButtonPrev->setEnabled(false);
        ui->pushButtonNext->setEnabled(true);
        ui->pushButtonLast->setEnabled(true);
    }
    else if(curIdx+1 == count)
    {
        ui->pushButtonFirst->setEnabled(true);
        ui->pushButtonPrev->setEnabled(true);
        ui->pushButtonNext->setEnabled(false);
        ui->pushButtonLast->setEnabled(false);
    }
    else
    {
        ui->pushButtonFirst->setEnabled(true);
        ui->pushButtonPrev->setEnabled(true);
        ui->pushButtonNext->setEnabled(true);
        ui->pushButtonLast->setEnabled(true);
    }
}
