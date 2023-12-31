#pragma once
#ifndef EVENTS_H
#define EVENTS_H

#include <QString>
#include <QMap>
#include <QSet>
#include <QPixmap>
#include <QJsonObject>
#include <QPointer>

#include "orderedjson.h"
using OrderedJson = poryjson::Json;


class Project;
class Map;
class EventFrame;
class ObjectFrame;
class CloneObjectFrame;
class WarpFrame;
class DraggablePixmapItem;

class Event;
class ObjectEvent;
class CloneObjectEvent;
class WarpEvent;
class CoordEvent;
class TriggerEvent;
class WeatherTriggerEvent;
class BgEvent;
class SignEvent;
class HiddenItemEvent;
class SecretBaseEvent;
class HealLocationEvent;

class EventVisitor {
public:
    virtual void nothing() { }
    virtual void visitObject(ObjectEvent *) = 0;
    virtual void visitTrigger(TriggerEvent *) = 0;
    virtual void visitSign(SignEvent *) = 0;
};

struct EventGraphics
{
    QImage spritesheet;
    int spriteWidth;
    int spriteHeight;
    bool inanimate;
};


///
/// Event base class -- purely virtual
///
class Event {
public:
    virtual ~Event();

    // disable copy constructor
    Event(const Event &other) = delete;

    // disable assignment operator
    Event& operator=(const Event &other) = delete;

protected:
    Event() {
        this->spriteWidth = 16;
        this->spriteHeight = 16;
        this->usingSprite = false;
    }

// public enums & static methods
public:
    enum class Type {
        Object, CloneObject,
        Warp,
        Trigger, WeatherTrigger,
        Sign, HiddenItem, SecretBase,
        HealLocation,
        Generic,
        None,
    };

    enum class Group {
        Object,
        Warp,
        Coord,
        Bg,
        Heal,
        None,
    };

    // all event groups except warps have IDs that start at 1
    static int getIndexOffset(Event::Group group) {
        return (group == Event::Group::Warp) ? 0 : 1;
    }

    static Event::Group typeToGroup(Event::Type type) {
        switch (type) {
        case Event::Type::Object:
        case Event::Type::CloneObject:
            return Event::Group::Object;
        case Event::Type::Warp:
            return Event::Group::Warp;
        case Event::Type::Trigger:
        case Event::Type::WeatherTrigger:
            return Event::Group::Coord;
        case Event::Type::Sign:
        case Event::Type::HiddenItem:
        case Event::Type::SecretBase:
            return Event::Group::Bg;
        case Event::Type::HealLocation:
            return Event::Group::Heal;
        default:
            return Event::Group::None;
        }
    }

    static QMap<Event::Group, const QPixmap*> icons;

// standard public methods
public:

    virtual Event *duplicate() = 0;

    void setMap(Map *newMap) { this->map = newMap; }
    Map *getMap() const { return this->map; }

    void modify();

    virtual void accept(EventVisitor *) { }

    void setX(int newX) { this->x = newX; }
    void setY(int newY) { this->y = newY; }
    void setZ(int newZ) { this->elevation = newZ; }
    void setElevation(int newElevation) { this->elevation = newElevation; }

    int getX() const { return this->x; }
    int getY() const { return this->y; }
    int getZ() const { return this->elevation; }
    int getElevation() const { return this->elevation; }

    int getPixelX() const { return (this->x * 16) - qMax(0, (this->spriteWidth - 16) / 2); }
    int getPixelY() const { return (this->y * 16) - qMax(0, this->spriteHeight - 16); }

    virtual EventFrame *getEventFrame();
    virtual EventFrame *createEventFrame() = 0;
    void destroyEventFrame();

    Event::Group getEventGroup() const { return this->eventGroup; }
    Event::Type getEventType() const { return this->eventType; }

    virtual OrderedJson::object buildEventJson(Project *project) = 0;
    virtual bool loadFromJson(QJsonObject json, Project *project) = 0;

    virtual void setDefaultValues(Project *project);

    virtual QSet<QString> getExpectedFields() = 0;
    void readCustomValues(QJsonObject values);
    void addCustomValuesTo(OrderedJson::object *obj);
    const QMap<QString, QJsonValue> getCustomValues() { return this->customValues; }
    void setCustomValues(const QMap<QString, QJsonValue> newCustomValues) { this->customValues = newCustomValues; }

    virtual void loadPixmap(Project *project);

    void setPixmap(QPixmap newPixmap) { this->pixmap = newPixmap; }
    QPixmap getPixmap() { return this->pixmap; }

    void setPixmapItem(DraggablePixmapItem *item);
    DraggablePixmapItem *getPixmapItem() { return this->pixmapItem; }

    void setUsingSprite(bool newUsingSprite) { this->usingSprite = newUsingSprite; }
    bool getUsingSprite() const { return this->usingSprite; }

    void setSpriteWidth(int newSpriteWidth) { this->spriteWidth = newSpriteWidth; }
    int getspriteWidth() const { return this->spriteWidth; }

    void setSpriteHeight(int newSpriteHeight) { this->spriteHeight = newSpriteHeight; }
    int getspriteHeight() const { return this->spriteHeight; }

    int getEventIndex();

    static QString eventGroupToString(Event::Group group);
    static QString eventTypeToString(Event::Type type);
    static Event::Type eventTypeFromString(QString type);
    static void setIcons();

// protected attributes
protected:
    Map *map = nullptr;

    Type eventType = Event::Type::None;
    Group eventGroup = Event::Group::None;

    // could be private?
    int x = 0;
    int y = 0;
    int elevation = 0;

    int spriteWidth = 16;
    int spriteHeight = 16;
    bool usingSprite = false;

    QMap<QString, QJsonValue> customValues;

    QPixmap pixmap;
    DraggablePixmapItem *pixmapItem = nullptr;

    QPointer<EventFrame> eventFrame;
};



///
/// Object Event
///
class ObjectEvent : public Event {
public:
    ObjectEvent() : Event() {
        this->eventGroup = Event::Group::Object;
        this->eventType = Event::Type::Object;
    }
    virtual ~ObjectEvent() {}

    virtual Event *duplicate() override;

    virtual void accept(EventVisitor *visitor) override { visitor->visitObject(this); }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    virtual void loadPixmap(Project *project) override;

    void setGfx(QString newGfx) { this->gfx = newGfx; }
    QString getGfx() { return this->gfx; }

    void setMovement(QString newMovement) { this->movement = newMovement; }
    QString getMovement() { return this->movement; }

    void setRadiusX(int newRadiusX) { this->radiusX = newRadiusX; }
    int getRadiusX() { return this->radiusX; }

    void setRadiusY(int newRadiusY) { this->radiusY = newRadiusY; }
    int getRadiusY() { return this->radiusY; }

    void setTrainerType(QString newTrainerType) { this->trainerType = newTrainerType; }
    QString getTrainerType() { return this->trainerType; }

    void setSightRadiusBerryTreeID(QString newValue) { this->sightRadiusBerryTreeID = newValue; }
    QString getSightRadiusBerryTreeID() { return this->sightRadiusBerryTreeID; }

    void setScript(QString newScript) { this->script = newScript; }
    QString getScript() { return this->script; }

    void setFlag(QString newFlag) { this->flag = newFlag; }
    QString getFlag() { return this->flag; }

public:
    void setFrameFromMovement(QString movement);
    void setPixmapFromSpritesheet(EventGraphics * gfx);


protected:
    QString gfx;
    QString movement;
    int radiusX = 0;
    int radiusY = 0;
    QString trainerType;
    QString sightRadiusBerryTreeID;
    QString script;
    QString flag;

    int frame = 0;
    bool hFlip = false;
    bool vFlip = false;
};



///
/// Clone Object Event
///
class CloneObjectEvent : public ObjectEvent {

public:
    CloneObjectEvent() : ObjectEvent() {
        this->eventGroup = Event::Group::Object;
        this->eventType = Event::Type::CloneObject;
    }
    virtual ~CloneObjectEvent() {}

    virtual Event *duplicate() override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    virtual void loadPixmap(Project *project) override;

    void setTargetMap(QString newTargetMap) { this->targetMap = newTargetMap; }
    QString getTargetMap() { return this->targetMap; }

    void setTargetID(int newTargetID) { this->targetID = newTargetID; }
    int getTargetID() { return this->targetID; }

private:
    QString targetMap;
    int targetID = 0;
};



///
/// Warp Event
///
class WarpEvent : public Event {

public:
    WarpEvent() : Event() {
        this->eventGroup = Event::Group::Warp;
        this->eventType = Event::Type::Warp;
    }
    virtual ~WarpEvent() {}

    virtual Event *duplicate() override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setDestinationMap(QString newDestinationMap) { this->destinationMap = newDestinationMap; }
    QString getDestinationMap() { return this->destinationMap; }

    void setDestinationWarpID(QString newDestinationWarpID) { this->destinationWarpID = newDestinationWarpID; }
    QString getDestinationWarpID() { return this->destinationWarpID; }

    void setWarningEnabled(bool enabled);

private:
    QString destinationMap;
    QString destinationWarpID;
};



///
/// Coord Event
///
class CoordEvent : public Event {

public:
    CoordEvent() : Event() {}
    virtual ~CoordEvent() {}

    virtual Event *duplicate() override = 0;

    virtual EventFrame *createEventFrame() override = 0;

    virtual OrderedJson::object buildEventJson(Project *project) override = 0;
    virtual bool loadFromJson(QJsonObject json, Project *project) override = 0;

    virtual void setDefaultValues(Project *project) override = 0;

    virtual QSet<QString> getExpectedFields() override = 0;
};



///
/// Trigger Event
///
class TriggerEvent : public CoordEvent {

public:
    TriggerEvent() : CoordEvent() {
        this->eventGroup = Event::Group::Coord;
        this->eventType = Event::Type::Trigger;
    }
    virtual ~TriggerEvent() {}

    virtual Event *duplicate() override;

    virtual void accept(EventVisitor *visitor) override { visitor->visitTrigger(this); }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setScriptVar(QString newScriptVar) { this->scriptVar = newScriptVar; }
    QString getScriptVar() { return this->scriptVar; }

    void setScriptVarValue(QString newScriptVarValue) { this->scriptVarValue = newScriptVarValue; }
    QString getScriptVarValue() { return this->scriptVarValue; }

    void setScriptLabel(QString newScriptLabel) { this->scriptLabel = newScriptLabel; }
    QString getScriptLabel() { return this->scriptLabel; }

private:
    QString scriptVar;
    QString scriptVarValue;
    QString scriptLabel;
};



///
/// Weather Trigger Event
///
class WeatherTriggerEvent : public CoordEvent {

public:
    WeatherTriggerEvent() : CoordEvent() {
        this->eventGroup = Event::Group::Coord;
        this->eventType = Event::Type::WeatherTrigger;
    }
    virtual ~WeatherTriggerEvent() {}

    virtual Event *duplicate() override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setWeather(QString newWeather) { this->weather = newWeather; }
    QString getWeather() { return this->weather; }

private:
    QString weather;
};



///
/// BG Event
///
class BGEvent : public Event {

public:
    BGEvent() : Event() {
        this->eventGroup = Event::Group::Bg;
    }
    virtual ~BGEvent() {}

    virtual Event *duplicate() override = 0;

    virtual EventFrame *createEventFrame() override = 0;

    virtual OrderedJson::object buildEventJson(Project *project) override = 0;
    virtual bool loadFromJson(QJsonObject json, Project *project) override = 0;

    virtual void setDefaultValues(Project *project) override = 0;

    virtual QSet<QString> getExpectedFields() override = 0;
};



///
/// Sign Event
///
class SignEvent : public BGEvent {

public:
    SignEvent() : BGEvent() {
        this->eventType = Event::Type::Sign;
    }
    virtual ~SignEvent() {}

    virtual Event *duplicate() override;

    virtual void accept(EventVisitor *visitor) override { visitor->visitSign(this); }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setFacingDirection(QString newFacingDirection) { this->facingDirection = newFacingDirection; }
    QString getFacingDirection() { return this->facingDirection; }

    void setScriptLabel(QString newScriptLabel) { this->scriptLabel = newScriptLabel; }
    QString getScriptLabel() { return this->scriptLabel; }

private:
    QString facingDirection;
    QString scriptLabel;
};



///
/// Hidden Item Event
///
class HiddenItemEvent : public BGEvent {

public:
    HiddenItemEvent() : BGEvent() {
        this->eventType = Event::Type::HiddenItem;
    }
    virtual ~HiddenItemEvent() {}

    virtual Event *duplicate() override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setItem(QString newItem) { this->item = newItem; }
    QString getItem() { return this->item; }

    void setFlag(QString newFlag) { this->flag = newFlag; }
    QString getFlag() { return this->flag; }

    void setQuantity(int newQuantity) { this->quantity = newQuantity; }
    int getQuantity() { return this->quantity; }

    void setUnderfoot(bool newUnderfoot) { this->underfoot = newUnderfoot; }
    bool getUnderfoot() { return this->underfoot; }

private:
    QString item;
    QString flag;

    // optional
    int quantity = 0;
    bool underfoot = false;
};



///
/// Secret Base Event
///
class SecretBaseEvent : public BGEvent {

public:
    SecretBaseEvent() : BGEvent() {
        this->eventType = Event::Type::SecretBase;
    }
    virtual ~SecretBaseEvent() {}

    virtual Event *duplicate() override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setBaseID(QString newBaseID) { this->baseID = newBaseID; }
    QString getBaseID() { return this->baseID; }

private:
    QString baseID;
};



///
/// Heal Location Event
///
class HealLocationEvent : public Event {

public:
    HealLocationEvent() : Event() {
        this->eventGroup = Event::Group::Heal;
        this->eventType = Event::Type::HealLocation;
    }
    virtual ~HealLocationEvent() {}

    virtual Event *duplicate() override { return nullptr; }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject, Project *) override { return false; }

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override { return QSet<QString>(); }

    void setIndex(int newIndex) { this->index = newIndex; }
    int getIndex() { return this->index; }

    void setLocationName(QString newLocationName) { this->locationName = newLocationName; }
    QString getLocationName() { return this->locationName; }

    void setIdName(QString newIdName) { this->idName = newIdName; }
    QString getIdName() { return this->idName; }

    void setRespawnMap(QString newRespawnMap) { this->respawnMap = newRespawnMap; }
    QString getRespawnMap() { return this->respawnMap; }

    void setRespawnNPC(uint8_t newRespawnNPC) { this->respawnNPC = newRespawnNPC; }
    uint8_t getRespawnNPC() { return this->respawnNPC; }

private:
    int index = -1;
    QString locationName;
    QString idName;
    QString respawnMap;
    uint8_t respawnNPC = 0;
};



///
/// Keeps track of scripts
///
class ScriptTracker : public EventVisitor {
public:
    virtual void visitObject(ObjectEvent *object) override { this->scripts << object->getScript(); };
    virtual void visitTrigger(TriggerEvent *trigger) override { this->scripts << trigger->getScriptLabel(); };
    virtual void visitSign(SignEvent *sign) override { this->scripts << sign->getScriptLabel(); };

    QStringList getScripts() { return this->scripts; }

private:
    QStringList scripts;
};


#endif // EVENTS_H
