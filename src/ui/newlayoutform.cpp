#include "newlayoutform.h"
#include "ui_newlayoutform.h"
#include "project.h"

const QString lineEdit_ErrorStylesheet = "QLineEdit { background-color: rgba(255, 0, 0, 25%) }";

NewLayoutForm::NewLayoutForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NewLayoutForm)
{
    ui->setupUi(this);

    ui->groupBox_BorderDimensions->setVisible(projectConfig.useCustomBorderSize);

    ui->spinBox_BorderWidth->setMaximum(MAX_BORDER_WIDTH);
    ui->spinBox_BorderHeight->setMaximum(MAX_BORDER_HEIGHT);

    connect(ui->spinBox_MapWidth,  QOverload<int>::of(&QSpinBox::valueChanged), [=](int){ validateMapDimensions(); });
    connect(ui->spinBox_MapHeight, QOverload<int>::of(&QSpinBox::valueChanged), [=](int){ validateMapDimensions(); });

    connect(ui->comboBox_PrimaryTileset,   &NoScrollComboBox::editingFinished, [this]{ validatePrimaryTileset(true); });
    connect(ui->comboBox_SecondaryTileset, &NoScrollComboBox::editingFinished, [this]{ validateSecondaryTileset(true); });
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
}

void NewLayoutForm::setDisabled(bool disabled) {
    ui->groupBox_MapDimensions->setDisabled(disabled);
    ui->groupBox_BorderDimensions->setDisabled(disabled);
    ui->groupBox_Tilesets->setDisabled(disabled);
}

void NewLayoutForm::setSettings(const Layout::Settings &settings) {
    ui->spinBox_MapWidth->setValue(settings.width);
    ui->spinBox_MapHeight->setValue(settings.height);
    ui->spinBox_BorderWidth->setValue(settings.borderWidth);
    ui->spinBox_BorderHeight->setValue(settings.borderHeight);
    ui->comboBox_PrimaryTileset->setTextItem(settings.primaryTilesetLabel);
    ui->comboBox_SecondaryTileset->setTextItem(settings.secondaryTilesetLabel);
}

Layout::Settings NewLayoutForm::settings() const {
    Layout::Settings settings;
    settings.width = ui->spinBox_MapWidth->value();
    settings.height = ui->spinBox_MapHeight->value();
    if (ui->groupBox_BorderDimensions->isVisible()) {
        settings.borderWidth = ui->spinBox_BorderWidth->value();
        settings.borderHeight = ui->spinBox_BorderHeight->value();
    } else {
        settings.borderWidth = DEFAULT_BORDER_WIDTH;
        settings.borderHeight = DEFAULT_BORDER_HEIGHT;
    }
    settings.primaryTilesetLabel = ui->comboBox_PrimaryTileset->currentText();
    settings.secondaryTilesetLabel = ui->comboBox_SecondaryTileset->currentText();
    return settings;
}

bool NewLayoutForm::validate() {
    // Make sure to call each validation function so that all errors are shown at once.
    bool valid = true;
    if (!validateMapDimensions()) valid = false;
    if (!validatePrimaryTileset()) valid = false;
    if (!validateSecondaryTileset()) valid = false;
    return valid;
}

bool NewLayoutForm::validateMapDimensions() {
    int size = m_project->getMapDataSize(ui->spinBox_MapWidth->value(), ui->spinBox_MapHeight->value());
    int maxSize = m_project->getMaxMapDataSize();

    // TODO: Get from project
    const int additionalWidth = 15;
    const int additionalHeight = 14;

    QString errorText;
    if (size > maxSize) {
        errorText = QString("The specified width and height are too large.\n"
                    "The maximum map width and height is the following: (width + %1) * (height + %2) <= %3\n"
                    "The specified map width and height was: (%4 + %1) * (%5 + %2) = %6")
                        .arg(additionalWidth)
                        .arg(additionalHeight)
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

bool NewLayoutForm::validatePrimaryTileset(bool allowEmpty) {
    const QString name = ui->comboBox_PrimaryTileset->currentText();

    QString errorText;
    if (name.isEmpty()) {
        if (!allowEmpty) errorText = QString("The Primary Tileset cannot be empty.");
    } else if (ui->comboBox_PrimaryTileset->findText(name) < 0) {
        errorText = QString("The Primary Tileset '%1' does not exist.").arg(ui->label_PrimaryTileset->text()).arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_PrimaryTilesetError->setText(errorText);
    ui->label_PrimaryTilesetError->setVisible(!isValid);
    ui->comboBox_PrimaryTileset->lineEdit()->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}

bool NewLayoutForm::validateSecondaryTileset(bool allowEmpty) {
    const QString name = ui->comboBox_SecondaryTileset->currentText();

    QString errorText;
    if (name.isEmpty()) {
        if (!allowEmpty) errorText = QString("The Secondary Tileset cannot be empty.");
    } else if (ui->comboBox_SecondaryTileset->findText(name) < 0) {
        errorText = QString("The Secondary Tileset '%1' does not exist.").arg(name);
    }

    bool isValid = errorText.isEmpty();
    ui->label_SecondaryTilesetError->setText(errorText);
    ui->label_SecondaryTilesetError->setVisible(!isValid);
    ui->comboBox_SecondaryTileset->lineEdit()->setStyleSheet(!isValid ? lineEdit_ErrorStylesheet : "");
    return isValid;
}
