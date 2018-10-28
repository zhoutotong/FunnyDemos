#include "cimagedisplay.h"
#include <QDebug>
CImageDisplay::CImageDisplay(QWidget *parent) : QWidget(parent)
  , is_full_show(false)
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setMinimumSize(1024, 780);
    m_layout = new QVBoxLayout(this);
    this->setLayout(m_layout);

    m_scene = new QGraphicsScene();
    m_view = new QGraphicsView();
    m_view->setScene(m_scene);
    m_layout->addWidget(m_view);

    // 截取鼠标事件，实现鼠标双击关闭等功能
    m_view->installEventFilter(this);
    m_scene->installEventFilter(this);
    this->installEventFilter(this);


    // 添加 pixmap item 用于显示图像
    m_show_pixmap = new QGraphicsPixmapItem();
    m_scene->addItem(m_show_pixmap);
}

void CImageDisplay::setPixmap(const QPixmap &pixmap)
{
    m_show_pixmap->setPixmap(pixmap);
}

bool CImageDisplay::eventFilter(QObject *watched, QEvent *event)
{
    QMouseEvent *mouse_evt = static_cast<QMouseEvent*>(event);
    if(watched == m_view)
    {
        if(mouse_evt->type() == QMouseEvent::MouseButtonDblClick)
        {
            if(this->isFullScreen())
            {
                this->showMaximized();
            }
            else
            {
                this->close();
                emit exit();
            }
            return true;
        }
        else if(mouse_evt->type() == QMouseEvent::MouseButtonPress)
        {
            m_click_point = this->pos() - mouse_evt->globalPos();
            return true;
        }
    }
    if(watched == m_scene)
    {
        if(mouse_evt->type() == QMouseEvent::MouseButtonDblClick)
        {
            this->close();
            emit exit();
            return true;
        }
    }
    if(watched == this)
    {
        if(mouse_evt->type() == QMouseEvent::MouseButtonDblClick)
        {
            if(this->isFullScreen())
            {
                this->showMaximized();
            }
            else
            {
                this->showFullScreen();
            }

            return true;
        }
        else if(mouse_evt->type() == QMouseEvent::MouseButtonPress)
        {
            m_click_point = this->pos() - mouse_evt->globalPos();
            return true;
        }
        else if(mouse_evt->type() == QMouseEvent::MouseMove)
        {
            this->move(mouse_evt->globalPos() + m_click_point);
            return true;
        }

    }
    return false;
}
