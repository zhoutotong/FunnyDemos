#include "mainwindow.h"
#include <QDebug>


#define ROLE_REC (0)
#define ROLE_SEN (1)
#define ROLE_SEL (2)

#define JRTP_PORT (8000)
#define FPS       (20)

#define IMG_CODE  "JPEG"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_net_work_thread(new QThread)
    , m_net_work(new CMediaTransport(FPS, 8000))
    , m_is_first_frame(true)
    , m_rec_bytes(0.0)
    , m_send_bytes(0.0)
{
    send_cnt = 0;
    rec_cnt = 0;
    QWidget *widget = new QWidget(this);
    this->setCentralWidget(widget);
    m_layout = new QVBoxLayout(widget);
    widget->setLayout(m_layout);
    this->setFixedSize(640, 320);
    // 初始化UI界面
    setupUI();
    loadIPList();

    connect(m_is_select_all, &QCheckBox::stateChanged, this, &MainWindow::isSelectAllIP);


    qDebug() << "main thread id: " << QThread::currentThreadId();
    m_net_work->moveToThread(m_net_work_thread);

    connect(m_net_work_thread, &QThread::finished, m_net_work, &CMediaTransport::deleteLater);
    connect(this, &MainWindow::sendData, m_net_work, &CMediaTransport::tryToSendData);
    connect(m_net_work, &CMediaTransport::dataIsIncoming, this, &MainWindow::readData);
    // 启动网络发送接收线程
    m_net_work_thread->start();

    // 连接开始按钮
    connect(m_show_display_btn, &QPushButton::clicked, this, &MainWindow::getReadyWork);
    connect(m_stop_btn, &QPushButton::clicked, this, &MainWindow::stopWork);
    connect(m_reset_iplist_btn, &QPushButton::clicked, this, &MainWindow::loadIPList);

    // 初始化显示界面
    m_display = new CImageDisplay;
    m_timer = new QTimer();
    connect(m_timer, &QTimer::timeout, this, &MainWindow::beginDisplay);
    connect(m_display, &CImageDisplay::exit, this, &MainWindow::stopWork);


    // 初始化编码器解码器
    QSize size = QGuiApplication::primaryScreen()->grabWindow(0).size();
    m_encoder = new CVideoCoder(this, size.width(), size.height());
    m_encoder->startEncoder(FPS, 10000000);
    m_decoder = new CVideoCoder(this, size.width(), size.height());
    m_decoder->startDecoder();

//    connect(m_brate_set_box, &QSpinBox::valueChanged, this, &MainWindow::resetParam);
    connect(m_brate_set_box, SIGNAL(valueChanged(int)), this, SLOT(resetParam(int)));
    connect(m_ftp_set_box, SIGNAL(valueChanged(int)), this, SLOT(resetParam(int)));

    m_file = new QFile("decode_h264.out");
    m_file->open(QIODevice::WriteOnly);


    m_net_rec_timer = new QTimer;
    connect(m_net_rec_timer, &QTimer::timeout, this, &MainWindow::readNetVel);
    m_net_rec_timer->start(1000);
}

MainWindow::~MainWindow()
{
    m_encoder->freeEncoder();
    m_decoder->freeDecoder();
    m_file->close();
    delete []m_file;
}

void MainWindow::setupUI()
{
    QHBoxLayout *first_line_layout = new QHBoxLayout();
    m_show_display_btn = new QPushButton("开启");
    m_stop_btn = new QPushButton("停止");
    m_ip_list_layout = new QVBoxLayout();
//    QWidget *ip_widget = new QWidget();

    QScrollArea *scroll = new QScrollArea;
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    QGroupBox *ip_group = new QGroupBox();
    ip_group->setLayout(m_ip_list_layout);
    scroll->setWidget(ip_group);
    ip_group->setGeometry(0, 0, 60, 120);

    QGridLayout *right_layout = new QGridLayout;

    m_reset_iplist_btn = new QPushButton("载入IP列表");
    m_is_select_all = new QCheckBox("全选");
    m_role_select = new QComboBox();
    m_role_select->addItem("接收端");
    m_role_select->addItem("发送端");
    m_role_select->addItem("自发自收");
    m_is_encode = new QCheckBox("使用264编码");
    m_brate_set_box = new QSpinBox();
    m_brate_set_box->setRange(10000, 10000000);
    m_brate_set_box->setValue(1000000);
    QLabel *brate_label = new QLabel("码率");
    m_ftp_set_box = new QSpinBox();
    m_ftp_set_box->setRange(10, 50);
    m_ftp_set_box->setValue(30);
    QLabel *ftp_label = new QLabel("帧率");
    m_net_rec_vel_label = new QLabel("rec: 0k/s");
    m_net_send_vel_label = new QLabel("send: 0k/s");
    right_layout->addWidget(m_reset_iplist_btn, 0, 0);
    right_layout->addWidget(m_role_select, 0, 1, 1, -1);


    right_layout->addWidget(m_is_select_all, 1, 0);
    right_layout->addWidget(m_is_encode, 1, 1);

    right_layout->addWidget(brate_label, 2, 0, Qt::AlignRight);
    right_layout->addWidget(m_brate_set_box, 2, 1);

    right_layout->addWidget(ftp_label, 3, 0, Qt::AlignRight);
    right_layout->addWidget(m_ftp_set_box, 3, 1);

    right_layout->addWidget(m_net_send_vel_label, 4, 0);
    right_layout->addWidget(m_net_rec_vel_label, 4, 1);

    right_layout->addWidget(m_show_display_btn, 5, 0);
    right_layout->addWidget(m_stop_btn, 5, 1, 1, -1);


    first_line_layout->addWidget(ip_group);
    first_line_layout->addLayout(right_layout);


    m_layout->addLayout(first_line_layout);
}

void MainWindow::loadIPList()
{
    QFile ip_file("iplist.file");
    ip_file.open(QIODevice::ReadOnly);
    QString ip_str = ip_file.readAll();
    ip_file.close();
    QStringList ip_list = ip_str.split("\n");

    QRegExp reg("[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}.[0-9]{1,3}");
    QRegExp regi("[\r ]");

    foreach(QCheckBox *check, m_ip_list)
    {
        delete check;
        m_ip_list.pop_front();
    }
    foreach(QString str, ip_list)
    {
        int start_cnt = reg.indexIn(str);
        int end_cnt = regi.indexIn(str, start_cnt);

        if(end_cnt == -1)
        {
             end_cnt = str.size() - start_cnt;
        }
        else
        {
            end_cnt -= start_cnt;
        }
        if(start_cnt == -1)
        {
            continue;
        }
        QString ip_addr = str.mid(start_cnt, end_cnt);
        m_ip_list.append(new QCheckBox(ip_addr));
        m_ip_list_layout->addWidget(m_ip_list.back());
    }
}

QPixmap MainWindow::cutWindow()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    return screen->grabWindow(0);
}

void MainWindow::sendMsg(QString msg)
{
    emit sendData(QByteArray::fromStdString(msg.toStdString()));
}

void MainWindow::sendByte(const QByteArray &data)
{
    m_send_bytes += data.size();
    emit sendData(data);
}

void MainWindow::readData(QByteArray data)
{
    QImage img;
    m_rec_bytes += data.size();
    if(m_is_encode->isChecked())
    {
        m_file->write(data);
        m_decoder->decode(data, img);
        if(img.isNull())
        {
            return;
        }
        m_display->setPixmap(QPixmap::fromImage(img));
    }
    else
    {
        QPixmap pixmap;
        pixmap.loadFromData(data);
        if(!pixmap.isNull())
        {
            m_display->setPixmap(pixmap);
        }
    }
}

void MainWindow::getReadyWork()
{
    m_net_work->ClearDestinations();
    switch(m_role_select->currentIndex())
    {
    case ROLE_REC:
        // 接收模式不做处理，仅对接收的数据进行显示
        m_display->show();
        break;
    case ROLE_SEN:
        // 发送端应将全部已选IP加入发送组
        foreach(QCheckBox *box, m_ip_list)
        {
            if(box->isChecked())
            {
                m_net_work->addReceiver(box->text(), JRTP_PORT);
            }
            m_timer->start(1000 / m_ftp_set_box->value());
        }
        break;
    case ROLE_SEL:
        // 自发自收模式将自身IP加入发送组
        m_net_work->addReceiver("127.0.0.1", JRTP_PORT);
        m_display->show();
        m_timer->start(1000 / m_ftp_set_box->value());
        break;
    default:
        break;
    }
}

void MainWindow::beginDisplay()
{
    QByteArray data;
    m_brate_set_box->setEnabled(false);
    // 截取当前屏幕，编码，发送
    QPixmap pixmap = cutWindow();

    if(m_is_encode->isChecked())
    {
        m_encoder->encode(pixmap.toImage(), data);
    }
    else
    {
        QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, IMG_CODE, -1);
        buffer.close();
    }
    sendByte(data);
}

void MainWindow::stopWork()
{
    m_brate_set_box->setEnabled(true);
    m_timer->stop();
}

void MainWindow::isSelectAllIP()
{
    foreach(QCheckBox *box, m_ip_list)
    {
        box->setChecked(m_is_select_all->isChecked());
    }
}

void MainWindow::resetParam(int value)
{
    Q_UNUSED(value);
    m_encoder->freeEncoder();
    m_encoder->startEncoder(m_ftp_set_box->value(), m_brate_set_box->value());
}

void MainWindow::readNetVel()
{
    m_net_rec_vel_label->setText(QString("rec: %1k/s").arg(m_rec_bytes / 1024.0f));
    m_net_send_vel_label->setText(QString("send: %1k/s").arg(m_send_bytes / 1024.0f));
    m_rec_bytes = 0.0f;
    m_send_bytes = 0.0f;
}


