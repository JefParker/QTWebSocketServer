#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QDateTime>
#include <QWebSocketServer>
#include <QtWebSockets/QtWebSockets>
#include "mainclass.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // create the main class
    MainClass myMain;

    // connect up the signals
    QObject::connect(&myMain, SIGNAL(finished()),
        &a, SLOT(quit()));
    QObject::connect(&a, SIGNAL(aboutToQuit()),
        &myMain, SLOT(aboutToQuitApp()));

    // This code will start the messaging engine in QT and in
    // 10ms it will start the execution in the MainClass.run routine;
    QTimer::singleShot(10, &myMain, SLOT(run()));

    return a.exec();
}
