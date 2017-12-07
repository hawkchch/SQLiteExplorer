#include "qsqlitetableview.h"
#include "mainwindow.h"
#include <QMessageBox>

QSQLiteTableView::QSQLiteTableView(QWidget *parent)
: QTableWidget(parent)
, m_pParent(nullptr)
, m_pCurSQLite3DB(nullptr)
{
    MainWindow* pMainWindow = qobject_cast<MainWindow*>(parent);
    if (pMainWindow)
    {
        m_pParent = pMainWindow;
    }
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

    table_content tb;
    cell_content header;
    QString errmsg = QString::fromStdString(m_pCurSQLite3DB->ExecuteCmd(sql.toStdString(), tb, header));

    if(!errmsg.isEmpty())
    {
        QMessageBox::information(this, tr("SQLiteExplorer"), errmsg);
    }
    else
    {
        setColumnCount(header.size());
        QStringList list;
        for(auto it=header.begin(); it!=header.end(); ++it)
        {
            list.push_back(QString::fromStdString(*it));
        }

        setHorizontalHeaderLabels(list);
        setRowCount(tb.size());

        for(size_t i=0; i<tb.size(); ++i)
        {
            cell_content& cc = tb[i];
            for(size_t j=0; j<cc.size(); ++j)
            {
                QTableWidgetItem *name=new QTableWidgetItem();//创建一个Item
                name->setText(QString::fromStdString(cc[j]));//设置内容
                setItem(i,j,name);//把这个Item加到第一行第二列中
            }
        }
    }
}
