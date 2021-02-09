#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    QApplication a(argc, argv);
    a.setStyle("fusion");
    MainWindow w(nullptr);
    w.show();

    return a.exec();
}
