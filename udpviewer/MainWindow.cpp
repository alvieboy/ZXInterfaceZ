#include "MainWindow.h"
#include <QMainWindow>
#include "SpectrumRenderArea.h"
#include <unistd.h>
#include <fcntl.h>
#include <QUdpSocket>
#include <QDebug>
#include "UDPListener.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTcpSocket>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include "VCDWriter.h"
#include <QApplication>
#include "ProgressBar.h"
#include "LineBuffer.h"
#include <exception>
#include <QHostInfo>
#include <QKeyEvent>
#include "keycapturer.h"
#include "../esp32/main/usb_hid_keys.h"

#define UDP_STREAM_PORT 8010

static const uint8_t bitRevTable[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};


class InvalidHostAddress: public std::exception
{
};

MainWindow::MainWindow(const char *host)
{
    setWindowTitle("ZX Interface Z");
    QWidget *mainWidget = new QWidget(this);

    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *fileMenu = new QMenu(tr("&File"));
    menuBar->addMenu(fileMenu);


    connect(fileMenu->addAction("&Upload firmware"), &QAction::triggered,
            this, &MainWindow::onUploadOTAClicked);

    connect(fileMenu->addAction("&Upload FPGA firmware"), &QAction::triggered,
            this, &MainWindow::onUploadFPGAClicked);

    connect(fileMenu->addAction("&Load resource"), &QAction::triggered,
            this, &MainWindow::onLoadResourceClicked);

    connect(fileMenu->addAction("E&xit"), &QAction::triggered, this, &MainWindow::onExit);
    qDebug()<<"Using host"<<host;
    m_zxaddress = QHostAddress(host);//?host:"192.168.120.1");
    if (m_zxaddress.isNull()) {
        // Look it up
        QHostInfo info = QHostInfo::fromName(host);

        if (!info.addresses().isEmpty()) {
            m_zxaddress = info.addresses().first();
        }
    }

    if (m_zxaddress.isNull()) {
        throw InvalidHostAddress();
    }

    m_renderer = new SpectrumRenderArea(mainWidget);

    setCentralWidget(mainWidget);
    m_statusbar = new QStatusBar(this);
    setStatusBar(m_statusbar);

    QHBoxLayout *hlayout1 = new QHBoxLayout(mainWidget);

    QVBoxLayout *vlayout1 = new QVBoxLayout();

    hlayout1->addItem(vlayout1);
    hlayout1->addWidget(m_renderer);

    m_udpsocket = new QUdpSocket();

#if 0
    if (!m_udpsocket->bind(QHostAddress::Any, 8010, QUdpSocket::ReuseAddressHint)) {
        qDebug()<<"Cannot bind";
        throw 1;
    }
#endif

    // Add buttons

    QPushButton *exit = new QPushButton("Exit", mainWidget);
    vlayout1->addWidget(exit);

    QPushButton *getfb = new QPushButton("Get FB", mainWidget);
    vlayout1->addWidget(getfb);

    QPushButton *stream = new QPushButton("Stream", mainWidget);
    vlayout1->addWidget(stream);

    QPushButton *upload = new QPushButton("Upload ROM", mainWidget);
    vlayout1->addWidget(upload);

    QPushButton *uploadsna = new QPushButton("Upload SNA", mainWidget);
    vlayout1->addWidget(uploadsna);

    QPushButton *reset = new QPushButton("Reset", mainWidget);
    vlayout1->addWidget(reset);

    QPushButton *resetcustom = new QPushButton("Reset to Custom ROM", mainWidget);
    vlayout1->addWidget(resetcustom);

    QPushButton *capture = new QPushButton("Capture", mainWidget);
    vlayout1->addWidget(capture);

    QPushButton *wide = new QPushButton("SetModeWide", mainWidget);
    vlayout1->addWidget(wide);

    

    m_connecting = false;

    connect(getfb, &QPushButton::clicked, this, &MainWindow::onGetFBClicked);
    connect(stream, &QPushButton::clicked, this, &MainWindow::onStreamClicked);
    connect(upload, &QPushButton::clicked, this, &MainWindow::onUploadROMClicked);
    connect(uploadsna, &QPushButton::clicked, this, &MainWindow::onUploadSNAClicked);
    connect(reset, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(resetcustom, &QPushButton::clicked, this, &MainWindow::onResetToCustomClicked);
    connect(capture, &QPushButton::clicked, this, &MainWindow::onCaptureClicked);
    connect(wide, &QPushButton::clicked, this, &MainWindow::onWideClicked);


    m_linebuffer = new LineBuffer(8192, this);

    connect(m_linebuffer, &LineBuffer::lineReceived,
            this, &MainWindow::onStatusLineReceived);


    m_listener = new UDPListener(m_udpsocket, m_zxaddress, 8002, m_renderer);

    connect(m_listener, &UDPListener::fpsUpdated,
            this, &MainWindow::onFpsUpdated);

    if (0){
        uint8_t data[16384];
        int fd = open("MANIC.SCR", O_RDONLY);
        if (fd<0)
            throw 1;
        read(fd, data, sizeof(data));
        ::close(fd);
        m_renderer->startFrame();
        m_renderer->renderSCR(data);
        m_renderer->finishFrame();
    }
    m_progress = NULL;
}


void MainWindow::onGetFBClicked()
{
    sendReceive(m_zxaddress, &MainWindow::fbReceived, false, "fb");
}

void MainWindow::fbReceived(QByteArray*b)
{
    if (!b) {
        qDebug()<<"NULL Fb received";
        return;
    }
    qDebug()<<"Received FB data size "<<b->length();
    if (b->length()==SPECTRUM_FRAME_SIZE) {
        m_renderer->startFrame();
        m_renderer->renderSCR((const uint8_t*)b->constData());
        m_renderer->finishFrame();
    }
}


bool MainWindow::sendReceive(const QHostAddress &address,
                             void (MainWindow::*callback)(QByteArray*),
                             bool callImmediatlyOnReception,
                             const QString &data,
                             const QByteArray &bindata,
                             bool showProgress)
{
    if (m_connecting)
        return false;

    m_cmdSocket = new QTcpSocket();

    connect(m_cmdSocket, &QAbstractSocket::connected, this, &MainWindow::commandSocketConnected);
    connect(m_cmdSocket,
            &QAbstractSocket::errorOccurred,
            this,
            &MainWindow::commandSocketError
           );
    connect(m_cmdSocket, &QAbstractSocket::readyRead, this, &MainWindow::commandSocketRead);

    m_connecting = true;
    m_cmd = data;
    m_cmdHandler = callback;
    m_result = NULL;
    m_callimmediatly = callImmediatlyOnReception;

    if (showProgress) {
        if (m_progress!=NULL) {
            delete(m_progress);
        }
        m_progress = new ProgressBar(this, "Uploading", ProgressBar::MODE_DUAL);
        m_progress->show();
    }

    if (m_progress) {
        QString s = "Connecting to " + address.toString();
        m_progress->setStatusText(s);
    }
    m_bindata = bindata;
    m_cmdSocket->connectToHost(address, 8003);

    return true;
}

void MainWindow::sendInChunks(QTcpSocket*sock, QByteArray&data)
{
    QByteArray qb;
    unsigned totalsize = data.length();
    unsigned uploaded = 0;

#define CHUNKSIZE 2048
    unsigned csize;

    while (uploaded<totalsize) {
        csize = data.length() < CHUNKSIZE ? data.length() : CHUNKSIZE;
        qb = m_bindata.left(csize);
        m_bindata.remove(0, csize);
        uploaded+=csize;
        if (m_progress) {
            m_progress->setPrimaryPercentage( (uploaded * 100)/totalsize );
        }
        qDebug()<<"Write chunk"<<qb.length();
        if (sock->write(qb)<0)
            return ;
    }
}

void MainWindow::commandSocketConnected()
{
    // Send data.
    qDebug()<<"Connected";
    QString tb = m_cmd + "\n";


    m_cmdSocket->write(tb.toLocal8Bit());

    if (m_progress) {
        QString s = "Connected, command sent.";
        m_progress->setStatusText(s);
    }

    if (m_bindata.length()) {
        {
            printf("Data: ");
            const unsigned char *p = (const unsigned char*)m_bindata.constData();
            for (int i=0;i<10;i++) {
                printf(" %02x", p[i]);
            }
            printf("\n");
        }
        sendInChunks(m_cmdSocket, m_bindata);
    }
}

void MainWindow::commandSocketRead()
{
    uint8_t data[8192];
    int r = m_cmdSocket->read((char*)data,sizeof(data));
    qDebug()<<"Socket: read "<<r<<"bytes";

    if (r<0) {
        return;
    }

    if ((r==0) && (m_callimmediatly==false)) {
        (this->*MainWindow::m_cmdHandler)(m_result);
        if (m_result) {
            delete(m_result);
        }
    } else {
        if (m_callimmediatly) {
            QByteArray *ba = new QByteArray((const char*)data, r);
            (this->*MainWindow::m_cmdHandler)(ba);
            delete(ba);
        } else {
            if (m_result==NULL) {
                m_result = new QByteArray();
            }
            m_result->append((const char*)data, r);
        }
    }
}


void MainWindow::commandSocketError(QAbstractSocket::SocketError error)
{
    //m_cmdSocket
    if (error==QAbstractSocket::RemoteHostClosedError) {
        qDebug()<<"Remote host closed connection";
        if (!m_callimmediatly) {
            (this->*MainWindow::m_cmdHandler)(m_result);
            if (m_result) {
                delete(m_result);
                m_result = NULL;
            }
        }
        m_connecting = false;
        return;
    }

    qDebug()<<"Cannot connect to host: "<<m_cmdSocket->errorString();
    delete(m_cmdSocket);
    m_cmdSocket=NULL;
    m_connecting = false;
    (this->*MainWindow::m_cmdHandler)(NULL);
}

void MainWindow::uploadROM(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    qDebug()<<"Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "uploadrom " + QString::number(data.length()), data);
}

void MainWindow::uploadSNA(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    qDebug()<<"SNA Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "uploadsna " + QString::number(data.length()), data);
}

void MainWindow::uploadResource(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    qDebug()<<"Resource Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "uploadres " + QString::number(data.length()), data);
}


void MainWindow::reverseBits(QByteArray &b)
{
    int i;
    for (i=0;i<b.size();i++) {
        b[i] = bitRevTable[(unsigned char)b[i]];
    }
}

void MainWindow::uploadOTA(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();

    qDebug()<<"OTA Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "ota " + QString::number(data.length()), data,
               true);
}

void MainWindow::uploadFPGA(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    reverseBits(data);

    qDebug()<<"OTA Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "fpgaota " + QString::number(data.length()), data,
               true);
}

void MainWindow::onResetClicked()
{
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "reset");
}

void MainWindow::onResetToCustomClicked()
{
    // capdata_s <= XCK_sync_s & XMREQ_sync_s & XIORQ_sync_s & XRD_sync_s & XWR_sync_s
    // & XA_sync_s & XD_sync_s;
    // Mask 0xA000000 val 0x0
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true, "resettocustom cap 0x0FFFFF00 0x05000100");
}

void MainWindow::onCaptureClicked()
{
    sendReceive(m_zxaddress, &MainWindow::captureReceived, false, "cap");
}

void MainWindow::captureReceived(QByteArray*data)
{
    VCDWriter *w = new VCDWriter();
    QString s = QFileDialog::getSaveFileName(this,"Save file", "", "*.vcd") ;
    if (s.length()>0) {
        w->write(data,s);
    }
    delete(w);
}

void MainWindow::onUploadROMClicked()
{
    QFileDialog *fc = new QFileDialog(this, "Open ROM file",
                                      ".",
                                      "ROM Files (*.rom *.bin)");
    fc->setFileMode(QFileDialog::ExistingFile);
    if (fc->exec()) {
        QStringList files = fc->selectedFiles();
        if (files.length()!=1)
            return;
        QString filename = files[0];
        qDebug()<<"Uploading "<<filename;
        uploadROM(filename);
    }
}

void MainWindow::onUploadOTAClicked()
{
    QFileDialog *fc = new QFileDialog(this, "Open firmware file",
                                      ".",
                                      "Firmware files (*.bin)");
    fc->setFileMode(QFileDialog::ExistingFile);
    if (fc->exec()) {
        QStringList files = fc->selectedFiles();
        if (files.length()!=1)
            return;
        QString filename = files[0];
        qDebug()<<"Uploading "<<filename;
        uploadOTA(filename);
    }
}

void MainWindow::onUploadFPGAClicked()
{
    QFileDialog *fc = new QFileDialog(this, "Open firmware file",
                                      ".",
                                      "FPGA Firmware files (*.rbf)");
    fc->setFileMode(QFileDialog::ExistingFile);
    if (fc->exec()) {
        QStringList files = fc->selectedFiles();
        if (files.length()!=1)
            return;
        QString filename = files[0];
        qDebug()<<"Uploading "<<filename;
        uploadFPGA(filename);
    }
}

void MainWindow::onLoadResourceClicked()
{
    QFileDialog *fc = new QFileDialog(this, "Open resource file",
                                      ".",
                                      "Firmware files (*.*)");
    fc->setFileMode(QFileDialog::ExistingFile);
    if (fc->exec()) {
        QStringList files = fc->selectedFiles();
        if (files.length()!=1)
            return;
        QString filename = files[0];
        qDebug()<<"Uploading "<<filename;
        uploadResource(filename);
    }
}

void MainWindow::onUploadSNAClicked()
{
    QFileDialog *fc = new QFileDialog(this, "Open SNA file",
                                      ".",
                                      "SNA Files (*.sna)");
    fc->setFileMode(QFileDialog::ExistingFile);
    if (fc->exec()) {
        QStringList files = fc->selectedFiles();
        if (files.length()!=1)
            return;
        QString filename = files[0];
        qDebug()<<"Uploading "<<filename;
        uploadSNA(filename);
    }
}

void MainWindow::progressReceived(QByteArray*a)
{
    m_linebuffer->append(*a);
    //Q_UNUSED(a);
}

void MainWindow::onStreamClicked()
{
    sendReceive(m_zxaddress, &MainWindow::progressReceived, true,  "stream 8010");
}

void MainWindow::onFpsUpdated(unsigned fps)
{
    //QString statustext = "FPS: " + QString::number(fps);
    //m_statusbar->showMessage(statustext);
    m_renderer->setFPS(fps);
}


void MainWindow::alert(const QString &s)
{
    QMessageBox::critical(this, "Error", s);
}

void MainWindow::onExit()
{
    QApplication::exit();
}

void MainWindow::processStatus(const QString &s)
{
    int delim_pos = s.indexOf('/');
    if (delim_pos>0) {
        QString total = s.left(delim_pos);
        QString current = s.right(s.length()-(delim_pos+1));

        bool ok;
        int total_int = total.toInt(&ok);
        if (!ok) {
            qDebug()<<"Cannot process: '"<<total<<"' is not a number";
            return;
        }
        int current_int = current.toInt(&ok);
        if (!ok) {
            qDebug()<<"Cannot process: '"<<current<<"' is not a number";
            return;
        }
        m_statusbar->showMessage("Progress: "+current+" of "+total);
        if (m_progress) {
            m_progress->setSecondaryPercentage( current_int*100 / total_int );
            m_progress->setStatusText("Progress: "+current+" of "+total);

        }
    } else {
        qDebug()<<"Cannot find delimiter in "<<s;
    }
}

void MainWindow::onStatusLineReceived(QString status)
{
    if (status=="OK") {
        m_statusbar->showMessage("OK");
        if (m_progress) {
            m_progress->completed();
        }
    } else if (status.startsWith("ERROR:")) {
//        QStringRef errtxt(&status, 7, status.length()-7);
        if (m_progress) {
            m_progress->error(status);
        }
        m_statusbar->showMessage(status);//status.right(status.lenght()-7));
    } else if (status.startsWith("P:")) {
        processStatus(status.right(status.length()-2));
    }
}

void MainWindow::onWideClicked()
{
}

/*
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    QKeyEvent *k = dynamic_cast<QKeyEvent*>(event);

    if (!k)
        return QMainWindow::eventFilter(obj, event);
    qDebug()<<"k ev";
    return false;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    quint32 scan = event->nativeScanCode();
    qDebug()<<"Press"<<scan;
    if (!m_keys.contains(scan)) {
        m_keys[scan] = true;
        sendUpdateKeys();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    quint32 scan = event->nativeScanCode();

    QMap<unsigned,bool>::iterator i = m_keys.find(scan);

    if (i!=m_keys.end()) {
        m_keys.erase(i);
        sendUpdateKeys();
    }
    }
    */

void MainWindow::onKeyChanged(uint64_t keys)
{
    sendUpdateKeys(keys);
}

void MainWindow::sendUpdateKeys(uint64_t keys)
{
    uint8_t datagram[8];
    memset(datagram,0xFF,sizeof(datagram));
    int i = 0;
    datagram[i++] = 0x01; // Key events

    // Keyboard is 8*5 keys, 40 keys total
    // it fits in 5 bytes.
    char tmp[128];
    sprintf(tmp,"%016lx", keys);
    qDebug()<<"Update keys"<<tmp;
    for(;i<6;i++) {
        // 5 bits per key, inverted
        datagram[i] = ~keys;
        keys>>=8;
    }

    m_udpsocket->writeDatagram((const char*)datagram,
                               sizeof(datagram),
                               m_zxaddress,
                               8002);
}

/*void MainWindow::focusOutEvent(QFocusEvent *)
{
    // Clear all pressed keys
    onKeyChanged(0);
}

  */