#include "graphwindow.h"
#include "ui_graphwindow.h"

GraphWindow::GraphWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GraphWindow)
{
    ui->setupUi(this);

    connect(ui->pushButton, SIGNAL(clicked(bool)), ui->graphWidget, SLOT(StartAnimate(bool)));

    m_graphicsScene = new QGraphicsScene;
    m_graphicsItem = new PixItem;
    m_graphicsView = new MyGraphicsView(this);
    m_graphicsScene->addItem(m_graphicsItem);
    m_graphicsView->setScene(m_graphicsScene);

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setMargin(0);
    layout->addWidget(m_graphicsView);
    ui->tab_2->setLayout(layout);
}

GraphWindow::~GraphWindow()
{
    delete ui;
}

void GraphWindow::SetPath(QString path)
{
    if(path == "tmp.plain")
    {
        ui->graphWidget->SetPath(path);
    }
    else
    {
        QPixmap px;
        px.load(path);

        m_graphicsItem->setPixmap(px);
        m_graphicsItem->setPos(0,0);
        m_graphicsItem->setZoomState(1);
        qreal w = px.width();
        qreal h = px.height();
        m_graphicsScene->setSceneRect(-1*(w/2), -1*(h/2), w, h);
    }
}
