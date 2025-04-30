#include "mainwindow.h"
#include "loadingscreen.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Round);
    QApplication a(argc, argv);
    a.setStyle("fusion");

    porysplash = new PorymapLoadingScreen;

    QObject::connect(&a, &QCoreApplication::aboutToQuit, [=]() { delete porysplash; });

    MainWindow w(nullptr);
    w.initialize();

    return a.exec();
}
