#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QPixmap>
#include <QMap>
#include <QJsonObject>

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
    Event(QJsonObject, QString);
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

    QJsonObject buildObjectEventJSON();
    QJsonObject buildWarpEventJSON(QMap<QString, QString>*);
    QJsonObject buildTriggerEventJSON();
    QJsonObject buildWeatherTriggerEventJSON();
    QJsonObject buildSignEventJSON();
    QJsonObject buildHiddenItemEventJSON();
    QJsonObject buildSecretBaseEventJSON();
    void setPixmapFromSpritesheet(QImage, int, int, int, bool);
    int getPixelX();
    int getPixelY();
    QMap<QString, bool> getExpectedFields();
    void readCustomValues(QJsonObject values);
    void addCustomValuesTo(QJsonObject *obj);
    void setFrameFromMovement(QString);

    QMap<QString, QString> values;
    QMap<QString, QString> customValues;
    QPixmap pixmap;
    int spriteWidth;
    int spriteHeight;
    int frame = 0;
    bool hFlip = false;
    bool usingSprite;
};

#endif // EVENT_H
