#include "events.h"

#include "eventframes.h"
#include "project.h"
#include "config.h"

Event* Event::create(Event::Type type) {
    switch (type) {
    case Event::Type::Object: return new ObjectEvent();
    case Event::Type::CloneObject: return new CloneObjectEvent();
    case Event::Type::Warp: return new WarpEvent();
    case Event::Type::Trigger: return new TriggerEvent();
    case Event::Type::WeatherTrigger: return new WeatherTriggerEvent();
    case Event::Type::Sign: return new SignEvent();
    case Event::Type::HiddenItem: return new HiddenItemEvent();
    case Event::Type::SecretBase: return new SecretBaseEvent();
    case Event::Type::HealLocation: return new HealLocationEvent();
    default: return nullptr;
    }
}

Event::~Event() {
    if (this->eventFrame)
        this->eventFrame->deleteLater();
}

EventFrame *Event::getEventFrame() {
    if (!this->eventFrame) createEventFrame();
    return this->eventFrame;
}

void Event::destroyEventFrame() {
    if (this->eventFrame) delete this->eventFrame;
    this->eventFrame = nullptr;
}

void Event::setPixmapItem(EventPixmapItem *item) {
    this->pixmapItem = item;
    if (this->eventFrame) {
        this->eventFrame->invalidateConnections();
    }
}

int Event::getEventIndex() {
    return this->map->getIndexOfEvent(this);
}

void Event::setDefaultValues(Project *) {
    this->setX(0);
    this->setY(0);
    this->setElevation(projectConfig.defaultElevation);
}

void Event::modify() {
    this->map->modify();
}

const QMap<Event::Group, QString> groupToStringMap = {
    {Event::Group::Object, "Object"},
    {Event::Group::Warp, "Warp"},
    {Event::Group::Coord, "Trigger"},
    {Event::Group::Bg, "BG"},
    {Event::Group::Heal, "Heal Location"},
};

QString Event::groupToString(Event::Group group) {
    return groupToStringMap.value(group);
}

QList<Event::Group> Event::groups() {
    static QList<Event::Group> groupList = groupToStringMap.keys();
    return groupList;
}

// These are the expected key names used in the map.json files.
// We re-use them for key names in the copy/paste JSON data,
const QMap<Event::Type, QString> typeToJsonKeyMap = {
    {Event::Type::Object, "object"},
    {Event::Type::CloneObject, "clone"},
    {Event::Type::Warp, "warp"},
    {Event::Type::Trigger, "trigger"},
    {Event::Type::WeatherTrigger, "weather"},
    {Event::Type::Sign, "sign"},
    {Event::Type::HiddenItem, "hidden_item"},
    {Event::Type::SecretBase, "secret_base"},
    {Event::Type::HealLocation, "heal_location"},
};

QString Event::typeToJsonKey(Event::Type type) {
    return typeToJsonKeyMap.value(type);
}

Event::Type Event::typeFromJsonKey(QString type) {
    return typeToJsonKeyMap.key(type, Event::Type::None);
}

QList<Event::Type> Event::types() {
    static QList<Event::Type> typeList = typeToJsonKeyMap.keys();
    return typeList;
}

QString Event::typeToString(Event::Type type) {
    static const QMap<Event::Type, QString> typeToStringMap = {
        {Event::Type::Object, "Object"},
        {Event::Type::CloneObject, "Clone Object"},
        {Event::Type::Warp, "Warp"},
        {Event::Type::Trigger, "Trigger"},
        {Event::Type::WeatherTrigger, "Weather Trigger"},
        {Event::Type::Sign, "Sign"},
        {Event::Type::HiddenItem, "Hidden Item"},
        {Event::Type::SecretBase, "Secret Base"},
        {Event::Type::HealLocation, "Heal Location"},
    };
    return typeToStringMap.value(type);
}

QPixmap Event::loadPixmap(Project *project) {
    this->pixmap = project->getEventPixmap(this->getEventGroup());
    this->usesDefaultPixmap = true;
    return this->pixmap;
}



Event *ObjectEvent::duplicate() const {
    ObjectEvent *copy = new ObjectEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setIdName(this->getIdName());
    copy->setGfx(this->getGfx());
    copy->setMovement(this->getMovement());
    copy->setRadiusX(this->getRadiusX());
    copy->setRadiusY(this->getRadiusY());
    copy->setTrainerType(this->getTrainerType());
    copy->setSightRadiusBerryTreeID(this->getSightRadiusBerryTreeID());
    copy->setScript(this->getScript());
    copy->setFlag(this->getFlag());
    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *ObjectEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new ObjectFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object ObjectEvent::buildEventJson(Project *) {
    OrderedJson::object objectJson;

    QString idName = this->getIdName();
    if (!idName.isEmpty())
        objectJson["local_id"] = idName;

    if (projectConfig.eventCloneObjectEnabled) {
        objectJson["type"] = Event::typeToJsonKey(Event::Type::Object);
    }
    objectJson["graphics_id"] = this->getGfx();
    objectJson["x"] = this->getX();
    objectJson["y"] = this->getY();
    objectJson["elevation"] = this->getElevation();
    objectJson["movement_type"] = this->getMovement();
    objectJson["movement_range_x"] = this->getRadiusX();
    objectJson["movement_range_y"] = this->getRadiusY();
    objectJson["trainer_type"] = this->getTrainerType();
    objectJson["trainer_sight_or_berry_tree_id"] = this->getSightRadiusBerryTreeID();
    objectJson["script"] = this->getScript();
    objectJson["flag"] = this->getFlag();

    OrderedJson::append(&objectJson, this->getCustomAttributes());
    return objectJson;
}

bool ObjectEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setIdName(readString(&json, "local_id"));
    this->setGfx(readString(&json, "graphics_id"));
    this->setMovement(readString(&json, "movement_type"));
    this->setRadiusX(readInt(&json, "movement_range_x"));
    this->setRadiusY(readInt(&json, "movement_range_y"));
    this->setTrainerType(readString(&json, "trainer_type"));
    this->setSightRadiusBerryTreeID(readString(&json, "trainer_sight_or_berry_tree_id"));
    this->setScript(readString(&json, "script"));
    this->setFlag(readString(&json, "flag"));
    
    this->setCustomAttributes(json);
    return true;
}

void ObjectEvent::setDefaultValues(Project *project) {
    this->setGfx(project->gfxDefines.key(0, "0"));
    this->setMovement(project->movementTypes.value(0, "0"));
    this->setScript("NULL");
    this->setTrainerType(project->trainerTypes.value(0, "0"));
    this->setFlag("0");
    this->setRadiusX(0);
    this->setRadiusY(0);
    this->setSightRadiusBerryTreeID("0");
}

QSet<QString> ObjectEvent::getExpectedFields() {
    QSet<QString> expectedFields = {
        "x",
        "y",
        "local_id",
        "graphics_id",
        "elevation",
        "movement_type",
        "movement_range_x",
        "movement_range_y",
        "trainer_type",
        "trainer_sight_or_berry_tree_id",
        "script",
        "flag",
    };
    if (projectConfig.eventCloneObjectEnabled) {
        expectedFields.insert("type");
    }
    return expectedFields;
}

QPixmap ObjectEvent::loadPixmap(Project *project) {
    this->pixmap = project->getEventPixmap(this->gfx, this->movement);
    if (!this->pixmap.isNull()) {
        this->usesDefaultPixmap = false;
        return this->pixmap;
    }
    return Event::loadPixmap(project);
}



Event *CloneObjectEvent::duplicate() const {
    CloneObjectEvent *copy = new CloneObjectEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setIdName(this->getIdName());
    copy->setGfx(this->getGfx());
    copy->setTargetID(this->getTargetID());
    copy->setTargetMap(this->getTargetMap());
    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *CloneObjectEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new CloneObjectFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object CloneObjectEvent::buildEventJson(Project *project) {
    OrderedJson::object cloneJson;

    QString idName = this->getIdName();
    if (!idName.isEmpty())
        cloneJson["local_id"] = idName;

    cloneJson["type"] = Event::typeToJsonKey(Event::Type::CloneObject);
    cloneJson["graphics_id"] = this->getGfx();
    cloneJson["x"] = this->getX();
    cloneJson["y"] = this->getY();
    cloneJson["target_local_id"] = this->getTargetID();
    const QString mapName = this->getTargetMap();
    cloneJson["target_map"] = project->getMapConstant(mapName, mapName);

    OrderedJson::append(&cloneJson, this->getCustomAttributes());
    return cloneJson;
}

bool CloneObjectEvent::loadFromJson(QJsonObject json, Project *project) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setIdName(readString(&json, "local_id"));
    this->setGfx(readString(&json, "graphics_id"));
    this->setTargetID(readString(&json, "target_local_id"));

    // Log a warning if "target_map" isn't a known map ID, but don't overwrite user data.
    const QString mapConstant = readString(&json, "target_map");
    if (!project->mapConstantsToMapNames.contains(mapConstant))
        logWarn(QString("Unknown Target Map constant '%1'.").arg(mapConstant));
    this->setTargetMap(project->mapConstantsToMapNames.value(mapConstant, mapConstant));

    this->setCustomAttributes(json);
    return true;
}

void CloneObjectEvent::setDefaultValues(Project *project) {
    this->setGfx(project->gfxDefines.key(0, "0"));
    this->setTargetID(QString::number(Event::getIndexOffset(Event::Group::Object)));
    if (this->getMap()) this->setTargetMap(this->getMap()->name());
}

QSet<QString> CloneObjectEvent::getExpectedFields() {
    static const QSet<QString> expectedFields = {
        "x",
        "y",
        "type",
        "local_id",
        "graphics_id",
        "target_local_id",
        "target_map",
    };
    return expectedFields;
}

QPixmap CloneObjectEvent::loadPixmap(Project *project) {
    // Try to get the targeted object to clone
    Map *clonedMap = project->loadMap(this->targetMap);
    Event *clonedEvent = clonedMap ? clonedMap->getEvent(Event::Group::Object, this->targetID) : nullptr;

    if (clonedEvent && clonedEvent->getEventType() == Event::Type::Object) {
        // Get graphics data from cloned object
        ObjectEvent *clonedObject = dynamic_cast<ObjectEvent *>(clonedEvent);
        this->gfx = clonedObject->getGfx();
        this->movement = clonedObject->getMovement();
    } else {
        // Invalid object specified, use default graphics data (as would be shown in-game)
        this->gfx = project->gfxDefines.key(0, "0");
        this->movement = project->movementTypes.value(0, "0");
    }
    return ObjectEvent::loadPixmap(project);
}



Event *WarpEvent::duplicate() const {
    WarpEvent *copy = new WarpEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setDestinationMap(this->getDestinationMap());
    copy->setDestinationWarpID(this->getDestinationWarpID());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *WarpEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new WarpFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object WarpEvent::buildEventJson(Project *project) {
    OrderedJson::object warpJson;

    warpJson["x"] = this->getX();
    warpJson["y"] = this->getY();
    warpJson["elevation"] = this->getElevation();
    const QString mapName = this->getDestinationMap();
    warpJson["dest_map"] = project->getMapConstant(mapName, mapName);
    warpJson["dest_warp_id"] = this->getDestinationWarpID();

    OrderedJson::append(&warpJson, this->getCustomAttributes());
    return warpJson;
}

bool WarpEvent::loadFromJson(QJsonObject json, Project *project) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setDestinationWarpID(readString(&json, "dest_warp_id"));

    // Log a warning if "dest_map" isn't a known map ID, but don't overwrite user data.
    const QString mapConstant = readString(&json, "dest_map");
    if (!project->mapConstantsToMapNames.contains(mapConstant))
        logWarn(QString("Unknown Destination Map constant '%1'.").arg(mapConstant));
    this->setDestinationMap(project->mapConstantsToMapNames.value(mapConstant, mapConstant));

    this->setCustomAttributes(json);
    return true;
}

void WarpEvent::setDefaultValues(Project *) {
    if (this->getMap()) this->setDestinationMap(this->getMap()->name());
    this->setDestinationWarpID("0");
    this->setElevation(0);
}

QSet<QString> WarpEvent::getExpectedFields() {
    static const QSet<QString> expectedFields = {
        "x",
        "y",
        "elevation",
        "dest_map",
        "dest_warp_id",
    };
    return expectedFields;
}

void WarpEvent::setWarningEnabled(bool enabled) {
    this->warningEnabled = enabled;

    // Don't call getEventFrame here, because it may create the event frame.
    // If the frame hasn't been created yet then we have nothing else to do.
    auto frame = static_cast<WarpFrame*>(this->eventFrame.data());
    if (frame && frame->warning)
        frame->warning->setVisible(enabled);
}



Event *TriggerEvent::duplicate() const {
    TriggerEvent *copy = new TriggerEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setScriptVar(this->getScriptVar());
    copy->setScriptVarValue(this->getScriptVarValue());
    copy->setScriptLabel(this->getScriptLabel());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *TriggerEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new TriggerFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object TriggerEvent::buildEventJson(Project *) {
    OrderedJson::object triggerJson;

    triggerJson["type"] = Event::typeToJsonKey(Event::Type::Trigger);
    triggerJson["x"] = this->getX();
    triggerJson["y"] = this->getY();
    triggerJson["elevation"] = this->getElevation();
    triggerJson["var"] = this->getScriptVar();
    triggerJson["var_value"] = this->getScriptVarValue();
    triggerJson["script"] = this->getScriptLabel();

    OrderedJson::append(&triggerJson, this->getCustomAttributes());
    return triggerJson;
}

bool TriggerEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setScriptVar(readString(&json, "var"));
    this->setScriptVarValue(readString(&json, "var_value"));
    this->setScriptLabel(readString(&json, "script"));

    this->setCustomAttributes(json);
    return true;
}

void TriggerEvent::setDefaultValues(Project *project) {
    this->setScriptLabel("NULL");
    this->setScriptVar(project->varNames.value(0, "0"));
    this->setScriptVarValue("0");
    this->setElevation(0);
}

QSet<QString> TriggerEvent::getExpectedFields() {
    static const QSet<QString> expectedFields = {
        "x",
        "y",
        "type",
        "elevation",
        "var",
        "var_value",
        "script",
    };
    return expectedFields;
}



Event *WeatherTriggerEvent::duplicate() const {
    WeatherTriggerEvent *copy = new WeatherTriggerEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setWeather(this->getWeather());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *WeatherTriggerEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new WeatherTriggerFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object WeatherTriggerEvent::buildEventJson(Project *) {
    OrderedJson::object weatherJson;

    weatherJson["type"] = Event::typeToJsonKey(Event::Type::WeatherTrigger);
    weatherJson["x"] = this->getX();
    weatherJson["y"] = this->getY();
    weatherJson["elevation"] = this->getElevation();
    weatherJson["weather"] = this->getWeather();

    OrderedJson::append(&weatherJson, this->getCustomAttributes());
    return weatherJson;
}

bool WeatherTriggerEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setWeather(readString(&json, "weather"));

    this->setCustomAttributes(json);
    return true;
}

void WeatherTriggerEvent::setDefaultValues(Project *project) {
    this->setWeather(project->coordEventWeatherNames.value(0, "0"));
    this->setElevation(0);
}

QSet<QString> WeatherTriggerEvent::getExpectedFields() {
    static const QSet<QString> expectedFields = {
        "x",
        "y",
        "type",
        "elevation",
        "weather",
    };
    return expectedFields;
}



Event *SignEvent::duplicate() const {
    SignEvent *copy = new SignEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setFacingDirection(this->getFacingDirection());
    copy->setScriptLabel(this->getScriptLabel());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *SignEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new SignFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object SignEvent::buildEventJson(Project *) {
    OrderedJson::object signJson;

    signJson["type"] = Event::typeToJsonKey(Event::Type::Sign);
    signJson["x"] = this->getX();
    signJson["y"] = this->getY();
    signJson["elevation"] = this->getElevation();
    signJson["player_facing_dir"] = this->getFacingDirection();
    signJson["script"] = this->getScriptLabel();

    OrderedJson::append(&signJson, this->getCustomAttributes());
    return signJson;
}

bool SignEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setFacingDirection(readString(&json, "player_facing_dir"));
    this->setScriptLabel(readString(&json, "script"));

    this->setCustomAttributes(json);
    return true;
}

void SignEvent::setDefaultValues(Project *project) {
    this->setFacingDirection(project->bgEventFacingDirections.value(0, "0"));
    this->setScriptLabel("NULL");
    this->setElevation(0);
}

QSet<QString> SignEvent::getExpectedFields() {
    static const QSet<QString> expectedFields = {
        "x",
        "y",
        "type",
        "elevation",
        "player_facing_dir",
        "script",
    };
    return expectedFields;
}



Event *HiddenItemEvent::duplicate() const {
    HiddenItemEvent *copy = new HiddenItemEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setItem(this->getItem());
    copy->setFlag(this->getFlag());
    copy->setQuantity(this->getQuantity());
    copy->setUnderfoot(this->getUnderfoot());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *HiddenItemEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new HiddenItemFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object HiddenItemEvent::buildEventJson(Project *) {
    OrderedJson::object hiddenItemJson;

    hiddenItemJson["type"] = Event::typeToJsonKey(Event::Type::HiddenItem);
    hiddenItemJson["x"] = this->getX();
    hiddenItemJson["y"] = this->getY();
    hiddenItemJson["elevation"] = this->getElevation();
    hiddenItemJson["item"] = this->getItem();
    hiddenItemJson["flag"] = this->getFlag();
    if (projectConfig.hiddenItemQuantityEnabled) {
        hiddenItemJson["quantity"] = this->getQuantity();
    }
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        hiddenItemJson["underfoot"] = this->getUnderfoot();
    }

    OrderedJson::append(&hiddenItemJson, this->getCustomAttributes());
    return hiddenItemJson;
}

bool HiddenItemEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setItem(readString(&json, "item"));
    this->setFlag(readString(&json, "flag"));
    if (projectConfig.hiddenItemQuantityEnabled) {
        this->setQuantity(readInt(&json, "quantity"));
    }
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        this->setUnderfoot(readBool(&json, "underfoot"));
    }

    this->setCustomAttributes(json);
    return true;
}

void HiddenItemEvent::setDefaultValues(Project *project) {
    this->setItem(project->itemNames.value(0, "0"));
    this->setFlag(project->flagNames.value(0, "0"));
    if (projectConfig.hiddenItemQuantityEnabled) {
        this->setQuantity(1);
    }
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        this->setUnderfoot(false);
    }
}

QSet<QString> HiddenItemEvent::getExpectedFields() {
    QSet<QString> expectedFields = {
        "x",
        "y",
        "type",
        "elevation",
        "item",
        "flag",
    };
    if (projectConfig.hiddenItemQuantityEnabled) {
        expectedFields << "quantity";
    }
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        expectedFields << "underfoot";
    }
    return expectedFields;
}



Event *SecretBaseEvent::duplicate() const {
    SecretBaseEvent *copy = new SecretBaseEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setBaseID(this->getBaseID());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *SecretBaseEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new SecretBaseFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object SecretBaseEvent::buildEventJson(Project *) {
    OrderedJson::object secretBaseJson;

    secretBaseJson["type"] = Event::typeToJsonKey(Event::Type::SecretBase);
    secretBaseJson["x"] = this->getX();
    secretBaseJson["y"] = this->getY();
    secretBaseJson["elevation"] = this->getElevation();
    secretBaseJson["secret_base_id"] = this->getBaseID();

    OrderedJson::append(&secretBaseJson, this->getCustomAttributes());
    return secretBaseJson;
}

bool SecretBaseEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setElevation(readInt(&json, "elevation"));
    this->setBaseID(readString(&json, "secret_base_id"));

    this->setCustomAttributes(json);
    return true;
}

void SecretBaseEvent::setDefaultValues(Project *project) {
    this->setBaseID(project->secretBaseIds.value(0, "0"));
    this->setElevation(0);
}

QSet<QString> SecretBaseEvent::getExpectedFields() {
    static const QSet<QString> expectedFields = {
        "x",
        "y",
        "type",
        "elevation",
        "secret_base_id",
    };
    return expectedFields;
}



Event *HealLocationEvent::duplicate() const {
    HealLocationEvent *copy = new HealLocationEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setIdName(this->getIdName());
    copy->setRespawnMapName(this->getRespawnMapName());
    copy->setRespawnNPC(this->getRespawnNPC());

    copy->setCustomAttributes(this->getCustomAttributes());

    return copy;
}

EventFrame *HealLocationEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new HealLocationFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

QString HealLocationEvent::getHostMapName() const {
    return this->getMap() ? this->getMap()->constantName() : this->hostMapName;
}

OrderedJson::object HealLocationEvent::buildEventJson(Project *project) {
    OrderedJson::object healLocationJson;

    healLocationJson["id"] = this->getIdName();
    healLocationJson["map"] = this->getHostMapName();
    healLocationJson["x"] = this->getX();
    healLocationJson["y"] = this->getY();
    if (projectConfig.healLocationRespawnDataEnabled) {
        const QString mapName = this->getRespawnMapName();
        healLocationJson["respawn_map"] = project->getMapConstant(mapName, mapName);
        healLocationJson["respawn_npc"] = this->getRespawnNPC();
    }

    OrderedJson::append(&healLocationJson, this->getCustomAttributes());
    return healLocationJson;
}

bool HealLocationEvent::loadFromJson(QJsonObject json, Project *project) {
    this->setX(readInt(&json, "x"));
    this->setY(readInt(&json, "y"));
    this->setIdName(readString(&json, "id"));
    this->setHostMapName(readString(&json, "map"));

    if (projectConfig.healLocationRespawnDataEnabled) {
        // Log a warning if "respawn_map" isn't a known map ID, but don't overwrite user data.
        const QString mapConstant = readString(&json, "respawn_map");
        if (!project->mapConstantsToMapNames.contains(mapConstant))
            logWarn(QString("Unknown Respawn Map constant '%1'.").arg(mapConstant));
        this->setRespawnMapName(project->mapConstantsToMapNames.value(mapConstant, mapConstant));
        this->setRespawnNPC(readString(&json, "respawn_npc"));
    }

    this->setCustomAttributes(json);
    return true;
}

void HealLocationEvent::setDefaultValues(Project *project) {
    if (this->map) {
        this->setIdName(project->getNewHealLocationName(this->map));
        this->setRespawnMapName(this->map->name());
    }
    this->setRespawnNPC(QString::number(0 + this->getIndexOffset(Event::Group::Object)));
}

const QSet<QString> expectedHealLocationFields = {

};

QSet<QString> HealLocationEvent::getExpectedFields() {
    QSet<QString> expectedFields = {
        "x",
        "y",
        "id",
        "map",
    };
    if (projectConfig.healLocationRespawnDataEnabled) {
        expectedFields.insert("respawn_map");
        expectedFields.insert("respawn_npc");
    }
    return expectedFields;
}
