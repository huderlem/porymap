#include "network.h"
#include "config.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QTimer>

// Fallback wait time (in seconds) for rate limiting
static const int DefaultWaitTime = 120;

NetworkAccessManager::NetworkAccessManager(QObject * parent) : QNetworkAccessManager(parent) {
    // We store rate limit end times in the user's config so that Porymap will still respect them after a restart.
    // To avoid reading/writing to a local file during network operations, we only read/write the file when the
    // manager is created/destroyed respectively.
    this->rateLimitTimes = porymapConfig.getRateLimitTimes();
    this->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
};

NetworkAccessManager::~NetworkAccessManager() {
    porymapConfig.setRateLimitTimes(this->rateLimitTimes);
    qDeleteAll(this->cache);
}

const QNetworkRequest NetworkAccessManager::getRequest(const QUrl &url) {
    QNetworkRequest request(url);

    // Set User-Agent to porymap/#.#.#
    request.setHeader(QNetworkRequest::UserAgentHeader, QString("%1/%2").arg(QCoreApplication::applicationName())
                                                                        .arg(QCoreApplication::applicationVersion()));

    // If we've made a successful request in this session already, set the If-None-Match header.
    // We'll only get a full response from the server if the data has changed since this last request.
    // This helps to avoid hitting rate limits.
    auto cacheEntry = this->cache.value(url, nullptr);
    if (cacheEntry)
        request.setHeader(QNetworkRequest::IfNoneMatchHeader, cacheEntry->eTag);

    return request;
}

NetworkReplyData * NetworkAccessManager::get(const QString &url) {
    return this->get(QUrl(url));
}

NetworkReplyData * NetworkAccessManager::get(const QUrl &url) {
    NetworkReplyData * data = new NetworkReplyData();
    data->m_url = url;

    // If we are rate-limited, don't send a new request.
    if (this->rateLimitTimes.contains(url)) {
        auto time = this->rateLimitTimes.value(url);
        if (!time.isNull() && time > QDateTime::currentDateTime()) {
            data->m_retryAfter = time;
            data->m_error = QString("Rate limit reached. Please try again after %1.").arg(data->m_retryAfter.toString());
            QTimer::singleShot(1000, data, &NetworkReplyData::finish); // We can't emit this signal before caller has a chance to connect
            return data;
        }
        // Rate limiting expired
        this->rateLimitTimes.remove(url);
    }

    QNetworkReply * reply = QNetworkAccessManager::get(this->getRequest(url));
    connect(reply, &QNetworkReply::finished, [this, reply, data] {
        this->processReply(reply, data);
        data->finish();
    });

    return data;
}

void NetworkAccessManager::processReply(QNetworkReply * reply, NetworkReplyData * data) {
    if (!reply || !reply->isFinished())
        return;

    // The url in the request and the url ultimately processed (reply->url()) may differ if the request was redirected.
    // For identification purposes (e.g. knowing if we are rate limited before a request is made) we use the url that
    // was originally given for the request.
    auto url = data->m_url;

    reply->deleteLater();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // Handle pagination (specifically, the format GitHub uses).
    // This header is still sent for a 304, so we don't need to bother caching it.
    if (reply->hasRawHeader("link")) {
        static const QRegularExpression regex("<(?<url>.+)?>; rel=\"next\"");
        QRegularExpressionMatch match = regex.match(QString(reply->rawHeader("link")));
        if (match.hasMatch())
            data->m_nextUrl = QUrl(match.captured("url"));
    }

    if (statusCode == 304) {
        // "Not Modified", data hasn't changed since our last request.
        auto cacheEntry = this->cache.value(url, nullptr);
        if (cacheEntry)
            data->m_body = cacheEntry->data;
        else
            data->m_error = "Failed to read webpage from cache.";
        return;
    }

    // Handle standard rate limit header
    if (reply->hasRawHeader("retry-after")) {
        auto retryAfter = QVariant(reply->rawHeader("retry-after"));
        if (retryAfter.canConvert<QDateTime>()) {
            data->m_retryAfter = retryAfter.toDateTime().toLocalTime();
        } else if (retryAfter.canConvert<int>()) {
            data->m_retryAfter = QDateTime::currentDateTime().addSecs(retryAfter.toInt());
        }
        if (data->m_retryAfter.isNull() || data->m_retryAfter <= QDateTime::currentDateTime()) {
            data->m_retryAfter = QDateTime::currentDateTime().addSecs(DefaultWaitTime);
        }
        if (statusCode == 429) {
            data->m_error = "Too many requests. ";
        } else if (statusCode == 503) {
            data->m_error = "Service busy or unavailable. ";
        }
        data->m_error.append(QString("Please try again after %1.").arg(data->m_retryAfter.toString()));
        this->rateLimitTimes.insert(url, data->m_retryAfter);
        return;
    }

    // Handle GitHub's rate limit headers. As of writing this is (without authentication) 60 requests per IP address per hour.
    bool ok;
    int limitRemaining = reply->rawHeader("x-ratelimit-remaining").toInt(&ok);
    if (ok && limitRemaining <= 0) {
        auto limitReset = reply->rawHeader("x-ratelimit-reset").toLongLong(&ok);
        data->m_retryAfter = ok ? QDateTime::fromSecsSinceEpoch(limitReset).toLocalTime()
                                : QDateTime::currentDateTime().addSecs(DefaultWaitTime);;
        data->m_error = QString("Too many requests. Please try again after %1.").arg(data->m_retryAfter.toString());
        this->rateLimitTimes.insert(url, data->m_retryAfter);
        return;
    }

    // Handle remaining errors generically
    auto error = reply->error();
    if (error != QNetworkReply::NoError) {
        data->m_error = reply->errorString();
        return;
    }

    // Successful reply, we've received new data. Insert this data in the cache.
    CacheEntry * cacheEntry = this->cache.value(url, nullptr);
    if (!cacheEntry) {
        cacheEntry = new CacheEntry;
        this->cache.insert(url, cacheEntry);
    }
    auto eTagHeader = reply->header(QNetworkRequest::ETagHeader);
    if (eTagHeader.isValid())
        cacheEntry->eTag = eTagHeader.toString();

    cacheEntry->data = data->m_body = reply->readAll();
}
