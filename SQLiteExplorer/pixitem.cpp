#include "pixitem.h"
#include <QWheelEvent>
#include <QPoint>
#include <QDebug>


#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QPointF>
#include <QGraphicsSceneDragDropEvent>
#include <QDrag>
#include <math.h>


//构造函数初始化了变量pix
PixItem::PixItem(QWidget *parent)
{
    setAcceptDrops(true);   //设置可拖拽
    m_scaleValue = 0;
    m_isMove = false;
}

void PixItem::setPixmap(const QPixmap &pixmap)
{
    pix = pixmap;
    setScale(1);
}

//实现自己的图元边界函数
QRectF PixItem::boundingRect() const
{
    return QRectF(-pix.width() / 2, -pix.height() / 2,
                  pix.width(), pix.height());
}

//只需QPainter的drawPixmap()函数将图元图片绘出即可
void PixItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *,
                    QWidget *)
{
    painter->drawPixmap(-pix.width() / 2, -pix.height() / 2, pix);
}

//鼠标点击事件  局部缩放
void PixItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_startPos = event->pos();
    m_isMove = true;

    int scaleValue = m_scaleValue;
    if(m_zoomState == ZOOM_IN)       //局部放大
    {
        scaleValue++;
    }
    else if(m_zoomState == ZOOM_OUT)        //局部缩小
    {
        scaleValue--;
    }

    if (scaleValue > ZOOM_IN_TIMES || scaleValue < ZOOM_OUT_TIMES)
        return;

    if (m_scaleValue != scaleValue)
    {
        setTransformOriginPoint(event->pos().x(), event->pos().y());
    }
    m_scaleValue = scaleValue;
    qreal s;
    //实现局部缩放
    if(m_scaleValue > 0)
    {
        s = pow(1.1, m_scaleValue);        //放大 计算x的y方次 参数都是double类型
    }else
    {
        s = pow(1 / 1.1, -m_scaleValue);      //缩小
    }
    //qDebug() << "mousePressEvent: s=" << s;
    setScale(s);
}

void PixItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(m_isMove)
    {
        QPointF point = event->pos() - m_startPos;
        moveBy(point.x(), point.y());
    }
}

void PixItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    m_isMove = false;
}

//使用滚轮整体缩放
void PixItem::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    setZoomState(NO_STATE);

    int scaleValue = m_scaleValue;
    if(event->delta() > 0)  //delta（）为正，滚轮向上滚
    {
        scaleValue++;
    }
    else
    {
        scaleValue--;
    }

    if (scaleValue > ZOOM_IN_TIMES || scaleValue < ZOOM_OUT_TIMES)
        return;

    m_scaleValue = scaleValue;
    qreal s;
    if(m_scaleValue > 0)
    {
        s = pow(1.1, m_scaleValue);        //放大 计算x的y方次 参数都是double类型
    }
    else
    {
        s = pow(1 / 1.1, -m_scaleValue);      //缩小
    }
    //qDebug() << "wheelEvent: s=" << s;
    setScale(s);
    setTransformOriginPoint(event->pos().x(), event->pos().y());
}

//从widget获取的缩放值，用于同步滚轮和按键
void PixItem::setScaleValue(const int &scaleValue)
{
    if (scaleValue > ZOOM_IN_TIMES || scaleValue < ZOOM_OUT_TIMES)
        return;

    m_scaleValue = scaleValue;

    qreal s;
    if(m_scaleValue > 0)
    {
        s = pow(1.1, m_scaleValue);        //放大 计算x的y方次 参数都是double类型
    }
    else
    {
        s = pow(1 / 1.1, -m_scaleValue);      //缩小
    }

    //qDebug() << "setScaleValue:" << s;
    setScale(s);
}

void PixItem::setZoomState(const int &zoomState)
{
    m_zoomState = zoomState;
    if (m_zoomState == RESET)
    {
        m_scaleValue = 0;
        //qDebug() << "setZoomState:" << 1;
        setScale(1);
        setTransformOriginPoint(0, 0);
    }
}

int PixItem::getScaleValue() const
{
    return m_scaleValue;
}


MyGraphicsView::MyGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    m_scalingOffset=1;
    m_isMove = false;
}

MyGraphicsView::~MyGraphicsView()
{

}

void MyGraphicsView::magnify()
{
    if(m_scalingOffset>=10.0)
        return;

    m_scalingOffset*=1.1;
    scaling(1.1);
}

void MyGraphicsView::shrink()
{
    if(m_scalingOffset<=0.1)
        return;

    m_scalingOffset*=0.9;
    scaling(0.9);
}

void MyGraphicsView::scaling(qreal scaleFactor)
{
    //qDebug()<<this->sceneRect() << " ScaleFactor = " << scaleFactor;
    scale(scaleFactor,scaleFactor);
}

void MyGraphicsView::mousePressEvent(QMouseEvent *event)
{
    m_startPos = event->pos();
    m_isMove = true;
}

void MyGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if(m_isMove)
    {
        QPointF point = event->pos() - m_startPos;
        //moveBy(point.x(), point.y());
        move(point.x(), point.y());
    }
}

void MyGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    m_isMove = false;
}

void MyGraphicsView::wheelEvent(QWheelEvent *event)
{
    QPoint sroll=event->angleDelta();
    //qDebug() << "wheelEvent: " << sroll;
    sroll.y()>0?magnify():shrink();
}
