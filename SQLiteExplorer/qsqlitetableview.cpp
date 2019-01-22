#include "qsqlitetableview.h"
#include "mainwindow.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QDebug>

QSQLiteTableView::QSQLiteTableView(QWidget *parent)
: QTableWidget(parent)
, m_pParent(nullptr)
, m_pCurSQLite3DB(nullptr)
, m_rowThresh(100)
{
    MainWindow* pMainWindow = qobject_cast<MainWindow*>(parent);
    if (pMainWindow)
    {
        m_pParent = pMainWindow;
    }

    QHeaderView *headers = horizontalHeader();
    //SortIndicator为水平标题栏文字旁边的三角指示器
    headers->setSortIndicator(0, Qt::AscendingOrder);
    headers->setSortIndicatorShown(true);
    connect(this->verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(onValueChanged(int)));
}

void QSQLiteTableView::onSQLiteQueryReceived(const QString &sql)
{
    if (m_pParent)
    {
        m_pCurSQLite3DB = m_pParent->GetCurSQLite3DB();
    }
    else
    {
        return;
    }

    clear();
    setColumnCount(0);
    setRowCount(0);
    m_rowThresh = 100;

    try
    {
        string s = sql.toStdString();
        m_curQuery = m_pCurSQLite3DB->execQuery(s.c_str());
        CppSQLite3Query& q = m_curQuery;

        QStringList headers;
        for(int i=0; i<q.numFields(); i++)
        {
            headers.push_back(QString::fromStdString(q.fieldName(i)));
        }
        setColumnCount(headers.size());

        setHorizontalHeaderLabels(headers);

        while (!q.eof())
        {
            insertRow(rowCount());
            for(int col=0; col<q.numFields(); col++)
            {
                QTableWidgetItem *name = new QTableWidgetItem();
                name->setText(QString::fromStdString(q.getStringField(col)));
                setItem(rowCount()-1, col, name);
            }
            q.nextRow();
            if(rowCount() >= m_rowThresh)
            {
                break;
            }
        }

        QString msg;
        if(!q.eof())
        {
            msg = QString("数据过多，已加载%1条记录").arg(rowCount());
        }
        else
        {
            msg = QString("数据加载完成，共加载%1条记录").arg(rowCount());
        }
        emit dataLoaded(msg);
        qDebug() << msg;
    }
    catch(CppSQLite3Exception& e)
    {
        QMessageBox::information(this, tr("SQLiteExplorer"), QString::fromStdString(e.errorMessage()));
    }
}

void QSQLiteTableView::onValueChanged(int value)
{
//    qDebug() << "value =" << value << ", VSBar Max =" << verticalScrollBar()->maximum()
//             << ", m_rowThresh =" << m_rowThresh;
    if(value == verticalScrollBar()->maximum() && !m_curQuery.eof())
    {
        //qDebug() << "Enter ";
        m_rowThresh *= 2;
        CppSQLite3Query& q = m_curQuery;
        while (!q.eof())
        {
            insertRow(rowCount());
            for(int col=0; col<q.numFields(); col++)
            {
                QTableWidgetItem *name = new QTableWidgetItem();
                name->setText(QString::fromStdString(q.getStringField(col)));
                setItem(rowCount()-1, col, name);
            }
            q.nextRow();
            if(rowCount() >= m_rowThresh)
            {
                break;
            }
        }
        QString msg;
        if(!q.eof())
        {
            msg = QString("数据过多，已加载%1条记录").arg(rowCount());
        }
        else
        {
            msg = QString("数据加载完成，共加载%1条记录").arg(rowCount());
        }
        emit dataLoaded(msg);
        qDebug() << msg;
    }
}
