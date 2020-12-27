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
#include "text.h"

extern "C" int fpga_set_comms_socket(int socket);
extern "C" int interfacez_main(int,char**);
extern "C" void init_pallete();
extern "C" void init_emul();
extern "C" void set_logger(void (*)(int level, const char *tag, char *fmt, va_list ap));

class HTMLLogger: public QObject
{
public:
    HTMLLogger(QPlainTextEdit *e): m_text(e)
    {
    }
public slots:
    void log(int level, QString str);
private:
    QPlainTextEdit *m_text;
};


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
static HTMLLogger *htmllogger;

static void logger(int level, const char *tag, char *fmt, va_list ap)
{
    logemitter->log(level, tag, fmt, ap);
}

void LogEmitter::log(int level, const char *tag, char *fmt, va_list ap)
{
    char line[512];
    vsprintf(line, fmt, ap);
    // Remove newlines
    chomp(line);
    printf("%s\n", line);
    emit logstring(level, QString(line));
}

extern "C" void ansi_get_stylesheet(char *dest);

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

    char style[1024];
    ansi_get_stylesheet(style);

    edit->setStyleSheet(style);
    //printf("Style: %s\n", style);

    htmllogger = new HTMLLogger(edit);


    QObject::connect( logemitter,
                     &LogEmitter::logstring,
                     htmllogger,
                     &HTMLLogger::log,
                     Qt::QueuedConnection);

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

extern "C" int ansi_convert_line_to_html(const char *input, char *output, unsigned maxlen);


void HTMLLogger::log(int level, QString str)
{
    char out[4096];
    ansi_convert_line_to_html(str.toLocal8Bit().constData(), out, 4096);
    m_text->appendHtml(out);
}
