#ifndef UNLOCKABLEICON_H
#define UNLOCKABLEICON_H

// Manages an icon loaded from an obfuscated data file containing the icon's image data and a key.
// The icon can only be accessed by inputting the correct key.

#include <QObject>
#include <QIcon>
#include <QString>
#include <QSet>

class UnlockableIcon : public QObject
{
    Q_OBJECT
public:
    UnlockableIcon(QObject* parent = nullptr);
    UnlockableIcon(const QString& dataFilepath, QObject* parent = nullptr);
    ~UnlockableIcon() {};

    // Create the obfuscated data file to load an unlockable icon from.
    // Normally unused, this is only needed to update the resource data file.
    static bool createDataFile(const QString& inputFilepath, const QString& outputFilepath, const QString& key);

    bool load(const QString& dataFilepath);
    void clear();

    // Try to unlock the icon by matching the next character in the key.
    // Progress resets if the character is not a match.
    void tryUnlock(const QChar& c);

    // Try to unlock the icon by matching the next character in the key.
    // Progress resets if none of the characters in the set are a match.
    void tryUnlock(const QSet<QChar>& cSet);

    // Try to unlock the icon by matching the remaining characters in the key.
    // Progress resets if any character in the string is not a match.
    void tryUnlock(const QString& key);

    bool isUnlocked() const;
    QIcon icon() const;

signals:
    void unlocked(const QIcon& icon);

private:
    QIcon m_icon;
    QString m_key;
    quint32 m_keyIndex = 0;
    bool m_loaded = false;

    bool canUnlock() const;
    bool tryKeyMatch(const QSet<QChar>& cSet);
};



#endif // UNLOCKABLEICON_H
