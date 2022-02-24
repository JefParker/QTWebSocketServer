#include "mainclass.h"
#include <QSsl>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslKey>
#include <QDebug>

MainClass::MainClass(QObject *parent)
    : QObject{parent} {

    // get the instance of the main application
    app = QCoreApplication::instance();
    // setup everything here
    // create any global objects
    // setup debug and warning mode
    ws_WS = new WSoc(this);
    qDebug() << ws_WS->errorString();

    quint16 port = 58000; // Secure
    //quint16 port = 57999; // NonSecure

    bool bSSL = QSslSocket::supportsSsl();
    qDebug() << "SSL possible: " << bSSL;

    QSslConfiguration sslConfiguration = QSslConfiguration::defaultConfiguration();

    QFile certFile(QStringLiteral("/etc/letsencrypt/live/cee2018.com/fullchain.pem"));
    QFile keyFile(QStringLiteral("/etc/letsencrypt/live/cee2018.com/privkey.pem"));
    certFile.open(QIODevice::ReadOnly);
    keyFile.open(QIODevice::ReadOnly);
    QSslCertificate certificate(&certFile, QSsl::Pem);
    QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
    certFile.close();
    keyFile.close();
    sslConfiguration.setLocalCertificate(certificate);
    sslConfiguration.setPrivateKey(sslKey);
    sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfiguration.setProtocol(QSsl::SecureProtocols);

    ws_WS->setSslConfiguration(sslConfiguration);

    qDebug() << ws_WS->errorString();

    if (ws_WS->listen(QHostAddress::AnyIPv4, port)) {
        qDebug() << "Yahtzee Server listening on port " << port;
    }

    qDebug() << ws_WS->errorString();

    connect(ws_WS, SIGNAL(receiveMessage(QString)), this, SLOT(receiveMessage(QString)));


}

MainClass::~MainClass() {
    ws_WS->close();
}

// 10ms after the application starts this method will run
// all QT messaging is running at this point so threads, signals and slots
// will all work as expected.
void MainClass::run() {
    // Add your main code here
    qDebug() << "MainClass.Run is executing";
    // you must call quit when complete or the program will stay in the
    // messaging loop
    //quit();
}

void MainClass::receiveMessage(const QString &sMessage) {
    Q_UNUSED(sMessage);
}

// call this routine to quit the application
void MainClass::quit() {
    // you can do some cleanup here
    // then do emit finished to signal CoreApplication to quit
    qDebug() << "Quit called";
    emit finished();
}

// shortly after quit is called the CoreApplication will signal this routine
// this is a good place to delete any objects that were created in the
// constructor and/or to stop any threads
void MainClass::aboutToQuitApp() {
    qDebug() << "About to quit called";
    // stop threads
    // sleep();   // wait for threads to stop.
    // delete any objects
}
