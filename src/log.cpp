#include "log.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QSysInfo>

// Enabling this does not seem to be simple to color console output
// on Windows for all CLIs without external libraries or extreme bloat.
#ifdef Q_OS_WIN
    #define ERROR_COLOR   ""
    #define WARNING_COLOR ""
    #define INFO_COLOR    ""
    #define CLEAR_COLOR   ""
#else
    #define ERROR_COLOR   "\033[31;1m"
    #define WARNING_COLOR "\033[1;33m"
    #define INFO_COLOR    "\033[32m"
    #define CLEAR_COLOR   "\033[0m"
#endif

void logInfo(QString message) {
    log(message, LogType::LOG_INFO);
}

void logWarn(QString message) {
    log(message, LogType::LOG_WARN);
}

static QString mostRecentError;

void logError(QString message) {
    mostRecentError = message;
    log(message, LogType::LOG_ERROR);
}

QString colorizeMessage(QString message, LogType type) {
    QString colorized = message;
    switch (type)
    {
    case LogType::LOG_INFO:
        colorized = colorized.replace("INFO", INFO_COLOR "INFO" CLEAR_COLOR);
        break;
    case LogType::LOG_WARN:
        colorized = colorized.replace("WARN", WARNING_COLOR "WARN" CLEAR_COLOR);
        break;
    case LogType::LOG_ERROR:
        colorized = colorized.replace("ERROR", ERROR_COLOR "ERROR" CLEAR_COLOR);
        break;
    }
    return colorized;
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

    qDebug().noquote() << colorizeMessage(message, type);
    QFile outFile(getLogPath());
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << message << Qt::endl;
}

QString getLogPath() {
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);
    return dir.absoluteFilePath("porymap.log");
}

QString getMostRecentError() {
    return mostRecentError;
}

bool cleanupLargeLog() {
    QFile logFile(getLogPath());
    if (logFile.size() < 20000000)
        return false;

    bool removed = logFile.remove();
    if (removed)
        logWarn(QString("Previous log file %1 was cleared due to being over 20MB in size.").arg(getLogPath()));
    return removed;
}
