#include "eventpropertiesframe.h"
#include "ui_objectpropertiesframe.h"

EventPropertiesFrame::EventPropertiesFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ObjectPropertiesFrame)
{
    ui->setupUi(this);
}

EventPropertiesFrame::~EventPropertiesFrame()
{
    delete ui;
}
