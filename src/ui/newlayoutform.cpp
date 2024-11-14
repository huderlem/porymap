#include "newlayoutform.h"
#include "ui_newlayoutform.h"
#include "project.h"

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewLayoutForm::NewLayoutForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewLayoutForm)
{
    ui->setupUi(this);

    // TODO: Read from project?
    ui->spinBox_BorderWidth->setMaximum(MAX_BORDER_WIDTH);
    ui->spinBox_BorderHeight->setMaximum(MAX_BORDER_HEIGHT);

    connect(ui->spinBox_MapWidth, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){validateMapDimensions();});
    connect(ui->spinBox_MapHeight, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){validateMapDimensions();});
}

NewLayoutForm::~NewLayoutForm()
{
    delete ui;
}

void NewLayoutForm::initUi(Project *project) {
    m_project = project;

    ui->comboBox_PrimaryTileset->clear();
    ui->comboBox_SecondaryTileset->clear();

    if (m_project) {
        ui->comboBox_PrimaryTileset->addItems(m_project->primaryTilesetLabels);
        ui->comboBox_SecondaryTileset->addItems(m_project->secondaryTilesetLabels);

        ui->spinBox_MapWidth->setMaximum(m_project->getMaxMapWidth());
        ui->spinBox_MapHeight->setMaximum(m_project->getMaxMapHeight());
    }

    ui->groupBox_BorderDimensions->setVisible(projectConfig.useCustomBorderSize);
}

void NewLayoutForm::setDisabled(bool disabled) {
    ui->groupBox_MapDimensions->setDisabled(disabled);
    ui->groupBox_BorderDimensions->setDisabled(disabled);
    ui->groupBox_Tilesets->setDisabled(disabled);
}

void NewLayoutForm::setSettings(const Settings &settings) {
    ui->spinBox_MapWidth->setValue(settings.width);
    ui->spinBox_MapHeight->setValue(settings.height);
    ui->spinBox_BorderWidth->setValue(settings.borderWidth);
    ui->spinBox_BorderHeight->setValue(settings.borderHeight);
    ui->comboBox_PrimaryTileset->setTextItem(settings.primaryTilesetLabel);
    ui->comboBox_SecondaryTileset->setTextItem(settings.secondaryTilesetLabel);
}

NewLayoutForm::Settings NewLayoutForm::settings() const {
    NewLayoutForm::Settings settings;
    settings.width = ui->spinBox_MapWidth->value();
    settings.height = ui->spinBox_MapHeight->value();
    settings.borderWidth = ui->spinBox_BorderWidth->value();
    settings.borderHeight = ui->spinBox_BorderHeight->value();
    settings.primaryTilesetLabel = ui->comboBox_PrimaryTileset->currentText();
    settings.secondaryTilesetLabel = ui->comboBox_SecondaryTileset->currentText();
    return settings;
}

bool NewLayoutForm::validate() {
    // Make sure to call each validation function so that all errors are shown at once.
    bool valid = true;
    if (!validateMapDimensions()) valid = false;
    if (!validateTilesets()) valid = false;
    return valid;
}

bool NewLayoutForm::validateMapDimensions() {
    int size = m_project->getMapDataSize(ui->spinBox_MapWidth->value(), ui->spinBox_MapHeight->value());
    int maxSize = m_project->getMaxMapDataSize();

    QString errorText;
    if (size > maxSize) {
        errorText = QString("The specified width and height are too large.\n"
                    "The maximum map width and height is the following: (width + 15) * (height + 14) <= %1\n"
                    "The specified map width and height was: (%2 + 15) * (%3 + 14) = %4")
                        .arg(maxSize)
                        .arg(ui->spinBox_MapWidth->value())
                        .arg(ui->spinBox_MapHeight->value())
                        .arg(size);
    }

    bool isValid = errorText.isEmpty();
    ui->label_MapDimensionsError->setText(errorText);
    ui->label_MapDimensionsError->setVisible(!isValid);
    return isValid;
}

bool NewLayoutForm::validateTilesets() {
    QString primaryTileset = ui->comboBox_PrimaryTileset->currentText();
    QString secondaryTileset = ui->comboBox_SecondaryTileset->currentText();

    QString primaryErrorText;
    if (primaryTileset.isEmpty()) {
        primaryErrorText = QString("The primary tileset cannot be empty.");
    } else if (ui->comboBox_PrimaryTileset->findText(primaryTileset) < 0) {
        primaryErrorText = QString("The specified primary tileset '%1' does not exist.").arg(primaryTileset);
    }

    QString secondaryErrorText;
    if (secondaryTileset.isEmpty()) {
        secondaryErrorText = QString("The secondary tileset cannot be empty.");
    } else if (ui->comboBox_SecondaryTileset->findText(secondaryTileset) < 0) {
        secondaryErrorText = QString("The specified secondary tileset '%2' does not exist.").arg(secondaryTileset);
    }

    QString errorText = QString("%1%2%3")
                        .arg(primaryErrorText)
                        .arg(!primaryErrorText.isEmpty() ? "\n" : "")
                        .arg(secondaryErrorText);

    bool isValid = errorText.isEmpty();
    ui->label_TilesetsError->setText(errorText);
    ui->label_TilesetsError->setVisible(!isValid);
    ui->comboBox_PrimaryTileset->lineEdit()->setStyleSheet(!primaryErrorText.isEmpty() ? lineEdit_ErrorStylesheet : "");
    ui->comboBox_SecondaryTileset->lineEdit()->setStyleSheet(!secondaryErrorText.isEmpty() ? lineEdit_ErrorStylesheet : "");
    return isValid;
}
