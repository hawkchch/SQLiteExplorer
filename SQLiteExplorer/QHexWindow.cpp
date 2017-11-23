#include "qhexwindow.h"
#include "ui_qhexwindow.h"

#include "mainwindow.h"
#include <QDebug>
#include <QTimer>

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

    connect(ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onComboxChanged(QString)));

    connect(ui->pushButtonPrev, SIGNAL(clicked(bool)), this, SLOT(onPrevBtnClicked()));
    connect(ui->pushButtonNext, SIGNAL(clicked(bool)), this, SLOT(onNextBtnClicked()));
    connect(ui->pushButtonFirst, SIGNAL(clicked(bool)), this, SLOT(onFirstBtnClicked()));
    connect(ui->pushButtonLast, SIGNAL(clicked(bool)), this, SLOT(onLastBtnClicked()));
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

    ui->comboBox->addItems(ids);

    if(pgnos.size() > 0)
    {
        onPageIdSelect(pgnos[0]);
    }
}

void QHexWindow::SetTableName(const QString &tableName)
{
    m_tableHeaders.clear();
    m_curTableName = tableName;
    vector<string> headers;
    m_pCurSQLite3DB->GetColumnNames(tableName.toStdString(), headers);
    for(auto it=headers.begin(); it!=headers.end(); ++it)
    {
        m_tableHeaders.push_back(QString::fromStdString(*it));
    }
}

void QHexWindow::onPageIdSelect(int pgno)
{
    string raw = m_pCurSQLite3DB->LoadPage(pgno);

    QByteArray ba = QByteArray::fromStdString(raw);
    QHexDocument* document = QHexDocument::fromMemory(ba);
    m_pHexEdit->setDocument(document);

    // Bulk metadata management (paints only one time)
    document->beginMetadata();

    ContentArea& pageHeaderArea = m_pCurSQLite3DB->m_pSqlite3Page->m_pageHeaderArea;
    ContentArea& cellidxArea = m_pCurSQLite3DB->m_pSqlite3Page->m_cellIndexArea;
    vector<ContentArea> payloadArea = m_pCurSQLite3DB->m_pSqlite3Page->m_payloadArea;  // payload区域
    m_payloadArea = payloadArea;
    ContentArea& unusedArea = m_pCurSQLite3DB->m_pSqlite3Page->m_unusedArea;            // 未使用区域

    document->highlightBackRange(pageHeaderArea.m_startAddr, pageHeaderArea.m_len, QColor(Qt::white));
    document->highlightBackRange(cellidxArea.m_startAddr, cellidxArea.m_len, QColor(Qt::white));
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
    document->endMetadata();
}

void QHexWindow::onComboxChanged(const QString &item)
{
    int pgno = item.toInt();
    onPageIdSelect(pgno);
}

void QHexWindow::onPrevBtnClicked()
{
    int curIdx = ui->comboBox->currentIndex();
    if(curIdx > 0)
    {
        curIdx -= 1;
        ui->comboBox->setCurrentIndex(curIdx);
        QString curText = ui->comboBox->currentText();
        int pgno = curText.toInt();
        onPageIdSelect(pgno);
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
        QString curText = ui->comboBox->currentText();
        int pgno = curText.toInt();
        onPageIdSelect(pgno);
    }
}

void QHexWindow::onFirstBtnClicked()
{
    int count = ui->comboBox->count();
    if(count > 0)
    {
        ui->comboBox->setCurrentIndex(0);
        QString text = ui->comboBox->currentText();
        int pgno = text.toInt();
        onPageIdSelect(pgno);
    }
}

void QHexWindow::onLastBtnClicked()
{
    int count = ui->comboBox->count();
    if(count > 0)
    {
        ui->comboBox->setCurrentIndex(count - 1);
        QString text = ui->comboBox->currentText();
        int pgno = text.toInt();
        onPageIdSelect(pgno);
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
            int idx = it - m_payloadArea.begin();
            int pgno = ui->comboBox->currentText().toInt();
            vector<SQLite3Variant> vars;
            m_pCurSQLite3DB->DecodeCell(pgno, idx, vars);

            m_pTableWdiget->setColumnCount(m_tableHeaders.size());
            m_pTableWdiget->setHorizontalHeaderLabels(m_tableHeaders);
            m_pTableWdiget->setRowCount(1);

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
                    val = "[BLOB]";
                    break;
                default:
                    break;
                }

                name->setText(val);//设置内容
                m_pTableWdiget->setItem(0,i,name);//把这个Item加到第一行第二列中
            }

            break;
        }
    }
}
