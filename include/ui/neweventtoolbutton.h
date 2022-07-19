#ifndef NEWEVENTTOOLBUTTON_H
#define NEWEVENTTOOLBUTTON_H

#include "events.h"
#include <QToolButton>

class NewEventToolButton : public QToolButton
{
    Q_OBJECT
public:
    explicit NewEventToolButton(QWidget *parent = nullptr);
    Event::Type getSelectedEventType();
    QAction *newObjectAction;
    QAction *newCloneObjectAction;
    QAction *newWarpAction;
    QAction *newHealLocationAction;
    QAction *newTriggerAction;
    QAction *newWeatherTriggerAction;
    QAction *newSignAction;
    QAction *newHiddenItemAction;
    QAction *newSecretBaseAction;
public slots:
    void newObject();
    void newCloneObject();
    void newWarp();
    void newHealLocation();
    void newTrigger();
    void newWeatherTrigger();
    void newSign();
    void newHiddenItem();
    void newSecretBase();
signals:
    void newEventAdded(Event::Type);
private:
    Event::Type selectedEventType;
    void init();
};

#endif // NEWEVENTTOOLBUTTON_H
