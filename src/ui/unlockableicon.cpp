#include "unlockableicon.h"
#include <QFile>
#include <QBuffer>
#include <QRandomGenerator>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 8, 0))
constexpr int Version = QDataStream::Qt_6_8;
#endif

UnlockableIcon::UnlockableIcon(QObject* parent) : QObject(parent) {};
UnlockableIcon::UnlockableIcon(const QString& dataFilepath, QObject* parent) : UnlockableIcon(parent) {
    load(dataFilepath);
};

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
bool UnlockableIcon::createDataFile(const QString&, const QString&, const QString&) { return false; }
#else
bool UnlockableIcon::createDataFile(const QString& inputFilepath, const QString& outputFilepath, const QString& key) {
    if (inputFilepath.isEmpty() || outputFilepath.isEmpty() || key.isEmpty()) return false;
    if (key.length() >= std::numeric_limits<quint8>::max()) return false;

    QByteArray key64 = key.toUtf8().toBase64();
    if (key64.length() >= std::numeric_limits<quint8>::max()) return false;

    QImage iconImage(inputFilepath);
    if (iconImage.isNull()) return false;

    QByteArray iconData;
    QBuffer buffer(&iconData);
    buffer.open(QIODevice::WriteOnly);
    iconImage.save(&buffer, "PNG");
    buffer.close();
    if (iconData.length() >= std::numeric_limits<quint16>::max()) return false;

    QByteArray iconData64 = iconData.toBase64();
    if (iconData64.length() >= std::numeric_limits<quint16>::max()) return false;

    QFile file(outputFilepath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&file);
    out.setVersion(Version);

    quint8 r = QRandomGenerator::global()->bounded(std::numeric_limits<quint8>::max());

    out << r;
    out << static_cast<quint8>(key.length() ^ r);
    out << static_cast<quint8>(key64.length() ^ r);
    for (const auto& byte : key64) out << static_cast<quint8>(byte ^ r);

    out << static_cast<quint16>(iconData.length() ^ (r | (r << 8)));
    out << static_cast<quint16>(iconData64.length() ^ (r | (r << 8)));
    for (const auto& byte : iconData64) out << static_cast<quint8>(byte ^ r);

    file.close();
    return true;
}
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
bool UnlockableIcon::load(const QString&) { return false; }
#else
bool UnlockableIcon::load(const QString& dataFilepath) {
    clear();

    QFile file(dataFilepath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream in(&file);
    in.setVersion(Version);

    quint8 r = 0;
    in >> r;

    quint8 keyLength = 0;
    in >> keyLength;
    keyLength ^= r;
    if (keyLength == 0) return false;

    quint8 key64Length = 0;
    in >> key64Length;
    key64Length ^= r;
    if (key64Length == 0) return false;

    QByteArray key64(key64Length,0);
    for (quint8 i = 0; i < key64Length; i++) {
        in >> key64[i];
        key64[i] ^= r;
    }
    QString key = QString(QByteArray::fromBase64(key64));
    if (key.length() != keyLength) return false;

    quint16 iconDataLength = 0;
    in >> iconDataLength;
    iconDataLength ^= (r | r << 8);
    if (iconDataLength == 0) return false;

    quint16 iconData64Length = 0;
    in >> iconData64Length;
    iconData64Length ^= (r | r << 8);
    if (iconData64Length == 0) return false;

    QByteArray iconData64(iconData64Length,0);
    for (quint16 i = 0; i < iconData64Length; i++) {
        in >> iconData64[i];
        iconData64[i] ^= r;
    }
    QByteArray iconData = QByteArray::fromBase64(iconData64);
    if (iconData.length() != iconDataLength) return false;

    QPixmap iconPixmap;
    iconPixmap.loadFromData(iconData);
    if (iconPixmap.isNull()) return false;

    m_icon = QIcon(iconPixmap);
    m_key = key;
    m_loaded = true;
    return true;
}
#endif

void UnlockableIcon::clear() {
    m_icon = QIcon();
    m_key = QString();
    m_keyIndex = 0;
    m_loaded = false;
}


#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
bool UnlockableIcon::isUnlocked() const { return false; }
#else
bool UnlockableIcon::isUnlocked() const {
    return m_loaded && m_keyIndex >= m_key.length();
}
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
bool UnlockableIcon::canUnlock() const { return false; }
#else
bool UnlockableIcon::canUnlock() const {
    return m_loaded && m_keyIndex < m_key.length();
}
#endif

QIcon UnlockableIcon::icon() const {
    return isUnlocked() ? m_icon : QIcon();
}

#if (QT_VERSION < QT_VERSION_CHECK(6, 8, 0))
bool UnlockableIcon::tryKeyMatch(const QSet<QChar>&) { return false; }
#else
bool UnlockableIcon::tryKeyMatch(const QSet<QChar>& cSet) {
    if (m_keyIndex >= m_key.length()) return false;
    if (!cSet.contains(m_key.at(m_keyIndex))) {
        m_keyIndex = 0;
        return false;
    }
    if (++m_keyIndex == m_key.length()) {
        emit unlocked(m_icon);
    }
    return true;
}
#endif

void UnlockableIcon::tryUnlock(const QSet<QChar>& cSet) {
    if (canUnlock()) tryKeyMatch(cSet);
}

void UnlockableIcon::tryUnlock(const QChar& c) {
    tryUnlock(QSet<QChar>{c});
}

void UnlockableIcon::tryUnlock(const QString& key) {
    if (!canUnlock()) return;
    for (const QChar& c : key) {
        if (!tryKeyMatch(QSet<QChar>{c})) return;
    }
}
