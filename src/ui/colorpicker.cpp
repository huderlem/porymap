#include "colorpicker.h"
#include "ui_colorpicker.h"

#include <QtWidgets>

const int zoom_box_dimensions = 15;

ColorPicker::ColorPicker(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ColorPicker)
{
    ui->setupUi(this);

    this->scene = new QGraphicsScene;

    // listen for spacebar press to take color
    QShortcut *takeColor = new QShortcut(Qt::Key_Space, this);
    QObject::connect(takeColor, &QShortcut::activated, [this](){
        timer->stop();
        this->accept();
    });

    // need to set up a timer because there is no good way to get global mouse movement
    // outside of the application in a cross-platform way
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, [this]() {
        QPoint cursorPos = QCursor::pos();
        if (lastCursorPos != cursorPos) {
            lastCursorPos = cursorPos;
            this->hover(cursorPos.x(), cursorPos.y());
        }
    });
    timer->start(10);
}

ColorPicker::~ColorPicker()
{
    delete scene;
    delete timer;
    delete ui;
}

void ColorPicker::hover(int mouseX, int mouseY) {
    QScreen *screen = QGuiApplication::primaryScreen();
    if (const QWindow *window = windowHandle())
        screen = window->screen();
    if (!screen)
        return;

    // 15 X 15 box with 8x magnification = 120px square)
    QRect zoomRect(mouseX - zoom_box_dimensions / 2, mouseY - zoom_box_dimensions / 2, zoom_box_dimensions, zoom_box_dimensions);
    QPixmap grab = screen->grabWindow(0, mouseX - zoom_box_dimensions / 2, mouseY - zoom_box_dimensions / 2, zoom_box_dimensions, zoom_box_dimensions);
    int pixelRatio = grab.devicePixelRatio();

    // TODO: investigate for high dpi displays why text is too high res
    grab.setDevicePixelRatio(1);
    QPixmap magnified = grab.scaled(zoom_box_dimensions * 8, zoom_box_dimensions * 8, Qt::KeepAspectRatio);

    QPainter painter(&magnified);
    painter.setRenderHint(QPainter::Antialiasing, false);
    QRectF rect(zoom_box_dimensions / 2 * 8 - 1, zoom_box_dimensions / 2 * 8 - 1, zoom_box_dimensions / 2 + 2, zoom_box_dimensions / 2 + 2);
    painter.drawRect(rect);
    painter.end();

    // TODO: bounds checking?

    this->color = grab.toImage().pixelColor(zoom_box_dimensions / 2 * pixelRatio, zoom_box_dimensions / 2 * pixelRatio);
    int r = this->color.red();
    int g = this->color.green();
    int b = this->color.blue();

    // update the displayed color value
    QString rgb = QString("rgb(%1, %2, %3)").arg(r).arg(g).arg(b);
    QString stylesheet = QString("background-color: %1;").arg(rgb);
    this->ui->frame_centralColor->setStyleSheet(stylesheet);

    this->ui->label_RGB->setText(rgb);
    this->ui->label_HEX->setText(color.name());

    this->ui->viewport->setPixmap(magnified);
}
