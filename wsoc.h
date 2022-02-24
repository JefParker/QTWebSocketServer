#ifndef WSOC_H
#define WSOC_H

#include <QObject>
#include <QWebSocketServer>
#include <QtWebSockets>
#include <QJsonDocument>
#include <QDateTime>

class WSocExtra {
public:
    QWebSocket *p_ws;
    qint64 nID;
    qint64 nGameID;
    qint64 nLogOnTime;
    QString sType;
    QString sName;
    QString sUserName;
    QString sPeerIP;
};


class WSoc : public QWebSocketServer {
    Q_OBJECT
public:
    explicit WSoc(QObject *parent = nullptr);

signals:
    void receiveMessage(QString);
private slots:
    void newWSConnection();
    void onMessageReceived(const QString &sMessage);

    QJsonObject MakeWhoAmI(QJsonObject &objData, int nIndex);
    void WhoAmI(const QString &sMessage);
    void SetGameID(const QString &sMessage);
    void socketDisconnected();
    void socketCleanUp(int nIndex);
    void AssociateID(const QString &sMessage);
    void Broadcast2Game(const QString &sMessage, QString sType, int nGameID);
    void Msg2ID(const QString &sMessage, QString sType, int nGameID);
    QString ListAllUsers();
    QString ListAllUsersDetail();
    QString ListAllUsersNames();
    QString ListNamesInGame(QString sType, int nGame);
    QString ListIDsInGameAsString(QString sType, int nGame);

private:
    QVector<WSocExtra*> socketVectorExtra;
    int m_nNextDefaultID = 0;
};

#endif // WSOC_H
