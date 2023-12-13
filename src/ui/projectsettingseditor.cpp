#include "projectsettingseditor.h"
#include "ui_projectsettingseditor.h"
#include "config.h"
#include "noscrollcombobox.h"
#include "prefab.h"

#include <QAbstractButton>
#include <QFormLayout>

/*
    Editor for the settings in a user's porymap.project.cfg file (and 'use_encounter_json' in porymap.user.cfg).
*/

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

    // Connect file selection buttons
    connect(ui->button_ChoosePrefabs,     &QAbstractButton::clicked, [this](bool) { this->choosePrefabsFile(); });
    connect(ui->button_CollisionGraphics, &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_CollisionGraphics); });
    connect(ui->button_ObjectsIcon,       &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_ObjectsIcon); });
    connect(ui->button_WarpsIcon,         &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_WarpsIcon); });
    connect(ui->button_TriggersIcon,      &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_TriggersIcon); });
    connect(ui->button_BGsIcon,           &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_BGsIcon); });
    connect(ui->button_HealspotsIcon,     &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_HealspotsIcon); });
    connect(ui->button_PokemonIcon,       &QAbstractButton::clicked, [this](bool) { this->chooseImageFile(ui->lineEdit_PokemonIcon); });

    // Record that there are unsaved changes if any of the settings are modified
    for (auto combo : ui->centralwidget->findChildren<NoScrollComboBox *>()){
        if (combo != ui->comboBox_IconSpecies) // Changes to the icon species combo box are just for info display, don't mark as unsaved
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

// Remember the current settings tab for future sessions
void ProjectSettingsEditor::on_mainTabs_tabBarClicked(int index) {
    porymapConfig.setProjectSettingsTab(index);
}

void ProjectSettingsEditor::initUi() {
    // Populate combo boxes
    if (project) {
        ui->comboBox_DefaultPrimaryTileset->addItems(project->primaryTilesetLabels);
        ui->comboBox_DefaultSecondaryTileset->addItems(project->secondaryTilesetLabels);
        ui->comboBox_IconSpecies->addItems(project->speciesToIconPath.keys());
        ui->comboBox_IconSpecies->setEditable(false);
    }
    ui->comboBox_BaseGameVersion->addItems(ProjectConfig::versionStrings);
    ui->comboBox_AttributesSize->addItems({"1", "2", "4"});

    // Select tab from last session
    ui->mainTabs->setCurrentIndex(porymapConfig.getProjectSettingsTab());

    // Validate that the border metatiles text is a comma-separated list of metatile values
    const QString regex_Hex = "(0[xX])?[A-Fa-f0-9]+";
    static const QRegularExpression expression(QString("^(%1,)*%1$").arg(regex_Hex)); // Comma-separated list of hex values
    QRegularExpressionValidator *validator = new QRegularExpressionValidator(expression);
    ui->lineEdit_BorderMetatiles->setValidator(validator);
    this->setBorderMetatilesUi(projectConfig.getUseCustomBorderSize());

    // Set spin box limits
    int maxMetatileId = Block::getMaxMetatileId();
    ui->spinBox_FillMetatile->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile1->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile2->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile3->setMaximum(maxMetatileId);
    ui->spinBox_BorderMetatile4->setMaximum(maxMetatileId);
    ui->spinBox_Elevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_Collision->setMaximum(Block::getMaxCollision());
    ui->spinBox_MaxElevation->setMaximum(Block::getMaxElevation());
    ui->spinBox_MaxCollision->setMaximum(Block::getMaxCollision());
    // TODO: Move to a global
    ui->spinBox_MetatileIdMask->setMinimum(0x1);
    ui->spinBox_MetatileIdMask->setMaximum(0xFFFF); // Metatile IDs can use all 16 bits of a block
    ui->spinBox_CollisionMask->setMaximum(0xFFFE); // Collision/elevation can only use 15; metatile IDs must have at least 1 bit
    ui->spinBox_ElevationMask->setMaximum(0xFFFE);
}

void ProjectSettingsEditor::setBorderMetatilesUi(bool customSize) {
    ui->widget_DefaultSizeBorderMetatiles->setVisible(!customSize);
    ui->widget_CustomSizeBorderMetatiles->setVisible(customSize);
}

void ProjectSettingsEditor::setBorderMetatileIds(bool customSize, QList<uint16_t> metatileIds) {
    if (customSize) {
        ui->lineEdit_BorderMetatiles->setText(Metatile::getMetatileIdStringList(metatileIds));
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

void ProjectSettingsEditor::updateAttributeLimits(const QString &attrSize) {
    QMap<QString, uint32_t> limits {
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
void ProjectSettingsEditor::updatePokemonIconPath(const QString &species) {
    if (!project) return;

    // If user was editing a path for a valid species, record filepath text before we wipe it.
    if (!this->prevIconSpecies.isEmpty() && this->project->speciesToIconPath.contains(species)) {
        this->editedPokemonIconPaths[this->prevIconSpecies] = ui->lineEdit_PokemonIcon->text();
    }

    QString editedPath = this->editedPokemonIconPaths.value(species);
    QString defaultPath = this->project->speciesToIconPath.value(species);

    ui->lineEdit_PokemonIcon->setText(this->stripProjectDir(editedPath));
    ui->lineEdit_PokemonIcon->setPlaceholderText(this->stripProjectDir(defaultPath));
    this->prevIconSpecies = species;
}

void ProjectSettingsEditor::createProjectPathsTable() {
    auto pathPairs = ProjectConfig::defaultPaths.values();
    for (auto pathPair : pathPairs) {
        // Name of the path
        auto name = new QLabel();
        name->setAlignment(Qt::AlignBottom);
        name->setText(pathPair.first);

        // Filepath line edit
        auto lineEdit = new QLineEdit();
        lineEdit->setObjectName(pathPair.first); // Used when saving the paths
        lineEdit->setPlaceholderText(pathPair.second);
        lineEdit->setClearButtonEnabled(true);

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

        // Add to list
        auto editArea = new QWidget();
        auto layout = new QHBoxLayout(editArea);
        layout->addWidget(lineEdit);
        layout->addWidget(button);
        ui->layout_ProjectPaths->addRow(name, editArea);
    }
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

    // Set spin box values
    ui->spinBox_Elevation->setValue(projectConfig.getNewMapElevation());
    ui->spinBox_Collision->setValue(projectConfig.getNewMapCollision());
    ui->spinBox_FillMetatile->setValue(projectConfig.getNewMapMetatileId());
    ui->spinBox_MaxElevation->setValue(projectConfig.getCollisionSheetHeight() - 1);
    ui->spinBox_MaxCollision->setValue(projectConfig.getCollisionSheetWidth() - 1);
    ui->spinBox_BehaviorMask->setValue(projectConfig.getMetatileBehaviorMask());
    ui->spinBox_EncounterTypeMask->setValue(projectConfig.getMetatileEncounterTypeMask());
    ui->spinBox_LayerTypeMask->setValue(projectConfig.getMetatileLayerTypeMask());
    ui->spinBox_TerrainTypeMask->setValue(projectConfig.getMetatileTerrainTypeMask());
    ui->spinBox_MetatileIdMask->setValue(projectConfig.getBlockMetatileIdMask());
    ui->spinBox_CollisionMask->setValue(projectConfig.getBlockCollisionMask());
    ui->spinBox_ElevationMask->setValue(projectConfig.getBlockElevationMask());

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
        lineEdit->setText(projectConfig.getFilePath(lineEdit->objectName(), true));

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

    // Save spin box settings
    projectConfig.setNewMapElevation(ui->spinBox_Elevation->value());
    projectConfig.setNewMapCollision(ui->spinBox_Collision->value());
    projectConfig.setNewMapMetatileId(ui->spinBox_FillMetatile->value());
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

    // Save border metatile IDs
    projectConfig.setNewMapBorderMetatileIds(this->getBorderMetatileIds(ui->checkBox_EnableCustomBorderSize->isChecked()));

    // Save pokemon icon paths
    this->editedPokemonIconPaths.insert(ui->comboBox_IconSpecies->currentText(), ui->lineEdit_PokemonIcon->text());
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
    // No need to set hasUnsavedChanges here.
    BaseGameVersion version = projectConfig.stringToBaseGameVersion(ui->comboBox_BaseGameVersion->currentText());
    if (prefab.tryImportDefaultPrefabs(this, version, ui->lineEdit_PrefabsPath->text())) {
        ui->lineEdit_PrefabsPath->setText(projectConfig.getPrefabFilepath()); // Refresh with new filepath
        this->projectNeedsReload = true;
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
        close();
    } else if (buttonRole == QDialogButtonBox::RejectRole) {
        // "Cancel" button
        if (!this->promptSaveChanges())
            return;
        close();
    } else if (buttonRole == QDialogButtonBox::ResetRole) {
        // "Restore Defaults" button
        this->promptRestoreDefaults();
    }
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
        if (this->prompt("Settings changed, reload project to apply changes?") == QMessageBox::Yes){
            // Reloading the project will destroy this window, no other work should happen after this signal is emitted
            emit this->reloadProject();
        }
    }
}
