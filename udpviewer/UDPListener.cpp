#include "UDPListener.h"
#include "SpectrumRenderArea.h"
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <cassert>

UDPListener::UDPListener(QUdpSocket *s, SpectrumRenderArea *r): m_socket(s), m_render(r)
{
    QObject::connect(s, &QUdpSocket::readyRead,  this, &UDPListener::onReadyRead);
    m_fpstimer = new QTimer(this);
    m_fpstimer->setSingleShot(false);
    m_fps = 0;
    connect(m_fpstimer, &QTimer::timeout, this, &UDPListener::updateFPS);
    m_fpstimer->start(1000);
    currentseq = 0;
}

void UDPListener::onReadyRead()
{

    while (m_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_socket->receiveDatagram();
        process(datagram);
    }
}

#define MAX_FRAME_PAYLOAD 2048

struct frame {
    uint8_t seq:7;
    uint8_t val:1;    /* 1: */
    uint8_t frag:4;
    uint8_t fragsize_bits:4;
    uint8_t payload[MAX_FRAME_PAYLOAD];
};


void UDPListener::updateFPS()
{
    emit fpsUpdated(m_fps);
    m_fps=0;
}

void UDPListener::process(QNetworkDatagram&datagram)
{
    QByteArray data = datagram.data();

    if (data.length()<2)
        return;
#if 0
    unsigned payloadlen = data.length() - 2;
    //qDebug()<<"Dgram";
    const struct frame *f = (const struct frame*)data.constData();

    unsigned maxpayload = 1<<((f->frag) >> 4);

    uint8_t lastfrag = (SPECTRUM_FRAME_SIZE+(maxpayload-1)/maxpayload)-1;
    uint8_t frag = f->frag & 0xf;

    memcpy( &m_framedata[ frag * maxpayload ], f->payload, payloadlen);
    if (frag>=lastfrag) {
        m_render->startFrame();
        m_render->renderSCR(m_framedata);
        m_render->finishFrame();
        m_fps++;
        emit frameReceived();
    }
#endif
    printf("Datagram len %d\n",data.length());
    int len = data.length();
    const uint8_t *d = (const uint8_t*)data.constData();

    while (len>0) {
        const struct frame *f = (const struct frame*)d;
        unsigned fragsize = 1<<f->fragsize_bits;
        printf(">> Seq %d val %d frag %d fragsize_bits %d(%d)\n\n",
               f->seq,
               f->val,
               f->frag,
               f->fragsize_bits,
               fragsize);
        len-=2;
        d+=2;
        if (f->val) {
            len -= fragsize;
        }
    }
}


