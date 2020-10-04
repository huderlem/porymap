#include "collabsession.h"
#include "log.h"

CollabSession::CollabSession(MainWindow *mainWindow) {
    this->mainWindow = mainWindow;
}

CollabSession *collabInstance = nullptr;

void CollabSession::init(MainWindow *mainWindow) {
    if (collabInstance) {
        delete collabInstance;
    }
    collabInstance = new CollabSession(mainWindow);
}

bool CollabSession::connect(QString host, int port) {
    if (!collabInstance) return false;

    collabInstance->socket = new QTcpSocket();
    collabInstance->socket->connectToHost(host, port);

    QObject::connect(collabInstance->socket, &QTcpSocket::connected, [=](){
        logInfo("Socket connected to collab server...");
    });
    QObject::connect(collabInstance->socket, &QTcpSocket::disconnected, [=](){
        logInfo("Socket disconnected from collab server...");
    });
    QObject::connect(collabInstance->socket, &QTcpSocket::readyRead, [=](){
        collabInstance->processMessage();
    });

    if (!collabInstance->socket->waitForConnected(10000)) {
        logError(collabInstance->socket->errorString());
        delete collabInstance->socket;
        collabInstance->socket = nullptr;
        return false;
    }

    return true;
}

void CollabSession::processMessage() {
    QByteArray newContent = collabInstance->socket->readAll();
    this->messageBuffer.append(newContent);
    // Process all the available messages in the read buffer.
    while (true) {
        if (this->messageBuffer.size() < 12) {
            break;
        }

        uint32_t signature = this->messageBuffer.at(0) |
                            (this->messageBuffer.at(1) << 8) |
                            (this->messageBuffer.at(2) << 16) |
                            (this->messageBuffer.at(3) << 24);
        if (signature != 0x98765432) {
            // Invalid signature from server. Unclear how to recover--just disconnect.
            logError("Invalid message signature received from collab server. Disconnecting...");
            this->socket->disconnectFromHost();
            delete this->socket;
            this->socket = nullptr;
            break;
        }

        uint32_t payloadSize = this->messageBuffer.at(4) |
                (this->messageBuffer.at(5) << 8) |
                (this->messageBuffer.at(6) << 16) |
                (this->messageBuffer.at(7) << 24);
        int messageSize = 12 + payloadSize;
        if (this->messageBuffer.size() < messageSize) {
            break;
        }

        uint32_t messageType = this->messageBuffer.at(8) |
                (this->messageBuffer.at(9) << 8) |
                (this->messageBuffer.at(10) << 16) |
                (this->messageBuffer.at(11) << 24);

        QByteArray message = this->messageBuffer.left(messageSize);
        message.remove(0, 12);
        this->messageBuffer = this->messageBuffer.remove(0, messageSize);
        if (message.size() < 2) {
            break;
        }

        switch (messageType) {
            case SERVER_MESSAGE_BROADCAST_COMMAND:
                int commandType = (message.at(1) << 8) | message.at(0);
                switch (commandType) {
                    case COMMAND_BLOCKS_CHANGED:
                        this->handleBlocksChangedCommand(message);
                        break;
                }
                break;
        }
    }
}

void CollabSession::createSession(QString sessionName) {
    QByteArray msg = prepareSocketMessage(sessionName.toLocal8Bit(), 0x1);
    logInfo(QString("Joining session: %1").arg(collabInstance->socket->write(QByteArray(msg))));
}

void CollabSession::joinSession(QString sessionName) {
    QByteArray msg = prepareSocketMessage(sessionName.toLocal8Bit(), 0x2);
    logInfo(QString("Joining session: %1").arg(collabInstance->socket->write(QByteArray(msg))));
}

QByteArray CollabSession::prepareSocketMessage(QByteArray message, int messageType) {
    QByteArray header = QByteArray();
    // 4-byte little endian signature is 0x12345678.
    header.append(0x78);
    header.append(0x56);
    header.append(0x34);
    header.append(0x12);
    // 4-byte little endian payload size.
    header.append(message.size() & 0xff);
    header.append((message.size() >> 8) & 0xff);
    header.append((message.size() >> 16) & 0xff);
    header.append((message.size() >> 24) & 0xff);
    // 4-byte little endian message type.
    header.append(messageType & 0xff);
    header.append((messageType >> 8) & 0xff);
    header.append((messageType >> 16) & 0xff);
    header.append((messageType >> 24) & 0xff);
    // Append payload.
    return header + message;
}

void CollabSession::handleBlocksChangedCommand(QByteArray message) {
    uint16_t numBlocks = (message.at(3) << 8) | message.at(2);
    if (message.size() != 4 + numBlocks * 6)
        return;

    for (int i = 0; i < numBlocks; i++) {
        int index = 4 + i * 6;
        int x = (uint8_t(message.at(index + 1)) << 8) | uint8_t(message.at(index));
        int y = (uint8_t(message.at(index + 3)) << 8) | uint8_t(message.at(index + 2));
        uint16_t metatileId = (uint8_t(message.at(index + 5)) << 8) | uint8_t(message.at(index + 4));
        collabInstance->mainWindow->setBlock(x, y, metatileId, 0, 0, true, false);
    }
}

void CollabSession::onBlockChanged(int x, int y, Block prevBlock, Block newBlock) {
    if (!collabInstance || !collabInstance->socket) return;
    if (prevBlock.rawValue() == newBlock.rawValue()) return;

    QByteArray message;
    message.append(COMMAND_BLOCKS_CHANGED & 0xFF);
    message.append((COMMAND_BLOCKS_CHANGED >> 8) & 0xFF);
    message.append(0x1);
    message.append('\0');
    message.append(x & 0xFF);
    message.append((x >> 8) & 0xFF);
    message.append(y & 0xFF);
    message.append((y >> 8) & 0xFF);
    message.append(newBlock.tile & 0xFF);
    message.append((newBlock.tile >> 8) & 0xFF);
    QByteArray msg = CollabSession::prepareSocketMessage(message, 0x3);
    collabInstance->socket->write(QByteArray(msg));
}
