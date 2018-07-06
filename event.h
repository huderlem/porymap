#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QPixmap>
#include <QMap>

class EventType
{
public:
    static QString Object;
    static QString Warp;
    static QString CoordScript;
    static QString CoordWeather;
    static QString Sign;
    static QString HiddenItem;
    static QString SecretBase;
};

class Event
{
public:
    Event();
public:
    int x() {
        return getInt("x");
    }
    int y() {
        return getInt("y");
    }
    int elevation() {
        return getInt("elevation");
    }
    void setX(int x) {
        put("x", x);
    }
    void setY(int y) {
        put("y", y);
    }
    QString get(QString key) {
        return values.value(key);
    }
    int getInt(QString key) {
        return values.value(key).toInt(nullptr, 0);
    }
    void put(QString key, int value) {
        put(key, QString("%1").arg(value));
    }
    void put(QString key, QString value) {
        values.insert(key, value);
    }

    QMap<QString, QString> values;
    QPixmap pixmap;
};

#endif // EVENT_H
