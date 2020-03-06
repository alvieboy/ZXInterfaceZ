#include <QApplication>
#include "MainWindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    MainWindow *w = new MainWindow();

    w->show();
    app.exec();
}
