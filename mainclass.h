#ifndef MAINCLASS_H
#define MAINCLASS_H

#include <QObject>
#include <QCoreApplication>
#include "wsoc.h"

class MainClass : public QObject
{
    Q_OBJECT
private:
    QCoreApplication *app;
    WSoc *ws_WS;
private slots:
    void receiveMessage(const QString &sMessage);
public:
    explicit MainClass(QObject *parent = nullptr);

    ~MainClass();
    /////////////////////////////////////////////////////////////
    /// Call this to quit application
    /////////////////////////////////////////////////////////////
    void quit();

signals:
    /////////////////////////////////////////////////////////////
    /// Signal to finish, this is connected to Application Quit
    /////////////////////////////////////////////////////////////
    void finished();

public slots:
    /////////////////////////////////////////////////////////////
    /// This is the slot that gets called from main to start everything
    /// but, everthing is set up in the Constructor
    /////////////////////////////////////////////////////////////
    void run();

    /////////////////////////////////////////////////////////////
    /// slot that get signal when that application is about to quit
    /////////////////////////////////////////////////////////////
    void aboutToQuitApp();

signals:

};

#endif // MAINCLASS_H
