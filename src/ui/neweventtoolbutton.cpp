#include "neweventtoolbutton.h"
#include <QMenu>

// Custom QToolButton which has a context menu that expands to allow
// selection of different types of map events.
NewEventToolButton::NewEventToolButton(QWidget *parent) :
    QToolButton(parent)
{
    setPopupMode(QToolButton::MenuButtonPopup);
    QObject::connect(this, &NewEventToolButton::triggered, this, &NewEventToolButton::setDefaultAction);

    this->menu = new QMenu(this);
    for (const auto &type : Event::types()) {
        addEventType(type);
    }
    setMenu(this->menu);
    setDefaultAction(this->menu->actions().constFirst());
}

void NewEventToolButton::addEventType(Event::Type type) {
    if (this->typeToAction.contains(type))
        return;

    auto action = new QAction(QStringLiteral("New ") + Event::typeToString(type), this);
    action->setIcon(QIcon(QStringLiteral(":/icons/add.ico")));
    connect(action, &QAction::triggered, [this, type] {
        this->selectedEventType = type;
        emit newEventAdded(this->selectedEventType);
    });

    this->typeToAction.insert(type, action);
    this->menu->addAction(action);
}

bool NewEventToolButton::selectEventType(Event::Type type) {
    auto action = this->typeToAction.value(type);
    if (!action || !action->isVisible())
        return false;

    this->selectedEventType = type;
    setDefaultAction(action);
    return true;
}

void NewEventToolButton::setEventTypeVisible(Event::Type type, bool visible) {
    auto action = this->typeToAction.value(type);
    if (!action)
        return;

    action->setVisible(visible);

    // If we just hid the currently-selected type we need to pick a new type.
    if (this->selectedEventType == type) {
        for (const auto &newType : Event::types()) {
            if (newType != type && selectEventType(newType)){
                break;
            }
        }
    }
}
