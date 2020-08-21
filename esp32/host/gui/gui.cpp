#include "SpectrumWidget.h"
#include "QtSpecem.h"
#include <sys/socket.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <unistd.h>
#include "interfacez/interfacez.h"
#include <QPlainTextEdit>
#include "LogEmitter.h"

extern "C" int fpga_set_comms_socket(int socket);
extern "C" int interfacez_main(int,char**);
extern "C" void init_pallete();
extern "C" void init_emul();
extern "C" void set_logger(void (*)(const char *type, const char *tag, char *fmt, va_list ap));


class IZThread: public QThread
{
public:
    void run() {
        interfacez_main(0,NULL);
    }
};

extern "C" unsigned char *mem;

static InterfaceZ *iz;


static QPlainTextEdit *edit;



static LogEmitter *logemitter;

static void logger(const char *type, const char *tag, char *fmt, va_list ap)
{
    logemitter->log(type, tag, fmt, ap);
}

void LogEmitter::log(const char *type, const char *tag, char *fmt, va_list ap)
{
    char line[512];
    char *p = line;
    p += sprintf(p, "%s %s: ", type, tag);
    vsprintf(p, fmt, ap);
    qDebug()<<line;

    emit logstring(QString(line));

    //edit->appendPlainText(line);
    //edit->moveCursor(QTextCursor::End);
}

static int setupgui(int sock)
{
    SpectrumWidget *spectrumWidget = new SpectrumWidget();

    QMainWindow *mainw = new EmulatorWindow();

    logemitter = new LogEmitter();

    KeyCapturer *keycapture = new KeyCapturer();
    qApp->installEventFilter(keycapture);
    //spectrumWidget->installEventFilter(mainw);

    QWidget *mainwidget = new QWidget();

    mainw->setCentralWidget(mainwidget);

    QVBoxLayout *l = new QVBoxLayout();
    l->setSpacing(0);


    mainwidget->setLayout(l);

    // Add buttons.
    QHBoxLayout *hl = new QHBoxLayout();
    QPushButton *nmi = new QPushButton("NMI");
    QPushButton *io0 = new QPushButton("IO0");
    hl->addWidget(nmi);
    hl->addWidget(io0);

    l->addLayout(hl);
    l->addWidget(spectrumWidget);

    edit = new QPlainTextEdit();
    l->addWidget(edit);
    edit->setReadOnly(true);
    edit->ensureCursorVisible();

    QObject::connect( logemitter, &LogEmitter::logstring, edit, &QPlainTextEdit::appendPlainText, Qt::QueuedConnection);

    set_logger(&logger);

    mainw->show();

    QString filename;

    filename = ":/rom/spectrum.rom";

    printf("Using rom %s\n", filename.toLatin1().constData());
    QFile file(filename);

    printf("Init pallete\n");
    init_pallete();
    
    printf("Init emul\n");
    init_emul();

    QByteArray data;
    const char *p;
    int i;

    if(file.open(QIODevice::ReadOnly)){
        data=file.readAll();
        file.close();
        p=data;
        for (i=0; i < 16384 ; i++)
            *(mem+i) = *(p++);
    } else {
        printf("Cannot open ROM file\n");
        return -1;
    }

    iz = InterfaceZ::get();

    if (iz->init()<0) {
        fprintf(stderr,"Cannot init InterfaceZ\n");
        return -1;
    }

    iz->linkGPIO(nmi, 34);
    iz->linkGPIO(io0, 0);

    iz->setCommsSocket(sock);

    QObject::connect( spectrumWidget, &SpectrumWidget::NMI, iz, &InterfaceZ::onNMI);

    spectrumWidget->show();
    return 0;
}


int main(int argc, char**argv)
{
    int sockets[2];
    QApplication app(argc, argv);
    IZThread *iz = new IZThread();

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, &sockets[0])<0)
        return -1;

    fpga_set_comms_socket(sockets[0]);

    setupgui(sockets[1]);
    usleep(1000000);
    iz->start();

    //iz->wait();
    return app.exec();
};
