#ifndef LOG_H
#define LOG_H

#include <QApplication>
#include <QtDebug>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDebug>

enum LogType {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
};

void logInfo(QString message);
void logWarn(QString message);
void logError(QString message);
void log(QString message, LogType type);
QString getLogPath();
QString getMostRecentError();

#endif // LOG_H
