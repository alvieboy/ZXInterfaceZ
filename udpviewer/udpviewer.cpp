#include <QApplication>
#include "MainWindow.h"
#include "keycapturer.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow *w = new MainWindow(argc>1 ? argv[1]: "192.168.120.1");

    KeyCapturer *keycapture = new KeyCapturer();
    app.installEventFilter(keycapture);
    QObject::connect(keycapture, &KeyCapturer::keyChanged, w, &MainWindow::onKeyChanged);

    w->show();
    app.exec();
}
