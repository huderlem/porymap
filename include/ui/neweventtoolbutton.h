#ifndef NEWEVENTTOOLBUTTON_H
#define NEWEVENTTOOLBUTTON_H

#include "events.h"
#include <QToolButton>

class NewEventToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit NewEventToolButton(QWidget *parent = nullptr);

    Event::Type getSelectedEventType() const { return this->selectedEventType; }
    bool selectEventType(Event::Type type);
    void setEventTypeVisible(Event::Type type, bool visible);

signals:
    void newEventAdded(Event::Type);

private:
    QMap<Event::Type,QAction*> typeToAction;
    Event::Type selectedEventType;
    QMenu* menu;

    void addEventType(Event::Type type);
};

#endif // NEWEVENTTOOLBUTTON_H
