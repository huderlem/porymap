#include "projectsettingseditor.h"
#include "config.h"
#include "noscrollcombobox.h"
#include "prefab.h"

#include <QAbstractButton>
#include <QFormLayout>

/*
    Editor for the settings in a user's porymap.project.cfg file (and 'use_encounter_json' in porymap.user.cfg).
    Disabling the warp behavior warning is actually part of porymap.cfg, but it's on this window because the
    related settings are here (and project-specific).
*/

const int ProjectSettingsEditor::eventsTab = 3;

ProjectSettingsEditor::ProjectSettingsEditor(QWidget *parent, Project *project) :
    QMainWindow(parent),
    ui(new Ui::ProjectSettingsEditor),
    project(project),
    baseDir(userConfig.getProjectDir() + QDir::separator())
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    this->initUi();
    this->createProjectPathsTable();
    this->createProjectIdentifiersTable();
    this->connectSignals();
    this->refresh();
    this->restoreWindowState();
}

ProjectSettingsEditor::~ProjectSettingsEditor()
{
    delete ui;
}

void ProjectSettingsEditor::connectSignals() {
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &ProjectSettingsEditor::dialogButtonClicked);
    connect(ui->button_ImportDefaultPrefabs, &QAbstractButton::clicked, this, &ProjectSettingsEditor::importDefaultPrefabsClicked);
    connect(ui->comboBox_BaseGameVersion, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::promptRestoreDefaults);
    connect(ui->comboBox_AttributesSize, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::updateAttributeLimits);
    connect(ui->comboBox_IconSpecies, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::updatePokemonIconPath);
    connect(ui->checkBox_EnableCustomBorderSize, &QCheckBox::stateChanged, [this](int state) {
        bool customSize = (state == Qt::Checked);
        // When switching between the spin boxes or line edit for border metatiles we set
        // the newly-shown UI using the values from the hidden UI.
        this->setBorderMetatileIds(customSize, this->getBorderMetatileIds(!customSize));
        this->setBorderMetatilesUi(customSize);
    });
    connect(ui->button_AddWarpBehavior,    &QAbstractButton::clicked, [this](bool) { this->updateWarpBehaviorsList(true); });
    connect(ui->button_RemoveWarpBehavior, &QAbstractButton::clicked, [this](bool) { this->updateWarpBehaviorsList(false); });

    // Connect file selection buttons
    connect(ui->button_ChoosePrefabs,     &QAbstractButton::clicked, [this](bool) { this->choosePrefabsFile(); });
    connect(ui->button_CollisionGraphics, &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_CollisionGraphics); });
    connect(ui->button_ObjectsIcon,       &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_ObjectsIcon); });
    connect(ui->button_WarpsIcon,         &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_WarpsIcon); });
    connect(ui->button_TriggersIcon,      &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_TriggersIcon); });
    connect(ui->button_BGsIcon,           &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_BGsIcon); });
    connect(ui->button_HealspotsIcon,     &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_HealspotsIcon); });
    connect(ui->button_PokemonIcon,       &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_PokemonIcon); });


    // Display a warning if a mask value overlaps with another mask in its group.
    connect(ui->spinBox_MetatileIdMask, &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateBlockMaskOverlapWarning);
    connect(ui->spinBox_CollisionMask,  &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateBlockMaskOverlapWarning);
    connect(ui->spinBox_ElevationMask,  &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateBlockMaskOverlapWarning);
    connect(ui->spinBox_BehaviorMask,      &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateAttributeMaskOverlapWarning);
    connect(ui->spinBox_LayerTypeMask,     &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateAttributeMaskOverlapWarning);
    connect(ui->spinBox_EncounterTypeMask, &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateAttributeMaskOverlapWarning);
    connect(ui->spinBox_TerrainTypeMask,   &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::updateAttributeMaskOverlapWarning);

    // Record that there are unsaved changes if any of the settings are modified
    for (auto combo : ui->centralwidget->findChildren<NoScrollComboBox *>()){
         // Changes to these two combo boxes are just for info display, don't mark as unsaved
        if (combo != ui->comboBox_IconSpecies && combo != ui->comboBox_WarpBehaviors)
            connect(combo, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::markEdited);
    }
    for (auto checkBox : ui->centralwidget->findChildren<QCheckBox *>())
        connect(checkBox, &QCheckBox::stateChanged, this, &ProjectSettingsEditor::markEdited);
    for (auto lineEdit : ui->centralwidget->findChildren<QLineEdit *>())
        connect(lineEdit, &QLineEdit::textEdited, this, &ProjectSettingsEditor::markEdited);
    for (auto spinBox : ui->centralwidget->findChildren<NoScrollSpinBox *>())
        connect(spinBox, &QSpinBox::textChanged, this, &ProjectSettingsEditor::markEdited);
    for (auto spinBox : ui->centralwidget->findChildren<UIntSpinBox *>())
        connect(spinBox, &UIntSpinBox::textChanged, this, &ProjectSettingsEditor::markEdited);
}

void ProjectSettingsEditor::markEdited() {
    // Don't treat signals emitted while the UI is refreshing as edits
    if (!this->refreshing)
        this->hasUnsavedChanges = true;
}

void ProjectSettingsEditor::initUi() {
    // Populate combo boxes
    if (project) {
        ui->comboBox_DefaultPrimaryTileset->addItems(project->primaryTilesetLabels);
        ui->comboBox_DefaultSecondaryTileset->addItems(project->secondaryTilesetLabels);
        ui->comboBox_IconSpecies->addItems(project->speciesToIconPath.keys());
        ui->comboBox_WarpBehaviors->addItems(project->metatileBehaviorMap.keys());
    }
    ui->comboBox_BaseGameVersion->addItems(ProjectConfig::versionStrings);
    ui->comboBox_AttributesSize->addItems({"1", "2", "4"});

    // Validate that the border metatiles text is a comma-separated list of metatile values
    static const QString regex_Hex = "(0[xX])?[A-Fa-f0-9]+";
    static const QRegularExpression expression_HexList(QString("^(%1,)*%1$").arg(regex_Hex)); // Comma-separated list of hex values
    QRegularExpressionValidator *validator_HexList = new QRegularExpressionValidator(expression_HexList);
    ui->lineEdit_BorderMetatiles->setValidator(validator_HexList);
    this->setBorderMetatilesUi(projectConfig.getUseCustomBorderSize());

    // Validate that the text added to the warp behavior list could be a valid define
    // (we don't care whether it actually is a metatile behavior define)
    static const QRegularExpression expression_Word("^[A-Za-z0-9_]*$");
    QRegularExpressionValidator *validator_Word = new QRegularExpressionValidator(expression_Word);
    ui->comboBox_WarpBehaviors->setValidator(validator_Word);
    ui->textEdit_WarpBehaviors->setTextColor(Qt::gray);

    // Set spin box limits
    uint16_t maxMetatileId = Block::getMaxMetatileId();
    ui->spinBox_FillMetatile->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile1->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile2->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile3->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile4->setMaximum(maxMetatileId);
    ui->spinBox_Elevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_Collision->setMaximum(Block::getMaxCollision());
    ui->spinBox_MaxElevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_MaxCollision->setMaximum(Block::getMaxCollision());
    ui->spinBox_MetatileIdMask->setMaximum(Block::maxValue);
    ui->spinBox_CollisionMask->setMaximum(Block::maxValue);
    ui->spinBox_ElevationMask->setMaximum(Block::maxValue);

    // Some settings can be determined by constants in the project.
    // We reflect that here by disabling their UI elements.
    if (project) {
        const QString maskFilepath = projectConfig.getFilePath(ProjectFilePath::global_fieldmap);
        const QString attrTableFilepath = projectConfig.getFilePath(ProjectFilePath::fieldmap);
        const QString metatileIdMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_metatile);
        const QString collisionMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_collision);
        const QString elevationMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_elevation);
        const QString behaviorMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_behavior);
        const QString layerTypeMaskName = projectConfig.getIdentifier(ProjectIdentifier::define_mask_layer);
        const QString behaviorTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_behavior);
        const QString layerTypeTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_layer);
        const QString encounterTypeTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_encounter);
        const QString terrainTypeTableName = projectConfig.getIdentifier(ProjectIdentifier::define_attribute_terrain);
        const QString attrTableName = projectConfig.getIdentifier(ProjectIdentifier::symbol_attribute_table);

        // Block masks
        if (project->disabledSettingsNames.contains(metatileIdMaskName))
            this->disableParsedSetting(ui->spinBox_MetatileIdMask, metatileIdMaskName, maskFilepath);
        if (project->disabledSettingsNames.contains(collisionMaskName))
            this->disableParsedSetting(ui->spinBox_CollisionMask, collisionMaskName, maskFilepath);
        if (project->disabledSettingsNames.contains(elevationMaskName))
            this->disableParsedSetting(ui->spinBox_ElevationMask, elevationMaskName, maskFilepath);

        // Behavior mask
        if (project->disabledSettingsNames.contains(behaviorMaskName))
            this->disableParsedSetting(ui->spinBox_BehaviorMask, behaviorMaskName, maskFilepath);
        else if (project->disabledSettingsNames.contains(behaviorTableName))
            this->disableParsedSetting(ui->spinBox_BehaviorMask, attrTableName, attrTableFilepath);

        // Layer type mask
        if (project->disabledSettingsNames.contains(layerTypeMaskName))
            this->disableParsedSetting(ui->spinBox_LayerTypeMask, layerTypeMaskName, maskFilepath);
        else if (project->disabledSettingsNames.contains(layerTypeTableName))
            this->disableParsedSetting(ui->spinBox_LayerTypeMask, attrTableName, attrTableFilepath);

        // Encounter and terrain type masks
        if (project->disabledSettingsNames.contains(encounterTypeTableName))
            this->disableParsedSetting(ui->spinBox_EncounterTypeMask, attrTableName, attrTableFilepath);
        if (project->disabledSettingsNames.contains(terrainTypeTableName))
            this->disableParsedSetting(ui->spinBox_TerrainTypeMask, attrTableName, attrTableFilepath);
    }
}

void ProjectSettingsEditor::disableParsedSetting(QWidget * widget, const QString &name, const QString &filepath) {
    widget->setEnabled(false);
    widget->setToolTip(QString("This value has been read from '%1' in %2").arg(name).arg(filepath));
}

// Remember the current settings tab for future sessions
void ProjectSettingsEditor::on_mainTabs_tabBarClicked(int index) {
    porymapConfig.setProjectSettingsTab(index);
}

void ProjectSettingsEditor::setTab(int index) {
    ui->mainTabs->setCurrentIndex(index);
    porymapConfig.setProjectSettingsTab(index);
}

void ProjectSettingsEditor::setBorderMetatilesUi(bool customSize) {
    ui->stackedWidget_BorderMetatiles->setCurrentIndex(customSize ? 0 : 1);
}

void ProjectSettingsEditor::setBorderMetatileIds(bool customSize, QList<uint16_t> metatileIds) {
    if (customSize) {
        ui->lineEdit_BorderMetatiles->setText(Metatile::getMetatileIdStrings(metatileIds));
    } else {
        ui->spinBox_BorderMetatile1->setValue(metatileIds.value(0, 0x0));
        ui->spinBox_BorderMetatile2->setValue(metatileIds.value(1, 0x0));
        ui->spinBox_BorderMetatile3->setValue(metatileIds.value(2, 0x0));
        ui->spinBox_BorderMetatile4->setValue(metatileIds.value(3, 0x0));
    }
}

QList<uint16_t> ProjectSettingsEditor::getBorderMetatileIds(bool customSize) {
    QList<uint16_t> metatileIds;
    if (customSize) {
        // Custom border size, read metatiles from line edit
        for (auto s : ui->lineEdit_BorderMetatiles->text().split(",")) {
            uint16_t metatileId = s.toUInt(nullptr, 0);
            metatileIds.append(qMin(metatileId, static_cast<uint16_t>(Project::getNumMetatilesTotal() - 1)));
        }
    } else {
        // Default border size, read metatiles from spin boxes
        metatileIds.append(ui->spinBox_BorderMetatile1->value());
        metatileIds.append(ui->spinBox_BorderMetatile2->value());
        metatileIds.append(ui->spinBox_BorderMetatile3->value());
        metatileIds.append(ui->spinBox_BorderMetatile4->value());
    }
    return metatileIds;
}

// Show/hide warning for overlapping mask values. These are technically ok, but probably not intended.
// Additionally, Porymap will not properly reflect that the values are linked.
void ProjectSettingsEditor::updateMaskOverlapWarning(QLabel * warning, QList<UIntSpinBox*> masks) {
    // Find any overlapping masks
    QMap<int, bool> overlapping;
    for (int i = 0; i < masks.length(); i++)
    for (int j = i + 1; j < masks.length(); j++) {
        if (masks.at(i)->value() & masks.at(j)->value())
            overlapping[i] = overlapping[j] = true;
    }

    // It'de nice if we could style this as a persistent red border around the line edit for any
    // overlapping masks. As it is editing the border undesirably modifies the arrow buttons.
    // This stylesheet will just highlight the currently selected line edit, which is fine enough.
    static const QString styleSheet = "QAbstractSpinBox { selection-background-color: rgba(255, 0, 0, 25%) }";

    // Update warning display
    if (warning) warning->setHidden(overlapping.isEmpty());
    for (int i = 0; i < masks.length(); i++)
        masks.at(i)->setStyleSheet(overlapping.contains(i) ? styleSheet : "");
}

void ProjectSettingsEditor::updateBlockMaskOverlapWarning() {
    const auto masks = QList<UIntSpinBox*>{
        ui->spinBox_MetatileIdMask,
        ui->spinBox_CollisionMask,
        ui->spinBox_ElevationMask,
    };
    this->updateMaskOverlapWarning(ui->label_OverlapWarningBlocks, masks);
}

void ProjectSettingsEditor::updateAttributeMaskOverlapWarning() {
    const auto masks = QList<UIntSpinBox*>{
        ui->spinBox_BehaviorMask,
        ui->spinBox_LayerTypeMask,
        ui->spinBox_EncounterTypeMask,
        ui->spinBox_TerrainTypeMask,
    };
    this->updateMaskOverlapWarning(ui->label_OverlapWarningMetatiles, masks);
}

void ProjectSettingsEditor::updateAttributeLimits(const QString &attrSize) {
    static const QMap<QString, uint32_t> limits {
        {"1", 0xFF},
        {"2", 0xFFFF},
        {"4", 0xFFFFFFFF},
    };
    uint32_t max = limits.value(attrSize, UINT_MAX);
    ui->spinBox_BehaviorMask->setMaximum(max);
    ui->spinBox_EncounterTypeMask->setMaximum(max);
    ui->spinBox_LayerTypeMask->setMaximum(max);
    ui->spinBox_TerrainTypeMask->setMaximum(max);
}

// Only one icon path is displayed at a time, so we need to keep track of the rest,
// and update the path edit when the user changes the selected species.
// The existing icon path map in ProjectConfig is left alone to allow unsaved changes.
void ProjectSettingsEditor::updatePokemonIconPath(const QString &newSpecies) {
    if (!project) return;

    // If user was editing a path for a valid species, record filepath text before we wipe it.
    if (!this->prevIconSpecies.isEmpty() && this->project->speciesToIconPath.contains(this->prevIconSpecies))
        this->editedPokemonIconPaths[this->prevIconSpecies] = ui->lineEdit_PokemonIcon->text();

    QString editedPath = this->editedPokemonIconPaths.value(newSpecies);
    QString defaultPath = this->project->speciesToIconPath.value(newSpecies);

    ui->lineEdit_PokemonIcon->setText(this->stripProjectDir(editedPath));
    ui->lineEdit_PokemonIcon->setPlaceholderText(this->stripProjectDir(defaultPath));
    this->prevIconSpecies = newSpecies;
}

QStringList ProjectSettingsEditor::getWarpBehaviorsList() {
    return ui->textEdit_WarpBehaviors->toPlainText().split("\n");
}

void ProjectSettingsEditor::setWarpBehaviorsList(QStringList list) {
    list.removeDuplicates();
    list.sort();
    ui->textEdit_WarpBehaviors->setText(list.join("\n"));
}

void ProjectSettingsEditor::updateWarpBehaviorsList(bool adding) {
    QString input = ui->comboBox_WarpBehaviors->currentText();
    if (input.isEmpty())
        return;

    // Check if input was a value string for a named behavior
    bool ok;
    uint32_t value = input.toUInt(&ok, 0);
    if (ok && project->metatileBehaviorMapInverse.contains(value))
        input = project->metatileBehaviorMapInverse.value(value);

    if (!project->metatileBehaviorMap.contains(input))
        return;

    QStringList list = this->getWarpBehaviorsList();
    int pos = list.indexOf(input);

    if (adding && pos < 0) {
        // Add text to list
        list.prepend(input);
    } else if (!adding && pos >= 0) {
        // Remove text from list
        list.removeAt(pos);
    } else {
        // Trying to add text already in list,
        // or trying to remove text not in list
        return;
    }

    this->setWarpBehaviorsList(list);
    this->hasUnsavedChanges = true;
}

// Dynamically populate the tabs for project files and identifiers
void ProjectSettingsEditor::createConfigTextTable(const QList<QPair<QString, QString>> configPairs, bool filesTab) {
    for (auto pair : configPairs) {
        const QString idName = pair.first;
        const QString defaultText = pair.second;

        auto name = new QLabel();
        name->setAlignment(Qt::AlignBottom);
        name->setText(idName);

        auto lineEdit = new QLineEdit();
        lineEdit->setObjectName(idName); // Used when saving
        lineEdit->setPlaceholderText(defaultText);
        lineEdit->setClearButtonEnabled(true);

        // Add to list
        auto editArea = new QWidget();
        auto layout = new QHBoxLayout(editArea);
        layout->addWidget(lineEdit);

        if (filesTab) {
            // "Choose file" button
            auto button = new QToolButton();
            button->setIcon(QIcon(":/icons/folder.ico"));
            connect(button, &QAbstractButton::clicked, [this, lineEdit](bool) {
                const QString path = this->chooseProjectFile(lineEdit->placeholderText());
                if (!path.isEmpty()) {
                    lineEdit->setText(path);
                    this->markEdited();
                }
            });
            layout->addWidget(button);

            ui->layout_ProjectPaths->addRow(name, editArea);
        } else {
            ui->layout_Identifiers->addRow(name, editArea);
        }
    }
}

void ProjectSettingsEditor::createProjectPathsTable() {
    auto pairs = ProjectConfig::defaultPaths.values();
    this->createConfigTextTable(pairs, true);
}

void ProjectSettingsEditor::createProjectIdentifiersTable() {
    auto pairs = ProjectConfig::defaultIdentifiers.values();
    this->createConfigTextTable(pairs, false);
}

QString ProjectSettingsEditor::chooseProjectFile(const QString &defaultFilepath) {
    const QString startDir = this->baseDir + defaultFilepath;

    QString path;
    if (defaultFilepath.endsWith("/")){
        // Default filepath is a folder, choose a new folder
        path = QFileDialog::getExistingDirectory(this, "Choose Project File Folder", startDir) + QDir::separator();
    } else{
        // Default filepath is not a folder, choose a new file
        path = QFileDialog::getOpenFileName(this, "Choose Project File", startDir);
    }

    if (!path.startsWith(this->baseDir)){
        // Most of Porymap's file-parsing code for project files will assume that filepaths
        // are relative to the root project folder, so we enforce that here.
        QMessageBox::warning(this, "Failed to set custom filepath",
                           QString("Custom filepaths must be inside the root project folder '%1'").arg(this->baseDir));
        return QString();
    }
    return path.remove(0, this->baseDir.length());
}

void ProjectSettingsEditor::restoreWindowState() {
    logInfo("Restoring project settings editor geometry from previous session.");
    const QMap<QString, QByteArray> geometry = porymapConfig.getProjectSettingsEditorGeometry();
    this->restoreGeometry(geometry.value("project_settings_editor_geometry"));
    this->restoreState(geometry.value("project_settings_editor_state"));
}

// Set UI states using config data
void ProjectSettingsEditor::refresh() {
    this->refreshing = true; // Block signals

    // Set combo box texts
    ui->comboBox_DefaultPrimaryTileset->setTextItem(projectConfig.getDefaultPrimaryTileset());
    ui->comboBox_DefaultSecondaryTileset->setTextItem(projectConfig.getDefaultSecondaryTileset());
    ui->comboBox_BaseGameVersion->setTextItem(projectConfig.getBaseGameVersionString());
    ui->comboBox_AttributesSize->setTextItem(QString::number(projectConfig.getMetatileAttributesSize()));
    this->updateAttributeLimits(ui->comboBox_AttributesSize->currentText());

    this->prevIconSpecies = QString();
    this->editedPokemonIconPaths = projectConfig.getPokemonIconPaths();
    this->updatePokemonIconPath(ui->comboBox_IconSpecies->currentText());

    // Set check box states
    ui->checkBox_UsePoryscript->setChecked(projectConfig.getUsePoryScript());
    ui->checkBox_ShowWildEncounterTables->setChecked(userConfig.getEncounterJsonActive());
    ui->checkBox_CreateTextFile->setChecked(projectConfig.getCreateMapTextFileEnabled());
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
    ui->checkBox_DisableWarning->setChecked(porymapConfig.getWarpBehaviorWarningDisabled());

    // Set spin box values
    ui->spinBox_Elevation->setValue(projectConfig.getDefaultElevation());
    ui->spinBox_Collision->setValue(projectConfig.getDefaultCollision());
    ui->spinBox_FillMetatile->setValue(projectConfig.getDefaultMetatileId());
    ui->spinBox_MaxElevation->setValue(projectConfig.getCollisionSheetHeight() - 1);
    ui->spinBox_MaxCollision->setValue(projectConfig.getCollisionSheetWidth() - 1);
    ui->spinBox_BehaviorMask->setValue(projectConfig.getMetatileBehaviorMask() & ui->spinBox_BehaviorMask->maximum());
    ui->spinBox_EncounterTypeMask->setValue(projectConfig.getMetatileEncounterTypeMask() & ui->spinBox_EncounterTypeMask->maximum());
    ui->spinBox_LayerTypeMask->setValue(projectConfig.getMetatileLayerTypeMask() & ui->spinBox_LayerTypeMask->maximum());
    ui->spinBox_TerrainTypeMask->setValue(projectConfig.getMetatileTerrainTypeMask() & ui->spinBox_TerrainTypeMask->maximum());
    ui->spinBox_MetatileIdMask->setValue(projectConfig.getBlockMetatileIdMask() & ui->spinBox_MetatileIdMask->maximum());
    ui->spinBox_CollisionMask->setValue(projectConfig.getBlockCollisionMask() & ui->spinBox_CollisionMask->maximum());
    ui->spinBox_ElevationMask->setValue(projectConfig.getBlockElevationMask() & ui->spinBox_ElevationMask->maximum());

    // Set (and sync) border metatile IDs
    auto metatileIds = projectConfig.getNewMapBorderMetatileIds();
    this->setBorderMetatileIds(false, metatileIds);
    this->setBorderMetatileIds(true, metatileIds);

    // Set line edit texts
    ui->lineEdit_PrefabsPath->setText(projectConfig.getPrefabFilepath());
    ui->lineEdit_CollisionGraphics->setText(projectConfig.getCollisionSheetPath());
    ui->lineEdit_ObjectsIcon->setText(projectConfig.getEventIconPath(Event::Group::Object));
    ui->lineEdit_WarpsIcon->setText(projectConfig.getEventIconPath(Event::Group::Warp));
    ui->lineEdit_TriggersIcon->setText(projectConfig.getEventIconPath(Event::Group::Coord));
    ui->lineEdit_BGsIcon->setText(projectConfig.getEventIconPath(Event::Group::Bg));
    ui->lineEdit_HealspotsIcon->setText(projectConfig.getEventIconPath(Event::Group::Heal));
    for (auto lineEdit : ui->scrollAreaContents_ProjectPaths->findChildren<QLineEdit*>())
        lineEdit->setText(projectConfig.getCustomFilePath(lineEdit->objectName()));
    for (auto lineEdit : ui->scrollAreaContents_Identifiers->findChildren<QLineEdit*>())
        lineEdit->setText(projectConfig.getCustomIdentifier(lineEdit->objectName()));

    // Set warp behaviors
    auto behaviorValues = projectConfig.getWarpBehaviors();
    QStringList behaviorNames;
    for (auto value : behaviorValues) {
        if (project->metatileBehaviorMapInverse.contains(value))
            behaviorNames.append(project->metatileBehaviorMapInverse.value(value));
    }
    this->setWarpBehaviorsList(behaviorNames);

    this->refreshing = false; // Allow signals
}

void ProjectSettingsEditor::save() {
    if (!this->hasUnsavedChanges)
        return;

    // Prevent a call to save() for each of the config settings
    projectConfig.setSaveDisabled(true);

    // Save combo box settings
    projectConfig.setDefaultPrimaryTileset(ui->comboBox_DefaultPrimaryTileset->currentText());
    projectConfig.setDefaultSecondaryTileset(ui->comboBox_DefaultSecondaryTileset->currentText());
    projectConfig.setBaseGameVersion(projectConfig.stringToBaseGameVersion(ui->comboBox_BaseGameVersion->currentText()));
    projectConfig.setMetatileAttributesSize(ui->comboBox_AttributesSize->currentText().toInt());

    // Save check box settings
    projectConfig.setUsePoryScript(ui->checkBox_UsePoryscript->isChecked());
    userConfig.setEncounterJsonActive(ui->checkBox_ShowWildEncounterTables->isChecked());
    projectConfig.setCreateMapTextFileEnabled(ui->checkBox_CreateTextFile->isChecked());
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
    porymapConfig.setWarpBehaviorWarningDisabled(ui->checkBox_DisableWarning->isChecked());

    // Save spin box settings
    projectConfig.setDefaultElevation(ui->spinBox_Elevation->value());
    projectConfig.setDefaultCollision(ui->spinBox_Collision->value());
    projectConfig.setDefaultMetatileId(ui->spinBox_FillMetatile->value());
    projectConfig.setCollisionSheetHeight(ui->spinBox_MaxElevation->value() + 1);
    projectConfig.setCollisionSheetWidth(ui->spinBox_MaxCollision->value() + 1);
    projectConfig.setMetatileBehaviorMask(ui->spinBox_BehaviorMask->value());
    projectConfig.setMetatileTerrainTypeMask(ui->spinBox_TerrainTypeMask->value());
    projectConfig.setMetatileEncounterTypeMask(ui->spinBox_EncounterTypeMask->value());
    projectConfig.setMetatileLayerTypeMask(ui->spinBox_LayerTypeMask->value());
    projectConfig.setBlockMetatileIdMask(ui->spinBox_MetatileIdMask->value());
    projectConfig.setBlockCollisionMask(ui->spinBox_CollisionMask->value());
    projectConfig.setBlockElevationMask(ui->spinBox_ElevationMask->value());

    // Save line edit settings
    projectConfig.setPrefabFilepath(ui->lineEdit_PrefabsPath->text());
    projectConfig.setCollisionSheetPath(ui->lineEdit_CollisionGraphics->text());
    projectConfig.setEventIconPath(Event::Group::Object, ui->lineEdit_ObjectsIcon->text());
    projectConfig.setEventIconPath(Event::Group::Warp, ui->lineEdit_WarpsIcon->text());
    projectConfig.setEventIconPath(Event::Group::Coord, ui->lineEdit_TriggersIcon->text());
    projectConfig.setEventIconPath(Event::Group::Bg, ui->lineEdit_BGsIcon->text());
    projectConfig.setEventIconPath(Event::Group::Heal, ui->lineEdit_HealspotsIcon->text());
    for (auto lineEdit : ui->scrollAreaContents_ProjectPaths->findChildren<QLineEdit*>())
        projectConfig.setFilePath(lineEdit->objectName(), lineEdit->text());
    for (auto lineEdit : ui->scrollAreaContents_Identifiers->findChildren<QLineEdit*>())
        projectConfig.setIdentifier(lineEdit->objectName(), lineEdit->text());

    // Save warp behaviors
    QStringList behaviorNames = this->getWarpBehaviorsList();
    QSet<uint32_t> behaviorValues;
    for (auto name : behaviorNames)
        behaviorValues.insert(project->metatileBehaviorMap.value(name));
    projectConfig.setWarpBehaviors(behaviorValues);

    // Save border metatile IDs
    projectConfig.setNewMapBorderMetatileIds(this->getBorderMetatileIds(ui->checkBox_EnableCustomBorderSize->isChecked()));

    // Save pokemon icon paths
    const QString species = ui->comboBox_IconSpecies->currentText();
    if (this->project->speciesToIconPath.contains(species))
        this->editedPokemonIconPaths.insert(species, ui->lineEdit_PokemonIcon->text());
    for (auto i = this->editedPokemonIconPaths.cbegin(), end = this->editedPokemonIconPaths.cend(); i != end; i++)
        projectConfig.setPokemonIconPath(i.key(), i.value());

    projectConfig.setSaveDisabled(false);
    projectConfig.save();
    this->hasUnsavedChanges = false;

    // Technically, a reload is not required for several of the config settings.
    // For simplicity we prompt the user to reload when any setting is changed regardless.
    this->projectNeedsReload = true;
}

// Pick a file to use as the new prefabs file path
void ProjectSettingsEditor::choosePrefabsFile() {
    this->chooseFile(ui->lineEdit_PrefabsPath, "Choose Prefabs File", "JSON Files (*.json)");
}

void ProjectSettingsEditor::chooseImageFile(QLineEdit * filepathEdit) {
    this->chooseFile(filepathEdit, "Choose Image File", "Images (*.png *.jpg)");
}

void ProjectSettingsEditor::chooseFile(QLineEdit * filepathEdit, const QString &description, const QString &extensions) {
    QString filepath = QFileDialog::getOpenFileName(this, description, this->project->importExportPath, extensions);
    if (filepath.isEmpty())
        return;
    this->project->setImportExportPath(filepath);

    if (filepathEdit)
        filepathEdit->setText(this->stripProjectDir(filepath));
    this->hasUnsavedChanges = true;
}

// Display relative path if this file is in the project folder
QString ProjectSettingsEditor::stripProjectDir(QString s) {
    if (s.startsWith(this->baseDir))
        s.remove(0, this->baseDir.length());
    return s;
}

void ProjectSettingsEditor::importDefaultPrefabsClicked(bool) {
    // If the prompt is accepted the prefabs file will be created and its filepath will be saved in the config.
    BaseGameVersion version = projectConfig.stringToBaseGameVersion(ui->comboBox_BaseGameVersion->currentText());
    if (prefab.tryImportDefaultPrefabs(this, version, ui->lineEdit_PrefabsPath->text())) {
        ui->lineEdit_PrefabsPath->setText(projectConfig.getPrefabFilepath()); // Refresh with new filepath
        this->hasUnsavedChanges = true;
    }
}

int ProjectSettingsEditor::prompt(const QString &text, QMessageBox::StandardButton defaultButton) {
    QMessageBox messageBox(this);
    messageBox.setText(text);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No | defaultButton);
    messageBox.setDefaultButton(defaultButton);
    return messageBox.exec();
}

bool ProjectSettingsEditor::promptSaveChanges() {
    if (!this->hasUnsavedChanges)
        return true;

    int result = this->prompt("Settings have been modified, save changes?", QMessageBox::Cancel);
    if (result == QMessageBox::Cancel)
        return false;

    if (result == QMessageBox::Yes)
        this->save();
    else if (result == QMessageBox::No)
        this->hasUnsavedChanges = false; // Discarding changes

    return true;
}

bool ProjectSettingsEditor::promptRestoreDefaults() {
    if (this->refreshing)
        return false;

    const QString versionText = ui->comboBox_BaseGameVersion->currentText();
    if (this->prompt(QString("Restore default config settings for %1?").arg(versionText)) == QMessageBox::No)
        return false;

    // Restore defaults by resetting config in memory, refreshing the UI, then restoring the config.
    // Don't want to save changes until user accepts them.
    ProjectConfig tempProject = projectConfig;
    projectConfig.reset(projectConfig.stringToBaseGameVersion(versionText));
    this->refresh();
    projectConfig = tempProject;

    this->hasUnsavedChanges = true;
    return true;
}

void ProjectSettingsEditor::dialogButtonClicked(QAbstractButton *button) {
    auto buttonRole = ui->buttonBox->buttonRole(button);
    if (buttonRole == QDialogButtonBox::AcceptRole) {
        // "OK" button
        this->save();
        this->close();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        // "Cancel" button
        this->close();
    } else if (buttonRole == QDialogButtonBox::ResetRole) {
        // "Restore Defaults" button
        this->promptRestoreDefaults();
    }
}

// Close event triggered by a project reload. User doesn't need any prompts, just close the window.
void ProjectSettingsEditor::closeQuietly() {
    // Turn off flags that trigger prompts
    this->hasUnsavedChanges = false;
    this->projectNeedsReload = false;
    this->close();
}

void ProjectSettingsEditor::closeEvent(QCloseEvent* event) {
    if (!this->promptSaveChanges()) {
        event->ignore();
        return;
    }

    porymapConfig.setProjectSettingsEditorGeometry(
        this->saveGeometry(),
        this->saveState()
    );

    if (this->projectNeedsReload) {
        // Note: Declining this prompt with changes that need a reload may cause problems
        if (this->prompt("Settings saved, reload project to apply changes?") == QMessageBox::Yes)
            emit this->reloadProject();
    }
    QMainWindow::closeEvent(event);
}
