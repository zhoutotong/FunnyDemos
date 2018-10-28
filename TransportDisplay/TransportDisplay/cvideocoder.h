#ifndef CVIDEOCODER_H
#define CVIDEOCODER_H

#include <QObject>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QImage>
#include <QFile>
#include <QTimer>
extern "C"
{
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavdevice/avdevice.h>
    #include <libavformat/version.h>
    #include <libavutil/time.h>
    #include <libavutil/mathematics.h>

    #include <libavutil/opt.h>
    #include <libavutil/imgutils.h>
}


class CVideoCoder : public QObject
{
    Q_OBJECT
public:
    explicit CVideoCoder(QObject *parent = nullptr, int width = 0, int height = 0);
    ~CVideoCoder();

    void startEncoder(int fps, int64_t bitrate);
    void freeEncoder();
    void encode(const QImage &image, QByteArray &data_buf);

    void startDecoder();
    void freeDecoder();
    int decode(QByteArray data_array, QImage &image);
private:
    const AVCodec *m_codec;
    AVCodecContext *m_c;
    AVPacket *m_pkt;
    AVFrame *m_frame;

    AVCodecParserContext *m_parser;

    // YUV转换RGB32参量
    AVFrame *m_frameRGB;
    SwsContext *img_convert_ctx;
    uint8_t *m_rgbbuffer;

    // Pixmap转换为YUV参量
    AVFrame *m_frameYUV;
    SwsContext *yuv_convert_ctx;
    uint8_t *m_yuvbuffer;

    // 图像格式控制
    AVPixelFormat m_pixformat;
    int m_width;
    int m_height;

    QFile *m_file;

    void initOutputFrame();
    void uninitOutputFrame();
    void pixmapToFrame(const QImage &image);


    void initOutputRGB32();
    void uninitOUtputRGB32();
    void frameToImage(AVFrame *frame, QImage &image);
    QString AVStrError(int errnum);
signals:
    void sendMsg(QString);
public slots:
    void logout(QString msg);
};

#endif // CVIDEOCODER_H
