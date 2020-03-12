#include <QApplication>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow *w = new MainWindow(argc>1 ? argv[1]: "192.168.120.1");

    w->show();
    app.exec();
}
