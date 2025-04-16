
#include "loadingscreen.h"
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

    this->setPixmap(QPixmap::fromImage(this->splashImage.frame(0)));

    connect(&this->timer, &QTimer::timeout, this, &PorymapLoadingScreen::updateFrame);
}

void PorymapLoadingScreen::setPixmap(QPixmap pixmap) {
    if (!this->isVisible()) return;
    this->ui->labelPixmap->setPixmap(pixmap);
}

void PorymapLoadingScreen::showMessage(QString text) {
    if (!this->isVisible()) return;
    this->ui->labelText->setText(text.mid(text.lastIndexOf("/") + 1));
    //this->updateFrame();

    QApplication::processEvents();
}

void PorymapLoadingScreen::updateFrame() {
    //
    this->frame = (this->frame + 1) % this->splashImage.frameCount();

    this->setPixmap(QPixmap::fromImage(this->splashImage.frame(this->frame)));

    QApplication::processEvents();

    //this->showMessage("Frame Number: " + QString::number(this->frame));

    //this->repaint();
}
