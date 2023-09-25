#include "customscriptslistitem.h"
#include "ui_customscriptslistitem.h"

CustomScriptsListItem::CustomScriptsListItem(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::CustomScriptsListItem)
{
    ui->setupUi(this);
}

CustomScriptsListItem::~CustomScriptsListItem()
{
    delete ui;
}
