#include "DataWindow.h"
#include "ui_DataWindow.h"
#include <mainwindow.h>
DataWindow::DataWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DataWindow)
{
    ui->setupUi(this);

    m_pParent = qobject_cast<MainWindow*>(parent);

    connect(ui->tableWidget, SIGNAL(dataLoaded(QString)), this, SLOT(onDataLoaded(QString)));
}

DataWindow::~DataWindow()
{
    delete ui;
}

void DataWindow::clear()
{
    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(0);
    ui->label->clear();
}

void DataWindow::onSQLiteQueryReceived(const QString &sql)
{
    if(m_pParent)
    {
        ui->tableWidget->SetDb(m_pParent->GetCurSQLite3DB());
    }
    ui->tableWidget->onSQLiteQueryReceived(sql);
}

void DataWindow::onDataLoaded(const QString &msg)
{
    ui->label->setText(msg);
}
