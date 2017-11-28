#include "objectpropertiesframe.h"
#include "ui_objectpropertiesframe.h"

ObjectPropertiesFrame::ObjectPropertiesFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::ObjectPropertiesFrame)
{
    ui->setupUi(this);
}

ObjectPropertiesFrame::~ObjectPropertiesFrame()
{
    delete ui;
}
