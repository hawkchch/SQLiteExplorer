#ifndef MYGRAPHICSVIEW_H
#define MYGRAPHICSVIEW_H

#include <QObject>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPixmap>
#include <QPainter>
#include <QRectF>
#include <QMouseEvent>
#include <QPointF>
#include <QDragEnterEvent>
#include <QGraphicsSceneWheelEvent>

QT_BEGIN_NAMESPACE
class QWheelEvent;
QT_END_NAMESPACE

enum Enum_ZoomState{
    NO_STATE,
    RESET,
    ZOOM_IN,
    ZOOM_OUT
};

enum Enum_ZoomTimes{
    ZOOM_IN_TIMES = 10,
    ZOOM_OUT_TIMES = -10,
};

class PixItem : public QGraphicsItem
{
public:
    PixItem(QWidget* parent = 0);   //构造函数初始化了变量pix
    void setPixmap(const QPixmap& pixmap);

    QRectF boundingRect() const;    //实现自己的boundingRect 图元边界方法，完成以图元坐标系为基础增加两个像素点的冗余的工作
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget); //重画图形函数

    void wheelEvent(QGraphicsSceneWheelEvent *event);
    void setZoomState(const int &zoomState);

    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    int getScaleValue() const;
    void setScaleValue(const int &);

private:
    qreal m_scaleValue;   //缩放值

    QPixmap pix;    //作为图元显示的图片
    int m_zoomState;

    bool m_isMove;
    QPointF m_startPos;
};


class MyGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit MyGraphicsView(QWidget *parent = 0);
    ~MyGraphicsView();

protected:
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void magnify();
    void shrink();
    void scaling(qreal scaleFactor);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    qreal m_scalingOffset;
    bool m_isMove;
    QPointF m_startPos;
};

#endif // MYGRAPHICSVIEW_H
