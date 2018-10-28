#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QScrollArea>

#include <QScreen>
#include <QGuiApplication>
#include <QTimer>

#include <QDataStream>
#include <QBuffer>
#include <QFile>

#include <QSpinBox>

#include "cimagedisplay.h"
#include "cmediatransport.h"
#include "cvideocoder.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    // 配置参数
    typedef struct _ConfigParam{

    }ConfigParam;
    ConfigParam m_config_param;


    QThread *m_net_work_thread;
    CMediaTransport *m_net_work;
    CVideoCoder *m_encoder;
    CVideoCoder *m_decoder;

    QVBoxLayout *m_layout;
    QPushButton *m_show_display_btn;
    QPushButton *m_stop_btn;

    CImageDisplay *m_display;

    qreal m_rec_bytes;
    qreal m_send_bytes;
    int send_cnt;
    int rec_cnt;

    QTimer *m_timer;
    QTimer *m_net_rec_timer;
    bool m_is_first_frame;

    QVBoxLayout *m_ip_list_layout;
    QVector<QCheckBox*> m_ip_list;
    QCheckBox *m_is_select_all;
    QComboBox *m_role_select;
    QCheckBox *m_is_encode;
    QSpinBox *m_brate_set_box;
    QSpinBox *m_ftp_set_box;
    QLabel *m_net_rec_vel_label;
    QLabel *m_net_send_vel_label;
    QPushButton *m_reset_iplist_btn;

    QFile *m_file;

    void setupUI();


    QPixmap cutWindow();

    void sendMsg(QString msg);
    void sendByte(const QByteArray &data);
signals:
    void sendData(QByteArray data);
public slots:
    void readData(QByteArray data);
    void getReadyWork();
    void beginDisplay();
    void stopWork();
    void isSelectAllIP();
    void resetParam(int value);
    void readNetVel();
    void loadIPList();
};

#endif // MAINWINDOW_H
