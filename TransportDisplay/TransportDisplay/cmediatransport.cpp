#include "cmediatransport.h"
#include <stdio.h>
#include <stdlib.h>
#include <QDebug>

#define MAX_PACKAGE_SIZE 1300

using namespace jrtplib;
CMediaTransport::CMediaTransport(QObject *parent) : QObject(parent)
{

}
CMediaTransport::CMediaTransport(double timestamp, uint16_t portbase)
{
#ifdef RTP_SOCKETTYPE_WINSOCK
    WSADATA dat;
    WSAStartup(MAKEWORD(2,2),&dat);
#endif // RTP_SOCKETTYPE_WINSOCK
    RTPSessionParams sessparams;
    RTPUDPv4TransmissionParams transparams;



    sessparams.SetOwnTimestampUnit(timestamp);
    sessparams.SetAcceptOwnPackets(true);

    transparams.SetPortbase(portbase);
    transparams.SetRTCPMultiplexing(true);
//    this->SetMaximumPacketSize(MAX_PACKAGE_SIZE);

    int status = this->Create(sessparams, &transparams);
    checkError(status);
    rec_cnt = 0;
    m_rec_data.clear();
}

CMediaTransport::~CMediaTransport()
{
    this->BYEDestroy(RTPTime(10, 0), 0, 0);
#ifdef RTP_SOCKETTYPE_WINSOCK
    WSACleanup();
#endif // RTP_SOCKETTYPE_WINSOCK
}

int CMediaTransport::addReceiver(const QString &ipv4, const uint16_t &port)
{
    // 转换IP地址格式
    QString ip_addr = ipv4;
    qDebug() << "set ip is: " << ip_addr;
    uint32_t recip = inet_addr(ip_addr.toStdString().data());
    if(recip == INADDR_NONE)
    {
        qDebug() << "error ip address";
    }
    recip = ntohl(recip);

    // 初始化jrtplib格式的地址信息
    RTPIPv4Address addr(recip, port, true);

    // 将地址信息添加进发送列表
    int status = this->AddDestination(addr);
    checkError(status);
    return status;
}

int CMediaTransport::sendPayLoad(const uint8_t *payload, const uint32_t size)
{
    // 超出MAX_PACKAGE_SIZE时应该分包发送

    int times = size / MAX_PACKAGE_SIZE;
    int last = size % MAX_PACKAGE_SIZE;
    int status = 0;
    int timestampinc = 10;
    if(times == 0)
    {
        status = this->SendPacket((void*)(payload), last, 0, true, timestampinc);
        checkError(status);
    }
    else
    {
        for(int i = 0; i < times - 1; i++)
        {
            status = this->SendPacket((void*)(payload + i * MAX_PACKAGE_SIZE), MAX_PACKAGE_SIZE, 0, false, timestampinc);
            checkError(status);
        }

        if(last > 0)
        {
            status = this->SendPacket((void*)(payload + size - last - MAX_PACKAGE_SIZE), MAX_PACKAGE_SIZE, 0, false, timestampinc);
            checkError(status);
            int status = this->SendPacket((void*)(payload + size - last), last, 0, true, timestampinc);
            checkError(status);
        }
        else
        {
            status = this->SendPacket((void*)(payload + size - MAX_PACKAGE_SIZE), MAX_PACKAGE_SIZE, 0, true, timestampinc);
            checkError(status);
        }
    }

#ifndef RTP_SUPPORT_THREAD
    status = this->Poll();
    checkError(status);
#endif // RTP_SUPPORT_THREAD
    return status;
}

int CMediaTransport::sendPayLoad(QByteArray payload)
{
    return sendPayLoad((uint8_t*)payload.toStdString().data(), payload.size());
//    return sendPayLoad((uint8_t*)payload.data(), payload.size());
}

void CMediaTransport::checkError(int rtperr)
{
    if (rtperr < 0)
    {
        qDebug() << "ERROR: " << QString::fromStdString(RTPGetErrorString(rtperr));
        exit(-1);
    }
}

void CMediaTransport::tryToSendData(QByteArray data)
{
    sendPayLoad(data);
}


void CMediaTransport::OnValidatedRTPPacket(RTPSourceData *srcdat, RTPPacket *rtppack, bool isonprobation, bool *ispackethandled)
{
    Q_UNUSED(srcdat);
    Q_UNUSED(isonprobation);
//    qDebug() << "Got packet in OnValidatedRTPPacket from source " << srcdat->GetSSRC();
//    qDebug() << "package: " << QString::fromLocal8Bit((char*)rtppack->GetPayloadData(), rtppack->GetPayloadLength()) << " size: " << rtppack->GetPayloadLength();
//    qDebug() << "time stamp: " << rtppack->GetTimestamp();
    uint8_t* pdata=new uint8_t[rtppack->GetPayloadLength()];

//    qDebug() << "rev data size is: " << rtppack->GetPayloadLength();

    memcpy(pdata, rtppack->GetPayloadData(), rtppack->GetPayloadLength());

//    emit dataIsIncoming(pdata, (int)rtppack->GetPayloadLength());

    m_rec_data.append((char*)pdata, (int)rtppack->GetPayloadLength());
    if(rtppack->HasMarker())
    {
        emit dataIsIncoming(m_rec_data);
        m_rec_data.clear();
    }


//    qDebug() << "receive data thread id: " << QThread::currentThreadId();
    DeletePacket(rtppack);
    *ispackethandled = true;

    releaseSource(pdata);
}
void CMediaTransport::releaseSource(uint8_t* c)
{
   delete c;
}
void CMediaTransport::OnRTCPSDESItem(RTPSourceData *srcdat, RTCPSDESPacket::ItemType t, const void *itemdata, size_t itemlength)
{
    char msg[1024];

    memset(msg, 0, sizeof(msg));
    if (itemlength >= sizeof(msg))
        itemlength = sizeof(msg)-1;

    memcpy(msg, itemdata, itemlength);
//    qDebug() << "SSRC " << (unsigned int)srcdat->GetSSRC() << ": Received SDES item " << t << ": " << msg;
}
