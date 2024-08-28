#ifndef NETWORK_H
#define NETWORK_H

/*
    The two classes defined here provide a simplified interface for Qt's network classes QNetworkAccessManager and QNetworkReply.

    With the Qt classes, the workflow for a GET is roughly: generate a QNetworkRequest, give this request to QNetworkAccessManager::get,
    connect the returned object to QNetworkReply::finished, and in the slot of that connection handle the various HTTP headers and attributes,
    then manage errors or process the webpage's body.

    These classes handle generating the QNetworkRequest with a given URL and manage the HTTP headers in the reply. They will automatically
    respect rate limits and return cached data if the webpage hasn't changed since previous requests. Instead of interacting with a QNetworkReply,
    callers interact with a simplified NetworkReplyData.
    Example that logs Porymap's description on GitHub:

    NetworkAccessManager * manager = new NetworkAccessManager(this);
    NetworkReplyData * reply = manager->get("https://api.github.com/repos/huderlem/porymap");
    connect(reply, &NetworkReplyData::finished, [reply] () {
        if (!reply->errorString().isEmpty()) {
            logError(QString("Failed to read description: %1").arg(reply->errorString()));
        } else {
            auto webpage = QJsonDocument::fromJson(reply->body());
            logInfo(QString("Porymap: %1").arg(webpage["description"].toString()));
        }
        reply->deleteLater();
    });
*/

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

class NetworkReplyData : public QObject
{
    Q_OBJECT

public:
    QUrl url() const { return m_url; }
    QUrl nextUrl() const { return m_nextUrl; }
    QByteArray body() const { return m_body; }
    QString errorString() const { return m_error; }
    QDateTime retryAfter() const { return m_retryAfter; }
    bool isFinished() const { return m_finished; }

    friend class NetworkAccessManager;

private:
    QUrl m_url;
    QUrl m_nextUrl;
    QByteArray m_body;
    QString m_error;
    QDateTime m_retryAfter;
    bool m_finished;

    void finish() {
        m_finished = true;
        emit finished();
    };

signals:
    void finished();
};

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    NetworkAccessManager(QObject * parent = nullptr);
    ~NetworkAccessManager();
    NetworkReplyData * get(const QString &url);
    NetworkReplyData * get(const QUrl &url);

private:
    // For a more complex cache we could implement a QAbstractCache for the manager
    struct CacheEntry {
        QString eTag;
        QByteArray data;
    };
    QMap<QUrl, CacheEntry*> cache;
    QMap<QUrl, QDateTime> rateLimitTimes;
    void processReply(QNetworkReply * reply, NetworkReplyData * data);
    const QNetworkRequest getRequest(const QUrl &url);
};

#endif // NETWORK_H
