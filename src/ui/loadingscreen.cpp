#include "loadingscreen.h"
#include "aboutporymap.h"
#include "ui_loadingscreen.h"
#include "qgifimage.h"

#include <QApplication>

PorymapLoadingScreen *porysplash = nullptr;



PorymapLoadingScreen::~PorymapLoadingScreen() {
    delete ui;
}

PorymapLoadingScreen::PorymapLoadingScreen(QWidget *parent) : QWidget(parent), ui(new Ui::LoadingScreen) {
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint);

    this->splashImage.load(":/images/porysplash.gif");

    connect(&this->timer, &QTimer::timeout, this, &PorymapLoadingScreen::updateFrame);
}

void PorymapLoadingScreen::start() {
    static bool shownVersion = false;
    if (!shownVersion) {
        this->ui->labelVersion->setText(AboutPorymap::getVersionString());
        shownVersion = true;
    }

    this->frame = 0;
    this->ui->labelPixmap->setPixmap(QPixmap::fromImage(this->splashImage.frame(this->frame)));

    this->ui->labelText->setText("");

    this->timer.start(120);
    this->show();
}

void PorymapLoadingScreen::stop () {
    this->timer.stop();
    this->hide();
}

void PorymapLoadingScreen::setPixmap(QPixmap pixmap) {
    if (!this->isVisible()) return;
    this->ui->labelPixmap->setPixmap(pixmap);
}

void PorymapLoadingScreen::showMessage(QString text) {
    if (!this->isVisible()) return;
    this->ui->labelText->setText(text.mid(text.lastIndexOf("/") + 1));

    QApplication::processEvents();
}

void PorymapLoadingScreen::updateFrame() {
    this->frame = (this->frame + 1) % this->splashImage.frameCount();
    this->setPixmap(QPixmap::fromImage(this->splashImage.frame(this->frame)));

    QApplication::processEvents();
}
