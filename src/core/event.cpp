#include "event.h"
#include "map.h"
#include "project.h"
#include "config.h"

QString EventGroup::Object = "object_event_group";
QString EventGroup::Warp = "warp_event_group";
QString EventGroup::Heal = "heal_event_group";
QString EventGroup::Coord = "coord_event_group";
QString EventGroup::Bg = "bg_event_group";

QString EventType::Object = "event_object";
QString EventType::CloneObject = "event_clone_object";
QString EventType::Warp = "event_warp";
QString EventType::Trigger = "event_trigger";
QString EventType::WeatherTrigger = "event_weather_trigger";
QString EventType::Sign = "event_sign";
QString EventType::HiddenItem = "event_hidden_item";
QString EventType::SecretBase = "event_secret_base";
QString EventType::HealLocation = "event_healspot";

const QMap<QString, QString> EventTypeTable = {
    {EventType::Object,         EventGroup::Object},
    {EventType::CloneObject,    EventGroup::Object},
    {EventType::Warp,           EventGroup::Warp},
    {EventType::Trigger,        EventGroup::Coord},
    {EventType::WeatherTrigger, EventGroup::Coord},
    {EventType::Sign,           EventGroup::Bg},
    {EventType::HiddenItem,     EventGroup::Bg},
    {EventType::SecretBase,     EventGroup::Bg},
    {EventType::HealLocation,   EventGroup::Heal},
};

Event::Event() :
    spriteWidth(16),
    spriteHeight(16),
    usingSprite(false)
{  }

Event::Event(const Event& toCopy) :
    values(toCopy.values),
    customValues(toCopy.customValues),
    pixmap(toCopy.pixmap),
    spriteWidth(toCopy.spriteWidth),
    spriteHeight(toCopy.spriteHeight),
    frame(toCopy.frame),
    hFlip(toCopy.hFlip),
    usingSprite(toCopy.usingSprite)
{  }

Event::Event(QJsonObject obj, QString type) : Event()
{
    this->put("event_type", type);
    this->readCustomValues(obj);
}

Event* Event::createNewEvent(QString event_type, QString map_name, Project *project)
{
    Event *event = nullptr;
    if (event_type == EventType::Object) {
        event = createNewObjectEvent(project);
        event->setFrameFromMovement(event->get("movement_type"));
    } else if (event_type == EventType::CloneObject) {
        event = createNewCloneObjectEvent(project, map_name);
    } else if (event_type == EventType::Warp) {
        event = createNewWarpEvent(map_name);
    } else if (event_type == EventType::HealLocation) {
        event = createNewHealLocationEvent(map_name);
    } else if (event_type == EventType::Trigger) {
        event = createNewTriggerEvent(project);
    } else if (event_type == EventType::WeatherTrigger) {
        event = createNewWeatherTriggerEvent(project);
    } else if (event_type == EventType::Sign) {
        event = createNewSignEvent(project);
    } else if (event_type == EventType::HiddenItem) {
        event = createNewHiddenItemEvent(project);
    } else if (event_type == EventType::SecretBase) {
        event = createNewSecretBaseEvent(project);
    } else {
        // should never be reached but just in case
        event = new Event;
    }

    event->put("event_type", event_type);
    event->put("event_group_type", typeToGroup(event_type));
    event->setX(0);
    event->setY(0);
    return event;
}

Event* Event::createNewObjectEvent(Project *project)
{
    Event *event = new Event;
    event->put("sprite", project->gfxDefines.keys().first());
    event->put("movement_type", project->movementTypes.first());
    event->put("radius_x", 0);
    event->put("radius_y", 0);
    event->put("script_label", "NULL");
    event->put("event_flag", "0");
    event->put("replacement", "0");
    event->put("trainer_type", project->trainerTypes.value(0, "0"));
    event->put("sight_radius_tree_id", 0);
    event->put("elevation", 3);
    return event;
}

Event* Event::createNewCloneObjectEvent(Project *project, QString map_name)
{
    Event *event = new Event;
    event->put("sprite", project->gfxDefines.keys().first());
    event->put("target_local_id", 1);
    event->put("target_map", map_name);
    return event;
}

Event* Event::createNewWarpEvent(QString map_name)
{
    Event *event = new Event;
    event->put("destination_warp", 0);
    event->put("destination_map_name", map_name);
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewHealLocationEvent(QString map_name)
{
    Event *event = new Event;
    event->put("loc_name", QString(Map::mapConstantFromName(map_name)).remove(0,4));
    event->put("id_name", map_name.replace(QRegularExpression("([a-z])([A-Z])"), "\\1_\\2").toUpper());
    event->put("elevation", 3);
    if (projectConfig.getHealLocationRespawnDataEnabled()) {
        event->put("respawn_map", map_name);
        event->put("respawn_npc", 1);
    }
    return event;
}

Event* Event::createNewTriggerEvent(Project *project)
{
    Event *event = new Event;
    event->put("script_label", "NULL");
    event->put("script_var", project->varNames.first());
    event->put("script_var_value", "0");
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewWeatherTriggerEvent(Project *project)
{
    Event *event = new Event;
    event->put("weather", project->coordEventWeatherNames.first());
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewSignEvent(Project *project)
{
    Event *event = new Event;
    event->put("player_facing_direction", project->bgEventFacingDirections.first());
    event->put("script_label", "NULL");
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewHiddenItemEvent(Project *project)
{
    Event *event = new Event;
    event->put("item", project->itemNames.first());
    event->put("flag", project->flagNames.first());
    event->put("elevation", 3);
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        event->put("quantity", 1);
    }
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
        event->put("underfoot", false);
    }
    return event;
}

Event* Event::createNewSecretBaseEvent(Project *project)
{
    Event *event = new Event;
    event->put("secret_base_id", project->secretBaseIds.first());
    event->put("elevation", 0);
    return event;
}

int Event::getPixelX()
{
    return (this->x() * 16) - qMax(0, (this->spriteWidth - 16) / 2);
}

int Event::getPixelY()
{
    return (this->y() * 16) - qMax(0, this->spriteHeight - 16);
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

const QSet<QString> expectedCloneObjectFields = {
    "type",
    "graphics_id",
    "target_local_id",
    "target_map",
};

const QSet<QString> expectedWarpFields = {
    "elevation",
    "dest_map",
    "dest_warp_id",
};

const QSet<QString> expectedTriggerFields = {
    "type",
    "elevation",
    "var",
    "var_value",
    "script",
};

const QSet<QString> expectedWeatherTriggerFields = {
    "type",
    "elevation",
    "weather",
};

const QSet<QString> expectedSignFields = {
    "type",
    "elevation",
    "player_facing_dir",
    "script",
};

const QSet<QString> expectedHiddenItemFields = {
    "type",
    "elevation",
    "item",
    "flag",
};

const QSet<QString> expectedSecretBaseFields = {
    "type",
    "elevation",
    "secret_base_id",
};

QSet<QString> Event::getExpectedFields()
{
    QString type = this->get("event_type");
    QSet<QString> expectedFields = QSet<QString>();
    if (type == EventType::Object) {
        expectedFields = expectedObjectFields;
        if (projectConfig.getEventCloneObjectEnabled()) {
            expectedFields.insert("type");
        }
    } else if (type == EventType::CloneObject) {
        expectedFields = expectedCloneObjectFields;
    } else if (type == EventType::Warp) {
        expectedFields = expectedWarpFields;
    } else if (type == EventType::Trigger) {
        expectedFields = expectedTriggerFields; 
    } else if (type == EventType::WeatherTrigger) {
        expectedFields = expectedWeatherTriggerFields;
    } else if (type == EventType::Sign) {
        expectedFields = expectedSignFields;
    } else if (type == EventType::HiddenItem) {
        expectedFields = expectedHiddenItemFields;
        if (projectConfig.getHiddenItemQuantityEnabled()) {
            expectedFields.insert("quantity");
        }
        if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
            expectedFields.insert("underfoot");
        }
    } else if (type == EventType::SecretBase) {
        expectedFields = expectedSecretBaseFields;
    }
    expectedFields << "x" << "y";
    return expectedFields;
};

void Event::readCustomValues(QJsonObject values)
{
    this->customValues.clear();
    QSet<QString> expectedFields = this->getExpectedFields();
    for (QString key : values.keys()) {
        if (!expectedFields.contains(key)) {
            this->customValues[key] = values[key].toString();
        }
    }
}

void Event::addCustomValuesTo(OrderedJson::object *obj)
{
    for (QString key : this->customValues.keys()) {
        if (!obj->contains(key)) {
            (*obj)[key] = this->customValues[key];
        }
    }
}

OrderedJson::object Event::buildObjectEventJSON()
{
    OrderedJson::object objectObj;
    if (projectConfig.getEventCloneObjectEnabled()) {
        objectObj["type"] = "object";
    }
    objectObj["graphics_id"] = this->get("sprite");
    objectObj["x"] = this->getS16("x");
    objectObj["y"] = this->getS16("y");
    objectObj["elevation"] = this->getInt("elevation");
    objectObj["movement_type"] = this->get("movement_type");
    objectObj["movement_range_x"] = this->getInt("radius_x");
    objectObj["movement_range_y"] = this->getInt("radius_y");
    objectObj["trainer_type"] = this->get("trainer_type");
    objectObj["trainer_sight_or_berry_tree_id"] = this->get("sight_radius_tree_id");
    objectObj["script"] = this->get("script_label");
    objectObj["flag"] = this->get("event_flag");
    this->addCustomValuesTo(&objectObj);

    return objectObj;
}

OrderedJson::object Event::buildCloneObjectEventJSON(const QMap<QString, QString> &mapNamesToMapConstants)
{
    OrderedJson::object cloneObj;
    cloneObj["type"] = "clone";
    cloneObj["graphics_id"] = this->get("sprite");
    cloneObj["x"] = this->getS16("x");
    cloneObj["y"] = this->getS16("y");
    cloneObj["target_local_id"] = this->getInt("target_local_id");
    cloneObj["target_map"] = mapNamesToMapConstants.value(this->get("target_map"));
    this->addCustomValuesTo(&cloneObj);

    return cloneObj;
}

OrderedJson::object Event::buildWarpEventJSON(const QMap<QString, QString> &mapNamesToMapConstants)
{
    OrderedJson::object warpObj;
    warpObj["x"] = this->getU16("x");
    warpObj["y"] = this->getU16("y");
    warpObj["elevation"] = this->getInt("elevation");
    warpObj["dest_map"] = mapNamesToMapConstants.value(this->get("destination_map_name"));
    warpObj["dest_warp_id"] = this->getInt("destination_warp");
    this->addCustomValuesTo(&warpObj);

    return warpObj;
}

OrderedJson::object Event::buildTriggerEventJSON()
{
    OrderedJson::object triggerObj;
    triggerObj["type"] = "trigger";
    triggerObj["x"] = this->getU16("x");
    triggerObj["y"] = this->getU16("y");
    triggerObj["elevation"] = this->getInt("elevation");
    triggerObj["var"] = this->get("script_var");
    triggerObj["var_value"] = this->get("script_var_value");
    triggerObj["script"] = this->get("script_label");
    this->addCustomValuesTo(&triggerObj);

    return triggerObj;
}

OrderedJson::object Event::buildWeatherTriggerEventJSON()
{
    OrderedJson::object weatherObj;
    weatherObj["type"] = "weather";
    weatherObj["x"] = this->getU16("x");
    weatherObj["y"] = this->getU16("y");
    weatherObj["elevation"] = this->getInt("elevation");
    weatherObj["weather"] = this->get("weather");
    this->addCustomValuesTo(&weatherObj);

    return weatherObj;
}

OrderedJson::object Event::buildSignEventJSON()
{
    OrderedJson::object signObj;
    signObj["type"] = "sign";
    signObj["x"] = this->getU16("x");
    signObj["y"] = this->getU16("y");
    signObj["elevation"] = this->getInt("elevation");
    signObj["player_facing_dir"] = this->get("player_facing_direction");
    signObj["script"] = this->get("script_label");
    this->addCustomValuesTo(&signObj);

    return signObj;
}

OrderedJson::object Event::buildHiddenItemEventJSON()
{
    OrderedJson::object hiddenItemObj;
    hiddenItemObj["type"] = "hidden_item";
    hiddenItemObj["x"] = this->getU16("x");
    hiddenItemObj["y"] = this->getU16("y");
    hiddenItemObj["elevation"] = this->getInt("elevation");
    hiddenItemObj["item"] = this->get("item");
    hiddenItemObj["flag"] = this->get("flag");
    if (projectConfig.getHiddenItemQuantityEnabled()) {
        hiddenItemObj["quantity"] = this->getInt("quantity");
    }
    if (projectConfig.getHiddenItemRequiresItemfinderEnabled()) {
        hiddenItemObj["underfoot"] = this->getInt("underfoot") > 0 || this->get("underfoot") == "TRUE";
    }
    this->addCustomValuesTo(&hiddenItemObj);

    return hiddenItemObj;
}

OrderedJson::object Event::buildSecretBaseEventJSON()
{
    OrderedJson::object secretBaseObj;
    secretBaseObj["type"] = "secret_base";
    secretBaseObj["x"] = this->getU16("x");
    secretBaseObj["y"] = this->getU16("y");
    secretBaseObj["elevation"] = this->getInt("elevation");
    secretBaseObj["secret_base_id"] = this->get("secret_base_id");
    this->addCustomValuesTo(&secretBaseObj);

    return secretBaseObj;
}

void Event::setPixmapFromSpritesheet(QImage spritesheet, int spriteWidth, int spriteHeight, bool inanimate)
{
    int frame = inanimate ? 0 : this->frame;
    QImage img = spritesheet.copy(frame * spriteWidth % spritesheet.width(), 0, spriteWidth, spriteHeight);
    if (this->hFlip && !inanimate) {
        img = img.transformed(QTransform().scale(-1, 1));
    }
    // Set first palette color fully transparent.
    img.setColor(0, qRgba(0, 0, 0, 0));
    pixmap = QPixmap::fromImage(img);
    this->spriteWidth = spriteWidth;
    this->spriteHeight = spriteHeight;
    this->usingSprite = true;
}

void Event::setFrameFromMovement(QString facingDir) {
    // defaults
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

// All event groups excepts warps have IDs that start at 1
int Event::getIndexOffset(QString group_type) {
    return (group_type == EventGroup::Warp) ? 0 : 1;
}

bool Event::isValidType(QString event_type) {
    return EventTypeTable.contains(event_type);
}

QString Event::typeToGroup(QString event_type) {
    return EventTypeTable.value(event_type, QString());
}
