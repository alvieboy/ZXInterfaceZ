#ifndef __UDPLISTENER_H__
#define __UDPLISTENER_H__

#include <QObject>
#include <QHostAddress>
#include <inttypes.h>

class QUdpSocket;
class SpectrumRenderArea;
class QNetworkDatagram;
class QTimer;

class UDPListener: public QObject
{
    Q_OBJECT
public:
    UDPListener(QUdpSocket*s, const QHostAddress &addr, quint16 port, SpectrumRenderArea *r);
    void onReadyRead();
    void process(QNetworkDatagram&datagram);
    void updateFPS();
    void sendConnect();
signals:
    void frameReceived();
    void fpsUpdated(unsigned);
private:
    QUdpSocket *m_socket;
    QTimer *m_fpstimer;
    SpectrumRenderArea *m_render;
    uint8_t m_framedata[16384];
    uint8_t currentseq;
    unsigned m_frames;
    unsigned m_fps;
    QHostAddress m_address;
    quint16 m_port;
};

#endif
