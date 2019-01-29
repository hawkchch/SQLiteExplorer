#include "SQLWindow.h"
#include "ui_sqlwindow.h"
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

    m_pParent = qobject_cast<MainWindow*>(parent);

    m_pTableView = new QSQLiteTableView(parent);
    connect(this, SIGNAL(signalSQLiteQuery(QString)), m_pTableView, SLOT(onSQLiteQueryReceived(QString)));
    connect(m_pTableView, SIGNAL(dataLoaded(QString)), this, SLOT(onDataLoaded(QString)));

    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(onExecuteBtnClicked()));
    connect(ui->pushButton_2, SIGNAL(clicked(bool)), this, SLOT(onExplainBtnClicked()));

    // Init Splitter
    m_pSplitter = new QSplitter(Qt::Vertical);
    m_pSplitter->addWidget(ui->textEdit);
    m_pSplitter->addWidget(ui->widget);
    m_pSplitter->addWidget(m_pTableView);
    m_pSplitter->addWidget(ui->widget_2);

    m_pSplitter->setStretchFactor(0, 4);
    m_pSplitter->setStretchFactor(1, 1);
    m_pSplitter->setStretchFactor(2, 4);
    m_pSplitter->setStretchFactor(3, 1);

    ui->centralWidget->setLayout(new QVBoxLayout);
    ui->centralWidget->layout()->setMargin(0);
    ui->centralWidget->layout()->setSpacing(0);
    ui->centralWidget->layout()->addWidget(m_pSplitter);
}

QSQLiteQueryWindow::~QSQLiteQueryWindow()
{
    delete ui;
}

void QSQLiteQueryWindow::onExecuteBtnClicked()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    QString sql = cursor.selectedText();
    if (sql.size() == 0)
    {
        sql = ui->textEdit->toPlainText();
    }

    m_pTableView->SetDb(m_pParent->GetCurSQLite3DB());
    emit signalSQLiteQuery(sql);
}

void QSQLiteQueryWindow::onExplainBtnClicked()
{
    QTextCursor cursor = ui->textEdit->textCursor();
    QString sql = cursor.selectedText();
    if (sql.size() == 0)
    {
        sql = ui->textEdit->toPlainText();
    }

    if (sql.size())
    {
        sql = "EXPLAIN " + sql;
    }

    m_pTableView->SetDb(m_pParent->GetCurSQLite3DB());
    emit signalSQLiteQuery(sql);
}

void QSQLiteQueryWindow::onDataLoaded(const QString &msg)
{
    ui->label->setText(msg);
}
