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

void PorymapLoadingScreen::setPixmap(const QPixmap &pixmap) {
    if (!this->isVisible()) return;
    this->ui->labelPixmap->setPixmap(pixmap);
}

// Displays the message 'prefixtext...'. The 'text' portion may be elided if it's too long.
void PorymapLoadingScreen::showMessage(const QString &prefix, const QString &text) {
    if (!this->isVisible()) return;

    // Limit text (excluding prefix) to avoid increasing the splash screen's width.
    static const QFontMetrics fontMetrics = this->ui->labelText->fontMetrics();
    static const int maxWidth = this->ui->labelText->width() + 1;
    int prefixWidth = fontMetrics.horizontalAdvance(prefix);
    QString message = fontMetrics.elidedText(text + QStringLiteral("..."), Qt::ElideLeft, qMax(maxWidth - prefixWidth, 0));
    message.prepend(prefix);

    this->ui->labelText->setText(message);

    QApplication::processEvents();
}

// Displays the message 'text...'
void PorymapLoadingScreen::showMessage(const QString &text) {
    showMessage("", text);
}

// Displays the message 'Loading text...'
void PorymapLoadingScreen::showLoadingMessage(const QString &text) {
    showMessage(QStringLiteral("Loading "), text);
}

void PorymapLoadingScreen::updateFrame() {
    this->frame = (this->frame + 1) % this->splashImage.frameCount();
    this->setPixmap(QPixmap::fromImage(this->splashImage.frame(this->frame)));

    QApplication::processEvents();
}
