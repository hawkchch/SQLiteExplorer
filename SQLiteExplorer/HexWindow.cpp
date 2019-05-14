#include "HexWindow.h"
#include "ui_hexwindow.h"

#include "mainwindow.h"
#include <QDebug>
#include <QTimer>

#include <set>
using std::set;

HexWindow::HexWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HexWindow)
{
    ui->setupUi(this);

    m_curPageNo = 0;

    // Init Splitter
    m_pHSplitter = new QSplitter(Qt::Horizontal);
    //m_pHSplitter->setStretchFactor(1, 1);

    m_pSplitter = new QSplitter(Qt::Vertical);
    //m_pSplitter->setStretchFactor(1, 1);

    // Init splitter two sub widget
    m_pHexEdit = new QHexEdit(this);
    m_pHexEdit->setReadOnly(true);
    connect(m_pHexEdit, SIGNAL(currentAddressChanged(qint64)), this, SLOT(onCurrentAddressChanged(qint64)));


    m_pPageView = new QTreeView(this);
    m_pPageViewModel = new QStandardItemModel(m_pPageView);
    m_pPageView->setModel(m_pPageViewModel);
    //m_pPageView->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置不可编辑
    m_pPageView->setAlternatingRowColors(true);
    m_pPageView->setStyleSheet("QTableView{background-color: rgb(250, 250, 115);"
        "alternate-background-color: rgb(141, 163, 215);}");


    m_pHSplitter->addWidget(m_pHexEdit);
    m_pHSplitter->addWidget(m_pPageView);


    m_pTableWdiget = new QTableWidget(this);
    m_pSplitter->addWidget(m_pHSplitter);
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

    connect(ui->checkBox, SIGNAL(clicked(bool)), this, SLOT(onCheckBoxStatChanged(bool)));

    m_pageTypeName[PAGE_TYPE_UNKNOWN] = "Unknown";
    m_pageTypeName[PAGE_TYPE_INDEX_INTERIOR] = "IndexInterior";
    m_pageTypeName[PAGE_TYPE_TABLE_INTERIOR] = "TableInterior";
    m_pageTypeName[PAGE_TYPE_INDEX_LEAF] = "IndexLeaf";
    m_pageTypeName[PAGE_TYPE_TABLE_LEAF] = "TableLeaf";
    m_pageTypeName[PAGE_TYPE_OVERFLOW] = "Overflow";
    m_pageTypeName[PAGE_TYPE_FREELIST_TRUNK] = "FreelistTrunk";
    m_pageTypeName[PAGE_TYPE_FREELIST_LEAF] = "FreelistLeaf";
    m_pageTypeName[PAGE_TYPE_PTR_MAP] = "PtrMap";
}

HexWindow::~HexWindow()
{
    delete ui;
}

void HexWindow::clear()
{
    ui->comboBox->clear();
    ui->comboBoxPageType->clear();
    m_pageNoAndTypes.clear();

    m_pTableWdiget->clear();
    m_pTableWdiget->setColumnCount(0);
    m_pPageViewModel->clear();
}

void HexWindow::SetPageNosAndType(const vector<pair<int, PageType> > &pgs)
{
    if (m_pParent)
    {
        m_pCurSQLite3DB = m_pParent->GetCurSQLite3DB();
    }
    if(m_pParent == NULL || pgs.empty() || m_pCurSQLite3DB == NULL)
    {
        clear();
        return;
    }

    ui->comboBox->blockSignals(true);
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
    ui->comboBox->blockSignals(false);
    ui->comboBoxPageType->addItems(pts);
}

void HexWindow::SetTableName(const QString &name, const QString &tableName, const QString &type)
{
    if (m_pParent)
    {
        m_pCurSQLite3DB = m_pParent->GetCurSQLite3DB();
    }
    else
    {
        return;
    }

    if(m_pCurSQLite3DB == NULL) return;
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

QString upperHex(const string& raw, int start, int len)
{
    return QString(QByteArray::fromStdString(raw.substr(start, len)).toHex().data()).toUpper();
}

void HexWindow::setPageHdrData(PageType type, ContentArea& pageHeaderArea, ContentArea& cellidxArea, ContentArea& unusedArea, int pgno, string raw)
{
    if(m_pCurSQLite3DB == NULL) return;
    int base = 10;
    QStandardItem* pghdr = new QStandardItem("PageHdr");
    m_pPageViewModel->appendRow(pghdr);
    int pghdrOffset = (pgno==1?100:0);
    int row = 0;
    int col = 1;
    m_pPageViewModel->setItem(pghdr->row(), col++, new QStandardItem(QString("PageHdr")));
    m_pPageViewModel->setItem(pghdr->row(), col++, new QStandardItem(QString::number(pageHeaderArea.m_startAddr, base)));
    m_pPageViewModel->setItem(pghdr->row(), col++, new QStandardItem(QString::number(pageHeaderArea.m_len, base)));
    m_pPageViewModel->setItem(pghdr->row(), col++, new QStandardItem(upperHex(raw, pageHeaderArea.m_startAddr, pageHeaderArea.m_len)));

    col = 0;
    pghdr->setChild(row, col++, GetItem(pghdrOffset, 1, "Type"));
    pghdr->setChild(row, col++, new QStandardItem(m_pageTypeName[type]));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(pghdrOffset, base)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(1, base)));
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset, 1)));

    row++;
    col = 0;
    QStandardItem* firstFreeBlockAddr = GetItem(pghdrOffset+1, 2, "FirstFreeBlockAddr");
    pghdr->setChild(row, col++, firstFreeBlockAddr);
    int firstaddr = decode_number((unsigned char*)raw.c_str(), pghdrOffset+1, 2);
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+1, 2)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(pghdrOffset+1, base)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(2, base)));
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+1, 2)));

    int next = decode_number((unsigned char*)raw.c_str(), pghdrOffset+1, 2);
    int r = 0;
    while(next != 0)
    {
        if(next >= m_pCurSQLite3DB->GetPageSize())
        {
            qDebug() << "Error Occured";
            break;
        }
        int nextAddr = decode_number((unsigned char*)raw.c_str(), next, 2);
        int len = decode_number((unsigned char*)raw.c_str(), next+2, 2);
        string hex = raw.substr(next, len);

        col = 0;
        firstFreeBlockAddr->setChild(r, col++, GetItem(next, len, QString("FreeBlock[%1]").arg(r)));
        firstFreeBlockAddr->setChild(r, col++, new QStandardItem(QString::fromStdString(hex)));
        firstFreeBlockAddr->setChild(r, col++, new QStandardItem(QString::number(next, base)));
        firstFreeBlockAddr->setChild(r, col++, new QStandardItem(QString::number(len, base)));
        firstFreeBlockAddr->setChild(r, col++, new QStandardItem(upperHex(hex, 0, hex.size())));
        r++;
        next = nextAddr;
    }


    row++;
    col = 0;
    pghdr->setChild(row, col++, GetItem(pghdrOffset+3, 2, "CellCounts"));
    int ncell = decode_number((unsigned char*)raw.c_str(), pghdrOffset+3, 2);
    pghdr->setChild(row, col++, new QStandardItem(QString::number(ncell, base)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(pghdrOffset+3, base)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(2, base)));
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+3, 2)));

    row++;
    col = 0;
    pghdr->setChild(row, col++, GetItem(pghdrOffset+5, 2, "StartOfCellContentAddr"));
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+5, 2)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(pghdrOffset+5, base)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(2, base)));
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+5, 2)));

    row++;
    col = 0;
    pghdr->setChild(row, col++, GetItem(pghdrOffset+7, 1, "FragmentBytes"));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(m_pCurSQLite3DB->m_pSqlite3Page->m_fragmentBytes)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(pghdrOffset+7, base)));
    pghdr->setChild(row, col++, new QStandardItem(QString::number(1, base)));
    pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+7, 1)));

    if(type == PAGE_TYPE_INDEX_INTERIOR ||type == PAGE_TYPE_TABLE_INTERIOR )
    {
        row++;
        col = 0;
        pghdr->setChild(row, col++, GetItem(pghdrOffset+8,4,"RightChild"));
        pghdr->setChild(row, col++, new QStandardItem(QString::number(m_pCurSQLite3DB->m_pSqlite3Page->m_rightChildPageNumber)));
        pghdr->setChild(row, col++, new QStandardItem(QString::number(pghdrOffset+8, base)));
        pghdr->setChild(row, col++, new QStandardItem(QString::number(4, base)));
        pghdr->setChild(row, col++, new QStandardItem(upperHex(raw, pghdrOffset+8, 4)));
    }

    QStandardItem* cellPtr = new QStandardItem("CellPtr");
    m_pPageViewModel->appendRow(cellPtr);

    col = 1;
    m_pPageViewModel->setItem(cellPtr->row(), col++, new QStandardItem(QString("CellPtr")));
    m_pPageViewModel->setItem(cellPtr->row(), col++, new QStandardItem(QString::number(cellidxArea.m_startAddr, base)));
    m_pPageViewModel->setItem(cellPtr->row(), col++, new QStandardItem(QString::number(cellidxArea.m_len, base)));
    m_pPageViewModel->setItem(cellPtr->row(), col++, new QStandardItem(upperHex(raw, cellidxArea.m_startAddr, cellidxArea.m_len)));

    for(int i=0; i<cellidxArea.m_len/2; i++)
    {
        col = 0;
        int start = cellidxArea.m_startAddr + i*2;
        int len = 2;

        cellPtr->setChild(i, col++, GetItem(start, len, QString("CellPtr[%1]").arg(i)));
        cellPtr->setChild(i, col++, new QStandardItem(upperHex(raw, start, len)));
        cellPtr->setChild(i, col++, new QStandardItem(QString::number(start, base)));
        cellPtr->setChild(i, col++, new QStandardItem(QString::number(len, base)));
        cellPtr->setChild(i, col++, new QStandardItem(upperHex(raw, start, len)));
    }

    QStandardItem* unused = GetItem(unusedArea.m_startAddr,unusedArea.m_len,"UnusedArea");
    m_pPageViewModel->appendRow(unused);
    col = 1;
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(QString("UnusedArea")));
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(QString::number(unusedArea.m_startAddr, base)));
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(QString::number(unusedArea.m_len, base)));
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(upperHex(raw, unusedArea.m_startAddr, unusedArea.m_len)));
}

void HexWindow::setFreeListPageHdrData(ContentArea &sNextTrunkPageNo, int &nNextTrunkPageNo,
                                       ContentArea &sLeafPageCounts, int &nLeafPageCounts,
                                       vector<ContentArea> &sLeafPageNos, vector<int> &nLeafPageNos,
                                       ContentArea &sUnused, string raw)
{
    int base = 10;

    // name, val, start, len, hex
    QStandardItem* next = GetItem(sNextTrunkPageNo.m_startAddr,sNextTrunkPageNo.m_len,"NextFreeListTrunkPage");
    m_pPageViewModel->appendRow(next);
    int col = 1;
    m_pPageViewModel->setItem(next->row(), col++, new QStandardItem(QString::number(nNextTrunkPageNo, base)));
    m_pPageViewModel->setItem(next->row(), col++, new QStandardItem(QString::number(sNextTrunkPageNo.m_startAddr, base)));
    m_pPageViewModel->setItem(next->row(), col++, new QStandardItem(QString::number(sNextTrunkPageNo.m_len, base)));
    m_pPageViewModel->setItem(next->row(), col++, new QStandardItem(upperHex(raw, sNextTrunkPageNo.m_startAddr, sNextTrunkPageNo.m_len)));


    QStandardItem* counts = GetItem(sLeafPageCounts.m_startAddr,sLeafPageCounts.m_len,"CellCounts");
    m_pPageViewModel->appendRow(counts);
    col = 1;
    m_pPageViewModel->setItem(counts->row(), col++, new QStandardItem(QString::number(nLeafPageCounts, base)));
    m_pPageViewModel->setItem(counts->row(), col++, new QStandardItem(QString::number(sLeafPageCounts.m_startAddr, base)));
    m_pPageViewModel->setItem(counts->row(), col++, new QStandardItem(QString::number(sLeafPageCounts.m_len, base)));
    m_pPageViewModel->setItem(counts->row(), col++, new QStandardItem(upperHex(raw, sLeafPageCounts.m_startAddr, sLeafPageCounts.m_len)));

    QStandardItem* cells = new QStandardItem("Cells");
    m_pPageViewModel->appendRow(cells);
    for(int row=0; row<sLeafPageNos.size(); row++)
    {
        const ContentArea& area = sLeafPageNos[row];
        col = 0;
        cells->setChild(row, col++, GetItem(area.m_startAddr,area.m_len,QString("Cell[%1]").arg(row)));
        cells->setChild(row, col++, new QStandardItem(QString::number(nLeafPageNos[row], base)));
        cells->setChild(row, col++, new QStandardItem(QString::number(area.m_startAddr, base)));
        cells->setChild(row, col++, new QStandardItem(QString::number(area.m_len, base)));
        cells->setChild(row, col++, new QStandardItem(upperHex(raw, area.m_startAddr, area.m_len)));
    }

    QStandardItem* unused = GetItem(sUnused.m_startAddr, sUnused.m_len, "UnusedArea");
    m_pPageViewModel->appendRow(unused);
    col = 1;
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(QString("UnusedArea")));
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(QString::number(sUnused.m_startAddr, base)));
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(QString::number(sUnused.m_len, base)));
    m_pPageViewModel->setItem(unused->row(), col++, new QStandardItem(upperHex(raw, sUnused.m_startAddr, sUnused.m_len)));
}


QStandardItem* HexWindow::setCellData(QStandardItem* parentItem, CSQLite3Payload &payload, ContentArea area, string raw)
{
    int base = 10;
    QStandardItem* cells = parentItem;
    if(cells == NULL)
    {
        cells = new QStandardItem("Cells");
        m_pPageViewModel->appendRow(cells);
    }

    qDebug() << "1";
    int col = 0;
    int row = cells->rowCount();
    QStandardItem* cell = new QStandardItem(QString("Cell[%1]").arg(row));
    cells->setChild(row, col++, cell);
    QString cellDesc = QString::fromStdString(raw.substr(area.m_startAddr, area.m_len));
    cellDesc = cellDesc.remove("\r").remove("\n");
    cells->setChild(row, col++, new QStandardItem(cellDesc));
    cells->setChild(row, col++, new QStandardItem(QString::number(area.m_startAddr, base)));
    cells->setChild(row, col++, new QStandardItem(QString::number(area.m_len, base)));
    cells->setChild(row, col++, new QStandardItem(upperHex(raw, area.m_startAddr, area.m_len)));

    qDebug() << "2";
    int offset = area.m_startAddr;
    row = col = 0;
    // leftChild
    if(payload.m_leftChildLen > 0)
    {
        cell->setChild(row, col++, GetItem(payload.m_leftChildStartAddr + offset, payload.m_leftChildLen, "LeftChild"));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_leftChild, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_leftChildStartAddr + offset, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_leftChildLen, base)));
        cell->setChild(row, col++, new QStandardItem(upperHex(payload.m_cellContent, payload.m_leftChildStartAddr, payload.m_leftChildLen)));
        row++;
        col = 0;
    }

    qDebug() << "3";
    // payloadSize
    if(payload.m_nPayloadLen > 0)
    {
        cell->setChild(row, col++, GetItem(payload.m_nPayloadStartAddr + offset, payload.m_nPayloadLen, "PayloadSize"));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_nPayload, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_nPayloadStartAddr + offset, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_nPayloadLen, base)));
        cell->setChild(row, col++, new QStandardItem(upperHex(payload.m_cellContent, payload.m_nPayloadStartAddr, payload.m_nPayloadLen)));
        row++;
        col = 0;
    }

    qDebug() << "4";
    // rowid
    if(payload.m_rowidLen > 0)
    {
        cell->setChild(row, col++, GetItem(payload.m_rowidStartAddr + offset, payload.m_rowidLen, "RowID"));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_rowid, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_rowidStartAddr + offset, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_rowidLen, base)));
        cell->setChild(row, col++, new QStandardItem(upperHex(payload.m_cellContent, payload.m_rowidStartAddr, payload.m_rowidLen)));
        row++;
        col = 0;
    }

    qDebug() << "5";
    // cellHeaderSize
    if(payload.m_cellHeaderSizeLen > 0)
    {
        qDebug() << "HeaderSize:" << payload.m_cellHeaderSize
                 << "startAddr:" << payload.m_cellHeaderSizeStartAddr << "offset:" << offset
                 << "len:" << payload.m_cellHeaderSizeLen;
        cell->setChild(row, col++, GetItem(payload.m_cellHeaderSizeStartAddr + offset, payload.m_cellHeaderSizeLen, "CellHeaderSize"));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_cellHeaderSize, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_cellHeaderSizeStartAddr + offset, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(payload.m_cellHeaderSizeLen, base)));
        cell->setChild(row, col++, new QStandardItem(upperHex(payload.m_cellContent, payload.m_cellHeaderSizeStartAddr, payload.m_cellHeaderSizeLen)));
        row++;
        col = 0;
    }

    qDebug() << "6";
    // typeAndLen
    for(int i=0; i<payload.m_datas.size(); i++)
    {
        SQLite3Variant& var = payload.m_datas[i];
        cell->setChild(row, col++, GetItem(var.tStartAddr + offset, var.tLen>payload.m_nLocal?payload.m_nLocal:var.tLen, QString("TypaAndLen[%1]").arg(i)));
        QString strDesc;
        switch(var.tVal)
        {
        case 0:
            strDesc = "NULL";
            break;
        case 1:
        case 2:
        case 3:
        case 4:
            strDesc = QString("Integer: %1-Bytes").arg(var.tVal);
            break;
        case 5:
            strDesc = QString("Integer: 6-Bytes");
            break;
        case 6:
            strDesc = QString("Integer: 8-Bytes");
            break;
        case 7:
            strDesc = QString("Double: 8-Bytes");
            break;
        case 8:
            strDesc = QString("Integer: 0");
            break;
        case 9:
            strDesc = QString("Integer: 1");
            break;
        case 10:
        case 11:
            strDesc = QString("Internal Use");
            break;

        default:
            if(var.tVal %2 == 0)
            {
                strDesc = QString("Blob: %1-Bytes").arg((var.tVal-12)/2);
            }
            else
            {
                strDesc = QString("Text: %1-Bytes").arg((var.tVal-13)/2);
            }
            break;
        }

        cell->setChild(row, col++, new QStandardItem(strDesc));
        cell->setChild(row, col++, new QStandardItem(QString::number(var.tStartAddr + offset, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(var.tLen, base)));
        cell->setChild(row, col++, new QStandardItem(upperHex(payload.m_cellContent, var.tStartAddr, var.tLen)));

        row++;
        col = 0;
    }

    qDebug() << "7";
    // VariableContent
    for(int i=0; i<payload.m_datas.size(); i++)
    {
        SQLite3Variant var = payload.m_datas[i];
        cell->setChild(row, col++, GetItem(var.valStartAddr + offset, var.valLen>payload.m_nLocal?payload.m_nLocal:var.valLen, QString("Variable[%1]").arg(i)));
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

        cell->setChild(row, col++, new QStandardItem(val));
        cell->setChild(row, col++, new QStandardItem(QString::number(var.valLen==0?0:var.valStartAddr+offset, base)));
        cell->setChild(row, col++, new QStandardItem(QString::number(var.valLen, base)));
        cell->setChild(row, col++, new QStandardItem(upperHex(payload.m_cellContent, var.valStartAddr, var.valLen)));

        row++;
        col = 0;
    }
    return cells;
}

QStandardItem *HexWindow::GetItem(int start, i64 len, QString txt)
{
    qDebug() << start << len << txt;
    QStandardItem* item = new QStandardItem(txt);

    for(int i=start; i<start+len; i++)
    {
        m_mapItems.insert(i, item);
    }

    return item;
}

void HexWindow::onPageIdSelect(int pgno, PageType type)
{
    if(m_pCurSQLite3DB == NULL) return;
    bool decode = (
        type == PAGE_TYPE_INDEX_INTERIOR ||type == PAGE_TYPE_TABLE_INTERIOR ||
        type == PAGE_TYPE_INDEX_LEAF || type == PAGE_TYPE_TABLE_LEAF);

    // 初始化PageViewModel
    m_pPageViewModel->clear();
    m_mapItems.clear();
    m_curPageNo = pgno;

    QStringList headers;
    headers << "Name" << "Desc" << "Start" << "Len" << "Hex";
    m_pPageViewModel->setColumnCount(headers.size());
    m_pPageViewModel->setHorizontalHeaderLabels(headers);
    m_pPageView->setColumnWidth(0, 200);

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

    if(ui->checkBox->checkState() == Qt::Checked) document->setBaseAddress((pgno-1)*m_pCurSQLite3DB->GetPageSize());
    else document->setBaseAddress(0);

    // Bulk metadata management (paints only one time)
    m_payloadArea.clear();
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

        setPageHdrData(type, pageHeaderArea, cellidxArea, unusedArea, pgno, raw);
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

    QStandardItem* cellContentParentItem = NULL;
    // 将该页中所有数据填充到m_pTableWdiget中
    if(type == PAGE_TYPE_TABLE_INTERIOR)
    {
        m_pTableWdiget->setColumnCount(2);
        QString pkName = "RowID";
        if(pkFiledName.size() == 1) pkName = QString::fromStdString(pkFiledName[0]);

        QStringList tableHeaders;
        tableHeaders << "LeftChild" << pkName;
        m_pTableWdiget->setHorizontalHeaderLabels(tableHeaders);
        m_pTableWdiget->setRowCount(m_payloadArea.size());

        for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
        {
            int idx = it - m_payloadArea.begin();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);
            cellContentParentItem = setCellData(cellContentParentItem, *m_pCurSQLite3DB->m_pSqlite3Payload, *it, raw);

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
            cellContentParentItem = setCellData(cellContentParentItem, *m_pCurSQLite3DB->m_pSqlite3Payload, *it, raw);

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
        QString pkName = "RowID";
        if(pkFiledName.size() == 1) pkName = QString::fromStdString(pkFiledName[0]);
        // 当主键和索引列名称一样时，取pkName为Rowid
        if(headers.size() == 1)
        {
            if(headers[0] == pkName) pkName = "RowID";
        }
        else if(headers.empty()) pkName = "RowID";

        // 如果建表时，使用了WITHOUT ROWID，对应表类型会是Index Interior/Index Leaf
        // 这时就不需要把主键名称放到后面
        if(withoutRowid && m_curTableName == m_curName) pkName = "";

        bool setHeaders = false;
        if(headers.size() > 0)
        {
            headers.push_front("LeftChild");
            if(pkName.size() > 0) headers.push_back(pkName);
            m_pTableWdiget->setColumnCount(headers.size());
            m_pTableWdiget->setHorizontalHeaderLabels(headers);
            setHeaders = true;

            //qDebug() << headers.size();
        }

        m_pTableWdiget->setRowCount(m_payloadArea.size());

        for(auto it=m_payloadArea.begin(); it!=m_payloadArea.end(); ++it)
        {
            int idx = it - m_payloadArea.begin();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);
            cellContentParentItem = setCellData(cellContentParentItem, *m_pCurSQLite3DB->m_pSqlite3Payload, *it, raw);
            //qDebug() << "m_pCurSQLite3DB->DecodeCell(pgno, idx, vars) [" << pgno << "," << idx << "," << vars.size() << "]";

            if(!setHeaders)
            {
                headers.push_back("LeftChild");
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
        QString pkName = "RowID";
        if(pkFiledName.size() == 1) pkName = QString::fromStdString(pkFiledName[0]);
        // 当主键和索引列名称一样时，取pkName为Rowid
        if(headers.size() == 1)
        {
            if(headers[0] == pkName) pkName = "RowID";
        }
        else if(headers.empty()) pkName = "RowID";

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
            cellContentParentItem = setCellData(cellContentParentItem, *m_pCurSQLite3DB->m_pSqlite3Payload, *it, raw);
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


        setFreeListPageHdrData(sNextTrunkPageNo, nNextTrunkPageNo,
            sLeafPageCounts,nLeafPageCounts,sLeafPageNos,nLeafPageNos,sUnused, raw);
        m_payloadArea = sLeafPageNos;

    }
    else
    {
        m_pTableWdiget->setColumnCount(0);
        m_pTableWdiget->setRowCount(0);
    }

    setPushBtnStats();

    m_pPageView->expandAll();
}

void HexWindow::onPageTypeChanged(const QString &pageType)
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

void HexWindow::onComboxChanged(const QString &item)
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

void HexWindow::onPrevBtnClicked()
{
    int curIdx = ui->comboBox->currentIndex();
    if(curIdx > 0)
    {
        curIdx -= 1;
        ui->comboBox->setCurrentIndex(curIdx);
    }
}

void HexWindow::onNextBtnClicked()
{
    int curIdx = ui->comboBox->currentIndex();
    int count = ui->comboBox->count();
    if(curIdx+1 < count)
    {
        curIdx += 1;
        ui->comboBox->setCurrentIndex(curIdx);
    }
}

void HexWindow::onFirstBtnClicked()
{
    int count = ui->comboBox->count();
    if(count > 0)
    {
        ui->comboBox->setCurrentIndex(0);
    }
}

void HexWindow::onLastBtnClicked()
{
    int count = ui->comboBox->count();
    if(count > 0)
    {
        ui->comboBox->setCurrentIndex(count - 1);
    }
}

void HexWindow::onCurrentAddressChanged(qint64 address)
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

    //qDebug() << "onCurrentAddressChanged" << address;
    //qDebug() << m_mapItems.size();
    if(m_mapItems.contains(address))
    {
        m_pPageView->setCurrentIndex(m_mapItems[address]->index());
    }
}

void HexWindow::onCheckBoxStatChanged(bool stat)
{
    QHexDocument* document = m_pHexEdit->document();
    if(document == NULL || m_pCurSQLite3DB == NULL) return;
    if(stat)
    {
        document->setBaseAddress((m_curPageNo-1)*m_pCurSQLite3DB->GetPageSize());
    }
    else
    {
        document->setBaseAddress(0);
    }

    document->beginMetadata();
    document->endMetadata();
}

void HexWindow::setPushBtnStats()
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
