#include "prefabframe.h"

#include "ui_prefabframe.h"

PrefabFrame::PrefabFrame(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::PrefabFrame)
{
    ui->setupUi(this);
}

PrefabFrame::~PrefabFrame()
{
    delete ui;
}
