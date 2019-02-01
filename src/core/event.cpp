#include "event.h"
#include "map.h"

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

Event* Event::createNewEvent(QString event_type, QString map_name)
{
    Event *event = new Event;
    if (event_type == EventType::Object) {
        event = createNewObjectEvent();
    } else if (event_type == EventType::Warp) {
        event = createNewWarpEvent(map_name);
    } else if (event_type == EventType::HealLocation) {
        event = createNewHealLocationEvent(map_name);
    } else if (event_type == EventType::Trigger) {
        event = createNewTriggerEvent();
    } else if (event_type == EventType::WeatherTrigger) {
        event = createNewWeatherTriggerEvent();
    } else if (event_type == EventType::Sign) {
        event = createNewSignEvent();
    } else if (event_type == EventType::HiddenItem) {
        event = createNewHiddenItemEvent();
    } else if (event_type == EventType::SecretBase) {
        event = createNewSecretBaseEvent();
    }

    event->setX(0);
    event->setY(0);
    event->put("elevation", 3);
    return event;
}

Event* Event::createNewObjectEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "object_event_group");
    event->put("event_type", EventType::Object);
    event->put("sprite", "EVENT_OBJ_GFX_BOY_1");
    event->put("movement_type", "MOVEMENT_TYPE_LOOK_AROUND");
    event->put("radius_x", 0);
    event->put("radius_y", 0);
    event->put("script_label", "NULL");
    event->put("event_flag", "0");
    event->put("replacement", "0");
    event->put("trainer_type", "0");
    event->put("sight_radius_tree_id", 0);
    return event;
}

Event* Event::createNewWarpEvent(QString map_name)
{
    Event *event = new Event;
    event->put("event_group_type", "warp_event_group");
    event->put("event_type", EventType::Warp);
    event->put("destination_warp", 0);
    event->put("destination_map_name", map_name);
    return event;
}

Event* Event::createNewHealLocationEvent(QString map_name)
{
    Event *event = new Event;
    event->put("event_group_type", "heal_event_group");
    event->put("event_type", EventType::HealLocation);
    event->put("loc_name", QString(Map::mapConstantFromName(map_name)).remove(0,4));
    return event;
}

Event* Event::createNewTriggerEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "coord_event_group");
    event->put("event_type", EventType::Trigger);
    event->put("script_label", "NULL");
    event->put("script_var", "VAR_TEMP_0");
    event->put("script_var_value", "0");
    return event;
}

Event* Event::createNewWeatherTriggerEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "coord_event_group");
    event->put("event_type", EventType::WeatherTrigger);
    event->put("weather", "COORD_EVENT_WEATHER_SUNNY");
    return event;
}

Event* Event::createNewSignEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::Sign);
    event->put("player_facing_direction", "BG_EVENT_PLAYER_FACING_ANY");
    event->put("script_label", "NULL");
    return event;
}

Event* Event::createNewHiddenItemEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::HiddenItem);
    event->put("item", "ITEM_POTION");
    event->put("flag", "FLAG_TEMP_1");
    return event;
}

Event* Event::createNewSecretBaseEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::SecretBase);
    event->put("secret_base_id", "SECRET_BASE_RED_CAVE2_1");
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

QJsonObject Event::buildObjectEventJSON()
{
    QJsonObject eventObj;
    eventObj["graphics_id"] = this->get("sprite");
    eventObj["x"] = this->getU16("x");
    eventObj["y"] = this->getU16("y");
    eventObj["elevation"] = this->getInt("elevation");
    eventObj["movement_type"] = this->get("movement_type");
    eventObj["movement_range_x"] = this->getInt("radius_x");
    eventObj["movement_range_y"] = this->getInt("radius_y");
    eventObj["trainer_type"] = this->get("trainer_type");
    eventObj["trainer_sight_or_berry_tree_id"] = this->get("sight_radius_tree_id");
    eventObj["script"] = this->get("script_label");
    eventObj["flag"] = this->get("event_flag");

    return eventObj;
}

QJsonObject Event::buildWarpEventJSON(QMap<QString, QString> *mapNamesToMapConstants)
{
    QJsonObject warpObj;
    warpObj["x"] = this->getU16("x");
    warpObj["y"] = this->getU16("y");
    warpObj["elevation"] = this->getInt("elevation");
    warpObj["dest_map"] = mapNamesToMapConstants->value(this->get("destination_map_name"));
    warpObj["dest_warp_id"] = this->getInt("destination_warp");

    return warpObj;
}

QJsonObject Event::buildTriggerEventJSON()
{
    QJsonObject triggerObj;
    triggerObj["type"] = "trigger";
    triggerObj["x"] = this->getU16("x");
    triggerObj["y"] = this->getU16("y");
    triggerObj["elevation"] = this->getInt("elevation");
    triggerObj["var"] = this->get("script_var");
    triggerObj["var_value"] = this->get("script_var_value");
    triggerObj["script"] = this->get("script_label");

    return triggerObj;
}

QJsonObject Event::buildWeatherTriggerEventJSON()
{
    QJsonObject weatherObj;
    weatherObj["type"] = "weather";
    weatherObj["x"] = this->getU16("x");
    weatherObj["y"] = this->getU16("y");
    weatherObj["elevation"] = this->getInt("elevation");
    weatherObj["weather"] = this->get("weather");

    return weatherObj;
}

QJsonObject Event::buildSignEventJSON()
{
    QJsonObject signObj;
    signObj["type"] = "sign";
    signObj["x"] = this->getU16("x");
    signObj["y"] = this->getU16("y");
    signObj["elevation"] = this->getInt("elevation");
    signObj["player_facing_dir"] = this->get("player_facing_direction");
    signObj["script"] = this->get("script_label");

    return signObj;
}

QJsonObject Event::buildHiddenItemEventJSON()
{
    QJsonObject hiddenItemObj;
    hiddenItemObj["type"] = "hidden_item";
    hiddenItemObj["x"] = this->getU16("x");
    hiddenItemObj["y"] = this->getU16("y");
    hiddenItemObj["elevation"] = this->getInt("elevation");
    hiddenItemObj["item"] = this->get("item");
    hiddenItemObj["flag"] = this->get("flag");

    return hiddenItemObj;
}

QJsonObject Event::buildSecretBaseEventJSON()
{
    QJsonObject secretBaseObj;
    secretBaseObj["type"] = "secret_base";
    secretBaseObj["x"] = this->getU16("x");
    secretBaseObj["y"] = this->getU16("y");
    secretBaseObj["elevation"] = this->getInt("elevation");
    secretBaseObj["secret_base_id"] = this->get("secret_base_id");

    return secretBaseObj;
}

void Event::setPixmapFromSpritesheet(QImage spritesheet, int spriteWidth, int spriteHeight)
{
    // Set first palette color fully transparent.
    QImage img = spritesheet.copy(0, 0, spriteWidth, spriteHeight);
    img.setColor(0, qRgba(0, 0, 0, 0));
    pixmap = QPixmap::fromImage(img);
    this->spriteWidth = spriteWidth;
    this->spriteHeight = spriteHeight;
    this->usingSprite = true;
}
