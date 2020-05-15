#include "event.h"
#include "map.h"
#include "project.h"
#include "config.h"

QString EventType::Object = "event_object";
QString EventType::Warp = "event_warp";
QString EventType::Trigger = "event_trigger";
QString EventType::WeatherTrigger = "event_weather_trigger";
QString EventType::Sign = "event_sign";
QString EventType::HiddenItem = "event_hidden_item";
QString EventType::SecretBase = "event_secret_base";
QString EventType::HealLocation = "event_heal_location";

Event::Event()
{
    this->spriteWidth = 16;
    this->spriteHeight = 16;
    this->usingSprite = false;
}

Event::Event(QJsonObject obj, QString type)
{
    Event();
    this->put("event_type", type);
    this->readCustomValues(obj);
}

Event* Event::createNewEvent(QString event_type, QString map_name, Project *project)
{
    Event *event = new Event;
    if (event_type == EventType::Object) {
        event = createNewObjectEvent(project);
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
    }

    event->setX(0);
    event->setY(0);
    return event;
}

Event* Event::createNewObjectEvent(Project *project)
{
    Event *event = new Event;
    event->put("event_group_type", "object_event_group");
    event->put("event_type", EventType::Object);
    event->put("sprite", project->getEventObjGfxConstants().keys().first());
    event->put("movement_type", project->movementTypes->first());
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        event->put("in_connection", false);
    }
    event->put("radius_x", 0);
    event->put("radius_y", 0);
    event->put("script_label", "NULL");
    event->put("event_flag", "0");
    event->put("replacement", "0");
    event->put("trainer_type", project->trainerTypes->value(0, "0"));
    event->put("sight_radius_tree_id", 0);
    event->put("elevation", 3);
    return event;
}

Event* Event::createNewWarpEvent(QString map_name)
{
    Event *event = new Event;
    event->put("event_group_type", "warp_event_group");
    event->put("event_type", EventType::Warp);
    event->put("destination_warp", 0);
    event->put("destination_map_name", map_name);
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewHealLocationEvent(QString map_name)
{
    Event *event = new Event;
    event->put("event_group_type", "heal_event_group");
    event->put("event_type", EventType::HealLocation);
    event->put("loc_name", QString(Map::mapConstantFromName(map_name)).remove(0,4));
    event->put("id_name", map_name.replace(QRegularExpression("([a-z])([A-Z])"), "\\1_\\2").toUpper());
    event->put("elevation", 3);
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        event->put("respawn_map", map_name);
        event->put("respawn_npc", 1);
    }
    return event;
}

Event* Event::createNewTriggerEvent(Project *project)
{
    Event *event = new Event;
    event->put("event_group_type", "coord_event_group");
    event->put("event_type", EventType::Trigger);
    event->put("script_label", "NULL");
    event->put("script_var", project->varNames->first());
    event->put("script_var_value", "0");
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewWeatherTriggerEvent(Project *project)
{
    Event *event = new Event;
    event->put("event_group_type", "coord_event_group");
    event->put("event_type", EventType::WeatherTrigger);
    event->put("weather", project->coordEventWeatherNames->first());
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewSignEvent(Project *project)
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::Sign);
    event->put("player_facing_direction", project->bgEventFacingDirections->first());
    event->put("script_label", "NULL");
    event->put("elevation", 0);
    return event;
}

Event* Event::createNewHiddenItemEvent(Project *project)
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::HiddenItem);
    event->put("item", project->itemNames->first());
    event->put("flag", project->flagNames->first());
    event->put("elevation", 3);
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        event->put("quantity", 1);
        event->put("underfoot", false);
    }
    return event;
}

Event* Event::createNewSecretBaseEvent(Project *project)
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::SecretBase);
    event->put("secret_base_id", project->secretBaseIds->first());
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

QMap<QString, bool> Event::getExpectedFields()
{
    QString type = this->get("event_type");
    if (type == EventType::Object) {
        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            return QMap<QString, bool> {
                {"graphics_id", true},
                {"in_connection", true},
                {"x", true},
                {"y", true},
                {"elevation", true},
                {"movement_type", true},
                {"movement_range_x", true},
                {"movement_range_y", true},
                {"trainer_type", true},
                {"trainer_sight_or_berry_tree_id", true},
                {"script", true},
                {"flag", true},
            };
        } else {
            return QMap<QString, bool> {
                {"graphics_id", true},
                {"x", true},
                {"y", true},
                {"elevation", true},
                {"movement_type", true},
                {"movement_range_x", true},
                {"movement_range_y", true},
                {"trainer_type", true},
                {"trainer_sight_or_berry_tree_id", true},
                {"script", true},
                {"flag", true},
            };
        }
    } else if (type == EventType::Warp) {
        return QMap<QString, bool> {
            {"x", true},
            {"y", true},
            {"elevation", true},
            {"dest_map", true},
            {"dest_warp_id", true},
        };
    } else if (type == EventType::Trigger) {
        return QMap<QString, bool> {
            {"type", true},
            {"x", true},
            {"y", true},
            {"elevation", true},
            {"var", true},
            {"var_value", true},
            {"script", true},
        };
    } else if (type == EventType::WeatherTrigger) {
        return QMap<QString, bool> {
            {"type", true},
            {"x", true},
            {"y", true},
            {"elevation", true},
            {"weather", true},
        };
    } else if (type == EventType::Sign) {
        return QMap<QString, bool> {
            {"type", true},
            {"x", true},
            {"y", true},
            {"elevation", true},
            {"player_facing_dir", true},
            {"script", true},
        };
    } else if (type == EventType::HiddenItem) {
        if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
            return QMap<QString, bool> {
                {"type", true},
                {"x", true},
                {"y", true},
                {"elevation", true},
                {"item", true},
                {"flag", true},
                {"quantity", true},
                {"underfoot", true},
            };
        } else {
            return QMap<QString, bool> {
                {"type", true},
                {"x", true},
                {"y", true},
                {"elevation", true},
                {"item", true},
                {"flag", true},
            };
        }
    } else if (type == EventType::SecretBase) {
        return QMap<QString, bool> {
            {"type", true},
            {"x", true},
            {"y", true},
            {"elevation", true},
            {"secret_base_id", true},
        };
    } else {
        return QMap<QString, bool>();
    }
};

void Event::readCustomValues(QJsonObject values)
{
    this->customValues.clear();
    QMap<QString, bool> expectedValues = this->getExpectedFields();
    for (QString key : values.keys()) {
        if (!expectedValues.contains(key)) {
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
    OrderedJson::object eventObj;
    eventObj["graphics_id"] = this->get("sprite");
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        eventObj["in_connection"] = this->getInt("in_connection") > 0 || this->get("in_connection") == "TRUE";
    }
    eventObj["x"] = this->getS16("x");
    eventObj["y"] = this->getS16("y");
    eventObj["elevation"] = this->getInt("elevation");
    eventObj["movement_type"] = this->get("movement_type");
    eventObj["movement_range_x"] = this->getInt("radius_x");
    eventObj["movement_range_y"] = this->getInt("radius_y");
    eventObj["trainer_type"] = this->get("trainer_type");
    eventObj["trainer_sight_or_berry_tree_id"] = this->get("sight_radius_tree_id");
    eventObj["script"] = this->get("script_label");
    eventObj["flag"] = this->get("event_flag");
    this->addCustomValuesTo(&eventObj);

    return eventObj;
}

OrderedJson::object Event::buildWarpEventJSON(QMap<QString, QString> *mapNamesToMapConstants)
{
    OrderedJson::object warpObj;
    warpObj["x"] = this->getU16("x");
    warpObj["y"] = this->getU16("y");
    warpObj["elevation"] = this->getInt("elevation");
    warpObj["dest_map"] = mapNamesToMapConstants->value(this->get("destination_map_name"));
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
    if (projectConfig.getBaseGameVersion() == BaseGameVersion::pokefirered) {
        hiddenItemObj["quantity"] = this->getInt("quantity");
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

void Event::setPixmapFromSpritesheet(QImage spritesheet, int spriteWidth, int spriteHeight, int frame, bool hFlip)
{
    // Set first palette color fully transparent.
    QImage img = spritesheet.copy(frame * spriteWidth % spritesheet.width(), 0, spriteWidth, spriteHeight);
    if (hFlip) {
        img = img.transformed(QTransform().scale(-1, 1));
    }
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
