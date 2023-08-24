#include "projectsettingseditor.h"
#include "ui_projectsettingseditor.h"
#include "config.h"
#include "noscrollcombobox.h"

#include <QAbstractButton>
#include <QFormLayout>

/*
    Editor for the settings in a user's porymap.project.cfg and porymap.user.cfg files.
*/

ProjectSettingsEditor::ProjectSettingsEditor(QWidget *parent, Project *project) :
    QMainWindow(parent),
    ui(new Ui::ProjectSettingsEditor),
    project(project),
    combo_defaultPrimaryTileset(nullptr),
    combo_defaultSecondaryTileset(nullptr),
    combo_baseGameVersion(nullptr),
    combo_attributesSize(nullptr)
{
    ui->setupUi(this);
    initUi();
    setAttribute(Qt::WA_DeleteOnClose);
    connectSignals();
    refresh();
}

ProjectSettingsEditor::~ProjectSettingsEditor()
{
    delete ui;
}

void ProjectSettingsEditor::connectSignals() {
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &ProjectSettingsEditor::dialogButtonClicked);

    // Connect combo boxes
    QList<NoScrollComboBox *> combos = ui->centralwidget->findChildren<NoScrollComboBox *>();
    foreach(auto i, combos)
        connect(i, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::markEdited);

    // Connect check boxes
    QList<QCheckBox *> checkboxes = ui->centralwidget->findChildren<QCheckBox *>();
    foreach(auto i, checkboxes)
        connect(i, &QCheckBox::stateChanged, this, &ProjectSettingsEditor::markEdited);

    // Connect spin boxes
    QList<QSpinBox *> spinBoxes = ui->centralwidget->findChildren<QSpinBox *>();
    foreach(auto i, spinBoxes)
        connect(i, QOverload<int>::of(&QSpinBox::valueChanged), [this](int) { this->markEdited(); });

    // Connect line edits
    QList<QLineEdit *> lineEdits = ui->centralwidget->findChildren<QLineEdit *>();
    foreach(auto i, lineEdits)
        connect(i, &QLineEdit::textEdited, this, &ProjectSettingsEditor::markEdited);
}

void ProjectSettingsEditor::markEdited() {
    this->hasUnsavedChanges = true;
}

void ProjectSettingsEditor::initUi() {
    // Create Default Tilesets combo boxes
    auto *defaultTilesetsLayout = new QFormLayout(ui->groupBox_DefaultTilesets);
    combo_defaultPrimaryTileset = new NoScrollComboBox(ui->groupBox_DefaultTilesets);
    combo_defaultSecondaryTileset = new NoScrollComboBox(ui->groupBox_DefaultTilesets);
    if (project) combo_defaultPrimaryTileset->addItems(project->primaryTilesetLabels);
    if (project) combo_defaultSecondaryTileset->addItems(project->secondaryTilesetLabels);
    defaultTilesetsLayout->addRow("Primary Tileset", combo_defaultPrimaryTileset);
    defaultTilesetsLayout->addRow("Secondary Tileset", combo_defaultSecondaryTileset);

    // Create Base game version combo box
    combo_baseGameVersion = new NoScrollComboBox(ui->widget_BaseGameVersion);
    combo_baseGameVersion->addItems(ProjectConfig::versionStrings);
    combo_baseGameVersion->setEditable(false);
    ui->layout_BaseGameVersion->insertRow(0, "Base game version", combo_baseGameVersion);

    // Create Attributes size combo box
    auto *attributesSizeLayout = new QFormLayout(ui->widget_SizeDropdown);
    combo_attributesSize = new NoScrollComboBox(ui->widget_SizeDropdown);
    combo_attributesSize->addItems({"1", "2", "4"});
    combo_attributesSize->setEditable(false);
    attributesSizeLayout->addRow("", combo_attributesSize);

    // Validate that the border metatiles text is a comma-separated list of hex values
    static const QRegularExpression expression("^((0[xX])?[A-Fa-f0-9]+,)*(0[xX])?[A-Fa-f0-9]$");
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression);
    ui->lineEdit_BorderMetatiles->setValidator(validator);

    ui->spinBox_Elevation->setMaximum(15);
    ui->spinBox_FillMetatile->setMaximum(Project::getNumMetatilesTotal() - 1);

    // TODO: These need to be subclassed to handle larger values
    ui->spinBox_BehaviorMask->setMaximum(INT_MAX);
    ui->spinBox_EncounterTypeMask->setMaximum(INT_MAX);
    ui->spinBox_LayerTypeMask->setMaximum(INT_MAX);
    ui->spinBox_TerrainTypeMask->setMaximum(INT_MAX);

    // TODO: File picker for prefabs?
}

// Set UI states using config data
void ProjectSettingsEditor::refresh() {
    const QSignalBlocker blocker0(combo_defaultPrimaryTileset);
    const QSignalBlocker blocker1(combo_defaultSecondaryTileset);
    const QSignalBlocker blocker2(combo_baseGameVersion);
    const QSignalBlocker blocker3(combo_attributesSize);
    const QSignalBlocker blocker4(ui->checkBox_UsePoryscript);
    const QSignalBlocker blocker5(ui->checkBox_ShowWildEncounterTables);
    const QSignalBlocker blocker6(ui->checkBox_CreateTextFile);
    const QSignalBlocker blocker7(ui->checkBox_PrefabImportPrompted);
    const QSignalBlocker blocker8(ui->checkBox_EnableTripleLayerMetatiles);
    const QSignalBlocker blocker9(ui->checkBox_EnableRequiresItemfinder);
    const QSignalBlocker blockerA(ui->checkBox_EnableQuantity);
    const QSignalBlocker blockerB(ui->checkBox_EnableCloneObjects);
    const QSignalBlocker blockerC(ui->checkBox_EnableWeatherTriggers);
    const QSignalBlocker blockerD(ui->checkBox_EnableSecretBases);
    const QSignalBlocker blockerE(ui->checkBox_EnableRespawn);
    const QSignalBlocker blockerF(ui->checkBox_EnableAllowFlags);
    const QSignalBlocker blocker10(ui->checkBox_EnableFloorNumber);
    const QSignalBlocker blocker11(ui->checkBox_EnableCustomBorderSize);
    const QSignalBlocker blocker12(ui->checkBox_OutputCallback);
    const QSignalBlocker blocker13(ui->checkBox_OutputIsCompressed);
    const QSignalBlocker blocker14(ui->spinBox_Elevation);
    const QSignalBlocker blocker15(ui->spinBox_FillMetatile);
    const QSignalBlocker blocker16(ui->spinBox_BehaviorMask);
    const QSignalBlocker blocker17(ui->spinBox_EncounterTypeMask);
    const QSignalBlocker blocker18(ui->spinBox_LayerTypeMask);
    const QSignalBlocker blocker19(ui->spinBox_TerrainTypeMask);
    const QSignalBlocker blocker1A(ui->lineEdit_BorderMetatiles);
    const QSignalBlocker blocker1B(ui->lineEdit_PrefabsPath);

    // Set combo box texts
    combo_defaultPrimaryTileset->setTextItem(projectConfig.getDefaultPrimaryTileset());
    combo_defaultSecondaryTileset->setTextItem(projectConfig.getDefaultSecondaryTileset());
    combo_baseGameVersion->setTextItem(projectConfig.getBaseGameVersionString());
    combo_attributesSize->setTextItem(QString::number(projectConfig.getMetatileAttributesSize()));

    // Set check box states
    ui->checkBox_UsePoryscript->setChecked(projectConfig.getUsePoryScript());
    ui->checkBox_ShowWildEncounterTables->setChecked(userConfig.getEncounterJsonActive());
    ui->checkBox_CreateTextFile->setChecked(projectConfig.getCreateMapTextFileEnabled());
    ui->checkBox_PrefabImportPrompted->setChecked(projectConfig.getPrefabImportPrompted());
    ui->checkBox_EnableTripleLayerMetatiles->setChecked(projectConfig.getTripleLayerMetatilesEnabled());
    ui->checkBox_EnableRequiresItemfinder->setChecked(projectConfig.getHiddenItemRequiresItemfinderEnabled());
    ui->checkBox_EnableQuantity->setChecked(projectConfig.getHiddenItemQuantityEnabled());
    ui->checkBox_EnableCloneObjects->setChecked(projectConfig.getEventCloneObjectEnabled());
    ui->checkBox_EnableWeatherTriggers->setChecked(projectConfig.getEventWeatherTriggerEnabled());
    ui->checkBox_EnableSecretBases->setChecked(projectConfig.getEventSecretBaseEnabled());
    ui->checkBox_EnableRespawn->setChecked(projectConfig.getHealLocationRespawnDataEnabled());
    ui->checkBox_EnableAllowFlags->setChecked(projectConfig.getMapAllowFlagsEnabled());
    ui->checkBox_EnableFloorNumber->setChecked(projectConfig.getFloorNumberEnabled());
    ui->checkBox_EnableCustomBorderSize->setChecked(projectConfig.getUseCustomBorderSize());
    ui->checkBox_OutputCallback->setChecked(projectConfig.getTilesetsHaveCallback());
    ui->checkBox_OutputIsCompressed->setChecked(projectConfig.getTilesetsHaveIsCompressed());

    // Set spin box values
    ui->spinBox_Elevation->setValue(projectConfig.getNewMapElevation());
    ui->spinBox_FillMetatile->setValue(projectConfig.getNewMapMetatileId());
    ui->spinBox_BehaviorMask->setValue(projectConfig.getMetatileBehaviorMask());
    ui->spinBox_EncounterTypeMask->setValue(projectConfig.getMetatileEncounterTypeMask());
    ui->spinBox_LayerTypeMask->setValue(projectConfig.getMetatileLayerTypeMask());
    ui->spinBox_TerrainTypeMask->setValue(projectConfig.getMetatileTerrainTypeMask());

    // Set line edit texts
    ui->lineEdit_BorderMetatiles->setText(projectConfig.getNewMapBorderMetatileIdsString());
    ui->lineEdit_PrefabsPath->setText(projectConfig.getPrefabFilepath(false));
}

// TODO: Certain setting changes may require project reload

void ProjectSettingsEditor::saveFields() {
    if (!this->hasUnsavedChanges)
        return;

    // Prevent a call to save() for each of the config settings
    projectConfig.setSaveDisabled(true);
    userConfig.setSaveDisabled(true);

    projectConfig.setDefaultPrimaryTileset(combo_defaultPrimaryTileset->currentText());
    projectConfig.setDefaultSecondaryTileset(combo_defaultSecondaryTileset->currentText());
    projectConfig.setBaseGameVersion(projectConfig.stringToBaseGameVersion(combo_baseGameVersion->currentText()));
    projectConfig.setMetatileAttributesSize(combo_attributesSize->currentText().toInt());
    projectConfig.setUsePoryScript(ui->checkBox_UsePoryscript->isChecked());
    userConfig.setEncounterJsonActive(ui->checkBox_ShowWildEncounterTables->isChecked());
    projectConfig.setCreateMapTextFileEnabled(ui->checkBox_CreateTextFile->isChecked());
    projectConfig.setPrefabImportPrompted(ui->checkBox_PrefabImportPrompted->isChecked());
    projectConfig.setTripleLayerMetatilesEnabled(ui->checkBox_EnableTripleLayerMetatiles->isChecked());
    projectConfig.setHiddenItemRequiresItemfinderEnabled(ui->checkBox_EnableRequiresItemfinder->isChecked());
    projectConfig.setHiddenItemQuantityEnabled(ui->checkBox_EnableQuantity->isChecked());
    projectConfig.setEventCloneObjectEnabled(ui->checkBox_EnableCloneObjects->isChecked());
    projectConfig.setEventWeatherTriggerEnabled(ui->checkBox_EnableWeatherTriggers->isChecked());
    projectConfig.setEventSecretBaseEnabled(ui->checkBox_EnableSecretBases->isChecked());
    projectConfig.setHealLocationRespawnDataEnabled(ui->checkBox_EnableRespawn->isChecked());
    projectConfig.setMapAllowFlagsEnabled(ui->checkBox_EnableAllowFlags->isChecked());
    projectConfig.setFloorNumberEnabled(ui->checkBox_EnableFloorNumber->isChecked());
    projectConfig.setUseCustomBorderSize(ui->checkBox_EnableCustomBorderSize->isChecked());
    projectConfig.setTilesetsHaveCallback(ui->checkBox_OutputCallback->isChecked());
    projectConfig.setTilesetsHaveIsCompressed(ui->checkBox_OutputIsCompressed->isChecked());
    projectConfig.setNewMapElevation(ui->spinBox_Elevation->value());
    projectConfig.setNewMapMetatileId(ui->spinBox_FillMetatile->value());
    projectConfig.setMetatileBehaviorMask(ui->spinBox_BehaviorMask->value());
    projectConfig.setMetatileTerrainTypeMask(ui->spinBox_EncounterTypeMask->value());
    projectConfig.setMetatileEncounterTypeMask(ui->spinBox_LayerTypeMask->value());
    projectConfig.setMetatileLayerTypeMask(ui->spinBox_TerrainTypeMask->value());
    projectConfig.setPrefabFilepath(ui->lineEdit_PrefabsPath->text());

    // Parse border metatile list
    QList<QString> metatileIdStrings = ui->lineEdit_BorderMetatiles->text().split(",");
    QList<uint16_t> metatileIds;
    for (auto s : metatileIdStrings) {
        uint16_t metatileId = s.toUInt(nullptr, 0);
        metatileIds.append(qMin(metatileId, static_cast<uint16_t>(Project::getNumMetatilesTotal() - 1)));
    }
    projectConfig.setNewMapBorderMetatileIds(metatileIds);

    projectConfig.setSaveDisabled(false);
    projectConfig.save();
    userConfig.setSaveDisabled(false);
    userConfig.save();

    this->hasUnsavedChanges = false;
    emit saved();
}

// TODO: Standard prompt replacement?
bool ProjectSettingsEditor::prompt(const QString &text) {
    QMessageBox messageBox(this);
    messageBox.setText(text);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No); // TODO: Cancel
    return messageBox.exec() == QMessageBox::Yes;
}

void ProjectSettingsEditor::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        saveFields();
        close();
    } else if (buttonRole == QDialogButtonBox::ApplyRole) {
        saveFields();
    } else if (buttonRole == QDialogButtonBox::ResetRole) {
        // Restore Defaults
        // TODO: Confirm dialogue?
        const QString versionText = combo_baseGameVersion->currentText();
        if (!prompt(QString("Restore default config settings for %1?").arg(versionText)))
            return;
        projectConfig.reset(projectConfig.stringToBaseGameVersion(versionText));
        projectConfig.save();
        userConfig.reset();
        userConfig.save();
        refresh();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        if (this->hasUnsavedChanges && !prompt(QString("Discard unsaved changes?"))) {
            // TODO:
            // Unsaved changes prompt
        }
        close();
    }
    // TODO: Save geometry on close
}
