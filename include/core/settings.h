#ifndef GUARD_SETTINGS_H
#define GUARD_SETTINGS_H

#include <QCursor>
#include <QString>
#include <QStringList>
//#include <QSet>
#include <QVariant>
#include <QDebug>

class Settings
{
private:
    QString path_;
    QStringList all_keys_;
    QMap<QString, QVariant> settings_;

public:// TODO: change this son (mappixmapitem.h)
    bool smartPathsEnabled;
    bool betterCursors;
    QCursor mapCursor;

private:
    void add_setting(QString);

public:
    Settings() = default;
    void setValue(QString, QVariant);
    QVariant value(QString);

    QStringList allKeys();
    bool contains(QString);

    void remove(QString);
    void clear();

    void save();
    void load(QString);

    friend QDebug operator<<(QDebug debug, const Settings&);

    QVariant  operator[](QString key) const {return settings_[key];}
    QVariant& operator[](QString key)       {return settings_[key];}// assignment
};

#endif // GUARD_SETTINGS_H
