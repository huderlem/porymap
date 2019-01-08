#include "aboutporymap.h"
#include "ui_aboutporymap.h"

AboutPorymap::AboutPorymap(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AboutPorymap)
{
    ui->setupUi(this);
}

AboutPorymap::~AboutPorymap()
{
    delete ui;
}
