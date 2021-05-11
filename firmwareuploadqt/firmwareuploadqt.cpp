#include <QWindow>
#include <QMainWindow>
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QProgressBar>
#include <QStatusBar>
#include <QWebSocket>
#include "firmwareuploadqt.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <unistd.h>

MyMainWindow::MyMainWindow()
{
    setup();
}

void MyMainWindow::setup()
{

    QWidget *w = new QWidget();

    QVBoxLayout *l = new QVBoxLayout();

    QHBoxLayout *h = new QHBoxLayout();
    h->addWidget(new QLabel("URL:"));
    m_url = new QLineEdit();
    h->addWidget(m_url);
    m_url->setText( m_settings.value("url").toString() );

    l->addLayout(h);


    QHBoxLayout *fh = new QHBoxLayout();
    fh->addWidget(new QLabel("File:"));
    m_filename = new QLineEdit();
    fh->addWidget(m_filename);

    m_filename->setText( m_settings.value("file").toString() );

    QPushButton *sel = new QPushButton("Browse...");

    connect(sel, &QPushButton::clicked, this, &MyMainWindow::selectFile);

    fh->addWidget(sel);

    l->addLayout(fh);

    m_phase = new QLabel("Test1");
    m_action= new QLabel("Test2");
    m_progress = new QProgressBar();
    m_progress->setMaximum(100);
    m_progress->setMinimum(0);

    l->addWidget(m_phase);
    l->addWidget(m_action);
    l->addWidget(m_progress);

    w->setLayout(l);
    setCentralWidget(w);

    m_status = new QStatusBar();
    setStatusBar(m_status);

    m_status->showMessage("Ready.");

    QDialogButtonBox*bb = new QDialogButtonBox();

    bb->addButton( new QPushButton("Start"), QDialogButtonBox::AcceptRole);
    bb->addButton( new QPushButton("Close"), QDialogButtonBox::RejectRole);

    connect(bb, &QDialogButtonBox::accepted, this, &MyMainWindow::upload);
    connect(bb, &QDialogButtonBox::rejected, this, &MyMainWindow::close);

    l->addWidget(bb);

    w->show();

    m_state = IDLE;
}

void MyMainWindow::selectFile()
{
    QString dir = m_settings.value("directory").toString();

    QFileDialog *d = new QFileDialog(NULL,
                                     "Open file",
                                     dir,
                                     "Firmware files (*.izf)"
                                    );
    if (d->exec()) {
        QDir dir = d->directory();
        m_settings.setValue("directory",dir.absolutePath());
        QStringList l = d->selectedFiles();
        if (l.size()==1) {
            QString f = l[0];
            m_settings.setValue("file", l[0]);
            m_filename->setText(l[0]);
        }
    }
}

void MyMainWindow::upload()
{
    m_settings.setValue("url", m_url->text());

    m_file =  new QFile(m_filename->text());

    if (!m_file->open(QIODevice::ReadOnly)) {
        m_status->showMessage("Cannot open file!");
        delete(m_file);
        return;
    }

    m_socket = new QWebSocket();
    connect(m_socket, &QWebSocket::textFrameReceived, this, &MyMainWindow::onTextFrameReceived);
    connect(m_socket, &QWebSocket::binaryFrameReceived, this, &MyMainWindow::onBinaryFrameReceived);
    connect(m_socket, &QWebSocket::connected, this, &MyMainWindow::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &MyMainWindow::onDisconnected);
    connect(m_socket, &QWebSocket::bytesWritten, this, &MyMainWindow::onBytesWritten);

    m_phase->clear();
    m_action->clear();
    m_progress->setValue(0);

    m_status->showMessage("Connecting to Interface Z");
    m_state = IDLE;
    m_socket->open(m_url->text());
}

void MyMainWindow::onConnected()
{
    m_status->showMessage("Connected to Interface Z");
    m_state = START;
    m_socket->sendTextMessage("START");
}

void MyMainWindow::onDisconnected()
{
    m_status->showMessage("Disconnected");
}

void MyMainWindow::updatePercent(int p)
{
    if (p<0)
        p=0;
    if (p>100)
        p=100;
    m_progress->setValue(p);
}

void MyMainWindow::updateAction(const QString &level, const QString&message)
{
    m_action->setText(message);
}

void MyMainWindow::updatePhase(const QString &phase)
{
    m_phase->setText(phase);
}

void MyMainWindow::onTextFrameReceived(const QString &frame, bool isLastFrame)
{
    QJsonDocument doc = QJsonDocument::fromJson(frame.toLocal8Bit());

    QJsonObject obj = doc.object();
    QJsonObject::iterator percent = obj.find("percent");
    QJsonObject::iterator action = obj.find("action");
    QJsonObject::iterator level = obj.find("level");
    QJsonObject::iterator phase = obj.find("phase");

    if (percent!=obj.end() && percent->isDouble()) {
        updatePercent((int)percent->toDouble());
    }
    if (action!=obj.end()) {
        updateAction(level->toString(), action->toString());
    }
    if (phase!=obj.end()) {
        updatePhase(phase->toString());
    }
}

void MyMainWindow::onBinaryFrameReceived(const QByteArray &bytes, bool isLastFrame)
{
}

void MyMainWindow::onBytesWritten(quint64 amount)
{
    QByteArray data;
    switch (m_state) {
    case START:
    case STREAM:
        data = m_file->read(512);
        if (data.length()>0) {
            //Small delay just to make sure we see some progress.
            usleep(10000);
            m_socket->sendBinaryMessage(data);
        } else {
            m_state = FLUSH;
        }
        break;
    default:
        break;
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MyMainWindow win;
    win.show();
    app.exec();
}
