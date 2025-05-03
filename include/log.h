#pragma once
#ifndef LOG_H
#define LOG_H

#include <QApplication>
#include <QtDebug>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QDebug>
#include <QStatusBar>

enum LogType {
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
};

void logInit();
void logInfo(const QString &message);
void logWarn(const QString &message);
void logError(const QString &message);
void log(const QString &message, LogType type);
QString getLogPath();
QString getMostRecentError();
void addLogStatusBar(QStatusBar *statusBar, const QSet<LogType> &types = {});
bool removeLogStatusBar(QStatusBar *statusBar);

#endif // LOG_H
