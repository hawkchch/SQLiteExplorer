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
class HexWindow;
}

class MainWindow;
class HexWindow : public QWidget
{
    Q_OBJECT

public:
    explicit HexWindow(QWidget *parent = 0);
    ~HexWindow();

    void SetPageNos(const vector<int>& pgnos);
    void SetPageNosAndType(const vector<pair<int, PageType>>& pgs);
    void SetTableName(const QString& name, const QString& tableName, const QString& type);

public slots:
    void onPageIdSelect(int pgno, PageType type);
    void onPageTypeChanged(const QString& item);
    void onComboxChanged(const QString& item);
    void onPrevBtnClicked();
    void onNextBtnClicked();
    void onFirstBtnClicked();
    void onLastBtnClicked();

    void onCurrentAddressChanged(qint64 address);

private:
    void setPushBtnStats();


private:
    Ui::HexWindow *ui;


    QSplitter* m_pSplitter;
    QHexEdit* m_pHexEdit;
    QTableWidget* m_pTableWdiget;

    MainWindow* m_pParent;
    CSQLite3DB* m_pCurSQLite3DB;

    QString m_curTableName;
    QString m_curName;

    QStringList m_tableHeaders;
    vector<ContentArea> m_payloadArea;

    QMap<PageType, QString> m_pageTypeName;
    QStringList m_pageNoAndTypes;
};

#endif // QHEXWINDOW_H
