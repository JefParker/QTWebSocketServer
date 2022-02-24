#include "wsoc.h"


WSoc::WSoc(QObject *parent): QWebSocketServer("WebSocServer", QWebSocketServer::SecureMode, parent) {
    connect(this, SIGNAL(newConnection()), this, SLOT(newWSConnection()));
}

void WSoc::newWSConnection() {
    WSocExtra *p_WSE = new WSocExtra;
    p_WSE->nID = 0;
    p_WSE->p_ws = nextPendingConnection();
    p_WSE->sPeerIP = p_WSE->p_ws->peerAddress().toString();
    socketVectorExtra.append(p_WSE);
    connect(p_WSE->p_ws, SIGNAL(textMessageReceived(QString)), this, SLOT(onMessageReceived(QString)));
    connect(p_WSE->p_ws, SIGNAL(disconnected()), this, SLOT(socketDisconnected()));
    qDebug() << "WSoc::newWSConnection " << socketVectorExtra.constLast()->p_ws;
}

void WSoc::onMessageReceived(const QString &sMessage) {
    emit receiveMessage(sMessage);
    QJsonDocument jDoc = QJsonDocument::fromJson(sMessage.toUtf8());
    QJsonObject objMsg = jDoc.object();
    QString sType = objMsg["Type"].toString();
    QString sMsg = objMsg["Message"].toString();
    int nGameID = objMsg["GameID"].toInt();

    if ("BCast2Game" == sMsg)
        Broadcast2Game(sMessage, sType, nGameID);
    else if ("Msg2ID" == sMsg)
        Msg2ID(sMessage, sType, nGameID);
    else if ("MyID" == sMsg)
        AssociateID(sMessage);
    else if ("SetGameID" == sMsg)
        SetGameID(sMessage);
    else if ("WhoAmI" == sMsg)
        WhoAmI(sMessage);
}

void WSoc::AssociateID(const QString &sMessage) {
    QJsonDocument jDoc = QJsonDocument::fromJson(sMessage.toUtf8());
    QJsonObject objMsg = jDoc.object();
    int nGameID = objMsg["GameID"].toInt();
    int nID = objMsg["ID"].toInt();
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WSocExtra *s;
    foreach(s, socketVectorExtra) {  // Match the sender with the database ID
        if (pClient == s->p_ws) {
            s->nID = nID == 0 ? ++m_nNextDefaultID : nID;
            s->sType = objMsg["Type"].toString();
            s->sName = objMsg["Name"].toString();
            s->nGameID = nGameID;
            s->sUserName = objMsg["UserName"].toString();
            s->nLogOnTime = QDateTime::currentDateTime().toMSecsSinceEpoch();

            QTime Now = QTime::currentTime();

            int nIndex = socketVectorExtra.indexOf(s);
            objMsg = MakeWhoAmI(objMsg, nIndex);
            objMsg["Announcement"] = s->sName + " is entering "+s->sType+" (" + Now.toString("h:mm:ss.zzz AP") + ").";

            QJsonDocument jsonArrivalData;
            jsonArrivalData.setObject(objMsg);
            s->p_ws->sendTextMessage(jsonArrivalData.toJson()); // Send to me

            objMsg["Message"] = "PlayerEnteringGame";
            jsonArrivalData.setObject(objMsg);
            Broadcast2Game(jsonArrivalData.toJson(), s->sType, s->nGameID); // Send to everybody in the game
            Broadcast2Game(jsonArrivalData.toJson(), "Chat", 0); // Send to the socket monitor

            qDebug() << "";
            qDebug() << "Name: " << s->sName << ".  User Name: " << s->sUserName;
            qDebug() << "User ID: " << s->nID << ".  Game ID: " << s->nGameID;
            qDebug() << "Type: " << s->sType << ".  Socket: " << s->p_ws;
            QDateTime dt(QDateTime::fromMSecsSinceEpoch(s->nLogOnTime, QTimeZone::systemTimeZone()));
            QString sTextDate = dt.toString("ddd, MMMM d, yyyy h:mm:ss.zzz AP");
            qDebug() << "Logged on: " << sTextDate << Qt::endl;
            qDebug() << "";
            break;
        }
    }
}

void WSoc::SetGameID(const QString &sMessage) {
    QJsonDocument jDoc = QJsonDocument::fromJson(sMessage.toUtf8());
    QJsonObject objMsg = jDoc.object();
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WSocExtra *s;
    int nGameID = objMsg["GameID"].toInt();
    int OldGame = 0;
    QString sType = objMsg["Type"].toString();
    QJsonDocument jsonData;
    foreach(s, socketVectorExtra) {
        if (pClient == s->p_ws) { // Send yourself a WhoAmI
            OldGame = s->nGameID;
            s->nGameID = nGameID;
            int nIndex = socketVectorExtra.indexOf(s);
            objMsg = MakeWhoAmI(objMsg, nIndex);
            jsonData.setObject(objMsg);
            s->p_ws->sendTextMessage(jsonData.toJson()); // To yourself
            qDebug() << "SetGameID:  " << nGameID << Qt::endl;
            break;
        }
    }
    // Now broadcast to everyone in the game you're entering that you're here...
    objMsg["Message"] = "PlayerEnteringGame";
    jsonData.setObject(objMsg);
    Broadcast2Game(jsonData.toJson(), sType, nGameID);

    // And say goodbye to everyone in the game you're leaving...
    objMsg["Message"] = "PlayerExitingGame";
    objMsg["PlayersNameList"] = ListNamesInGame(sType, OldGame);
    objMsg["PlayersIDHere"] = ListIDsInGameAsString(sType, OldGame);
    jsonData.setObject(objMsg);
    Broadcast2Game(jsonData.toJson(), sType, OldGame);
}

QJsonObject WSoc::MakeWhoAmI(QJsonObject &objData, int nIndex) {
    objData["Message"] = "WhoAmI";
    objData["ID"] = socketVectorExtra[nIndex]->nID;
    objData["Type"] = socketVectorExtra[nIndex]->sType;
    objData["Name"] = socketVectorExtra[nIndex]->sName;
    objData["UserName"] = socketVectorExtra[nIndex]->sUserName;
    objData["GameID"] = socketVectorExtra[nIndex]->nGameID;
    objData["PeerIP"] = socketVectorExtra[nIndex]->sPeerIP;
    objData["LogOnTime"] = socketVectorExtra[nIndex]->nLogOnTime;
    objData["PlayersNameList"] = ListNamesInGame(socketVectorExtra[nIndex]->sType, socketVectorExtra[nIndex]->nGameID);
    objData["PlayersIDHere"] = ListIDsInGameAsString(socketVectorExtra[nIndex]->sType, socketVectorExtra[nIndex]->nGameID);
    objData["Announcement"] = "";
    objData["ListAllUsersNames"] = ListAllUsersNames();
    objData["ListAllUsersDetail"] = ListAllUsersDetail();
    objData["ListAllUsers"] = ListAllUsers();
    return objData;
}

void WSoc::WhoAmI(const QString &sMessage) {
    QJsonDocument jDoc = QJsonDocument::fromJson(sMessage.toUtf8());
    QJsonObject objData = jDoc.object();
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (pClient == s->p_ws) {
            int nIndex = socketVectorExtra.indexOf(s);
            objData = MakeWhoAmI(objData, nIndex);
            QJsonDocument jsonData;
            jsonData.setObject(objData);
            s->p_ws->sendTextMessage(jsonData.toJson());
            break;
        }
    }
}

void WSoc::Broadcast2Game(const QString &sMessage, QString sType, int nGameID) {
    qDebug() << "Broadcast to game..." << Qt::endl;
    if (socketVectorExtra.isEmpty())
        return;
    QJsonDocument jDoc = QJsonDocument::fromJson(sMessage.toUtf8());
    QJsonObject objMsg = jDoc.object();
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (pClient != s->p_ws) { // Send to everyone but originator.
            if (sType == s->sType && nGameID == s->nGameID) {
                    s->p_ws->sendTextMessage(sMessage); // Everyone of a type (Chat, Yahtzee, Jake...)
            }
        }
    }
}

void WSoc::Msg2ID(const QString &sMessage, QString sType, int nGameID) {
    if (socketVectorExtra.isEmpty())
        return;
    qDebug() << "Send to ID..." << Qt::endl;
    QJsonDocument jDoc = QJsonDocument::fromJson(sMessage.toUtf8());
    QJsonObject objMsg = jDoc.object();
    int nToID = objMsg["ToID"].toInt();
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (pClient != s->p_ws) { // Send to anyone but originator...
            if (sType == s->sType && nGameID == s->nGameID && nToID == s->nID) {
                    s->p_ws->sendTextMessage(sMessage);
            }
        }
    }
}

QString WSoc::ListAllUsers() {
    std::sort(socketVectorExtra.begin(), socketVectorExtra.end(), [](WSocExtra *a, WSocExtra *b){return a->nID < b->nID;});
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    QString sTemp = QString::number(socketVectorExtra.count()) + " Users: ";
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (pClient == s->p_ws)
            sTemp += QString::number(s->nID) + " (" + s->sName + ", me)";
        else {
            sTemp += QString::number(s->nID);
            if (s->sName.length()) {
                sTemp += " (" + s->sName;
                if (s->sType.length())
                    sTemp += ", " + s->sType;
                sTemp += ")";
            }
            else if (s->sType.length())
                sTemp += " (" + s->sType + ")";
        }
        if (socketVectorExtra.indexOf(s) < socketVectorExtra.count() -1)
            sTemp += ", ";
    }
    return sTemp;
}

QString WSoc::ListAllUsersNames() {
    QStringList lNames;
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (s->sName.length()) {
            lNames << s->sName;
        }
    }
    lNames.sort();
    return lNames.join(", ");
}

QString WSoc::ListNamesInGame(QString sType, int nGame) {
    QStringList lNames;
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (sType == s->sType && s->sName.length() > 0 && s->nGameID == nGame)
            lNames << s->sName;
    }
    lNames.sort();
    lNames.removeDuplicates();
    return lNames.join(", ");
}

QString WSoc::ListIDsInGameAsString(QString sType, int nGame) {
    QStringList lIDs;
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (s->sType == sType && s->sName.length() > 0 && s->nGameID == nGame) {
            lIDs << QString::number(s->nID);
        }
    }
    lIDs.sort();
    lIDs.removeDuplicates();
    return lIDs.join(", ");
}

QString WSoc::ListAllUsersDetail() {
    std::sort(socketVectorExtra.begin(), socketVectorExtra.end(), [](WSocExtra *a, WSocExtra *b){return a->nID < b->nID;});
    WSocExtra *s;
    QString sTemp = QString::number(socketVectorExtra.count()) + " Users:<br><br>";
    foreach(s, socketVectorExtra) {
        sTemp += "Name: " + s->sName + "<br>";
        sTemp += "User Name: " + s->sUserName + "<br>";
        sTemp += "ID: " + QString::number(s->nID) + "<br>";
        sTemp += "Peer IP: <a href='https://whatismyipaddress.com/ip/" + s->sPeerIP + "' target='_blank'>" + s->sPeerIP + "</a><br>";
        sTemp += "Type: " + s->sType + "<br>";
        QString sValid = s->p_ws->isValid() ? "True" : "False";
        sTemp += "Is Socket Valid: " + sValid + "<br>";
        QDateTime dt(QDateTime::fromMSecsSinceEpoch(s->nLogOnTime, QTimeZone::systemTimeZone()));
        const QString sTextDate = dt.toString("ddd, MMMM d, yyyy h:mm:ss.zzz AP");
        sTemp += "Logged on: " + sTextDate + "<br>";
        sTemp += "Game ID: " + QString::number(s->nGameID) + "<br><br>";
    }
    return sTemp;
}

void WSoc::socketCleanUp(int nIndex) {
    if (socketVectorExtra.isEmpty())
        return;
    if (0 == socketVectorExtra[nIndex]->sName.length())
        socketVectorExtra[nIndex]->sName = QString::number(socketVectorExtra[nIndex]->nID);
    qDebug() << "Deleting: " << socketVectorExtra[nIndex]->nID << " " << socketVectorExtra[nIndex]->sName << Qt::endl;
    socketVectorExtra.remove(nIndex);
    qDebug() << "Removed: " + QString::number(nIndex) << Qt::endl;
}

void WSoc::socketDisconnected() {
    qDebug() << "YahtzeeSocket::socketDisconnected" << Qt::endl;
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    WSocExtra *s;
    foreach(s, socketVectorExtra) {
        if (pClient == s->p_ws) {
            int nIndex = socketVectorExtra.indexOf(s);
            QJsonObject objBroadcastNames;
            objBroadcastNames = MakeWhoAmI(objBroadcastNames, nIndex);
            socketCleanUp(nIndex);

            // Prepare to say goodbye...
            objBroadcastNames["Message"] = "PlayerExitingGame";
            QTime Now = QTime::currentTime();
            objBroadcastNames["Announcement"] = s->sName + " <a href='https://whatismyipaddress.com/ip/" + s->sPeerIP + "' target='_blank'>has</a> exited "+s->sType+" (" + Now.toString("h:mm:ss.zzz AP") + ").";
            objBroadcastNames["PlayersNameList"] = ListNamesInGame(s->sType, s->nGameID);
            objBroadcastNames["PlayersNameList"] = ListNamesInGame(s->sType, s->nGameID);
            objBroadcastNames["PlayersIDHere"] = ListIDsInGameAsString(s->sType, s->nGameID);
            objBroadcastNames["ListAllUsersNames"] = ListAllUsersNames();
            objBroadcastNames["ListAllUsersDetail"] = ListAllUsersDetail();
            objBroadcastNames["ListAllUsers"] = ListAllUsers();
            QJsonDocument jsonData;
            jsonData.setObject(objBroadcastNames);
            Broadcast2Game(jsonData.toJson(), s->sType, s->nGameID); // Say goodbye to everyone in the game you're leaving
            s->p_ws->sendTextMessage(jsonData.toJson()); // Send a copy to yourself
            Broadcast2Game(jsonData.toJson(), "Chat", 0); // Say goodbye to socket monitor
            break;
        }
    }
    foreach(s, socketVectorExtra) {  // Clear away any zombie sockets
        if (!s->p_ws->isValid())
            socketCleanUp(socketVectorExtra.indexOf(s));
    }
}

// /home/jparker/Downloads/Cron/./WSS
// @reboot /home/jparker/Downloads/Cron/./WSS
