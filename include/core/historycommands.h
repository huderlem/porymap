#ifndef GUARD_HISTORY_COMMANDS_H
#define GUARD_HISTORY_COMMANDS_H

#include "history.h"
//#include "map.h"
#include "blockdata.h"


class Map;
class Event;
// just call this edit map and reset both events and metatiles 
// or use enum class to keep track of which this changed,
// but have member for both metatiles and events
class EditMap : public HistoryCommand {

public:
    enum class EditType {
        Metatiles,
        Events,// generic event edit
        EventCreate,
        EventDelete,
        EventMove,
    };

    EditMap(Map *map, EditType type, QString message = QString());
    ~EditMap();

    void execute() override;

private:
    Map *map;
    EditType type;
    Blockdata *metatiles;
    int layout_width;
    int layout_height;
    QMap<QString, QList<Event*>> events;
};

#endif // GUARD_HISTORY_COMMANDS_H
