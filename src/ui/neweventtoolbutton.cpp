#include "neweventtoolbutton.h"
#include <QMenu>

// Custom QToolButton which has a context menu that expands to allow
// selection of different types of map events.
NewEventToolButton::NewEventToolButton(QWidget *parent) :
    QToolButton(parent)
{
    setPopupMode(QToolButton::MenuButtonPopup);
    QObject::connect(this, SIGNAL(triggered(QAction*)),
                     this, SLOT(setDefaultAction(QAction*)));
    this->init();
}

void NewEventToolButton::init()
{
    // Add a context menu to select different types of map events.
    this->newObjectAction = new QAction("New Object", this);
    this->newObjectAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newObjectAction, SIGNAL(triggered(bool)), this, SLOT(newObject()));

    this->newWarpAction = new QAction("New Warp", this);
    this->newWarpAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newWarpAction, SIGNAL(triggered(bool)), this, SLOT(newWarp()));

    /* // disable this functionality for now
    this->newHealLocationAction = new QAction("New Heal Location", this);
    this->newHealLocationAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newHealLocationAction, SIGNAL(triggered(bool)), this, SLOT(newHealLocation()));
    */

    this->newTriggerAction = new QAction("New Trigger", this);
    this->newTriggerAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newTriggerAction, SIGNAL(triggered(bool)), this, SLOT(newTrigger()));

    this->newWeatherTriggerAction = new QAction("New Weather Trigger", this);
    this->newWeatherTriggerAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newWeatherTriggerAction, SIGNAL(triggered(bool)), this, SLOT(newWeatherTrigger()));

    this->newSignAction = new QAction("New Sign", this);
    this->newSignAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newSignAction, SIGNAL(triggered(bool)), this, SLOT(newSign()));

    this->newHiddenItemAction = new QAction("New Hidden Item", this);
    this->newHiddenItemAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newHiddenItemAction, SIGNAL(triggered(bool)), this, SLOT(newHiddenItem()));

    this->newSecretBaseAction = new QAction("New Secret Base", this);
    this->newSecretBaseAction->setIcon(QIcon(":/icons/add.ico"));
    connect(this->newSecretBaseAction, SIGNAL(triggered(bool)), this, SLOT(newSecretBase()));

    QMenu *alignMenu = new QMenu();
    alignMenu->addAction(this->newObjectAction);
    alignMenu->addAction(this->newWarpAction);
    //alignMenu->addAction(this->newHealLocationAction);
    alignMenu->addAction(this->newTriggerAction);
    alignMenu->addAction(this->newWeatherTriggerAction);
    alignMenu->addAction(this->newSignAction);
    alignMenu->addAction(this->newHiddenItemAction);
    alignMenu->addAction(this->newSecretBaseAction);
    this->setMenu(alignMenu);
    this->setDefaultAction(this->newObjectAction);
}

QString NewEventToolButton::getSelectedEventType()
{
    return this->selectedEventType;
}

void NewEventToolButton::newObject()
{
    this->selectedEventType = EventType::Object;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newWarp()
{
    this->selectedEventType = EventType::Warp;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newHealLocation()
{
    this->selectedEventType = EventType::HealLocation;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newTrigger()
{
    this->selectedEventType = EventType::Trigger;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newWeatherTrigger()
{
    this->selectedEventType = EventType::WeatherTrigger;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newSign()
{
    this->selectedEventType = EventType::Sign;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newHiddenItem()
{
    this->selectedEventType = EventType::HiddenItem;
    emit newEventAdded(this->selectedEventType);
}

void NewEventToolButton::newSecretBase()
{
    this->selectedEventType = EventType::SecretBase;
    emit newEventAdded(this->selectedEventType);
}
