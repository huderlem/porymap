#include "ui_gridsettingsdialog.h"
#include "gridsettingsdialog.h"

// TODO: Add color picker
// TODO: Add styles
// TODO: Update units in UI
// TODO: Add linking chain button to width/height
// TODO: Add "snap to metatile" check box?
// TODO: Save settings in config
// TODO: Look into custom painting to improve performance
// TODO: Add tooltips

GridSettingsDialog::GridSettingsDialog(GridSettings *settings, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GridSettingsDialog),
    settings(settings)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    // TODO: Populate comboBox_Style

    ui->spinBox_Width->setMaximum(INT_MAX);
    ui->spinBox_Height->setMaximum(INT_MAX);
    ui->spinBox_X->setMaximum(INT_MAX);
    ui->spinBox_Y->setMaximum(INT_MAX);

    // Initialize UI values
    if (!this->settings)
        this->settings = new GridSettings; // TODO: Don't leak this
    this->originalSettings = *this->settings;
    reset(true);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &GridSettingsDialog::dialogButtonClicked);

    // TODO: Connect color picker
    // connect(ui->, &, this, &GridSettingsDialog::changedGridSettings);
}

void GridSettingsDialog::reset(bool force) {
    if (!force && *this->settings == this->originalSettings)
        return;
    *this->settings = this->originalSettings;

    // Avoid sending changedGridSettings multiple times
    const QSignalBlocker b_Width(ui->spinBox_Width);
    const QSignalBlocker b_Height(ui->spinBox_Height);
    const QSignalBlocker b_X(ui->spinBox_X);
    const QSignalBlocker b_Y(ui->spinBox_Y);

    ui->spinBox_Width->setValue(this->settings->width);
    ui->spinBox_Height->setValue(this->settings->height);
    ui->spinBox_X->setValue(this->settings->offsetX);
    ui->spinBox_Y->setValue(this->settings->offsetY);
    // TODO: Initialize comboBox_Style with settings->style
    // TODO: Initialize color with settings-color

    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Width_valueChanged(int value) {
    this->settings->width = value;
    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Height_valueChanged(int value) {
    this->settings->height = value;
    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_X_valueChanged(int value) {
    this->settings->offsetX = value;
    emit changedGridSettings();
}

void GridSettingsDialog::on_spinBox_Y_valueChanged(int value) {
    this->settings->offsetY = value;
    emit changedGridSettings();
}

void GridSettingsDialog::on_comboBox_Style_currentTextChanged(QString text) {
    this->settings->style = text;
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
        reset();
    }
}

GridSettingsDialog::~GridSettingsDialog() {
    delete ui;
}
