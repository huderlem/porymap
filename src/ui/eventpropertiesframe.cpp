#include "eventpropertiesframe.h"
#include "ui_eventpropertiesframe.h"

EventPropertiesFrame::EventPropertiesFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::EventPropertiesFrame)
{
    ui->setupUi(this);
}

EventPropertiesFrame::~EventPropertiesFrame()
{
    delete ui;
}
