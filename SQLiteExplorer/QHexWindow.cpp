#include "qhexwindow.h"
#include "ui_qhexwindow.h"

#include "mainwindow.h"
#include <QDebug>

QHexWindow::QHexWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QHexWindow)
{
    ui->setupUi(this);

    m_pHexEdit = new QHexEdit(this);
    ui->widget_2->setLayout(new QHBoxLayout);
    ui->widget_2->layout()->setMargin(0);
    ui->widget_2->layout()->addWidget(m_pHexEdit);

    MainWindow* pMainWindow = qobject_cast<MainWindow*>(parentWidget());
    if (pMainWindow)
    {
        m_pParent = pMainWindow;
    }

    connect(ui->comboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onComboxChanged(QString)));

    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(onPrevBtnClicked()));
    connect(ui->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onNextBtnClicked()));
}

QHexWindow::~QHexWindow()
{
    delete ui;
}

void QHexWindow::SetPageIds(const vector<int> &pgids)
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
    for(auto it=pgids.begin(); it!=pgids.end(); ++it)
    {
        ids.push_back(QString("%1").arg(*it));
    }

    ui->comboBox->addItems(ids);

    if(pgids.size() > 0)
    {
        onPageIdSelect(pgids[0]);
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

    qDebug() << curIdx << ":" << count;

    if(curIdx+1 < count)
    {
        curIdx += 1;
        ui->comboBox->setCurrentIndex(curIdx);
        QString curText = ui->comboBox->currentText();
        int pgno = curText.toInt();
        onPageIdSelect(pgno);
    }
}
