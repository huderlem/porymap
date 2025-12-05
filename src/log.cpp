#include "log.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QSysInfo>
#include <QLabel>
#include <QPointer>
#include <QTimer>

namespace Log {
    static QString mostRecentError;
    static QString path;
    static QFile file;
    static QTextStream textStream;
    static bool initialized = false;

    struct Display {
        QPointer<QStatusBar> statusBar;
        QPointer<QLabel> message;
        QPointer<QLabel> icon;
        QSet<LogType> acceptedTypes;
    };
    static QList<Display> displays;
    static QTimer displayClearTimer;
};

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

void logInfo(const QString &message) {
    log(message, LogType::LOG_INFO);
}

void logWarn(const QString &message) {
    log(message, LogType::LOG_WARN);
}

void logError(const QString &message) {
    Log::mostRecentError = message;
    log(message, LogType::LOG_ERROR);
}

QString colorizeMessage(const QString &message, LogType type) {
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

void addLogStatusBar(QStatusBar *statusBar, const QSet<LogType> &acceptedTypes) {
    if (!statusBar) return;

    static const QSet<LogType> allTypes = {LOG_ERROR, LOG_WARN, LOG_INFO};

    Log::Display display = {
        .statusBar = statusBar,
        .message = new QLabel(statusBar),
        .icon = new QLabel(statusBar),
        .acceptedTypes = acceptedTypes.isEmpty() ? allTypes : acceptedTypes,
    };
    statusBar->addWidget(display.icon);
    statusBar->addWidget(display.message);
    Log::displays.append(display);
}

void removeLogStatusBar(int index) {
    Log::Display display = Log::displays.takeAt(index);
    display.statusBar->removeWidget(display.icon);
    display.statusBar->removeWidget(display.message);
    delete display.icon;
    delete display.message;
}

bool removeLogStatusBar(QStatusBar *statusBar) {
    if (!statusBar) return false;

    for (int i = 0; i < Log::displays.length(); i++) {
        if (Log::displays.at(i).statusBar == statusBar) {
            removeLogStatusBar(i);
            return true;
        }
    }
    return false;
}

void pruneLogDisplays() {
    auto it = QMutableListIterator<Log::Display>(Log::displays);
    while (it.hasNext()) {
        auto display = it.next();
        if (!display.statusBar) {
            // Status bar was deleted externally, remove entry from the list.
            it.remove();
        }
    }
}

void updateLogDisplays(const QString &message, LogType type) {
    static const QMap<LogType, QPixmap> icons = {
        {LogType::LOG_INFO,  QPixmap(QStringLiteral(":/icons/information.ico"))},
        {LogType::LOG_WARN,  QPixmap(QStringLiteral(":/icons/warning.ico"))},
        {LogType::LOG_ERROR, QPixmap(QStringLiteral(":/icons/error.ico"))},
    };

    pruneLogDisplays();
    bool startTimer = false;
    for (const auto &display : Log::displays) {
        // Update the display, but only if it accepts this message type.
        if (display.acceptedTypes.contains(type)) {
            display.icon->setPixmap(icons.value(type));
            display.statusBar->clearMessage();
            display.message->setText(message);
            startTimer = true;
        }
    }

    // Auto-hide status bar messages after a set period of time
    if (startTimer) Log::displayClearTimer.start(5000);
}

void clearLogDisplays() {
    pruneLogDisplays();
    for (const auto &display : Log::displays) {
        display.icon->setPixmap(QPixmap());
        display.message->setText(QString());
    }
}

void log(const QString &message, LogType type) {
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString typeString = "";
    switch (type)
    {
    case LogType::LOG_INFO:
        typeString = QStringLiteral(" [INFO]");
        break;
    case LogType::LOG_WARN:
        typeString = QStringLiteral(" [WARN]");
        break;
    case LogType::LOG_ERROR:
        typeString = QStringLiteral("[ERROR]");
        break;
    }

    QString fullMessage = QString("%1 %2 %3").arg(now).arg(typeString).arg(message);

    qDebug().noquote() << colorizeMessage(fullMessage, type);

    if (!Log::initialized) {
        return;
    }

    updateLogDisplays(message, type);

    Log::textStream << fullMessage << Qt::endl;
    Log::file.flush();
}

QString getLogPath() {
    return Log::path;
}

QString getMostRecentError() {
    return Log::mostRecentError;
}

bool cleanupLargeLog() {
    return Log::file.size() >= 20000000 && Log::file.resize(0);
}

void logInit() {
    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(settingsPath);
    if (!dir.exists())
        dir.mkpath(settingsPath);
    Log::path = dir.absoluteFilePath(QStringLiteral("porymap.log"));
    Log::file.setFileName(Log::path);
    if (!Log::file.open(QIODevice::WriteOnly | QIODevice::Append)) return;
    Log::textStream.setDevice(&Log::file);

    QObject::connect(&Log::displayClearTimer, &QTimer::timeout, [=] {
        clearLogDisplays();
    });

    Log::initialized = true;

    if (cleanupLargeLog()) {
        logWarn(QString("Previous log file %1 was cleared due to being over 20MB in size.").arg(Log::path));
    }
}
