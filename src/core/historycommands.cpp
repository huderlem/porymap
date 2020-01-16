#include "historycommands.h"
#include "map.h"
#include "event.h"

EditMap::EditMap(Map *map_, EditType type_, QString message_)
  : map(map_), type(type_), HistoryCommand(message_) {
//(QString("Edit") + map_->name + QString("Metatiles")) {
//  : map(map_), metatiles(map_->layout->blockdata->copy()),
//    layoutWidth(map_->getWidth()),
//    layoutHeight(map_->getHeight()) {
    //
    switch (type) {
        case EditType::Metatiles:
            this->metatiles = map->layout->blockdata->copy();
            this->layout_width = map->getWidth();
            this->layout_height = map->getHeight();
            map->layout->has_unsaved_changes = true; // TODO: set to false when at current history
            break;
        case EditType::Events:
        case EditType::EventCreate:
        case EditType::EventDelete:
        case EditType::EventMove:
            
        //    break;

            this->events = map->copyEvents();
            break;
    }
}

EditMap::~EditMap() {
    if (metatiles) delete metatiles;
}

// emit a map changed signal ??
void EditMap::execute() {
    switch (type) {
        case EditType::Metatiles:
            if (map->layout->blockdata) {
                map->layout->blockdata->copyFrom(this->metatiles);
                if (this->layout_width != map->getWidth() || this->layout_height != map->getHeight()) {
                    map->setDimensions(this->layout_width, this->layout_height, false);
                }
                map->layout->has_unsaved_changes = true; // TODO: set to false when at current history
            }
            break;
        case EditType::Events:
        case EditType::EventCreate:
        case EditType::EventDelete:
        case EditType::EventMove:
        //    break;
        
            map->setEvents(this->events);
            // have to modify map->events which is
            // QMap<QString, QList<Event*>> events;
            break;
    }
}


