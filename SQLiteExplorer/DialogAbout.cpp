#include "DialogAbout.h"
#include "ui_DialogAbout.h"

DialogAbout::DialogAbout(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogAbout)
{
    ui->setupUi(this);

    setWindowTitle("SQLiteExplorer");
}

DialogAbout::~DialogAbout()
{
    delete ui;
}
