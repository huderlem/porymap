#include "colorinputwidget.h"
#include "ui_colorinputwidget.h"
#include "colorpicker.h"
#include "validator.h"

#include <cmath>

static inline int rgb5(int rgb) { return round(static_cast<double>(rgb * 31) / 255.0); }
static inline int rgb8(int rgb) { return round(rgb * 255. / 31.); }
static inline int gbaRed(int rgb) { return rgb & 0x1f; }
static inline int gbaGreen(int rgb) { return (rgb >> 5) & 0x1f; }
static inline int gbaBlue(int rgb) { return (rgb >> 10) & 0x1f; }

ColorInputWidget::ColorInputWidget(QWidget *parent) :
    QGroupBox(parent),
    ui(new Ui::ColorInputWidget)
{
    init();
}

ColorInputWidget::ColorInputWidget(const QString &title, QWidget *parent) :
    QGroupBox(title, parent),
    ui(new Ui::ColorInputWidget)
{
    init();
}

void ColorInputWidget::init() {
    ui->setupUi(this);

    // Connect color change signals
    connect(ui->slider_Red,   &QSlider::valueChanged, this, &ColorInputWidget::setRgbFromSliders);
    connect(ui->slider_Green, &QSlider::valueChanged, this, &ColorInputWidget::setRgbFromSliders);
    connect(ui->slider_Blue,  &QSlider::valueChanged, this, &ColorInputWidget::setRgbFromSliders);

    connect(ui->spinBox_Red,   QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorInputWidget::setRgbFromSpinners);
    connect(ui->spinBox_Green, QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorInputWidget::setRgbFromSpinners);
    connect(ui->spinBox_Blue,  QOverload<int>::of(&QSpinBox::valueChanged), this, &ColorInputWidget::setRgbFromSpinners);

    static const UppercaseValidator uppercaseValidator;
    ui->lineEdit_Hex->setValidator(&uppercaseValidator);
    connect(ui->lineEdit_Hex, &QLineEdit::textEdited, this, &ColorInputWidget::setRgbFromHexString);

    // We have separate signals for when color input editing finishes.
    // This is mostly useful for external commit histories, esp. for the sliders which can rapidly emit color change signals.
    connect(ui->slider_Red,   &QSlider::sliderReleased, this, &ColorInputWidget::editingFinished);
    connect(ui->slider_Green, &QSlider::sliderReleased, this, &ColorInputWidget::editingFinished);
    connect(ui->slider_Blue,  &QSlider::sliderReleased, this, &ColorInputWidget::editingFinished);

    connect(ui->spinBox_Red,   &QSpinBox::editingFinished, this, &ColorInputWidget::editingFinished);
    connect(ui->spinBox_Green, &QSpinBox::editingFinished, this, &ColorInputWidget::editingFinished);
    connect(ui->spinBox_Blue,  &QSpinBox::editingFinished, this, &ColorInputWidget::editingFinished);

    connect(ui->lineEdit_Hex, &QLineEdit::editingFinished, this, &ColorInputWidget::editingFinished);

    // Connect color picker
    connect(ui->button_Eyedrop, &QToolButton::clicked, this, &ColorInputWidget::pickColor);

    setBitDepth(24);
}

ColorInputWidget::~ColorInputWidget() {
    delete ui;
}

void ColorInputWidget::updateColorUi() {
    blockEditSignals(true);

    int red = qRed(m_color);
    int green = qGreen(m_color);
    int blue = qBlue(m_color);

    if (m_bitDepth == 15) {
        // Sliders
        ui->slider_Red->setValue(rgb5(red));
        ui->slider_Green->setValue(rgb5(green));
        ui->slider_Blue->setValue(rgb5(blue));

        // Hex
        int hex15 = (rgb5(blue) << 10) | (rgb5(green) << 5) | rgb5(red);
        ui->lineEdit_Hex->setText(QString("%1").arg(hex15, 4, 16, QLatin1Char('0')).toUpper());

        // Spinners
        ui->spinBox_Red->setValue(rgb5(red));
        ui->spinBox_Green->setValue(rgb5(green));
        ui->spinBox_Blue->setValue(rgb5(blue));
    } else {
        // Sliders
        ui->slider_Red->setValue(red);
        ui->slider_Green->setValue(green);
        ui->slider_Blue->setValue(blue);

        // Hex
        QColor color(red, green, blue);
        ui->lineEdit_Hex->setText(color.name().remove(0, 1).toUpper());

        // Spinners
        ui->spinBox_Red->setValue(red);
        ui->spinBox_Green->setValue(green);
        ui->spinBox_Blue->setValue(blue);
    }

    ui->frame_ColorDisplay->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(red).arg(green).arg(blue));

    blockEditSignals(false);
}

void ColorInputWidget::blockEditSignals(bool block) {
    ui->slider_Red->blockSignals(block);
    ui->slider_Green->blockSignals(block);
    ui->slider_Blue->blockSignals(block);

    ui->spinBox_Red->blockSignals(block);
    ui->spinBox_Green->blockSignals(block);
    ui->spinBox_Blue->blockSignals(block);

    ui->lineEdit_Hex->blockSignals(block);
}

bool ColorInputWidget::setBitDepth(int bits) {
    if (m_bitDepth == bits)
        return true;

    int singleStep, pageStep, maximum;
    QString hexInputMask;
    if (bits == 15) {
        singleStep = 1;
        pageStep = 4;
        maximum = 31;
        hexInputMask = "HHHH";
    } else if (bits == 24) {
        singleStep = 8;
        pageStep = 24;
        maximum = 255;
        hexInputMask = "HHHHHH";
    } else {
        // Unsupported bit depth
        return false;
    }
    m_bitDepth = bits;

    blockEditSignals(true);
    ui->slider_Red->setSingleStep(singleStep);
    ui->slider_Green->setSingleStep(singleStep);
    ui->slider_Blue->setSingleStep(singleStep);
    ui->slider_Red->setPageStep(pageStep);
    ui->slider_Green->setPageStep(pageStep);
    ui->slider_Blue->setPageStep(pageStep);
    ui->slider_Red->setMaximum(maximum);
    ui->slider_Green->setMaximum(maximum);
    ui->slider_Blue->setMaximum(maximum);

    ui->spinBox_Red->setSingleStep(singleStep);
    ui->spinBox_Green->setSingleStep(singleStep);
    ui->spinBox_Blue->setSingleStep(singleStep);
    ui->spinBox_Red->setMaximum(maximum);
    ui->spinBox_Green->setMaximum(maximum);
    ui->spinBox_Blue->setMaximum(maximum);

    ui->lineEdit_Hex->setInputMask(hexInputMask);
    ui->lineEdit_Hex->setMaxLength(hexInputMask.length());

    updateColorUi();
    blockEditSignals(false);
    emit bitDepthChanged(m_bitDepth);
    return true;
}

void ColorInputWidget::setColor(QRgb rgb) {
    if (m_color == rgb)
        return;
    m_color = rgb;
    updateColorUi();
    emit colorChanged(m_color);
}

void ColorInputWidget::setRgbFromSliders() {
    if (m_bitDepth == 15) {
        setColor(qRgb(rgb8(ui->slider_Red->value()),
                      rgb8(ui->slider_Green->value()),
                      rgb8(ui->slider_Blue->value())));
    } else {
        setColor(qRgb(ui->slider_Red->value(),
                      ui->slider_Green->value(),
                      ui->slider_Blue->value()));
    }
}

void ColorInputWidget::setRgbFromSpinners() {
    if (m_bitDepth == 15) {
        setColor(qRgb(rgb8(ui->spinBox_Red->value()), rgb8(ui->spinBox_Green->value()), rgb8(ui->spinBox_Blue->value())));
    } else {
        setColor(qRgb(ui->spinBox_Red->value(), ui->spinBox_Green->value(), ui->spinBox_Blue->value()));
    }
}

void ColorInputWidget::setRgbFromHexString(const QString &text) {
    if ((m_bitDepth == 24 && text.length() != 6)
     || (m_bitDepth == 15 && text.length() != 4))
        return;

    bool ok = false;
    int rgb = text.toInt(&ok, 16);
    if (!ok) rgb = 0xFFFFFFFF;

    if (m_bitDepth == 15) {
        int rc = gbaRed(rgb);
        int gc = gbaGreen(rgb);
        int bc = gbaBlue(rgb);
        setColor(qRgb(rgb8(rc), rgb8(gc), rgb8(bc)));
    } else {
        setColor(qRgb(qRed(rgb), qGreen(rgb), qBlue(rgb)));
    }
}

void ColorInputWidget::pickColor() {
    ColorPicker picker(this);
    if (picker.exec() == QDialog::Accepted) {
        QColor c = picker.getColor();
        setColor(c.rgb());
        emit editingFinished();
    }
}
