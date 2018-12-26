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
    static QString Trigger;
    static QString WeatherTrigger;
    static QString Sign;
    static QString HiddenItem;
    static QString SecretBase;
    static QString HealLocation;
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
    uint16_t getU16(QString key) {
        return values.value(key).toUShort(nullptr, 0);
    }
    void put(QString key, int value) {
        put(key, QString("%1").arg(value));
    }
    void put(QString key, QString value) {
        values.insert(key, value);
    }

    static Event* createNewEvent(QString, QString);
    static Event* createNewObjectEvent();
    static Event* createNewWarpEvent(QString);
    static Event* createNewHealLocationEvent(QString);
    static Event* createNewTriggerEvent();
    static Event* createNewWeatherTriggerEvent();
    static Event* createNewSignEvent();
    static Event* createNewHiddenItemEvent();
    static Event* createNewSecretBaseEvent();

    QString buildObjectEventMacro(int);
    QString buildWarpEventMacro(QMap<QString, QString>*);
    QString buildTriggerEventMacro();
    QString buildWeatherTriggerEventMacro();
    QString buildSignEventMacro();
    QString buildHiddenItemEventMacro();
    QString buildSecretBaseEventMacro();
    void setPixmapFromSpritesheet(QImage, int, int);
    int getPixelX();
    int getPixelY();

    QMap<QString, QString> values;
    QPixmap pixmap;
    int spriteWidth;
    int spriteHeight;
};

#endif // EVENT_H
