#include "projectsettingseditor.h"
#include "config.h"
#include "noscrollcombobox.h"
#include "prefab.h"
#include "filedialog.h"
#include "newdefinedialog.h"
#include "utility.h"

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
    baseDir(projectConfig.projectDir() + "/")
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
    connect(ui->button_HelpFiles, &QAbstractButton::clicked, this, &ProjectSettingsEditor::openFilesHelp);
    connect(ui->button_HelpIdentifiers, &QAbstractButton::clicked, this, &ProjectSettingsEditor::openIdentifiersHelp);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &ProjectSettingsEditor::dialogButtonClicked);
    connect(ui->button_ImportDefaultPrefabs, &QAbstractButton::clicked, this, &ProjectSettingsEditor::importDefaultPrefabsClicked);
    connect(ui->comboBox_BaseGameVersion, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::promptRestoreDefaults);
    connect(ui->comboBox_AttributesSize, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::updateAttributeLimits);
    connect(ui->comboBox_IconSpecies, &QComboBox::currentTextChanged, this, &ProjectSettingsEditor::updatePokemonIconPath);
    connect(ui->checkBox_EnableCustomBorderSize, &QCheckBox::toggled, [this](bool enabled) {
        // When switching between the spin boxes or line edit for border metatiles we set
        // the newly-shown UI using the values from the hidden UI.
        this->setBorderMetatileIds(enabled, this->getBorderMetatileIds(!enabled));
        this->setBorderMetatilesUi(enabled);
    });
    connect(ui->button_AddWarpBehavior,    &QAbstractButton::clicked, [this](bool) { this->updateWarpBehaviorsList(true); });
    connect(ui->button_RemoveWarpBehavior, &QAbstractButton::clicked, [this](bool) { this->updateWarpBehaviorsList(false); });

    connect(ui->button_AddGlobalConstantsFile, &QAbstractButton::clicked, this, &ProjectSettingsEditor::addNewGlobalConstantsFilepath);
    connect(ui->button_AddGlobalConstant,      &QAbstractButton::clicked, this, &ProjectSettingsEditor::addNewGlobalConstant);

    // Connect file selection buttons
    connect(ui->button_ChoosePrefabs,     &QAbstractButton::clicked, [this](bool) { this->choosePrefabsFile(); });
    connect(ui->button_CollisionGraphics, &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_CollisionGraphics); });
    connect(ui->button_ObjectsIcon,       &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_ObjectsIcon); });
    connect(ui->button_WarpsIcon,         &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_WarpsIcon); });
    connect(ui->button_TriggersIcon,      &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_TriggersIcon); });
    connect(ui->button_BGsIcon,           &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_BGsIcon); });
    connect(ui->button_HealLocationsIcon, &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_HealLocationsIcon); });
    connect(ui->button_PokemonIcon,       &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_PokemonIcon); });
    connect(ui->button_EventsTabIcon,     &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_EventsTabIcon); });


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
        connect(checkBox, &QCheckBox::toggled, this, &ProjectSettingsEditor::markEdited);
    for (auto radioButton : ui->centralwidget->findChildren<QRadioButton *>())
        connect(radioButton, &QRadioButton::toggled, this, &ProjectSettingsEditor::markEdited);
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
        ui->comboBox_IconSpecies->addItems(project->speciesNames);
        ui->comboBox_WarpBehaviors->addItems(project->metatileBehaviorMap.keys());
    }
    ui->comboBox_BaseGameVersion->addItems(ProjectConfig::versionStrings);
    ui->comboBox_AttributesSize->addItems({"1", "2", "4"});

    ui->comboBox_EventsTabIcon->addItem("Automatic",         "");
    ui->comboBox_EventsTabIcon->addItem("Brendan (Emerald)", ProjectConfig::getPlayerIconPath(BaseGameVersion::pokeemerald, 0));
    ui->comboBox_EventsTabIcon->addItem("Brendan (R/S)",     ProjectConfig::getPlayerIconPath(BaseGameVersion::pokeruby,    0));
    ui->comboBox_EventsTabIcon->addItem("May (Emerald)",     ProjectConfig::getPlayerIconPath(BaseGameVersion::pokeemerald, 1));
    ui->comboBox_EventsTabIcon->addItem("May (R/S)",         ProjectConfig::getPlayerIconPath(BaseGameVersion::pokeruby,    1));
    ui->comboBox_EventsTabIcon->addItem("Red",               ProjectConfig::getPlayerIconPath(BaseGameVersion::pokefirered, 0));
    ui->comboBox_EventsTabIcon->addItem("Green",             ProjectConfig::getPlayerIconPath(BaseGameVersion::pokefirered, 1));
    ui->comboBox_EventsTabIcon->addItem("Custom",            "Custom");
    connect(ui->comboBox_EventsTabIcon, QOverload<int>::of(&NoScrollComboBox::currentIndexChanged), [this](int index) {
        bool usingCustom = (index == ui->comboBox_EventsTabIcon->findText("Custom"));
        ui->lineEdit_EventsTabIcon->setVisible(usingCustom);
        ui->button_EventsTabIcon->setVisible(usingCustom);
    });

    // Validate that the border metatiles text is a comma-separated list of metatile values
    static const QString regex_Hex = "(0[xX])?[A-Fa-f0-9]+";
    static const QRegularExpression expression_HexList(QString("^(%1,)*%1$").arg(regex_Hex)); // Comma-separated list of hex values
    QRegularExpressionValidator *validator_HexList = new QRegularExpressionValidator(expression_HexList, this);
    ui->lineEdit_BorderMetatiles->setValidator(validator_HexList);
    this->setBorderMetatilesUi(projectConfig.useCustomBorderSize);

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
    ui->spinBox_UnusedTileNormal->setMaximum(Tile::maxValue);
    ui->spinBox_UnusedTileCovered->setMaximum(Tile::maxValue);
    ui->spinBox_UnusedTileSplit->setMaximum(Tile::maxValue);
    ui->spinBox_MaxEvents->setMaximum(INT_MAX);
    ui->spinBox_MapWidth->setMaximum(INT_MAX);
    ui->spinBox_MapHeight->setMaximum(INT_MAX);
    ui->spinBox_PlayerViewDistance_West->setMaximum(INT_MAX);
    ui->spinBox_PlayerViewDistance_North->setMaximum(INT_MAX);
    ui->spinBox_PlayerViewDistance_East->setMaximum(INT_MAX);
    ui->spinBox_PlayerViewDistance_South->setMaximum(INT_MAX);

    // The values for some of the settings we provide in this window can be determined using constants in the user's projects.
    // If the user has these constants we disable these settings in the UI -- they can modify them using their constants.
    const QString globalFieldmapPath = projectConfig.getFilePath(ProjectFilePath::global_fieldmap);
    const QString constantsFieldmapPath = projectConfig.getFilePath(ProjectFilePath::constants_fieldmap);
    const QString fieldmapPath = projectConfig.getFilePath(ProjectFilePath::fieldmap);

    // Block masks
    this->disableParsedSetting(ui->spinBox_MetatileIdMask, projectConfig.getIdentifier(ProjectIdentifier::define_mask_metatile), globalFieldmapPath);
    this->disableParsedSetting(ui->spinBox_CollisionMask, projectConfig.getIdentifier(ProjectIdentifier::define_mask_collision), globalFieldmapPath);
    this->disableParsedSetting(ui->spinBox_ElevationMask, projectConfig.getIdentifier(ProjectIdentifier::define_mask_elevation), globalFieldmapPath);

    // Behavior mask
    if (!this->disableParsedSetting(ui->spinBox_BehaviorMask, projectConfig.getIdentifier(ProjectIdentifier::define_mask_behavior), globalFieldmapPath))
        this->disableParsedSetting(ui->spinBox_BehaviorMask, projectConfig.getIdentifier(ProjectIdentifier::define_attribute_behavior), fieldmapPath);

    // Layer type mask
    if (!this->disableParsedSetting(ui->spinBox_LayerTypeMask, projectConfig.getIdentifier(ProjectIdentifier::define_mask_layer), globalFieldmapPath))
        this->disableParsedSetting(ui->spinBox_LayerTypeMask, projectConfig.getIdentifier(ProjectIdentifier::define_attribute_layer), fieldmapPath);

    // Encounter and terrain type masks
    this->disableParsedSetting(ui->spinBox_EncounterTypeMask, projectConfig.getIdentifier(ProjectIdentifier::define_attribute_encounter), fieldmapPath);
    this->disableParsedSetting(ui->spinBox_TerrainTypeMask, projectConfig.getIdentifier(ProjectIdentifier::define_attribute_terrain), fieldmapPath);

    // Tripe layer metatiles
    this->disableParsedSetting(ui->checkBox_EnableTripleLayerMetatiles, projectConfig.getIdentifier(ProjectIdentifier::define_tiles_per_metatile), constantsFieldmapPath);
}

bool ProjectSettingsEditor::disableParsedSetting(QWidget * widget, const QString &identifier, const QString &filepath) {
    if (project && project->disabledSettingsNames.contains(identifier)) {
        widget->setEnabled(false);
        QString toolTip = QString("This value has been set using '%1' in %2").arg(identifier).arg(filepath);
        if (!widget->toolTip().isEmpty())
            toolTip.prepend(QString("%1\n\n").arg(widget->toolTip()));
        widget->setToolTip(Util::toHtmlParagraph(toolTip));
        return true;
    }
    return false;
}

// Remember the current settings tab for future sessions
void ProjectSettingsEditor::on_mainTabs_tabBarClicked(int index) {
    porymapConfig.projectSettingsTab = index;
}

void ProjectSettingsEditor::setTab(int index) {
    ui->mainTabs->setCurrentIndex(index);
    porymapConfig.projectSettingsTab = index;
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
    if (!this->prevIconSpecies.isEmpty() && this->project->speciesNames.contains(this->prevIconSpecies))
        this->editedPokemonIconPaths[this->prevIconSpecies] = ui->lineEdit_PokemonIcon->text();

    QString editedPath = this->editedPokemonIconPaths.value(newSpecies);
    QString defaultPath = this->project->getDefaultSpeciesIconPath(newSpecies);

    ui->lineEdit_PokemonIcon->setText(this->stripProjectDir(editedPath));
    ui->lineEdit_PokemonIcon->setPlaceholderText(this->stripProjectDir(defaultPath));
    this->prevIconSpecies = newSpecies;
}

QStringList ProjectSettingsEditor::getWarpBehaviorsList() {
    return ui->textEdit_WarpBehaviors->toPlainText().split("\n");
}

void ProjectSettingsEditor::setWarpBehaviorsList(QStringList list) {
    list.removeDuplicates();
    Util::numericalModeSort(list);
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
        path = FileDialog::getExistingDirectory(this, "Choose Project File Folder", startDir) + "/";
    } else{
        // Default filepath is not a folder, choose a new file
        path = FileDialog::getOpenFileName(this, "Choose Project File", startDir);
    }
    if (path.isEmpty())
        return path;

    if (!path.startsWith(this->baseDir)){
        // Most of Porymap's file-parsing code for project files will assume that filepaths
        // are relative to the root project folder, so we enforce that here.
        QMessageBox::warning(this, QApplication::applicationName(),
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
    ui->comboBox_DefaultPrimaryTileset->setTextItem(projectConfig.defaultPrimaryTileset);
    ui->comboBox_DefaultSecondaryTileset->setTextItem(projectConfig.defaultSecondaryTileset);
    ui->comboBox_BaseGameVersion->setTextItem(projectConfig.getBaseGameVersionString());
    ui->comboBox_AttributesSize->setTextItem(QString::number(projectConfig.metatileAttributesSize));
    this->updateAttributeLimits(ui->comboBox_AttributesSize->currentText());

    this->prevIconSpecies = QString();
    this->editedPokemonIconPaths = projectConfig.getPokemonIconPaths();
    this->updatePokemonIconPath(ui->comboBox_IconSpecies->currentText());

    // Set check box states
    ui->checkBox_UsePoryscript->setChecked(projectConfig.usePoryScript);
    ui->checkBox_ShowWildEncounterTables->setChecked(userConfig.useEncounterJson);
    ui->checkBox_CreateTextFile->setChecked(projectConfig.createMapTextFileEnabled);
    ui->checkBox_EnableTripleLayerMetatiles->setChecked(projectConfig.tripleLayerMetatilesEnabled);
    ui->checkBox_EnableRequiresItemfinder->setChecked(projectConfig.hiddenItemRequiresItemfinderEnabled);
    ui->checkBox_EnableQuantity->setChecked(projectConfig.hiddenItemQuantityEnabled);
    ui->checkBox_EnableCloneObjects->setChecked(projectConfig.eventCloneObjectEnabled);
    ui->checkBox_EnableWeatherTriggers->setChecked(projectConfig.eventWeatherTriggerEnabled);
    ui->checkBox_EnableSecretBases->setChecked(projectConfig.eventSecretBaseEnabled);
    ui->checkBox_EnableRespawn->setChecked(projectConfig.healLocationRespawnDataEnabled);
    ui->checkBox_EnableAllowFlags->setChecked(projectConfig.mapAllowFlagsEnabled);
    ui->checkBox_EnableFloorNumber->setChecked(projectConfig.floorNumberEnabled);
    ui->checkBox_EnableCustomBorderSize->setChecked(projectConfig.useCustomBorderSize);
    ui->checkBox_OutputCallback->setChecked(projectConfig.tilesetsHaveCallback);
    ui->checkBox_OutputIsCompressed->setChecked(projectConfig.tilesetsHaveIsCompressed);
    ui->checkBox_DisableWarning->setChecked(porymapConfig.warpBehaviorWarningDisabled);
    ui->checkBox_PreserveMatchingOnlyData->setChecked(projectConfig.preserveMatchingOnlyData);

    // Radio buttons
    if (projectConfig.setTransparentPixelsBlack)
        ui->radioButton_RenderBlack->setChecked(true);
    else
        ui->radioButton_RenderFirstPalColor->setChecked(true);

    // Set spin box values
    ui->spinBox_Elevation->setValue(projectConfig.defaultElevation);
    ui->spinBox_Collision->setValue(projectConfig.defaultCollision);
    ui->spinBox_FillMetatile->setValue(projectConfig.defaultMetatileId);
    ui->spinBox_MapWidth->setValue(projectConfig.defaultMapSize.width());
    ui->spinBox_MapHeight->setValue(projectConfig.defaultMapSize.height());
    ui->spinBox_MaxElevation->setValue(projectConfig.collisionSheetSize.height() - 1);
    ui->spinBox_MaxCollision->setValue(projectConfig.collisionSheetSize.width() - 1);
    ui->spinBox_BehaviorMask->setValue(projectConfig.metatileBehaviorMask & ui->spinBox_BehaviorMask->maximum());
    ui->spinBox_EncounterTypeMask->setValue(projectConfig.metatileEncounterTypeMask & ui->spinBox_EncounterTypeMask->maximum());
    ui->spinBox_LayerTypeMask->setValue(projectConfig.metatileLayerTypeMask & ui->spinBox_LayerTypeMask->maximum());
    ui->spinBox_TerrainTypeMask->setValue(projectConfig.metatileTerrainTypeMask & ui->spinBox_TerrainTypeMask->maximum());
    ui->spinBox_MetatileIdMask->setValue(projectConfig.blockMetatileIdMask & ui->spinBox_MetatileIdMask->maximum());
    ui->spinBox_CollisionMask->setValue(projectConfig.blockCollisionMask & ui->spinBox_CollisionMask->maximum());
    ui->spinBox_ElevationMask->setValue(projectConfig.blockElevationMask & ui->spinBox_ElevationMask->maximum());
    ui->spinBox_UnusedTileNormal->setValue(projectConfig.unusedTileNormal);
    ui->spinBox_UnusedTileCovered->setValue(projectConfig.unusedTileCovered);
    ui->spinBox_UnusedTileSplit->setValue(projectConfig.unusedTileSplit);
    ui->spinBox_MaxEvents->setValue(projectConfig.maxEventsPerGroup);
    ui->spinBox_PlayerViewDistance_West->setValue(projectConfig.playerViewDistance.left());
    ui->spinBox_PlayerViewDistance_North->setValue(projectConfig.playerViewDistance.top());
    ui->spinBox_PlayerViewDistance_East->setValue(projectConfig.playerViewDistance.right());
    ui->spinBox_PlayerViewDistance_South->setValue(projectConfig.playerViewDistance.bottom());

    // Set (and sync) border metatile IDs
    this->setBorderMetatileIds(false, projectConfig.newMapBorderMetatileIds);
    this->setBorderMetatileIds(true, projectConfig.newMapBorderMetatileIds);

    // Set line edit texts
    ui->lineEdit_PrefabsPath->setText(projectConfig.prefabFilepath);
    ui->lineEdit_CollisionGraphics->setText(projectConfig.collisionSheetPath);
    ui->lineEdit_ObjectsIcon->setText(projectConfig.getEventIconPath(Event::Group::Object));
    ui->lineEdit_WarpsIcon->setText(projectConfig.getEventIconPath(Event::Group::Warp));
    ui->lineEdit_TriggersIcon->setText(projectConfig.getEventIconPath(Event::Group::Coord));
    ui->lineEdit_BGsIcon->setText(projectConfig.getEventIconPath(Event::Group::Bg));
    ui->lineEdit_HealLocationsIcon->setText(projectConfig.getEventIconPath(Event::Group::Heal));
    for (auto lineEdit : ui->scrollAreaContents_ProjectPaths->findChildren<QLineEdit*>())
        lineEdit->setText(projectConfig.getCustomFilePath(lineEdit->objectName()));
    for (auto lineEdit : ui->scrollAreaContents_Identifiers->findChildren<QLineEdit*>())
        lineEdit->setText(projectConfig.getCustomIdentifier(lineEdit->objectName()));
    for (const auto &path : projectConfig.globalConstantsFilepaths) {
        addGlobalConstantsFilepath(path);
    }
    for (auto it = projectConfig.globalConstants.constBegin(); it != projectConfig.globalConstants.constEnd(); it++) {
        addGlobalConstant(it.key(), it.value());
    }

    // Set warp behaviors
    QStringList behaviorNames;
    for (const auto &value : projectConfig.warpBehaviors) {
        if (project->metatileBehaviorMapInverse.contains(value))
            behaviorNames.append(project->metatileBehaviorMapInverse.value(value));
    }
    this->setWarpBehaviorsList(behaviorNames);

    int index = ui->comboBox_EventsTabIcon->findData(projectConfig.eventsTabIconPath);
    if (index < 0) {
        index = ui->comboBox_EventsTabIcon->findData("Custom");
        ui->lineEdit_EventsTabIcon->setText(projectConfig.eventsTabIconPath);
    } else {
        ui->lineEdit_EventsTabIcon->setText("");
    }
    ui->comboBox_EventsTabIcon->setCurrentIndex(index);

    this->refreshing = false; // Allow signals
}

void ProjectSettingsEditor::save() {
    if (!this->hasUnsavedChanges)
        return;

    // Save combo box settings
    projectConfig.defaultPrimaryTileset = ui->comboBox_DefaultPrimaryTileset->currentText();
    projectConfig.defaultSecondaryTileset = ui->comboBox_DefaultSecondaryTileset->currentText();
    projectConfig.baseGameVersion = projectConfig.stringToBaseGameVersion(ui->comboBox_BaseGameVersion->currentText());
    projectConfig.metatileAttributesSize = ui->comboBox_AttributesSize->currentText().toInt();

    // Save check box settings
    projectConfig.usePoryScript = ui->checkBox_UsePoryscript->isChecked();
    userConfig.useEncounterJson = ui->checkBox_ShowWildEncounterTables->isChecked();
    projectConfig.createMapTextFileEnabled = ui->checkBox_CreateTextFile->isChecked();
    projectConfig.tripleLayerMetatilesEnabled = ui->checkBox_EnableTripleLayerMetatiles->isChecked();
    projectConfig.hiddenItemRequiresItemfinderEnabled = ui->checkBox_EnableRequiresItemfinder->isChecked();
    projectConfig.hiddenItemQuantityEnabled = ui->checkBox_EnableQuantity->isChecked();
    projectConfig.eventCloneObjectEnabled = ui->checkBox_EnableCloneObjects->isChecked();
    projectConfig.eventWeatherTriggerEnabled = ui->checkBox_EnableWeatherTriggers->isChecked();
    projectConfig.eventSecretBaseEnabled = ui->checkBox_EnableSecretBases->isChecked();
    projectConfig.healLocationRespawnDataEnabled = ui->checkBox_EnableRespawn->isChecked();
    projectConfig.mapAllowFlagsEnabled = ui->checkBox_EnableAllowFlags->isChecked();
    projectConfig.floorNumberEnabled = ui->checkBox_EnableFloorNumber->isChecked();
    projectConfig.useCustomBorderSize = ui->checkBox_EnableCustomBorderSize->isChecked();
    projectConfig.tilesetsHaveCallback = ui->checkBox_OutputCallback->isChecked();
    projectConfig.tilesetsHaveIsCompressed = ui->checkBox_OutputIsCompressed->isChecked();
    porymapConfig.warpBehaviorWarningDisabled = ui->checkBox_DisableWarning->isChecked();
    projectConfig.setTransparentPixelsBlack = ui->radioButton_RenderBlack->isChecked();
    projectConfig.preserveMatchingOnlyData = ui->checkBox_PreserveMatchingOnlyData->isChecked();

    // Save spin box settings
    projectConfig.defaultElevation = ui->spinBox_Elevation->value();
    projectConfig.defaultCollision = ui->spinBox_Collision->value();
    projectConfig.defaultMetatileId = ui->spinBox_FillMetatile->value();
    projectConfig.defaultMapSize = QSize(ui->spinBox_MapWidth->value(), ui->spinBox_MapHeight->value());
    projectConfig.collisionSheetSize = QSize(ui->spinBox_MaxCollision->value() + 1, ui->spinBox_MaxElevation->value() + 1);
    projectConfig.metatileBehaviorMask = ui->spinBox_BehaviorMask->value();
    projectConfig.metatileTerrainTypeMask = ui->spinBox_TerrainTypeMask->value();
    projectConfig.metatileEncounterTypeMask = ui->spinBox_EncounterTypeMask->value();
    projectConfig.metatileLayerTypeMask = ui->spinBox_LayerTypeMask->value();
    projectConfig.blockMetatileIdMask = ui->spinBox_MetatileIdMask->value();
    projectConfig.blockCollisionMask = ui->spinBox_CollisionMask->value();
    projectConfig.blockElevationMask = ui->spinBox_ElevationMask->value();
    projectConfig.unusedTileNormal = ui->spinBox_UnusedTileNormal->value();
    projectConfig.unusedTileCovered = ui->spinBox_UnusedTileCovered->value();
    projectConfig.unusedTileSplit = ui->spinBox_UnusedTileSplit->value();
    projectConfig.maxEventsPerGroup = ui->spinBox_MaxEvents->value();
    projectConfig.playerViewDistance = QMargins(ui->spinBox_PlayerViewDistance_West->value(),
                                                ui->spinBox_PlayerViewDistance_North->value(),
                                                ui->spinBox_PlayerViewDistance_East->value(),
                                                ui->spinBox_PlayerViewDistance_South->value());

    // Save line edit settings
    projectConfig.prefabFilepath = ui->lineEdit_PrefabsPath->text();
    projectConfig.collisionSheetPath = ui->lineEdit_CollisionGraphics->text();
    projectConfig.setEventIconPath(Event::Group::Object, ui->lineEdit_ObjectsIcon->text());
    projectConfig.setEventIconPath(Event::Group::Warp, ui->lineEdit_WarpsIcon->text());
    projectConfig.setEventIconPath(Event::Group::Coord, ui->lineEdit_TriggersIcon->text());
    projectConfig.setEventIconPath(Event::Group::Bg, ui->lineEdit_BGsIcon->text());
    projectConfig.setEventIconPath(Event::Group::Heal, ui->lineEdit_HealLocationsIcon->text());
    for (auto lineEdit : ui->scrollAreaContents_ProjectPaths->findChildren<QLineEdit*>())
        projectConfig.setFilePath(lineEdit->objectName(), lineEdit->text());
    for (auto lineEdit : ui->scrollAreaContents_Identifiers->findChildren<QLineEdit*>())
        projectConfig.setIdentifier(lineEdit->objectName(), lineEdit->text());

    // Save global constants
    projectConfig.globalConstantsFilepaths = getGlobalConstantsFilepaths();
    projectConfig.globalConstants = getGlobalConstants();

    // Save warp behaviors
    projectConfig.warpBehaviors.clear();
    const QStringList behaviorNames = this->getWarpBehaviorsList();
    for (auto name : behaviorNames)
        projectConfig.warpBehaviors.append(project->metatileBehaviorMap.value(name));

    // Save border metatile IDs
    projectConfig.newMapBorderMetatileIds = this->getBorderMetatileIds(ui->checkBox_EnableCustomBorderSize->isChecked());

    // Save pokemon icon paths
    const QString species = ui->comboBox_IconSpecies->currentText();
    if (this->project->speciesNames.contains(species))
        this->editedPokemonIconPaths.insert(species, ui->lineEdit_PokemonIcon->text());
    for (auto i = this->editedPokemonIconPaths.cbegin(), end = this->editedPokemonIconPaths.cend(); i != end; i++)
        projectConfig.setPokemonIconPath(i.key(), i.value());

    QString eventsTabIconPath;
    QVariant data = ui->comboBox_EventsTabIcon->currentData();
    if (data.isValid() && data.canConvert<QString>()) {
        eventsTabIconPath = data.toString();
        if (eventsTabIconPath == "Custom") {
            eventsTabIconPath = ui->lineEdit_EventsTabIcon->text();
        }
    }
    projectConfig.eventsTabIconPath = eventsTabIconPath;

    projectConfig.save();
    userConfig.save();
    porymapConfig.save();

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
    QString filepath = FileDialog::getOpenFileName(this, description, "", extensions);
    if (filepath.isEmpty())
        return;

    if (filepathEdit)
        filepathEdit->setText(this->stripProjectDir(filepath));
    this->hasUnsavedChanges = true;
}

void ProjectSettingsEditor::addNewGlobalConstantsFilepath() {
    QString filepath = stripProjectDir(FileDialog::getOpenFileName(this, "Choose Global Constants File"));
    if (filepath.isEmpty() || getGlobalConstantsFilepaths().contains(filepath))
        return;

    addGlobalConstantsFilepath(filepath);
    this->hasUnsavedChanges = true;
}

void ProjectSettingsEditor::addGlobalConstantsFilepath(const QString &filepath) {
    auto filepathLabel = new QLabel(filepath, this);
    filepathLabel->setFrameStyle(QFrame::Panel | QFrame::Raised);
    filepathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    int newRow = ui->gridLayout_GlobalConstantsFiles->rowCount();
    ui->gridLayout_GlobalConstantsFiles->addWidget(filepathLabel, newRow, 0);

    auto deleteButton = new QToolButton();
    deleteButton->setIcon(QIcon(":/icons/delete.ico"));
    connect(deleteButton, &QAbstractButton::clicked, [this, filepathLabel, deleteButton](bool) {
        ui->gridLayout_GlobalConstantsFiles->removeWidget(filepathLabel);
        ui->gridLayout_GlobalConstantsFiles->removeWidget(deleteButton);
        delete filepathLabel;
        delete deleteButton;
        this->hasUnsavedChanges = true;
    });
    ui->gridLayout_GlobalConstantsFiles->addWidget(deleteButton, newRow, 1);
}

QStringList ProjectSettingsEditor::getGlobalConstantsFilepaths() {
    QStringList paths;
    for (int row = 1; row < ui->gridLayout_GlobalConstantsFiles->rowCount(); row++) {
        auto item = ui->gridLayout_GlobalConstantsFiles->itemAtPosition(row, 0);
        if (!item) continue;
        auto pathLabel = dynamic_cast<QLabel*>(item->widget());
        if (!pathLabel) continue;
        paths.append(pathLabel->text());
    }
    return paths;
}

void ProjectSettingsEditor::addNewGlobalConstant() {
    auto dialog = new NewDefineDialog(this);
    connect(dialog, &NewDefineDialog::createdDefine, [this](const QString &name, const QString &expression) {
        if (!getGlobalConstants().contains(name)) {
            addGlobalConstant(name, expression);
            this->hasUnsavedChanges = true;
        }
    });
    dialog->open();
}

void ProjectSettingsEditor::addGlobalConstant(const QString &name, const QString &expression) {
    auto nameLabel = new QLabel(name, this);
    nameLabel->setFrameStyle(QFrame::Panel | QFrame::Raised);
    nameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto expressionLineEdit = new QLineEdit(expression, this);

    int newRow = ui->gridLayout_GlobalConstants->rowCount();
    ui->gridLayout_GlobalConstants->addWidget(nameLabel, newRow, 0);
    ui->gridLayout_GlobalConstants->addWidget(expressionLineEdit, newRow, 1);

    auto deleteButton = new QToolButton();
    deleteButton->setIcon(QIcon(":/icons/delete.ico"));
    connect(deleteButton, &QAbstractButton::clicked, [this, nameLabel, expressionLineEdit, deleteButton](bool) {
        ui->gridLayout_GlobalConstants->removeWidget(nameLabel);
        ui->gridLayout_GlobalConstants->removeWidget(expressionLineEdit);
        ui->gridLayout_GlobalConstants->removeWidget(deleteButton);
        delete nameLabel;
        delete expressionLineEdit;
        delete deleteButton;
        this->hasUnsavedChanges = true;
    });
    ui->gridLayout_GlobalConstants->addWidget(deleteButton, newRow, 2);
}

QMap<QString,QString> ProjectSettingsEditor::getGlobalConstants() {
    QMap<QString,QString> constants;
    for (int row = 1; row < ui->gridLayout_GlobalConstants->rowCount(); row++) {
        auto nameItem = ui->gridLayout_GlobalConstants->itemAtPosition(row, 0);
        auto expressionItem = ui->gridLayout_GlobalConstants->itemAtPosition(row, 1);
        if (!nameItem || !expressionItem) continue;
        auto nameLabel = dynamic_cast<QLabel*>(nameItem->widget());
        auto expressionLineEdit = dynamic_cast<QLineEdit*>(expressionItem->widget());
        if (!nameLabel || !expressionLineEdit) continue;
        constants.insert(nameLabel->text(), expressionLineEdit->text());
    }
    return constants;
}

// Display relative path if this file is in the project folder
QString ProjectSettingsEditor::stripProjectDir(QString s) {
    return Util::stripPrefix(s, this->baseDir);
}

void ProjectSettingsEditor::importDefaultPrefabsClicked(bool) {
    // If the prompt is accepted the prefabs file will be created and its filepath will be saved in the config.
    BaseGameVersion version = projectConfig.stringToBaseGameVersion(ui->comboBox_BaseGameVersion->currentText());
    if (prefab.tryImportDefaultPrefabs(this, version, ui->lineEdit_PrefabsPath->text())) {
        ui->lineEdit_PrefabsPath->setText(projectConfig.prefabFilepath); // Refresh with new filepath
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

void ProjectSettingsEditor::openFilesHelp() {
    static const QUrl url("https://huderlem.github.io/porymap/manual/project-files.html#files");
    QDesktopServices::openUrl(url);
}

void ProjectSettingsEditor::openIdentifiersHelp() {
    static const QUrl url("https://huderlem.github.io/porymap/manual/project-files.html#identifiers");
    QDesktopServices::openUrl(url);
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
