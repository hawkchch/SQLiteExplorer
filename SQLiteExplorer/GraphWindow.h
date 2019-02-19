#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QWidget>
#include "pixitem.h"

namespace Ui {
class GraphWindow;
}

class GraphWindow : public QWidget
{
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = 0);
    ~GraphWindow();

    void clear();

    void SetPath(QString path);
private:
    Ui::GraphWindow *ui;

    QGraphicsScene* m_graphicsScene;
    PixItem*        m_graphicsItem;
    QGraphicsView*  m_graphicsView;
};

#endif // GRAPHWINDOW_H
