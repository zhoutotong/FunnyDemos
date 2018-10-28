#ifndef CMEDIATRANSPORT_H
#define CMEDIATRANSPORT_H

#include <QObject>

#include "rtpsession.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtpsessionparams.h"
#include "rtperrors.h"
#include "rtplibraryversion.h"
#include "rtpsourcedata.h"
#include "jthread.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>

#include <QString>
#include <QByteArray>

#include <QThread>

using namespace jrtplib;
using namespace jthread;

class CMediaTransport : public QObject, public RTPSession
{
    Q_OBJECT
public:
    explicit CMediaTransport(QObject *parent = nullptr);
    explicit CMediaTransport(double timestamp = 1.0 / 100.0, uint16_t portbase = 8000);
    ~CMediaTransport();


    int addReceiver(const QString &ipv4, const uint16_t &port);
    int sendPayLoad(const uint8_t* payload, const uint32_t size);
    int sendPayLoad(QByteArray payload);

    int readDataSize();

    void releaseSource(uint8_t* c);
    uint8_t* readData();
protected:
    void OnValidatedRTPPacket(RTPSourceData *srcdat, RTPPacket *rtppack, bool isonprobation, bool *ispackethandled);
    void OnRTCPSDESItem(RTPSourceData *srcdat, RTCPSDESPacket::ItemType t, const void *itemdata, size_t itemlength);
private:

    void checkError(int rtperr);

    QByteArray m_rec_data;
    int rec_cnt=0;
signals:
//    void dataIsIncoming(uint8_t* pdata, int size);
    void dataIsIncoming(QByteArray data);

public slots:
    void tryToSendData(QByteArray data);
};

#endif // CMEDIATRANSPORT_H
