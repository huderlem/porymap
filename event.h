#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QPixmap>

class Event
{
public:
    Event();

public:
    int x() {
        return x_.toInt(nullptr, 0);
    }
    int y() {
        return y_.toInt(nullptr, 0);
    }
    int elevation() {
        return elevation_.toInt(nullptr, 0);
    }
    void setX(int x) {
        x_ = QString("%1").arg(x);
    }
    void setY(int y) {
        y_ = QString("%1").arg(y);
    }

    QString x_;
    QString y_;
    QString elevation_;
    QPixmap pixmap;
};

class ObjectEvent : public Event {
public:
    ObjectEvent();

public:
    QString sprite;
    QString replacement; // ????
    QString behavior;
    QString radius_x;
    QString radius_y;
    QString property;
    QString sight_radius;
    QString script_label;
    QString event_flag;
};

class Warp : public Event {
public:
    Warp();

public:
    QString destination_warp;
    QString destination_map;
};

class CoordEvent : public Event {
public:
    CoordEvent();

public:
    QString unknown1;
    QString unknown2;
    QString unknown3;
    QString unknown4;
    QString script_label;
};

class BGEvent : public Event {
public:
    BGEvent();
public:
    bool is_item() {
        return type.toInt(nullptr, 0) >= 5;
    }
    QString type;
};

class Sign : public BGEvent {
public:
    Sign();
    Sign(const BGEvent&);
public:
    QString script_label;
};

class HiddenItem : public BGEvent {
public:
    HiddenItem();
    HiddenItem(const BGEvent&);
public:
    QString item;
    QString unknown5;
    QString unknown6;
};

#endif // EVENT_H
