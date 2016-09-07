#include "event.h"

Event::Event()
{

}

ObjectEvent::ObjectEvent()
{

}

Warp::Warp()
{

}

CoordEvent::CoordEvent()
{

}

BGEvent::BGEvent()
{

}

Sign::Sign()
{

}

Sign::Sign(const BGEvent &bg)
{
    x_ = bg.x_;
    y_ = bg.y_;
    elevation_ = bg.elevation_;
    type = bg.type;
}

HiddenItem::HiddenItem()
{

}

HiddenItem::HiddenItem(const BGEvent &bg)
{
    x_ = bg.x_;
    y_ = bg.y_;
    elevation_ = bg.elevation_;
    type = bg.type;
}
