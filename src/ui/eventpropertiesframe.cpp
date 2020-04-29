#include "eventpropertiesframe.h"
#include "customattributestable.h"

#include "ui_eventpropertiesframe.h"

EventPropertiesFrame::EventPropertiesFrame(Event *event, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::EventPropertiesFrame)
{
    ui->setupUi(this);
    this->event = event;
    this->firstShow = true;
}

EventPropertiesFrame::~EventPropertiesFrame()
{
    delete ui;
}

void EventPropertiesFrame::paintEvent(QPaintEvent *painter) {
    // Custom fields table.
    if (firstShow && event->get("event_type") != EventType::HealLocation) {
        CustomAttributesTable *customAttributes = new CustomAttributesTable(event, this);
        this->layout()->addWidget(customAttributes);
    }
    QFrame::paintEvent(painter);
    firstShow = false;
}
