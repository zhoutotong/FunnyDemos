#ifndef CIMAGEDISPLAY_H
#define CIMAGEDISPLAY_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QPoint>
#include <QCursor>
#include <QMouseEvent>


class CImageDisplay : public QWidget
{
    Q_OBJECT
public:
    explicit CImageDisplay(QWidget *parent = nullptr);

    QVBoxLayout *m_layout;

    QGraphicsView *m_view;
    QGraphicsScene *m_scene;
    QGraphicsPixmapItem *m_show_pixmap;
    QPoint m_click_point;
    bool is_full_show;

    void setPixmap(const QPixmap &pixmap);

private:
    bool eventFilter(QObject *watched, QEvent *event);
signals:
    void exit();

public slots:
};

#endif // CIMAGEDISPLAY_H
