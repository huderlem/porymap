#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QPixmap>
#include <QMap>

#include "orderedjson.h"

using OrderedJson = poryjson::Json;

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

class Project;
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
    int16_t getS16(QString key) {
        return values.value(key).toShort(nullptr, 0);
    }
    void put(QString key, int value) {
        put(key, QString("%1").arg(value));
    }
    void put(QString key, QString value) {
        values.insert(key, value);
    }

    static Event* createNewEvent(QString, QString, Project*);
    static Event* createNewObjectEvent(Project*);
    static Event* createNewWarpEvent(QString);
    static Event* createNewHealLocationEvent(QString);
    static Event* createNewTriggerEvent(Project*);
    static Event* createNewWeatherTriggerEvent(Project*);
    static Event* createNewSignEvent(Project*);
    static Event* createNewHiddenItemEvent(Project*);
    static Event* createNewSecretBaseEvent(Project*);

    OrderedJson::object buildObjectEventJSON();
    OrderedJson::object buildWarpEventJSON(QMap<QString, QString>*);
    OrderedJson::object buildTriggerEventJSON();
    OrderedJson::object buildWeatherTriggerEventJSON();
    OrderedJson::object buildSignEventJSON();
    OrderedJson::object buildHiddenItemEventJSON();
    OrderedJson::object buildSecretBaseEventJSON();
    void setPixmapFromSpritesheet(QImage, int, int, int, bool);
    int getPixelX();
    int getPixelY();
    QMap<QString, bool> getExpectedFields();
    void readCustomValues(QJsonObject values);
    void addCustomValuesTo(OrderedJson::object *obj);
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
