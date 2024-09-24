#include "aboutporymap.h"
#include "ui_aboutporymap.h"
#include "log.h"

AboutPorymap::AboutPorymap(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AboutPorymap)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    this->ui->label_Version->setText(QString("Version %1 - %2").arg(QCoreApplication::applicationVersion()).arg(QStringLiteral(__DATE__)));
    this->ui->textBrowser->setSource(QUrl("qrc:/CHANGELOG.md"));
}

AboutPorymap::~AboutPorymap()
{
    delete ui;
}
