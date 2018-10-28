#include "cvideocoder.h"
#include <QDebug>
#include <QTimer>


//------------------------------------------------------
/**
 * @brief CVideoCoder::CVideoCoder
 * @param parent
 */
//------------------------------------------------------
CVideoCoder::CVideoCoder(QObject *parent, int width, int height) : QObject(parent)
  , m_codec(NULL)
  , m_c(NULL)
  , m_pkt(NULL)
  , m_frame(NULL)
  , m_parser(NULL)
  , m_frameRGB(NULL)
  , img_convert_ctx(NULL)
  , m_rgbbuffer(NULL)
  , m_pixformat(AV_PIX_FMT_YUV420P)
  , m_width(width)
  , m_height(height)
{
    logout(avcodec_configuration());
    unsigned version = avcodec_version();
    QString str = QString::number(version, 10);
    logout("version: " + str);
    m_file = new QFile("h264.out");
    m_file->open(QIODevice::WriteOnly);
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::~CVideoCoder
 */
//------------------------------------------------------
CVideoCoder::~CVideoCoder()
{
    m_file->close();
    delete []m_file;
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::startEncoder -- 启动编码器
 */
//------------------------------------------------------
void CVideoCoder::startEncoder(int fps, int64_t bitrate)
{
    logout(QString("width: %1, height: %2").arg(m_width).arg(m_height));
    do{
        // Find a registered encoder with a matching codec ID.
        m_codec= avcodec_find_encoder(AV_CODEC_ID_H264);
        if(!m_codec){
            logout("can not find AV_CODEC_ID_H264");
        }else{
            logout("AV_CODEC_ID_H264 is ready");
        }

        // Allocate an AVCodecContext and set its fields to default values. The
        // resulting struct should be freed with avcodec_free_context().
        m_c = avcodec_alloc_context3(m_codec);
        if(!m_c){
            logout("Could not allocate video codec context\n");
            break;
        }
        logout("alloc context ok");

        // Allocate an AVPacket and set its fields to default values.  The resulting
        // struct must be freed using av_packet_free().
        m_pkt = av_packet_alloc();
        if(!m_pkt)
        {
            logout("packet alloc failed");
            break;
        }


        m_c->bit_rate = bitrate;    // the average bitrate
        m_c->width = m_width;
        m_c->height = m_height;

        m_c->time_base = AVRational{1, fps};
        m_c->framerate = AVRational{fps, 1};

        // the number of pictures in a group of pictures, or 0 for intra_only
        m_c->gop_size = 2;
        m_c->max_b_frames = 2;
        m_c->pix_fmt = m_pixformat;
//        if(m_codec->id == AV_CODEC_ID_H264)
//        {
//            av_opt_set(m_c->priv_data, "preset", "slow", 0);
//        }
        av_opt_set(m_c->priv_data, "preset", "slow", 0);
//        av_opt_set(m_c->priv_data, "preset", "superfast", 0);
        av_opt_set(m_c->priv_data, "tune", "zerolatency", 0);

        int ret = avcodec_open2(m_c, m_codec, NULL);
        if(ret < 0)
        {
            logout("Could not open codec: " + AVStrError(ret));
            break;
        }

        // create a frame to collet image data
        m_frame = av_frame_alloc();
        if (!m_frame) {
            logout("Could not allocate video frame");
            break;
        }
        m_frame->format = m_c->pix_fmt;
        m_frame->width  = m_width;
        m_frame->height = m_height;
        m_frame->pts = 0;

        ret = av_frame_get_buffer(m_frame, 16);
        if (ret < 0) {
            logout("Could not allocate the video frame data");
            break;
        }
    }while(0);

    initOutputFrame();
    return;
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::freeEncoder -- 释放编码器
 */
//------------------------------------------------------
void CVideoCoder::freeEncoder()
{
    // free m_c
    if(m_c)
    {
        avcodec_free_context(&m_c);
        m_c = NULL;
    }
    // free m_frame
    if(m_frame)
    {
        av_frame_free(&m_frame);
        m_frame = NULL;
    }
    // free m_pkt
    if(m_pkt)
    {
        av_packet_free(&m_pkt);
        m_pkt = NULL;
    }
    m_codec = NULL;
    uninitOutputFrame();
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::startDecoder -- 启动解码器
 */
//------------------------------------------------------
void CVideoCoder::startDecoder()
{
    do{
        m_pkt = av_packet_alloc();
        if(!m_pkt)
        {
            logout("can not alloc pkt");
            break;
        }
        m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
        if(!m_codec)
        {
            logout("codec not found");
            break;
        }
        m_parser = av_parser_init(m_codec->id);
        if(!m_parser)
        {
            logout("parser not found");
            break;
        }
        m_c = avcodec_alloc_context3(m_codec);
        if(!m_c)
        {
            logout("could not allocate video codec context");
            break;
        }

        if(avcodec_open2(m_c, m_codec, NULL) < 0)
        {
            logout("could not open codec");
            break;
        }

        m_frame = av_frame_alloc();
        if(!m_frame)
        {
            logout("could not allocate video frame");
            break;
        }
        initOutputRGB32();
    }while(0);
    return;
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::freeDecoder -- 释放解码器
 */
//------------------------------------------------------
void CVideoCoder::freeDecoder()
{
    // free m_c
    if(m_c)
    {
        avcodec_free_context(&m_c);
        m_c = NULL;
    }
    // free m_frame
    if(m_frame)
    {
        av_frame_free(&m_frame);
        m_frame = NULL;
    }
    // free m_pkt
    if(m_pkt)
    {
        av_packet_free(&m_pkt);
        m_pkt = NULL;
    }
    // free m_parser
    if(!m_parser)
    {
        av_parser_close(m_parser);
    }
    m_codec = NULL;
    uninitOUtputRGB32();
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::initOutputRGB32 -- 格式转换初始化
 */
//------------------------------------------------------
void CVideoCoder::initOutputRGB32()
{
    m_frameRGB = av_frame_alloc();

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_width, m_height, 1);
    m_rgbbuffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize, m_rgbbuffer, AV_PIX_FMT_RGB32, m_width, m_height, 1);
    img_convert_ctx = sws_getContext(m_width, m_height, m_pixformat, m_width, m_height, AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::uninitOUtputRGB32 -- 格式转换去初始化
 */
//------------------------------------------------------
void CVideoCoder::uninitOUtputRGB32()
{
    if(m_frameRGB)
    {
        av_frame_free(&m_frameRGB);
        m_frameRGB = NULL;
    }
    if(m_rgbbuffer)
    {
        av_free(m_rgbbuffer);
        m_rgbbuffer = NULL;
    }
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::encode -- 编码
 * @param frame
 * @param pkt
 * @param outfile
 */
//------------------------------------------------------
void CVideoCoder::encode(const QImage &image, QByteArray &data_buf)
{
    int ret;
    AVFrame *frame = m_frame;
    /* make sure the frame data is writable */
    ret = av_frame_make_writable(frame);
    if (ret < 0)
    {
        logout("can not make writable");
        return;
    }

    pixmapToFrame(image);
    av_image_fill_arrays(m_frame->data, m_frame->linesize, m_yuvbuffer, m_pixformat, m_width, m_height, 1);


    /* send the frame to the encoder */
    if (frame)
        logout(QString("Send frame %1\n").arg(frame->pts));

    ret = avcodec_send_frame(m_c, frame);
    if (ret < 0) {
        logout("Error sending a frame for encoding\n");
        return;
    }
    frame->pts++;
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_c, m_pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            logout("Error during encoding\n");
            break;
        }

        logout(QString("Write packet %1 (size=%2)\n").arg(m_pkt->pts).arg(m_pkt->size));

        // save data as QByteArray
        data_buf.insert(data_buf.size(), (char*)m_pkt->data, m_pkt->size);
        m_file->write(data_buf);
        // clear AVPacket
        av_packet_unref(m_pkt);
    }
    return;
}
//------------------------------------------------------
/**
* @brief CVideoCoder::decode -- 解码
* @param dec_ctx
* @param frame
* @param pkt
* @param pixmap
*/
//------------------------------------------------------
int CVideoCoder::decode(QByteArray data_array, QImage &image)
{
    int ret = 0;
    int res = 0;
    int size = 0;

    uint8_t *data = (uint8_t*)data_array.data();
    int data_size = data_array.size();
    // 返回使用的字节数
    size = av_parser_parse2(m_parser, m_c, &m_pkt->data, &m_pkt->size,
                               data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    qDebug() << "decoder used data size is: " << size;
    if(size < 0)
    {
        return 0;
    }
    if(!m_pkt->size)
    {
        return 0;
    }

    ret = avcodec_send_packet(m_c, m_pkt);
    res = ret;
    logout(QString("res is: %1").arg(res));
    if (ret < 0) {
        logout("Error sending a packet for decoding\n");
        return 0;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(m_c, m_frame);
        if (ret == AVERROR(EAGAIN))
        {
            // 当前状态不可用，需要发送新数据
            return 0;
        }
        else if(ret == AVERROR_EOF)
        {
            // 解码器已刷新，不再输出帧
            logout("AVERROR_EOF\n");
            return 0;
        }
        else if (ret < 0) {
            logout("Error during decoding\n");
            return 0;
        }

        logout(QString("saving frame %1\n").arg(m_c->frame_number));
        fflush(stdout);
        frameToImage(m_frame, image);
    }
    return res;
}

void CVideoCoder::initOutputFrame()
{
    // init frame RGB
    m_frameRGB = av_frame_alloc();

    int numBytesRGB = av_image_get_buffer_size(AV_PIX_FMT_RGB32, m_width, m_height, 1);
    m_rgbbuffer = (uint8_t*)av_malloc(numBytesRGB * sizeof(uint8_t));

    av_image_fill_arrays(m_frameRGB->data, m_frameRGB->linesize, m_rgbbuffer, AV_PIX_FMT_RGB32, m_width, m_height, 1);

    // init frame YUV
    m_frameYUV = av_frame_alloc();

    int numBytesYUV = av_image_get_buffer_size(m_pixformat, m_width, m_height, 1);
    m_yuvbuffer = (uint8_t*)av_malloc(numBytesYUV * sizeof(uint8_t));

    av_image_fill_arrays(m_frameYUV->data, m_frameYUV->linesize, m_yuvbuffer, AV_PIX_FMT_YUV420P, m_width, m_height, 1);


    yuv_convert_ctx = sws_getContext(m_width, m_height, AV_PIX_FMT_RGB32, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
}

void CVideoCoder::uninitOutputFrame()
{
    if(m_frameRGB)
    {
        av_frame_free(&m_frameRGB);
        m_frameRGB = NULL;
    }
    if(m_frameYUV)
    {
        av_frame_free(&m_frameYUV);
        m_frameYUV = NULL;
    }
    if(m_yuvbuffer)
    {
        av_free(m_yuvbuffer);
        m_yuvbuffer = NULL;
    }
}

void CVideoCoder::pixmapToFrame(const QImage &image)
{
    QImage img = image.convertToFormat(QImage::Format_RGB32);
    m_frameRGB->data[0] = (uint8_t*)img.bits();
    sws_scale(yuv_convert_ctx, (uint8_t const * const*)&m_frameRGB->data,\
              m_frameRGB->linesize, 0, m_height, m_frameYUV->data, m_frameYUV->linesize);
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::frameToPixmap -- 将AVFrame数据转换为QPixmap数据
 * @param frame
 * @param pixmap
 */
//------------------------------------------------------
void CVideoCoder::frameToImage(AVFrame *frame, QImage &image)
{
    sws_scale(img_convert_ctx, (uint8_t const * const*)frame->data,\
              frame->linesize, 0, m_height, m_frameRGB->data, m_frameRGB->linesize);
    QImage tmpImg((uchar*)m_rgbbuffer, m_width, m_height, QImage::Format_RGB32);
    image = tmpImg.copy();
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::AVStrError -- mmfpeg错误码解析
 * @param errnum
 * @return
 */
//------------------------------------------------------
QString CVideoCoder::AVStrError(int errnum)
{
    char* errbuf = new char[1024];
    av_strerror(errnum, errbuf, 1024);
    QString errstr = QString(errbuf);
    delete []errbuf;

    return errstr;
}
//------------------------------------------------------
/**
 * @brief CVideoCoder::logout -- 运行消息打印
 * @param msg
 */
//------------------------------------------------------
void CVideoCoder::logout(QString msg)
{
    qDebug() << msg;
    emit sendMsg(msg);
}
