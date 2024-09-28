#include "ui_gridsettingsdialog.h"
#include "gridsettingsdialog.h"

// TODO: Save settings in config
// TODO: Look into custom painting to improve performance
// TODO: Add tooltips

const QList<QPair<QString, Qt::PenStyle>> penStyleMap = {
    {"Solid",          Qt::SolidLine},
    {"Large Dashes",   Qt::DashLine},
    {"Small Dashes",   Qt::DotLine},
    {"Dots",           Qt::CustomDashLine}, // TODO: Implement a custom pattern for this
};

GridSettingsDialog::GridSettingsDialog(GridSettings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GridSettingsDialog),
    settings(settings)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    // Populate the styles combo box
    for (const auto &pair : penStyleMap)
        ui->comboBox_Style->addItem(pair.first, static_cast<int>(pair.second));

    ui->spinBox_Width->setMaximum(INT_MAX);
    ui->spinBox_Height->setMaximum(INT_MAX);
    ui->spinBox_X->setMaximum(INT_MAX);
    ui->spinBox_Y->setMaximum(INT_MAX);

    ui->button_LinkDimensions->setChecked(this->dimensionsLinked);
    ui->button_LinkOffsets->setChecked(this->offsetsLinked);

    // Initialize the settings
    if (!this->settings)
        this->settings = new GridSettings; // TODO: Don't leak this
    this->originalSettings = *this->settings;
    reset(true);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &GridSettingsDialog::dialogButtonClicked);
    connect(ui->button_LinkDimensions, &QAbstractButton::toggled, [this](bool on) { this->dimensionsLinked = on; });
    connect(ui->button_LinkOffsets, &QAbstractButton::toggled, [this](bool on) { this->offsetsLinked = on; });
    connect(ui->colorInput, &ColorInputWidget::colorChanged, this, &GridSettingsDialog::onColorChanged);
}

GridSettingsDialog::~GridSettingsDialog() {
    delete ui;
}

void GridSettingsDialog::reset(bool force) {
    if (!force && *this->settings == this->originalSettings)
        return;
    *this->settings = this->originalSettings;

    setWidth(this->settings->width);
    setHeight(this->settings->height);
    setOffsetX(this->settings->offsetX);
    setOffsetY(this->settings->offsetY);

    const QSignalBlocker b_Color(ui->colorInput);
    ui->colorInput->setColor(this->settings->color.rgb());

    const QSignalBlocker b_Style(ui->comboBox_Style);
    // TODO: Debug
    //ui->comboBox_Style->setCurrentIndex(ui->comboBox_Style->findData(static_cast<int>(this->settings->style)));
    for (const auto &pair : penStyleMap) {
        if (pair.second == this->settings->style) {
            ui->comboBox_Style->setCurrentText(pair.first);
            break;
        }
    }

    emit changedGridSettings();
}

void GridSettingsDialog::setWidth(int value) {
    const QSignalBlocker b(ui->spinBox_Width);
    ui->spinBox_Width->setValue(value);
    this->settings->width = value;
}

void GridSettingsDialog::setHeight(int value) {
    const QSignalBlocker b(ui->spinBox_Height);
    ui->spinBox_Height->setValue(value);
    this->settings->height = value;
}

void GridSettingsDialog::setOffsetX(int value) {
    const QSignalBlocker b(ui->spinBox_X);
    ui->spinBox_X->setValue(value);
    this->settings->offsetX = value;
}

void GridSettingsDialog::setOffsetY(int value) {
    const QSignalBlocker b(ui->spinBox_Y);
    ui->spinBox_Y->setValue(value);
    this->settings->offsetY = value;
}

void GridSettingsDialog::on_spinBox_Width_valueChanged(int value) {
    setWidth(value);
    if (this->dimensionsLinked)
        setHeight(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Height_valueChanged(int value) {
    setHeight(value);
    if (this->dimensionsLinked)
        setWidth(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_X_valueChanged(int value) {
    setOffsetX(value);
    if (this->offsetsLinked)
        setOffsetY(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Y_valueChanged(int value) {
    setOffsetY(value);
    if (this->offsetsLinked)
        setOffsetX(value);

    emit changedGridSettings();
}

void GridSettingsDialog::on_comboBox_Style_currentIndexChanged(int index) {
    if (index < 0 || index >= penStyleMap.length())
        return;

    this->settings->style = penStyleMap.at(index).second;
    emit changedGridSettings();
}

void GridSettingsDialog::onColorChanged(QRgb color) {
    this->settings->color = QColor::fromRgb(color);
    emit changedGridSettings();
}

void GridSettingsDialog::dialogButtonClicked(QAbstractButton *button) {
    auto role = ui->buttonBox->buttonRole(button);
    if (role == QDialogButtonBox::AcceptRole) {
        close();
    } else if (role == QDialogButtonBox::RejectRole) {
        reset();
        close();
    } else if (role == QDialogButtonBox::ResetRole) {
        reset(); // TODO: We should restore to original defaults, not to the values when the window was opened.
    }
}
