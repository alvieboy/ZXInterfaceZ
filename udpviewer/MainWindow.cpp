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

#define UDP_STREAM_PORT 8010

MainWindow::MainWindow()
{
    setWindowTitle("ZX Interface Z");
    QWidget *mainWidget = new QWidget(this);

    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    QMenu *fileMenu = new QMenu(tr("&File"));
    menuBar->addMenu(fileMenu);


    connect(fileMenu->addAction("&Upload firmware"), &QAction::triggered,
            this, &MainWindow::onUploadOTAClicked);

    connect(fileMenu->addAction("&Load resource"), &QAction::triggered,
            this, &MainWindow::onLoadResourceClicked);

    connect(fileMenu->addAction("E&xit"), &QAction::triggered, this, &MainWindow::onExit);

    m_zxaddress = QHostAddress("192.168.120.1");
    //m_zxaddress = QHostAddress("127.0.0.1");

    m_renderer = new SpectrumRenderArea(mainWidget);

    setCentralWidget(mainWidget);
    m_statusbar = new QStatusBar(this);
    setStatusBar(m_statusbar);

    QHBoxLayout *hlayout1 = new QHBoxLayout(mainWidget);

    QVBoxLayout *vlayout1 = new QVBoxLayout();

    hlayout1->addItem(vlayout1);
    hlayout1->addWidget(m_renderer);

    m_udpsocket = new QUdpSocket();

    if (!m_udpsocket->bind(QHostAddress::Any, 8010, QUdpSocket::ReuseAddressHint)) {
        qDebug()<<"Cannot bind";
        throw 1;
    }


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

    

    m_connecting = false;

    connect(getfb, &QPushButton::clicked, this, &MainWindow::onGetFBClicked);
    connect(stream, &QPushButton::clicked, this, &MainWindow::onStreamClicked);
    connect(upload, &QPushButton::clicked, this, &MainWindow::onUploadROMClicked);
    connect(uploadsna, &QPushButton::clicked, this, &MainWindow::onUploadSNAClicked);
    connect(reset, &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(resetcustom, &QPushButton::clicked, this, &MainWindow::onResetToCustomClicked);
    connect(capture, &QPushButton::clicked, this, &MainWindow::onCaptureClicked);

    
    m_listener = new UDPListener(m_udpsocket, m_renderer);

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
    sendReceive(m_zxaddress, &MainWindow::fbReceived, "fb");
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


bool MainWindow::sendReceive(const QHostAddress &address, void (MainWindow::*callback)(QByteArray*), const QString &data,
                            const QByteArray &bindata, bool showProgress)
{
    if (m_connecting)
        return false;

    m_cmdSocket = new QTcpSocket();

    connect(m_cmdSocket, &QAbstractSocket::connected, this, &MainWindow::commandSocketConnected);
    connect(m_cmdSocket,
            qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error),
            this,
            &MainWindow::commandSocketError
           );
    connect(m_cmdSocket, &QAbstractSocket::readyRead, this, &MainWindow::commandSocketRead);

    m_connecting = true;
    m_cmd = data;
    m_cmdHandler = callback;
    m_result = NULL;

    if (showProgress) {
        if (m_progress!=NULL) {
            delete(m_progress);
        }
        m_progress = new ProgressBar(this, "Uploading");
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
    if (r==0) {
        (this->*MainWindow::m_cmdHandler)(m_result);
        if (m_result) {
            delete(m_result);
        }
    } else {
        if (m_result==NULL) {
            m_result = new QByteArray();
        }
        m_result->append((const char*)data, r);
    }
}



void MainWindow::commandSocketError(QAbstractSocket::SocketError error)
{
    //m_cmdSocket
    if (error==QAbstractSocket::RemoteHostClosedError) {
        qDebug()<<"Remote host closed connection";
        (this->*MainWindow::m_cmdHandler)(m_result);
        if (m_result) {
            delete(m_result);
            m_result = NULL;
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
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "uploadrom " + QString::number(data.length()), data);
}

void MainWindow::uploadSNA(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    qDebug()<<"SNA Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "uploadsna " + QString::number(data.length()), data);
}

void MainWindow::uploadResource(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    qDebug()<<"Resource Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "uploadres " + QString::number(data.length()), data);
}

void MainWindow::uploadOTA(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        alert("Cannot open file.");
    }

    QByteArray data = f.readAll();
    qDebug()<<"OTA Uploading "<<data.length()<<"bytes";
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "ota " + QString::number(data.length()), data,
               true);
}

void MainWindow::onResetClicked()
{
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "reset cap 0x0FFFFF00 0x05000100 ");
}

void MainWindow::onResetToCustomClicked()
{
    // capdata_s <= XCK_sync_s & XMREQ_sync_s & XIORQ_sync_s & XRD_sync_s & XWR_sync_s
    // & XA_sync_s & XD_sync_s;
    // Mask 0xA000000 val 0x0
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "resettocustom cap 0x0FFFFF00 0x05000100");
}

void MainWindow::onCaptureClicked()
{
    sendReceive(m_zxaddress, &MainWindow::captureReceived, "cap");
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

void MainWindow::nullReceived(QByteArray*a)
{
    Q_UNUSED(a);
}

void MainWindow::onStreamClicked()
{
    sendReceive(m_zxaddress, &MainWindow::nullReceived, "stream 8010");
}

void MainWindow::onFpsUpdated(unsigned fps)
{
    QString statustext = "FPS: " + QString::number(fps);
    m_statusbar->showMessage(statustext);
}


void MainWindow::alert(const QString &s)
{
    QMessageBox::critical(this, "Error", s);
}

void MainWindow::onExit()
{
    QApplication::exit();
}