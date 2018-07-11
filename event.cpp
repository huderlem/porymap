#include "event.h"

QString EventType::Object = "event_object";
QString EventType::Warp = "event_warp";
QString EventType::CoordScript = "event_trap";
QString EventType::CoordWeather = "event_trap_weather";
QString EventType::Sign = "event_sign";
QString EventType::HiddenItem = "event_hidden_item";
QString EventType::SecretBase = "event_secret_base";

Event::Event()
{
}

Event* Event::createNewEvent(QString event_type, QString map_name)
{
    Event *event;
    if (event_type == EventType::Object) {
        event = createNewObjectEvent();
    } else if (event_type == EventType::Warp) {
        event = createNewWarpEvent(map_name);
    } else if (event_type == EventType::CoordScript) {
        event = createNewCoordScriptEvent();
    } else if (event_type == EventType::CoordWeather) {
        event = createNewCoordWeatherEvent();
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
    event->put("movement_type", "1");
    event->put("radius_x", 0);
    event->put("radius_y", 0);
    event->put("script_label", "NULL");
    event->put("event_flag", "0");
    event->put("replacement", "0");
    event->put("trainer_see_type", "0");
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

Event* Event::createNewCoordScriptEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "coord_event_group");
    event->put("event_type", EventType::CoordScript);
    event->put("script_label", "NULL");
    event->put("script_var", "VAR_TEMP_0");
    event->put("script_var_value", "0");
    return event;
}

Event* Event::createNewCoordWeatherEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "coord_event_group");
    event->put("event_type", EventType::CoordWeather);
    event->put("weather", "COORD_EVENT_WEATHER_SUNNY");
    return event;
}

Event* Event::createNewSignEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::Sign);
    event->put("player_facing_direction", "0");
    event->put("script_label", "NULL");
    return event;
}

Event* Event::createNewHiddenItemEvent()
{
    Event *event = new Event;
    event->put("event_group_type", "bg_event_group");
    event->put("event_type", EventType::HiddenItem);
    event->put("item", "ITEM_POTION");
    event->put("flag", "FLAG_HIDDEN_ITEM_0");
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

QString Event::buildObjectEventMacro(int item_index)
{
    int radius_x = this->getInt("radius_x");
    int radius_y = this->getInt("radius_y");
    uint16_t x = this->getInt("x");
    uint16_t y = this->getInt("y");

    QString text = "";
    text += QString("\tobject_event %1").arg(item_index + 1);
    text += QString(", %1").arg(this->get("sprite"));
    text += QString(", %1").arg(this->get("replacement"));
    text += QString(", %1").arg(x);
    text += QString(", %1").arg(y);
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("movement_type"));
    text += QString(", %1").arg(radius_x);
    text += QString(", %1").arg(radius_y);
    text += QString(", %1").arg(this->get("trainer_see_type"));
    text += QString(", %1").arg(this->get("sight_radius_tree_id"));
    text += QString(", %1").arg(this->get("script_label"));
    text += QString(", %1").arg(this->get("event_flag"));
    text += "\n";
    return text;
}

QString Event::buildWarpEventMacro(QMap<QString, QString> *mapNamesToMapConstants)
{
    QString text = "";
    text += QString("\twarp_def %1").arg(this->get("x"));
    text += QString(", %1").arg(this->get("y"));
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("destination_warp"));
    text += QString(", %1").arg(mapNamesToMapConstants->value(this->get("destination_map_name")));
    text += "\n";
    return text;
}

QString Event::buildCoordScriptEventMacro()
{
    QString text = "";
    text += QString("\tcoord_event %1").arg(this->get("x"));
    text += QString(", %1").arg(this->get("y"));
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("script_var"));
    text += QString(", %1").arg(this->get("script_var_value"));
    text += QString(", %1").arg(this->get("script_label"));
    text += "\n";
    return text;
}

QString Event::buildCoordWeatherEventMacro()
{
    QString text = "";
    text += QString("\tcoord_weather_event %1").arg(this->get("x"));
    text += QString(", %1").arg(this->get("y"));
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("weather"));
    text += "\n";
    return text;
}

QString Event::buildSignEventMacro()
{
    QString text = "";
    text += QString("\tbg_event %1").arg(this->get("x"));
    text += QString(", %1").arg(this->get("y"));
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("player_facing_direction"));
    text += QString(", %1").arg(this->get("script_label"));
    text += "\n";
    return text;
}

QString Event::buildHiddenItemEventMacro()
{
    QString text = "";
    text += QString("\tbg_hidden_item_event %1").arg(this->get("x"));
    text += QString(", %1").arg(this->get("y"));
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("item"));
    text += QString(", %1").arg(this->get("flag"));
    text += "\n";
    return text;
}

QString Event::buildSecretBaseEventMacro()
{
    QString text = "";
    text += QString("\tbg_secret_base_event %1").arg(this->get("x"));
    text += QString(", %1").arg(this->get("y"));
    text += QString(", %1").arg(this->get("elevation"));
    text += QString(", %1").arg(this->get("secret_base_id"));
    text += "\n";
    return text;
}
