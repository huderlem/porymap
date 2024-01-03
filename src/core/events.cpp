#include "events.h"

#include "eventframes.h"
#include "project.h"
#include "config.h"

QMap<Event::Group, const QPixmap*> Event::icons;

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
    return this->map->events.value(this->getEventGroup()).indexOf(this);
}

void Event::setDefaultValues(Project *) {
    this->setX(0);
    this->setY(0);
    this->setElevation(projectConfig.getDefaultElevation());
}

void Event::readCustomValues(QJsonObject values) {
    this->customValues.clear();
    QSet<QString> expectedFields = this->getExpectedFields();
    for (QString key : values.keys()) {
        if (!expectedFields.contains(key)) {
            this->customValues[key] = values[key];
        }
    }
}

void Event::addCustomValuesTo(OrderedJson::object *obj) {
    for (QString key : this->customValues.keys()) {
        if (!obj->contains(key)) {
            (*obj)[key] = OrderedJson::fromQJsonValue(this->customValues[key]);
        }
    }
}

void Event::modify() {
    this->map->modify();
}

QString Event::eventGroupToString(Event::Group group) {
    switch (group) {
    case Event::Group::Object:
        return "Object";
    case Event::Group::Warp:
        return "Warp";
    case Event::Group::Coord:
        return "Trigger";
    case Event::Group::Bg:
        return "BG";
    case Event::Group::Heal:
        return "Healspot";
    default:
        return "";
    }
}

QString Event::eventTypeToString(Event::Type type) {
    switch (type) {
    case Event::Type::Object:
        return "event_object";
    case Event::Type::CloneObject:
        return "event_clone_object";
    case Event::Type::Warp:
        return "event_warp";
    case Event::Type::Trigger:
        return "event_trigger";
    case Event::Type::WeatherTrigger:
        return "event_weather_trigger";
    case Event::Type::Sign:
        return "event_sign";
    case Event::Type::HiddenItem:
        return "event_hidden_item";
    case Event::Type::SecretBase:
        return "event_secret_base";
    case Event::Type::HealLocation:
        return "event_healspot";
    default:
        return "";
    }
}

Event::Type Event::eventTypeFromString(QString type) {
    if (type == "event_object") {
        return Event::Type::Object;
    } else if (type == "event_clone_object") {
        return Event::Type::CloneObject;
    } else if (type == "event_warp") {
        return Event::Type::Warp;
    } else if (type == "event_trigger") {
        return Event::Type::Trigger;
    } else if (type == "event_weather_trigger") {
        return Event::Type::WeatherTrigger;
    } else if (type == "event_sign") {
        return Event::Type::Sign;
    } else if (type == "event_hidden_item") {
        return Event::Type::HiddenItem;
    } else if (type == "event_secret_base") {
        return Event::Type::SecretBase;
    } else if (type == "event_healspot") {
        return Event::Type::HealLocation;
    } else {
        return Event::Type::None;
    }
}

void Event::loadPixmap(Project *) {
    const QPixmap * pixmap = Event::icons.value(this->getEventGroup());
    this->pixmap = pixmap ? *pixmap : QPixmap();
}

void Event::setIcons() {
    qDeleteAll(icons);
    icons.clear();

    const int w = 16;
    const int h = 16;
    static const QPixmap defaultIcons = QPixmap(":/images/Entities_16x16.png");

    // Custom event icons may be provided by the user.
    const int numIcons = qMin(defaultIcons.width() / w, static_cast<int>(Event::Group::None));
    for (int i = 0; i < numIcons; i++) {
        Event::Group group = static_cast<Event::Group>(i);
        QString customIconPath = projectConfig.getEventIconPath(group);
        if (customIconPath.isEmpty()) {
            // No custom icon specified, use the default icon.
            icons[group] = new QPixmap(defaultIcons.copy(i * w, 0, w, h));
            continue;
        }

        // Try to load custom icon
        QString validPath = Project::getExistingFilepath(customIconPath);
        if (!validPath.isEmpty()) customIconPath = validPath; // Otherwise allow it to fail with the original path
        const QPixmap customIcon = QPixmap(customIconPath);
        if (customIcon.isNull()) {
            // Custom icon failed to load, use the default icon.
            icons[group] = new QPixmap(defaultIcons.copy(i * w, 0, w, h));
            logWarn(QString("Failed to load custom event icon '%1', using default icon.").arg(customIconPath));
        } else {
            icons[group] = new QPixmap(customIcon.scaled(w, h));
        }
    }
}


Event *ObjectEvent::duplicate() {
    ObjectEvent *copy = new ObjectEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setGfx(this->getGfx());
    copy->setMovement(this->getMovement());
    copy->setRadiusX(this->getRadiusX());
    copy->setRadiusY(this->getRadiusY());
    copy->setTrainerType(this->getTrainerType());
    copy->setSightRadiusBerryTreeID(this->getSightRadiusBerryTreeID());
    copy->setScript(this->getScript());
    copy->setFlag(this->getFlag());
    copy->setCustomValues(this->getCustomValues());

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

    if (projectConfig.getEventCloneObjectEnabled()) {
        objectJson["type"] = "object";
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
    this->addCustomValuesTo(&objectJson);

    return objectJson;
}

bool ObjectEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setGfx(ParseUtil::jsonToQString(json["graphics_id"]));
    this->setMovement(ParseUtil::jsonToQString(json["movement_type"]));
    this->setRadiusX(ParseUtil::jsonToInt(json["movement_range_x"]));
    this->setRadiusY(ParseUtil::jsonToInt(json["movement_range_y"]));
    this->setTrainerType(ParseUtil::jsonToQString(json["trainer_type"]));
    this->setSightRadiusBerryTreeID(ParseUtil::jsonToQString(json["trainer_sight_or_berry_tree_id"]));
    this->setScript(ParseUtil::jsonToQString(json["script"]));
    this->setFlag(ParseUtil::jsonToQString(json["flag"]));
    
    this->readCustomValues(json);

    return true;
}

void ObjectEvent::setDefaultValues(Project *project) {
    this->setGfx(project->gfxDefines.keys().first());
    this->setMovement(project->movementTypes.first());
    this->setScript("NULL");
    this->setTrainerType(project->trainerTypes.value(0, "0"));
    this->setFlag("0");
    this->setRadiusX(0);
    this->setRadiusY(0);
    this->setSightRadiusBerryTreeID("0");
    this->setFrameFromMovement(project->facingDirections.value(this->getMovement()));
}

const QSet<QString> expectedObjectFields = {
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
    if (projectConfig.getEventCloneObjectEnabled()) {
        expectedFields.insert("type");
    }
    expectedFields << "x" << "y";
    return expectedFields;
}

void ObjectEvent::loadPixmap(Project *project) {
    EventGraphics *eventGfx = project->eventGraphicsMap.value(this->gfx, nullptr);
    if (!eventGfx) {
        // Invalid gfx constant.
        // If this is a number, try to use that instead.
        bool ok;
        int altGfx = ParseUtil::gameStringToInt(this->gfx, &ok);
        if (ok && (altGfx < project->gfxDefines.count())) {
            eventGfx = project->eventGraphicsMap.value(project->gfxDefines.key(altGfx, "NULL"), nullptr);
        }
    }
    if (!eventGfx || eventGfx->spritesheet.isNull()) {
        // No sprite associated with this gfx constant.
        // Use default sprite instead.
        Event::loadPixmap(project);
        this->spriteWidth = 16;
        this->spriteHeight = 16;
        this->usingSprite = false;
    } else {
        this->setFrameFromMovement(project->facingDirections.value(this->movement));
        this->setPixmapFromSpritesheet(eventGfx);
    }
}

void ObjectEvent::setPixmapFromSpritesheet(EventGraphics * gfx)
{
    QImage img;
    if (gfx->inanimate) {
        img = gfx->spritesheet.copy(0, 0, gfx->spriteWidth, gfx->spriteHeight);
    } else {
        int x = 0;
        int y = 0;

        // Get frame's position in spritesheet.
        // Assume horizontal layout. If position would exceed sheet width, try vertical layout.
        if ((this->frame + 1) * gfx->spriteWidth <= gfx->spritesheet.width()) {
            x = this->frame * gfx->spriteWidth;
        } else if ((this->frame + 1) * gfx->spriteHeight <= gfx->spritesheet.height()) {
            y = this->frame * gfx->spriteHeight;
        }

        img = gfx->spritesheet.copy(x, y, gfx->spriteWidth, gfx->spriteHeight);

        // Right-facing sprite is just the left-facing sprite mirrored
        if (this->hFlip) {
            img = img.transformed(QTransform().scale(-1, 1));
        }
    }
    // Set first palette color fully transparent.
    img.setColor(0, qRgba(0, 0, 0, 0));
    pixmap = QPixmap::fromImage(img);
    this->spriteWidth = gfx->spriteWidth;
    this->spriteHeight = gfx->spriteHeight;
    this->usingSprite = true;
}

void ObjectEvent::setFrameFromMovement(QString facingDir) {
    // defaults
    // TODO: read this from a file somewhere?
    this->frame = 0;
    this->hFlip = false;
    if (facingDir == "DIR_NORTH") {
        this->frame = 1;
        this->hFlip = false;
    } else if (facingDir == "DIR_SOUTH") {
        this->frame = 0;
        this->hFlip = false;
    } else if (facingDir == "DIR_WEST") {
        this->frame = 2;
        this->hFlip = false;
    } else if (facingDir == "DIR_EAST") {
        this->frame = 2;
        this->hFlip = true;
    }
}



Event *CloneObjectEvent::duplicate() {
    CloneObjectEvent *copy = new CloneObjectEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setGfx(this->getGfx());
    copy->setTargetID(this->getTargetID());
    copy->setTargetMap(this->getTargetMap());
    copy->setCustomValues(this->getCustomValues());

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

    cloneJson["type"] = "clone";
    cloneJson["graphics_id"] = this->getGfx();
    cloneJson["x"] = this->getX();
    cloneJson["y"] = this->getY();
    cloneJson["target_local_id"] = this->getTargetID();
    cloneJson["target_map"] = project->mapNamesToMapConstants.value(this->getTargetMap());
    this->addCustomValuesTo(&cloneJson);

    return cloneJson;
}

bool CloneObjectEvent::loadFromJson(QJsonObject json, Project *project) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setGfx(ParseUtil::jsonToQString(json["graphics_id"]));
    this->setTargetID(ParseUtil::jsonToInt(json["target_local_id"]));

    // Ensure the target map constant is valid before adding it to the events.
    const QString dynamicMapConstant = project->getDynamicMapDefineName();
    QString mapConstant = ParseUtil::jsonToQString(json["target_map"]);
    if (project->mapConstantsToMapNames.contains(mapConstant)) {
        this->setTargetMap(project->mapConstantsToMapNames.value(mapConstant));
    } else if (mapConstant == dynamicMapConstant) {
        this->setTargetMap(DYNAMIC_MAP_NAME);
    } else {
        logWarn(QString("Target Map constant '%1' is invalid. Using default '%2'.").arg(mapConstant).arg(dynamicMapConstant));
        this->setTargetMap(DYNAMIC_MAP_NAME);
    }

    this->readCustomValues(json);

    return true;
}

void CloneObjectEvent::setDefaultValues(Project *project) {
    this->setGfx(project->gfxDefines.keys().first());
    this->setTargetID(1);
    if (this->getMap()) this->setTargetMap(this->getMap()->name);
}

const QSet<QString> expectedCloneObjectFields = {
    "type",
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
    Event *clonedEvent = clonedMap ? clonedMap->events[Event::Group::Object].value(eventIndex, nullptr) : nullptr;

    if (clonedEvent && clonedEvent->getEventType() == Event::Type::Object) {
        // Get graphics data from cloned object
        ObjectEvent *clonedObject = dynamic_cast<ObjectEvent *>(clonedEvent);
        this->gfx = clonedObject->getGfx();
        this->movement = clonedObject->getMovement();
    } else {
        // Invalid object specified, use default graphics data (as would be shown in-game)
        this->gfx = project->gfxDefines.key(0);
        this->movement = project->movementTypes.first();
    }

    EventGraphics *eventGfx = project->eventGraphicsMap.value(gfx, nullptr);
    if (!eventGfx || eventGfx->spritesheet.isNull()) {
        // No sprite associated with this gfx constant.
        // Use default sprite instead.
        Event::loadPixmap(project);
        this->spriteWidth = 16;
        this->spriteHeight = 16;
        this->usingSprite = false;
    } else {
        this->setFrameFromMovement(project->facingDirections.value(this->movement));
        this->setPixmapFromSpritesheet(eventGfx);
    }
}



Event *WarpEvent::duplicate() {
    WarpEvent *copy = new WarpEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setDestinationMap(this->getDestinationMap());
    copy->setDestinationWarpID(this->getDestinationWarpID());

    copy->setCustomValues(this->getCustomValues());

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
    warpJson["dest_map"] = project->mapNamesToMapConstants.value(this->getDestinationMap());
    warpJson["dest_warp_id"] = this->getDestinationWarpID();

    this->addCustomValuesTo(&warpJson);

    return warpJson;
}

bool WarpEvent::loadFromJson(QJsonObject json, Project *project) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setDestinationWarpID(ParseUtil::jsonToQString(json["dest_warp_id"]));

    // Ensure the warp destination map constant is valid before adding it to the warps.
    const QString dynamicMapConstant = project->getDynamicMapDefineName();
    QString mapConstant = ParseUtil::jsonToQString(json["dest_map"]);
    if (project->mapConstantsToMapNames.contains(mapConstant)) {
        this->setDestinationMap(project->mapConstantsToMapNames.value(mapConstant));
    } else if (mapConstant == dynamicMapConstant) {
        this->setDestinationMap(DYNAMIC_MAP_NAME);
    } else {
        logWarn(QString("Destination Map constant '%1' is invalid. Using default '%2'.").arg(mapConstant).arg(dynamicMapConstant));
        this->setDestinationMap(DYNAMIC_MAP_NAME);
    }

    this->readCustomValues(json);

    return true;
}

void WarpEvent::setDefaultValues(Project *) {
    if (this->getMap()) this->setDestinationMap(this->getMap()->name);
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



Event *TriggerEvent::duplicate() {
    TriggerEvent *copy = new TriggerEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setScriptVar(this->getScriptVar());
    copy->setScriptVarValue(this->getScriptVarValue());
    copy->setScriptLabel(this->getScriptLabel());

    copy->setCustomValues(this->getCustomValues());

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

    triggerJson["type"] = "trigger";
    triggerJson["x"] = this->getX();
    triggerJson["y"] = this->getY();
    triggerJson["elevation"] = this->getElevation();
    triggerJson["var"] = this->getScriptVar();
    triggerJson["var_value"] = this->getScriptVarValue();
    triggerJson["script"] = this->getScriptLabel();

    this->addCustomValuesTo(&triggerJson);

    return triggerJson;
}

bool TriggerEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setScriptVar(ParseUtil::jsonToQString(json["var"]));
    this->setScriptVarValue(ParseUtil::jsonToQString(json["var_value"]));
    this->setScriptLabel(ParseUtil::jsonToQString(json["script"]));

    this->readCustomValues(json);

    return true;
}

void TriggerEvent::setDefaultValues(Project *project) {
    this->setScriptLabel("NULL");
    this->setScriptVar(project->varNames.first());
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



Event *WeatherTriggerEvent::duplicate() {
    WeatherTriggerEvent *copy = new WeatherTriggerEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setWeather(this->getWeather());

    copy->setCustomValues(this->getCustomValues());

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

    weatherJson["type"] = "weather";
    weatherJson["x"] = this->getX();
    weatherJson["y"] = this->getY();
    weatherJson["elevation"] = this->getElevation();
    weatherJson["weather"] = this->getWeather();

    this->addCustomValuesTo(&weatherJson);

    return weatherJson;
}

bool WeatherTriggerEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setWeather(ParseUtil::jsonToQString(json["weather"]));

    this->readCustomValues(json);

    return true;
}

void WeatherTriggerEvent::setDefaultValues(Project *project) {
    this->setWeather(project->coordEventWeatherNames.first());
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



Event *SignEvent::duplicate() {
    SignEvent *copy = new SignEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setFacingDirection(this->getFacingDirection());
    copy->setScriptLabel(this->getScriptLabel());

    copy->setCustomValues(this->getCustomValues());

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

    signJson["type"] = "sign";
    signJson["x"] = this->getX();
    signJson["y"] = this->getY();
    signJson["elevation"] = this->getElevation();
    signJson["player_facing_dir"] = this->getFacingDirection();
    signJson["script"] = this->getScriptLabel();

    this->addCustomValuesTo(&signJson);

    return signJson;
}

bool SignEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setFacingDirection(ParseUtil::jsonToQString(json["player_facing_dir"]));
    this->setScriptLabel(ParseUtil::jsonToQString(json["script"]));

    this->readCustomValues(json);

    return true;
}

void SignEvent::setDefaultValues(Project *project) {
    this->setFacingDirection(project->bgEventFacingDirections.first());
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



Event *HiddenItemEvent::duplicate() {
    HiddenItemEvent *copy = new HiddenItemEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setItem(this->getItem());
    copy->setFlag(this->getFlag());
    copy->setQuantity(this->getQuantity());
    copy->setQuantity(this->getQuantity());

    copy->setCustomValues(this->getCustomValues());

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

    hiddenItemJson["type"] = "hidden_item";
    hiddenItemJson["x"] = this->getX();
    hiddenItemJson["y"] = this->getY();
    hiddenItemJson["elevation"] = this->getElevation();
    hiddenItemJson["item"] = this->getItem();
    hiddenItemJson["flag"] = this->getFlag();
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        hiddenItemJson["quantity"] = this->getQuantity();
    }
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
        hiddenItemJson["underfoot"] = this->getUnderfoot();
    }

    this->addCustomValuesTo(&hiddenItemJson);

    return hiddenItemJson;
}

bool HiddenItemEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setItem(ParseUtil::jsonToQString(json["item"]));
    this->setFlag(ParseUtil::jsonToQString(json["flag"]));
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        this->setQuantity(ParseUtil::jsonToInt(json["quantity"]));
    }
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
        this->setUnderfoot(ParseUtil::jsonToBool(json["underfoot"]));
    }

    this->readCustomValues(json);

    return true;
}

void HiddenItemEvent::setDefaultValues(Project *project) {
    this->setItem(project->itemNames.first());
    this->setFlag(project->flagNames.first());
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        this->setQuantity(1);
    }
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
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
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        expectedFields << "quantity";
    }
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
        expectedFields << "underfoot";
    }
    expectedFields << "x" << "y";
    return expectedFields;
}



Event *SecretBaseEvent::duplicate() {
    SecretBaseEvent *copy = new SecretBaseEvent();

    copy->setX(this->getX());
    copy->setY(this->getY());
    copy->setElevation(this->getElevation());
    copy->setBaseID(this->getBaseID());

    copy->setCustomValues(this->getCustomValues());

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

    secretBaseJson["type"] = "secret_base";
    secretBaseJson["x"] = this->getX();
    secretBaseJson["y"] = this->getY();
    secretBaseJson["elevation"] = this->getElevation();
    secretBaseJson["secret_base_id"] = this->getBaseID();

    this->addCustomValuesTo(&secretBaseJson);

    return secretBaseJson;
}

bool SecretBaseEvent::loadFromJson(QJsonObject json, Project *) {
    this->setX(ParseUtil::jsonToInt(json["x"]));
    this->setY(ParseUtil::jsonToInt(json["y"]));
    this->setElevation(ParseUtil::jsonToInt(json["elevation"]));
    this->setBaseID(ParseUtil::jsonToQString(json["secret_base_id"]));

    this->readCustomValues(json);

    return true;
}

void SecretBaseEvent::setDefaultValues(Project *project) {
    this->setBaseID(project->secretBaseIds.first());
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



EventFrame *HealLocationEvent::createEventFrame() {
    if (!this->eventFrame) {
        this->eventFrame = new HealLocationFrame(this);
        this->eventFrame->setup();
    }
    return this->eventFrame;
}

OrderedJson::object HealLocationEvent::buildEventJson(Project *) {
    return OrderedJson::object();
}

void HealLocationEvent::setDefaultValues(Project *) {
    this->setElevation(projectConfig.getDefaultElevation());
    if (!this->getMap())
        return;
    bool respawnEnabled = projectConfig.getHealLocationRespawnDataEnabled();
    const QString mapConstant = Map::mapConstantFromName(this->getMap()->name, false);
    const QString prefix = projectConfig.getIdentifier(respawnEnabled ? ProjectIdentifier::define_spawn_prefix
                                                                      : ProjectIdentifier::define_heal_locations_prefix);
    this->setLocationName(mapConstant);
    this->setIdName(prefix + mapConstant);
    if (respawnEnabled) {
        this->setRespawnMap(this->getMap()->name);
        this->setRespawnNPC(1);
    }
}
