#ifndef COLLABSESSION_H
#define COLLABSESSION_H

#include "mainwindow.h"
#include <QString>

#define COMMAND_BLOCKS_CHANGED 0x1

#define SERVER_MESSAGE_CREATED_SESSION   0x1
#define SERVER_MESSAGE_JOINED_SESSION    0x2
#define SERVER_MESSAGE_BROADCAST_COMMAND 0x3

class CollabSession
{
public:
    CollabSession(MainWindow *mainWindow);
    static void init(MainWindow *mainWindow);
    static bool connect(QString host, int port);
    static void createSession(QString sessionName);
    static void joinSession(QString sessionName);
    static QByteArray prepareSocketMessage(QByteArray message, int messageType);
    static void onBlockChanged(int x, int y, Block prevBlock, Block newBlock);
    void processMessage();
    MainWindow *mainWindow = nullptr;
private:
    static void handleBlocksChangedCommand(QByteArray message);
    QTcpSocket *socket = nullptr;
    QByteArray messageBuffer;
};

#endif // COLLABSESSION_H
