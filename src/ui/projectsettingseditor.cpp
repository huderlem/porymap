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
    connect(ui->buttonBox, &QDialogButtonBox::clicked,
            this, &ProjectSettingsEditor::dialogButtonClicked);
}

ProjectSettingsEditor::~ProjectSettingsEditor()
{
    delete ui;
}

void ProjectSettingsEditor::initUi() {
    // Block signals while setting initial UI states
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
    const QSignalBlocker blocker15(ui->lineEdit_BorderMetatiles);
    const QSignalBlocker blocker16(ui->lineEdit_FillMetatile);
    const QSignalBlocker blocker17(ui->lineEdit_PrefabsPath);
    const QSignalBlocker blocker18(ui->lineEdit_BehaviorMask);
    const QSignalBlocker blocker19(ui->lineEdit_EncounterTypeMask);
    const QSignalBlocker blocker1A(ui->lineEdit_LayerTypeMask);
    const QSignalBlocker blocker1B(ui->lineEdit_TerrainTypeMask);

    // Create Default Tilesets combo boxes
    auto *defaultTilesetsLayout = new QFormLayout(ui->groupBox_DefaultTilesets);
    combo_defaultPrimaryTileset = new NoScrollComboBox(ui->groupBox_DefaultTilesets);
    combo_defaultSecondaryTileset = new NoScrollComboBox(ui->groupBox_DefaultTilesets);
    if (project) combo_defaultPrimaryTileset->addItems(project->primaryTilesetLabels);
    if (project) combo_defaultSecondaryTileset->addItems(project->secondaryTilesetLabels);
    combo_defaultPrimaryTileset->setTextItem(projectConfig.getDefaultPrimaryTileset());
    combo_defaultSecondaryTileset->setTextItem(projectConfig.getDefaultSecondaryTileset());
    defaultTilesetsLayout->addRow("Primary Tileset", combo_defaultPrimaryTileset);
    defaultTilesetsLayout->addRow("Secondary Tileset", combo_defaultSecondaryTileset);

    // Create Base game version combo box
    combo_baseGameVersion = new NoScrollComboBox(ui->widget_BaseGameVersion);
    combo_baseGameVersion->addItems(ProjectConfig::baseGameVersions);
    combo_baseGameVersion->setTextItem(projectConfig.getBaseGameVersionString());
    combo_baseGameVersion->setEditable(false);
    ui->layout_BaseGameVersion->insertRow(0, "Base game version", combo_baseGameVersion);

    // Create Attributes size combo box
    auto *attributesSizeLayout = new QFormLayout(ui->widget_SizeDropdown);
    combo_attributesSize = new NoScrollComboBox(ui->widget_SizeDropdown);
    combo_attributesSize->addItems({"1", "2", "4"});
    combo_attributesSize->setTextItem(QString::number(projectConfig.getMetatileAttributesSize()));
    combo_attributesSize->setEditable(false);
    attributesSizeLayout->addRow("", combo_attributesSize);

    // Init check boxes
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

    // Init spinners
    ui->spinBox_Elevation->setRange(0, 15);
    ui->spinBox_Elevation->setValue(projectConfig.getNewMapElevation());

    // Init text boxes
    // TODO: Validator for Border Metatiles and Fill Metatile
    ui->lineEdit_BorderMetatiles->setText(projectConfig.getNewMapBorderMetatileIdsString());
    ui->lineEdit_FillMetatile->setText(projectConfig.getNewMapMetatileIdString());
    ui->lineEdit_PrefabsPath->setText(projectConfig.getPrefabFilepath(false));
    QString mask = ProjectConfig::getMaskString(projectConfig.getMetatileBehaviorMask());
    ui->lineEdit_BehaviorMask->setText(mask);
    mask = ProjectConfig::getMaskString(projectConfig.getMetatileEncounterTypeMask());
    ui->lineEdit_EncounterTypeMask->setText(mask);
    mask = ProjectConfig::getMaskString(projectConfig.getMetatileLayerTypeMask());
    ui->lineEdit_LayerTypeMask->setText(mask);
    mask = ProjectConfig::getMaskString(projectConfig.getMetatileTerrainTypeMask());
    ui->lineEdit_TerrainTypeMask->setText(mask);
}

void ProjectSettingsEditor::saveFields() {
    // TODO
    /*
        TODO    combo_defaultPrimaryTileset
        TODO    combo_defaultSecondaryTileset
        setBaseGameVersion    combo_baseGameVersion
        TODO    combo_attributesSize
        setUsePoryScript    ui->checkBox_UsePoryscript
        userConfig.setEncounterJsonActive    ui->checkBox_ShowWildEncounterTables
        setCreateMapTextFileEnabled    ui->checkBox_CreateTextFile
        setPrefabImportPrompted    ui->checkBox_PrefabImportPrompted
        setTripleLayerMetatilesEnabled    ui->checkBox_EnableTripleLayerMetatiles
        setHiddenItemRequiresItemfinderEnabled    ui->checkBox_EnableRequiresItemfinder
        setHiddenItemQuantityEnabled    ui->checkBox_EnableQuantity
        setEventCloneObjectEnabled    ui->checkBox_EnableCloneObjects
        setEventWeatherTriggerEnabled    ui->checkBox_EnableWeatherTriggers
        setEventSecretBaseEnabled    ui->checkBox_EnableSecretBases
        setHealLocationRespawnDataEnabled    ui->checkBox_EnableRespawn
        TODO    ui->checkBox_EnableAllowFlags
        setFloorNumberEnabled    ui->checkBox_EnableFloorNumber
        setUseCustomBorderSize    ui->checkBox_EnableCustomBorderSize
        setTilesetsHaveCallback    ui->checkBox_OutputCallback
        setTilesetsHaveIsCompressed    ui->checkBox_OutputIsCompressed
        setNewMapElevation    ui->spinBox_Elevation
        setPrefabFilepath    ui->lineEdit_PrefabsPath
        TODO    ui->lineEdit_BehaviorMask
        TODO    ui->lineEdit_EncounterTypeMask
        TODO    ui->lineEdit_LayerTypeMask
        TODO    ui->lineEdit_TerrainTypeMask
        setNewMapMetatileId    ui->lineEdit_FillMetatile
        setNewMapBorderMetatileIds    ui->lineEdit_BorderMetatiles
    */
    emit saved();
}

void ProjectSettingsEditor::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        // TODO: Prompt for unsaved changes
        saveFields();
        close();
    } else if (buttonRole == QDialogButtonBox::ResetRole) {
        // TODO
    } else if (buttonRole == QDialogButtonBox::ApplyRole) {
        saveFields();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        close();
    }
    // TODO: Save geometry on close
}
