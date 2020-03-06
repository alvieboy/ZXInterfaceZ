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
    uint8_t seq;
    uint8_t frag;
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

    unsigned payloadlen = data.length() - 2;

    const struct frame *f = (const struct frame*)data.constData();
    switch (f->frag) {
    case 0:
        assert( payloadlen == MAX_FRAME_PAYLOAD );
        memcpy( &m_framedata[0], f->payload, MAX_FRAME_PAYLOAD);
        break;
    case 1:
        assert( payloadlen == MAX_FRAME_PAYLOAD );
        memcpy( &m_framedata[MAX_FRAME_PAYLOAD], f->payload, MAX_FRAME_PAYLOAD);
        break;
    case 2:
        assert( payloadlen == MAX_FRAME_PAYLOAD );
        memcpy( &m_framedata[MAX_FRAME_PAYLOAD*2], f->payload, MAX_FRAME_PAYLOAD);
        break;
    case 3:
        memcpy( &m_framedata[MAX_FRAME_PAYLOAD*3], f->payload, payloadlen);
        m_render->renderSCR(m_framedata);
        m_render->finishFrame();
        m_fps++;
        emit frameReceived();
        break;

    default:
        break;
    }
}


