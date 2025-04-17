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
#include "parseutil.h"


class Project;
class Map;
class EventFrame;
class ObjectFrame;
class CloneObjectFrame;
class WarpFrame;
class EventPixmapItem;

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
    Event() {}

// public enums & static methods
public:
    enum class Type {
        Object, CloneObject,
        Warp,
        Trigger, WeatherTrigger,
        Sign, HiddenItem, SecretBase,
        HealLocation,
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

    // Normally we refer to events using their index in the list of that group's events.
    // Object events often get referred to with a special "local ID", which is really just the index + 1.
    // We use this local ID number in the index spinner for object events instead of the actual index.
    // This distinction is only really important for object and warp events, because these are normally
    // the only two groups of events that need to be explicitly referred to.
    static int getIndexOffset(Event::Group group) {
        return (group == Event::Group::Object) ? 1 : 0;
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

    static Event* create(Event::Type type);

// standard public methods
public:

    virtual Event *duplicate() const = 0;

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

    int getPixelX() const { return (this->x * 16) - qMax(0, (pixmap.width() - 16) / 2); }
    int getPixelY() const { return (this->y * 16) - qMax(0, pixmap.height() - 16); }

    virtual EventFrame *getEventFrame();
    virtual EventFrame *createEventFrame() = 0;
    void destroyEventFrame();

    Event::Group getEventGroup() const { return this->eventGroup; }
    Event::Type getEventType() const { return this->eventType; }

    virtual OrderedJson::object buildEventJson(Project *project) = 0;
    virtual bool loadFromJson(QJsonObject json, Project *project) = 0;

    virtual void setDefaultValues(Project *project);

    virtual QSet<QString> getExpectedFields() = 0;

    QJsonObject getCustomAttributes() const { return this->customAttributes; }
    void setCustomAttributes(const QJsonObject &newCustomAttributes) { this->customAttributes = newCustomAttributes; }

    virtual void loadPixmap(Project *project);

    void setPixmap(QPixmap newPixmap) { this->pixmap = newPixmap; }
    QPixmap getPixmap() const { return this->pixmap; }

    void setPixmapItem(EventPixmapItem *item);
    EventPixmapItem *getPixmapItem() const { return this->pixmapItem; }

    void setUsesDefaultPixmap(bool newUsesDefaultPixmap) { this->usesDefaultPixmap = newUsesDefaultPixmap; }
    bool getUsesDefaultPixmap() const { return this->usesDefaultPixmap; }

    int getEventIndex();

    void setIdName(QString newIdName) { this->idName = newIdName; }
    QString getIdName() const { return this->idName; }

    static QString groupToString(Event::Group group);
    static QString typeToString(Event::Type type);
    static QString typeToJsonKey(Event::Type type);
    static Event::Type typeFromJsonKey(QString type);
    static QList<Event::Type> types();
    static QList<Event::Group> groups();

// protected attributes
protected:
    Map *map = nullptr;

    Type eventType = Event::Type::None;
    Group eventGroup = Event::Group::None;

    // could be private?
    int x = 0;
    int y = 0;
    int elevation = 0;

    bool usesDefaultPixmap = true;

    // Some events can have an associated #define name that should be unique to this event.
    // e.g. object events can have a 'LOCALID', or Heal Locations have a 'HEAL_LOCATION' id.
    // When deleting events like this we want to warn the user that the #define may also be deleted.
    QString idName;

    QJsonObject customAttributes;

    QPixmap pixmap;
    EventPixmapItem *pixmapItem = nullptr;

    QPointer<EventFrame> eventFrame;

    static QString readString(QJsonObject *object, const QString &key) { return ParseUtil::jsonToQString(object->take(key)); }
    static int readInt(QJsonObject *object, const QString &key) { return ParseUtil::jsonToInt(object->take(key)); }
    static bool readBool(QJsonObject *object, const QString &key) { return ParseUtil::jsonToBool(object->take(key)); }
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

    virtual Event *duplicate() const override;

    virtual void accept(EventVisitor *visitor) override { visitor->visitObject(this); }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    virtual void loadPixmap(Project *project) override;

    void setGfx(QString newGfx) { this->gfx = newGfx; }
    QString getGfx() const { return this->gfx; }

    void setMovement(QString newMovement) { this->movement = newMovement; }
    QString getMovement() const { return this->movement; }

    void setRadiusX(int newRadiusX) { this->radiusX = newRadiusX; }
    int getRadiusX() const { return this->radiusX; }

    void setRadiusY(int newRadiusY) { this->radiusY = newRadiusY; }
    int getRadiusY() const { return this->radiusY; }

    void setTrainerType(QString newTrainerType) { this->trainerType = newTrainerType; }
    QString getTrainerType() const { return this->trainerType; }

    void setSightRadiusBerryTreeID(QString newValue) { this->sightRadiusBerryTreeID = newValue; }
    QString getSightRadiusBerryTreeID() const { return this->sightRadiusBerryTreeID; }

    void setScript(QString newScript) { this->script = newScript; }
    QString getScript() const { return this->script; }

    void setFlag(QString newFlag) { this->flag = newFlag; }
    QString getFlag() const { return this->flag; }


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

    virtual Event *duplicate() const override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    virtual void loadPixmap(Project *project) override;

    void setTargetMap(QString newTargetMap) { this->targetMap = newTargetMap; }
    QString getTargetMap() const { return this->targetMap; }

    void setTargetID(QString newTargetID) { this->targetID = newTargetID; }
    QString getTargetID() const { return this->targetID; }

private:
    QString targetMap;
    QString targetID;
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

    virtual Event *duplicate() const override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setDestinationMap(QString newDestinationMap) { this->destinationMap = newDestinationMap; }
    QString getDestinationMap() const { return this->destinationMap; }

    void setDestinationWarpID(QString newDestinationWarpID) { this->destinationWarpID = newDestinationWarpID; }
    QString getDestinationWarpID() const { return this->destinationWarpID; }

    void setWarningEnabled(bool enabled);
    bool getWarningEnabled() const { return this->warningEnabled; }

private:
    QString destinationMap;
    QString destinationWarpID;
    bool warningEnabled = false;
};



///
/// Coord Event
///
class CoordEvent : public Event {

public:
    CoordEvent() : Event() {}
    virtual ~CoordEvent() {}

    virtual Event *duplicate() const override = 0;

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

    virtual Event *duplicate() const override;

    virtual void accept(EventVisitor *visitor) override { visitor->visitTrigger(this); }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setScriptVar(QString newScriptVar) { this->scriptVar = newScriptVar; }
    QString getScriptVar() const { return this->scriptVar; }

    void setScriptVarValue(QString newScriptVarValue) { this->scriptVarValue = newScriptVarValue; }
    QString getScriptVarValue() const { return this->scriptVarValue; }

    void setScriptLabel(QString newScriptLabel) { this->scriptLabel = newScriptLabel; }
    QString getScriptLabel() const { return this->scriptLabel; }

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

    virtual Event *duplicate() const override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setWeather(QString newWeather) { this->weather = newWeather; }
    QString getWeather() const { return this->weather; }

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

    virtual Event *duplicate() const override = 0;

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

    virtual Event *duplicate() const override;

    virtual void accept(EventVisitor *visitor) override { visitor->visitSign(this); }

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setFacingDirection(QString newFacingDirection) { this->facingDirection = newFacingDirection; }
    QString getFacingDirection() const { return this->facingDirection; }

    void setScriptLabel(QString newScriptLabel) { this->scriptLabel = newScriptLabel; }
    QString getScriptLabel() const { return this->scriptLabel; }

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

    virtual Event *duplicate() const override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setItem(QString newItem) { this->item = newItem; }
    QString getItem() const { return this->item; }

    void setFlag(QString newFlag) { this->flag = newFlag; }
    QString getFlag() const { return this->flag; }

    void setQuantity(int newQuantity) { this->quantity = newQuantity; }
    int getQuantity() const { return this->quantity; }

    void setUnderfoot(bool newUnderfoot) { this->underfoot = newUnderfoot; }
    bool getUnderfoot() const { return this->underfoot; }

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

    virtual Event *duplicate() const override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setBaseID(QString newBaseID) { this->baseID = newBaseID; }
    QString getBaseID() const { return this->baseID; }

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

    virtual Event *duplicate() const override;

    virtual EventFrame *createEventFrame() override;

    virtual OrderedJson::object buildEventJson(Project *project) override;
    virtual bool loadFromJson(QJsonObject json, Project *project) override;

    virtual void setDefaultValues(Project *project) override;

    virtual QSet<QString> getExpectedFields() override;

    void setHostMapName(QString newHostMapName) { this->hostMapName = newHostMapName; }
    QString getHostMapName() const;

    void setRespawnMapName(QString newRespawnMapName) { this->respawnMapName = newRespawnMapName; }
    QString getRespawnMapName() const { return this->respawnMapName; }

    void setRespawnNPC(QString newRespawnNPC) { this->respawnNPC = newRespawnNPC; }
    QString getRespawnNPC() const { return this->respawnNPC; }

private:
    QString respawnMapName;
    QString respawnNPC;
    QString hostMapName; // Only needed if the host map fails to load.
};


inline uint qHash(const Event::Group &key, uint seed = 0) {
    return qHash(static_cast<int>(key), seed);
}


///
/// Keeps track of scripts
///
class ScriptTracker : public EventVisitor {
public:
    virtual void visitObject(ObjectEvent *object) override { this->scripts << object->getScript(); };
    virtual void visitTrigger(TriggerEvent *trigger) override { this->scripts << trigger->getScriptLabel(); };
    virtual void visitSign(SignEvent *sign) override { this->scripts << sign->getScriptLabel(); };

    QStringList getScripts() const { return this->scripts; }

private:
    QStringList scripts;
};


#endif // EVENTS_H
