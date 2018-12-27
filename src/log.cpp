#include "log.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

void logInfo(QString message) {
    log(message, LogType::LOG_INFO);
}

void logWarn(QString message) {
    log(message, LogType::LOG_WARN);
}

void logError(QString message) {
    log(message, LogType::LOG_ERROR);
}

void log(QString message, LogType type) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString typeString = "";
    switch (type)
    {
    case LogType::LOG_INFO:
        typeString = " [INFO]";
        break;
    case LogType::LOG_WARN:
        typeString = " [WARN]";
        break;
    case LogType::LOG_ERROR:
        typeString = "[ERROR]";
        break;
    }

    message = QString("%1 %2 %3").arg(now).arg(typeString).arg(message);

    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);

    QString logPath = dir.absoluteFilePath("porymap.log");

    qDebug() << message;
    QFile outFile(logPath);
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << message << endl;
}
