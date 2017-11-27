#include "qsqlitequerywindow.h"
#include "ui_qsqlitequerywindow.h"
#include "mainwindow.h"
#include "qsqlitetableview.h"

#include <QLayout>
#include <QVBoxLayout>

#include <QTextEdit>

QSQLiteQueryWindow::QSQLiteQueryWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::QSQLiteQueryWindow)
{
    ui->setupUi(this);

    m_pTableView = new QSQLiteTableView(parent);
    connect(this, SIGNAL(signalSQLiteQuery(QString)), m_pTableView, SLOT(onSQLiteQueryReceived(QString)));

    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(onExecuteBtnClicked()));

    // Init Splitter
    m_pSplitter = new QSplitter(Qt::Vertical);
    m_pSplitter->addWidget(ui->textEdit);
    m_pSplitter->addWidget(ui->widget);
    m_pSplitter->addWidget(m_pTableView);
    m_pSplitter->setStretchFactor(0, 4);
    m_pSplitter->setStretchFactor(1, 1);
    m_pSplitter->setStretchFactor(2, 4);

    ui->centralWidget->setLayout(new QVBoxLayout);
    ui->centralWidget->layout()->addWidget(m_pSplitter);
}

QSQLiteQueryWindow::~QSQLiteQueryWindow()
{
    delete ui;
}

void QSQLiteQueryWindow::onExecuteBtnClicked()
{
    QString sql = ui->textEdit->toPlainText();
    emit signalSQLiteQuery(sql);
}
