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

void Event::setPixmapItem(DraggablePixmapItem *item) {
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

void Event::readCustomAttributes(const QJsonObject &json) {
    this->customAttributes.clear();
    const QSet<QString> expectedFields = this->getExpectedFields();
    for (auto i = json.constBegin(); i != json.constEnd(); i++) {
        if (!expectedFields.contains(i.key())) {
            this->customAttributes[i.key()] = i.value();
        }
    }
}

void Event::addCustomAttributesTo(OrderedJson::object *obj) const {
    for (auto i = this->customAttributes.constBegin(); i != this->customAttributes.constEnd(); i++) {
        if (!obj->contains(i.key())) {
            (*obj)[i.key()] = OrderedJson::fromQJsonValue(i.value());
        }
    }
}

void Event::modify() {
    this->map->modify();
}

QString Event::groupToString(Event::Group group) {
    static const QMap<Event::Group, QString> groupToStringMap = {
        {Event::Group::Object, "Object"},
        {Event::Group::Warp, "Warp"},
        {Event::Group::Coord, "Trigger"},
        {Event::Group::Bg, "BG"},
        {Event::Group::Heal, "Heal Location"},
    };
    return groupToStringMap.value(group);
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
        {Event::Type::WeatherTrigger, "Weather"},
        {Event::Type::Sign, "Sign"},
        {Event::Type::HiddenItem, "Hidden Item"},
        {Event::Type::SecretBase, "Secret Base"},
        {Event::Type::HealLocation, "Heal Location"},
    };
    return typeToStringMap.value(type);
}

void Event::loadPixmap(Project *project) {
    this->pixmap = project->getEventPixmap(this->getEventGroup());
    this->usesDefaultPixmap = true;
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

    if (projectConfig.eventCloneObjectEnabled) {
        objectJson["type"] = Event::typeToJsonKey(Event::Type::Object);
    }
    QString idName = this->getIdName();
    if (!idName.isEmpty())
        objectJson["local_id"] = idName;
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
    this->addCustomAttributesTo(&objectJson);

    return objectJson;
}

bool ObjectEvent::loadFromJson(const QJsonObject &json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setIdName(ParseUtil::jsonToQString(json["local_id"]));
    this->setGfx(ParseUtil::jsonToQString(json["graphics_id"]));
    this->setMovement(ParseUtil::jsonToQString(json["movement_type"]));
    this->setRadiusX(ParseUtil::jsonToInt(json["movement_range_x"]));
    this->setRadiusY(ParseUtil::jsonToInt(json["movement_range_y"]));
    this->setTrainerType(ParseUtil::jsonToQString(json["trainer_type"]));
    this->setSightRadiusBerryTreeID(ParseUtil::jsonToQString(json["trainer_sight_or_berry_tree_id"]));
    this->setScript(ParseUtil::jsonToQString(json["script"]));
    this->setFlag(ParseUtil::jsonToQString(json["flag"]));
    
    this->readCustomAttributes(json);

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

const QSet<QString> expectedObjectFields = {
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

QSet<QString> ObjectEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedObjectFields;
    if (projectConfig.eventCloneObjectEnabled) {
        expectedFields.insert("type");
    }
    expectedFields << "x" << "y";
    return expectedFields;
}

void ObjectEvent::loadPixmap(Project *project) {
    this->pixmap = project->getEventPixmap(this->gfx, this->movement);
    if (!this->pixmap.isNull()) {
        this->usesDefaultPixmap = false;
    } else {
        Event::loadPixmap(project);
    }
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

    cloneJson["type"] = Event::typeToJsonKey(Event::Type::CloneObject);
    QString idName = this->getIdName();
    if (!idName.isEmpty())
        cloneJson["local_id"] = idName;
    cloneJson["graphics_id"] = this->getGfx();
    cloneJson["x"] = this->getX();
    cloneJson["y"] = this->getY();
    cloneJson["target_local_id"] = this->getTargetID();
    const QString mapName = this->getTargetMap();
    cloneJson["target_map"] = project->mapNamesToMapConstants.value(mapName, mapName);
    this->addCustomAttributesTo(&cloneJson);

    return cloneJson;
}

bool CloneObjectEvent::loadFromJson(const QJsonObject &json, Project *project) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setIdName(ParseUtil::jsonToQString(json["local_id"]));
    this->setGfx(ParseUtil::jsonToQString(json["graphics_id"]));
    this->setTargetID(ParseUtil::jsonToInt(json["target_local_id"]));

    // Log a warning if "target_map" isn't a known map ID, but don't overwrite user data.
    const QString mapConstant = ParseUtil::jsonToQString(json["target_map"]);
    if (!project->mapConstantsToMapNames.contains(mapConstant))
        logWarn(QString("Unknown Target Map constant '%1'.").arg(mapConstant));
    this->setTargetMap(project->mapConstantsToMapNames.value(mapConstant, mapConstant));

    this->readCustomAttributes(json);

    return true;
}

void CloneObjectEvent::setDefaultValues(Project *project) {
    this->setGfx(project->gfxDefines.key(0, "0"));
    this->setTargetID(1);
    if (this->getMap()) this->setTargetMap(this->getMap()->name());
}

const QSet<QString> expectedCloneObjectFields = {
    "type",
    "local_id",
    "graphics_id",
    "target_local_id",
    "target_map",
};

QSet<QString> CloneObjectEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedCloneObjectFields;
    expectedFields << "x" << "y";
    return expectedFields;
}

void CloneObjectEvent::loadPixmap(Project *project) {
    // Try to get the targeted object to clone
    int eventIndex = this->targetID - 1;
    Map *clonedMap = project->getMap(this->targetMap);
    Event *clonedEvent = clonedMap ? clonedMap->getEvent(Event::Group::Object, eventIndex) : nullptr;

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
    ObjectEvent::loadPixmap(project);
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
    warpJson["dest_map"] = project->mapNamesToMapConstants.value(mapName, mapName);
    warpJson["dest_warp_id"] = this->getDestinationWarpID();

    this->addCustomAttributesTo(&warpJson);

    return warpJson;
}

bool WarpEvent::loadFromJson(const QJsonObject &json, Project *project) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setDestinationWarpID(ParseUtil::jsonToQString(json["dest_warp_id"]));

    // Log a warning if "dest_map" isn't a known map ID, but don't overwrite user data.
    const QString mapConstant = ParseUtil::jsonToQString(json["dest_map"]);
    if (!project->mapConstantsToMapNames.contains(mapConstant))
        logWarn(QString("Unknown Destination Map constant '%1'.").arg(mapConstant));
    this->setDestinationMap(project->mapConstantsToMapNames.value(mapConstant, mapConstant));

    this->readCustomAttributes(json);

    return true;
}

void WarpEvent::setDefaultValues(Project *) {
    if (this->getMap()) this->setDestinationMap(this->getMap()->name());
    this->setDestinationWarpID("0");
    this->setElevation(0);
}

const QSet<QString> expectedWarpFields = {
    "elevation",
    "dest_map",
    "dest_warp_id",
};

QSet<QString> WarpEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedWarpFields;
    expectedFields << "x" << "y";
    return expectedFields;
}

void WarpEvent::setWarningEnabled(bool enabled) {
    WarpFrame * frame = static_cast<WarpFrame*>(this->getEventFrame());
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

    this->addCustomAttributesTo(&triggerJson);

    return triggerJson;
}

bool TriggerEvent::loadFromJson(const QJsonObject &json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setScriptVar(ParseUtil::jsonToQString(json["var"]));
    this->setScriptVarValue(ParseUtil::jsonToQString(json["var_value"]));
    this->setScriptLabel(ParseUtil::jsonToQString(json["script"]));

    this->readCustomAttributes(json);

    return true;
}

void TriggerEvent::setDefaultValues(Project *project) {
    this->setScriptLabel("NULL");
    this->setScriptVar(project->varNames.value(0, "0"));
    this->setScriptVarValue("0");
    this->setElevation(0);
}

const QSet<QString> expectedTriggerFields = {
    "type",
    "elevation",
    "var",
    "var_value",
    "script",
};

QSet<QString> TriggerEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedTriggerFields;
    expectedFields << "x" << "y";
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

    this->addCustomAttributesTo(&weatherJson);

    return weatherJson;
}

bool WeatherTriggerEvent::loadFromJson(const QJsonObject &json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setWeather(ParseUtil::jsonToQString(json["weather"]));

    this->readCustomAttributes(json);

    return true;
}

void WeatherTriggerEvent::setDefaultValues(Project *project) {
    this->setWeather(project->coordEventWeatherNames.value(0, "0"));
    this->setElevation(0);
}

const QSet<QString> expectedWeatherTriggerFields = {
    "type",
    "elevation",
    "weather",
};

QSet<QString> WeatherTriggerEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedWeatherTriggerFields;
    expectedFields << "x" << "y";
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

    this->addCustomAttributesTo(&signJson);

    return signJson;
}

bool SignEvent::loadFromJson(const QJsonObject &json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setFacingDirection(ParseUtil::jsonToQString(json["player_facing_dir"]));
    this->setScriptLabel(ParseUtil::jsonToQString(json["script"]));

    this->readCustomAttributes(json);

    return true;
}

void SignEvent::setDefaultValues(Project *project) {
    this->setFacingDirection(project->bgEventFacingDirections.value(0, "0"));
    this->setScriptLabel("NULL");
    this->setElevation(0);
}

const QSet<QString> expectedSignFields = {
    "type",
    "elevation",
    "player_facing_dir",
    "script",
};

QSet<QString> SignEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedSignFields;
    expectedFields << "x" << "y";
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
    copy->setQuantity(this->getQuantity());

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

    this->addCustomAttributesTo(&hiddenItemJson);

    return hiddenItemJson;
}

bool HiddenItemEvent::loadFromJson(const QJsonObject &json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setItem(ParseUtil::jsonToQString(json["item"]));
    this->setFlag(ParseUtil::jsonToQString(json["flag"]));
    if (projectConfig.hiddenItemQuantityEnabled) {
        this->setQuantity(ParseUtil::jsonToInt(json["quantity"]));
    }
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        this->setUnderfoot(ParseUtil::jsonToBool(json["underfoot"]));
    }

    this->readCustomAttributes(json);

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

const QSet<QString> expectedHiddenItemFields = {
    "type",
    "elevation",
    "item",
    "flag",
};

QSet<QString> HiddenItemEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedHiddenItemFields;
    if (projectConfig.hiddenItemQuantityEnabled) {
        expectedFields << "quantity";
    }
    if (projectConfig.hiddenItemRequiresItemfinderEnabled) {
        expectedFields << "underfoot";
    }
    expectedFields << "x" << "y";
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

    this->addCustomAttributesTo(&secretBaseJson);

    return secretBaseJson;
}

bool SecretBaseEvent::loadFromJson(const QJsonObject &json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setBaseID(ParseUtil::jsonToQString(json["secret_base_id"]));

    this->readCustomAttributes(json);

    return true;
}

void SecretBaseEvent::setDefaultValues(Project *project) {
    this->setBaseID(project->secretBaseIds.value(0, "0"));
    this->setElevation(0);
}

const QSet<QString> expectedSecretBaseFields = {
    "type",
    "elevation",
    "secret_base_id",
};

QSet<QString> SecretBaseEvent::getExpectedFields() {
    QSet<QString> expectedFields = QSet<QString>();
    expectedFields = expectedSecretBaseFields;
    expectedFields << "x" << "y";
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

OrderedJson::object HealLocationEvent::buildEventJson(Project *project) {
    OrderedJson::object healLocationJson;

    healLocationJson["id"] = this->getIdName();
    // This field doesn't need to be stored in the Event itself, so it's output only.
    healLocationJson["map"] = this->getMap() ? this->getMap()->constantName() : QString();
    healLocationJson["x"] = this->getX();
    healLocationJson["y"] = this->getY();
    if (projectConfig.healLocationRespawnDataEnabled) {
        const QString mapName = this->getRespawnMapName();
        healLocationJson["respawn_map"] = project->mapNamesToMapConstants.value(mapName, mapName);
        healLocationJson["respawn_npc"] = this->getRespawnNPC();
    }

    this->addCustomAttributesTo(&healLocationJson);

    return healLocationJson;
}

bool HealLocationEvent::loadFromJson(const QJsonObject &json, Project *project) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setIdName(ParseUtil::jsonToQString(json["id"]));

    if (projectConfig.healLocationRespawnDataEnabled) {
        // Log a warning if "respawn_map" isn't a known map ID, but don't overwrite user data.
        const QString mapConstant = ParseUtil::jsonToQString(json["respawn_map"]);
        if (!project->mapConstantsToMapNames.contains(mapConstant))
            logWarn(QString("Unknown Respawn Map constant '%1'.").arg(mapConstant));
        this->setRespawnMapName(project->mapConstantsToMapNames.value(mapConstant, mapConstant));
        this->setRespawnNPC(ParseUtil::jsonToQString(json["respawn_npc"]));
    }

    this->readCustomAttributes(json);
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
    "id",
    "map"
};

QSet<QString> HealLocationEvent::getExpectedFields() {
    QSet<QString> expectedFields = expectedHealLocationFields;
    if (projectConfig.healLocationRespawnDataEnabled) {
        expectedFields.insert("respawn_map");
        expectedFields.insert("respawn_npc");
    }
    expectedFields << "x" << "y";
    return expectedFields;
}
