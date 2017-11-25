#ifndef QHEXWINDOW_H
#define QHEXWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QSplitter>

#include <QHexEdit/qhexedit.h>

#include "SQLite3DB.h"

#include <vector>
using namespace std;

namespace Ui {
class QHexWindow;
}

class MainWindow;
class QHexWindow : public QWidget
{
    Q_OBJECT

public:
    explicit QHexWindow(QWidget *parent = 0);
    ~QHexWindow();

    void SetPageNos(const vector<int>& pgnos);
    void SetTableName(const QString& tableName);

public slots:
    void onPageIdSelect(int pgno);
    void onComboxChanged(const QString& item);
    void onPrevBtnClicked();
    void onNextBtnClicked();
    void onFirstBtnClicked();
    void onLastBtnClicked();

    void onCurrentAddressChanged(qint64 address);


private:
    Ui::QHexWindow *ui;


    QSplitter* m_pSplitter;
    QHexEdit* m_pHexEdit;
    QTableWidget* m_pTableWdiget;

    MainWindow* m_pParent;
    CSQLite3DB* m_pCurSQLite3DB;

    QString m_curTableName;
    QStringList m_tableHeaders;
    vector<ContentArea> m_payloadArea;
};

#endif // QHEXWINDOW_H