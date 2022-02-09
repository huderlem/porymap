#pragma once
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
    static QString CloneObject;
    static QString Warp;
    static QString Trigger;
    static QString WeatherTrigger;
    static QString Sign;
    static QString HiddenItem;
    static QString SecretBase;
    static QString HealLocation;
};

class EventGroup
{
public:
    static QString Object;
    static QString Warp;
    static QString Coord;
    static QString Bg;
    static QString Heal;
};

class DraggablePixmapItem;
class Project;
class Event
{
public:
    Event();
    Event(const Event&);
    Event(QJsonObject, QString);
public:
    int x() const {
        return getInt("x");
    }
    int y() const {
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
    QString get(const QString &key) const {
        return values.value(key);
    }
    int getInt(const QString &key) const {
        return values.value(key).toInt(nullptr, 0);
    }
    uint16_t getU16(const QString &key) const {
        return values.value(key).toUShort(nullptr, 0);
    }
    int16_t getS16(const QString &key) const {
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
    static Event* createNewCloneObjectEvent(Project*, QString);
    static Event* createNewWarpEvent(QString);
    static Event* createNewHealLocationEvent(QString);
    static Event* createNewTriggerEvent(Project*);
    static Event* createNewWeatherTriggerEvent(Project*);
    static Event* createNewSignEvent(Project*);
    static Event* createNewHiddenItemEvent(Project*);
    static Event* createNewSecretBaseEvent(Project*);
    static bool isValidType(QString event_type);
    static QString typeToGroup(QString event_type);
    static int getIndexOffset(QString group_type);

    OrderedJson::object buildObjectEventJSON();
    OrderedJson::object buildCloneObjectEventJSON(const QMap<QString, QString> &);
    OrderedJson::object buildWarpEventJSON(const QMap<QString, QString> &);
    OrderedJson::object buildTriggerEventJSON();
    OrderedJson::object buildWeatherTriggerEventJSON();
    OrderedJson::object buildSignEventJSON();
    OrderedJson::object buildHiddenItemEventJSON();
    OrderedJson::object buildSecretBaseEventJSON();
    void setPixmapFromSpritesheet(QImage, int, int, bool);
    int getPixelX();
    int getPixelY();
    QSet<QString> getExpectedFields();
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

    DraggablePixmapItem *pixmapItem = nullptr;
    void setPixmapItem(DraggablePixmapItem *item) { pixmapItem = item; }
};

#endif // EVENT_H
