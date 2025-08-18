#include "regionmapeditor.h"
#include "ui_regionmapeditor.h"
#include "regionmappropertiesdialog.h"
#include "regionmapeditcommands.h"
#include "imageexport.h"
#include "shortcut.h"
#include "config.h"
#include "log.h"
#include "utility.h"

#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QColor>
#include <QMessageBox>
#include <QTransform>
#include <math.h>

RegionMapEditor::RegionMapEditor(QWidget *parent, Project *project) :
    QMainWindow(parent),
    ui(new Ui::RegionMapEditor)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->ui->setupUi(this);
    this->project = project;
    connect(this->project, &Project::mapSectionIdNamesChanged, this, &RegionMapEditor::setLocations);
    connect(ui->checkBox_tileHFlip, &QCheckBox::toggled, this, &RegionMapEditor::setTileHFlip);
    connect(ui->checkBox_tileVFlip, &QCheckBox::toggled, this, &RegionMapEditor::setTileVFlip);


    this->configFilepath = QString("%1/%2").arg(this->project->root).arg(projectConfig.getFilePath(ProjectFilePath::json_region_porymap_cfg));
    this->initShortcuts();
    this->restoreWindowState();
}

RegionMapEditor::~RegionMapEditor()
{
    this->region_map = nullptr;
    // deletion must be done in this order else crashes
    auto stacks = this->history.stacks();
    for (auto *stack : stacks) {
        this->history.removeStack(stack);
    }
    for (auto p : this->region_maps) {
        delete p.second;
    }
    delete region_map_item;
    delete mapsquare_selector_item;
    delete region_map_layout_item;
    delete scene_region_map_image;
    delete scene_region_map_layout;
    delete scene_region_map_tiles;
    delete ui;
}

void RegionMapEditor::restoreWindowState() {
    logInfo("Restoring region map editor geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getRegionMapEditorGeometry();
    this->restoreGeometry(geometry.value("region_map_editor_geometry"));
    this->restoreState(geometry.value("region_map_editor_state"));
}

void RegionMapEditor::initShortcuts() {
    auto *shortcut_RM_Options_delete = new Shortcut(
            {QKeySequence("Del"), QKeySequence("Backspace")}, this, SLOT(on_pushButton_RM_Options_delete_clicked()));
    shortcut_RM_Options_delete->setObjectName("shortcut_RM_Options_delete");
    shortcut_RM_Options_delete->setWhatsThis("Map Layout: Delete Square");

    QAction *undoAction = this->history.createUndoAction(this, tr("&Undo"));
    undoAction->setObjectName("action_RegionMap_Undo");
    undoAction->setShortcut(QKeySequence("Ctrl+Z"));

    QAction *redoAction = this->history.createRedoAction(this, tr("&Redo"));
    redoAction->setObjectName("action_RegionMap_Redo");
    redoAction->setShortcuts({QKeySequence("Ctrl+Y"), QKeySequence("Ctrl+Shift+Z")});

    ui->menuEdit->addAction(undoAction);
    ui->menuEdit->addAction(redoAction);

    shortcutsConfig.load();
    shortcutsConfig.setDefaultShortcuts(shortcutableObjects());
    applyUserShortcuts();

    connect(&(this->history), &QUndoGroup::indexChanged, [this](int) {
        on_tabWidget_Region_Map_currentChanged(this->ui->tabWidget_Region_Map->currentIndex());
    });
}

QObjectList RegionMapEditor::shortcutableObjects() const {
    QObjectList shortcutable_objects;

    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcutable_objects.append(qobject_cast<QObject *>(shortcut));

    return shortcutable_objects;
}

void RegionMapEditor::applyUserShortcuts() {
    for (auto *action : findChildren<QAction *>())
        if (!action->objectName().isEmpty())
            action->setShortcuts(shortcutsConfig.userShortcuts(action));
    for (auto *shortcut : findChildren<Shortcut *>())
        if (!shortcut->objectName().isEmpty())
            shortcut->setKeys(shortcutsConfig.userShortcuts(shortcut));
}

bool RegionMapEditor::loadRegionMapEntries() {
    this->region_map_entries = this->project->getRegionMapEntries();
    return true;
}

bool RegionMapEditor::saveRegionMapEntries() {
    this->project->setRegionMapEntries(this->region_map_entries);
    this->project->saveRegionMapSections();
    return true;
}

void buildEmeraldDefaults(poryjson::Json &json) {
    ParseUtil parser;
    QString emeraldDefault = parser.readTextFile(":/text/region_map_default_emerald.json");
    json = poryjson::Json::parse(emeraldDefault);
}

void buildRubyDefaults(poryjson::Json &json) {
    ParseUtil parser;
    QString emeraldDefault = parser.readTextFile(":/text/region_map_default_ruby.json");
    json = poryjson::Json::parse(emeraldDefault);
}

void buildFireredDefaults(poryjson::Json &json) {

    ParseUtil parser;
    QString fireredDefault = parser.readTextFile(":/text/region_map_default_firered.json");    
    json = poryjson::Json::parse(fireredDefault);
}

poryjson::Json RegionMapEditor::buildDefaultJson() {
    poryjson::Json defaultJson;
    switch (projectConfig.baseGameVersion) {
        case BaseGameVersion::pokeemerald:
            buildEmeraldDefaults(defaultJson);
            break;
        case BaseGameVersion::pokeruby:
            buildRubyDefaults(defaultJson);
            break;
        case BaseGameVersion::pokefirered:
            buildFireredDefaults(defaultJson);
            break;
        default:
            break;
    }

    return defaultJson;
}

bool RegionMapEditor::buildConfigDialog() {
    // use this temporary object while window is active, save only when user accepts
    poryjson::Json rmConfigJsonUpdate = this->rmConfigJson;

    QDialog dialog(this);
    dialog.setWindowTitle("Configure Region Maps");
    dialog.setWindowModality(Qt::WindowModal);

    QFormLayout form(&dialog);

    // need to display current region map(s) if there are some
    QListWidget *regionMapList = new QListWidget(&dialog);

    form.addRow(new QLabel("Region Map List:"));
    form.addRow(regionMapList);

    // lambda to update the current map list displayed in the config window with a
    // widget item that contains that region map's json data
    auto updateMapList = [regionMapList, &rmConfigJsonUpdate]() {
        regionMapList->clear();
        poryjson::Json::object mapsObject = rmConfigJsonUpdate.object_items();
        for (auto o : mapsObject["region_maps"].array_items()) {
            poryjson::Json::object object = o.object_items();
            QListWidgetItem *newItem = new QListWidgetItem;
            newItem->setText(object["alias"].string_value());
            QString objectString;
            int i = 0;
            o.dump(objectString, &i);
            newItem->setData(Qt::UserRole, objectString);
            regionMapList->addItem(newItem);
        }
    };
    updateMapList();

    auto updateJsonFromList = [regionMapList, &rmConfigJsonUpdate]() {
        poryjson::Json::object newJson;
        poryjson::Json::array mapArr;
        for (auto item : regionMapList->findItems("*", Qt::MatchWildcard)) {
            poryjson::Json itemJson = poryjson::Json::parse(item->data(Qt::UserRole).toString());
            mapArr.append(itemJson);
        }
        newJson["region_maps"] = mapArr;
        rmConfigJsonUpdate = poryjson::Json(newJson);
    };

    // when double clicking a region map from the list, bring up the configuration window 
    // and populate it with the current values
    // the user can edit and save the config for an existing map this way
    connect(regionMapList, &QListWidget::itemDoubleClicked, [this, &rmConfigJsonUpdate, updateMapList, regionMapList](QListWidgetItem *item) {
        int itemIndex = regionMapList->row(item);

        poryjson::Json clickedJson = poryjson::Json::parse(item->data(Qt::UserRole).toString());

        RegionMapPropertiesDialog dialog(this);
        dialog.setProject(this->project);

        // populate
        dialog.setProperties(clickedJson);

        if (dialog.exec() == QDialog::Accepted) {
            // update the object then update the map list
            poryjson::Json updatedMap = dialog.saveToJson();
            poryjson::Json::object previousObj = rmConfigJsonUpdate.object_items();
            poryjson::Json::array newMapList = previousObj["region_maps"].array_items(); // copy
            newMapList[itemIndex] = updatedMap;
            previousObj["region_maps"] = newMapList;
            rmConfigJsonUpdate = poryjson::Json(previousObj);

            updateMapList();
        }
    });

    QPushButton *addMapButton = new QPushButton("Add Region Map...");
    form.addRow(addMapButton);

    // allow user to add region maps
    connect(addMapButton, &QPushButton::clicked, [this, regionMapList, &updateJsonFromList] {
        poryjson::Json resultJson = configRegionMapDialog();
        poryjson::Json::object resultObj = resultJson.object_items();

        QString resultStr;
        int tab = 0;
        resultJson.dump(resultStr, &tab);

        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(resultObj["alias"].string_value());
        newItem->setData(Qt::UserRole, resultStr);
        regionMapList->addItem(newItem);
        updateJsonFromList();
    });

    QPushButton *delMapButton = new QPushButton("Delete Selected Region Map");
    form.addRow(delMapButton);

    connect(delMapButton, &QPushButton::clicked, [regionMapList, &updateJsonFromList] {
        QListWidgetItem *item = regionMapList->currentItem();
        if (item) {
            regionMapList->removeItemWidget(item);
            delete item;
            updateJsonFromList();
        }
    });

    // TODO: city maps (leaving this for now)
    // QListWidget *cityMapList = new QListWidget(&dialog);
    // // TODO: double clicking can delete them? bring up the tiny menu thing like sory by maps
    // //cityMapList->setSelectionMode(QAbstractItemView::NoSelection);
    // //cityMapList->setFocusPolicy(Qt::NoFocus);
    // form.addRow(new QLabel("City Map List:"));
    // form.addRow(cityMapList);
    //
    // for (int i = 0; i < 20; i++) {
    //     QListWidgetItem *newItem = new QListWidgetItem;
    //     newItem->setText(QString("city %1").arg(i));
    //     cityMapList->addItem(newItem);
    // }
    //
    // QPushButton *addCitiesButton = new QPushButton("Configure City Maps...");
    // form.addRow(addCitiesButton);
    // // needs to pick tilemaps and such here too


    // for sake of convenience, option to just use defaults for each basegame version
    QPushButton *config_useProjectDefault = nullptr;
    switch (projectConfig.baseGameVersion) {
        case BaseGameVersion::pokefirered:
            config_useProjectDefault = new QPushButton("\nUse pokefirered defaults\n");
            break;
        case BaseGameVersion::pokeemerald:
            config_useProjectDefault = new QPushButton("\nUse pokeemerald defaults\n");
            break;
        case BaseGameVersion::pokeruby:
            config_useProjectDefault = new QPushButton("\nUse pokeruby defaults\n");
            break;
        default:
            break;
    }
    form.addRow(config_useProjectDefault);

    connect(config_useProjectDefault, &QPushButton::clicked, [this, &rmConfigJsonUpdate, updateMapList](){
        poryjson::Json newJson = buildDefaultJson();
        // no error checking required because these are well-formed files
        rmConfigJsonUpdate = newJson;
        updateMapList();
    });

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    form.addRow(&buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        // if everything looks good, we can update the master json
        this->rmConfigJson = rmConfigJsonUpdate;
        this->configSaved = false;
        return true;
    }

    return false;
}

poryjson::Json RegionMapEditor::configRegionMapDialog() {

    RegionMapPropertiesDialog dialog(this);
    dialog.setProject(this->project);

    if (dialog.exec() == QDialog::Accepted) {
        poryjson::Json regionMapJson = dialog.saveToJson();
        return regionMapJson;
    }

    return poryjson::Json();
}

bool RegionMapEditor::verifyConfig(poryjson::Json cfg) {
    OrderedJson::object obj = cfg.object_items();

    if (!obj.contains("region_maps")) {
        logError("Region map config json has no map list.");
        return false;
    }
    return true;
}

void RegionMapEditor::clear() {
    // clear everything that gets loaded in a way that will not crash the destructor
    // except the config json

    auto stacks = this->history.stacks();
    this->history.blockSignals(true);
    for (auto *stack : stacks) {
        this->history.removeStack(stack);
    }
    this->history.blockSignals(false);
    for (auto p : this->region_maps) {
        delete p.second;
    }
    this->region_map = nullptr;
    this->region_maps.clear();
    this->region_map_entries.clear();

    this->currIndex = 0;
    this->activeEntry = QString();
    this->entriesFirstDraw = true;

    delete region_map_item;
    region_map_item = nullptr;

    delete mapsquare_selector_item;
    mapsquare_selector_item = nullptr;

    delete region_map_layout_item;
    region_map_layout_item = nullptr;

    delete scene_region_map_image;
    scene_region_map_image = nullptr;

    delete scene_region_map_layout;
    scene_region_map_layout = nullptr;

    delete scene_region_map_tiles;
    scene_region_map_tiles = nullptr;
}

bool RegionMapEditor::reload() {
    clear();
    return setup();
}

bool RegionMapEditor::setup() {
    // if all has gone well, this->rmConfigJson should be populated
    // next, load the entries
    loadRegionMapEntries();

    // load the region maps into this->region_maps
    poryjson::Json::object regionMapObjectCopy = this->rmConfigJson.object_items();
    for (auto o : regionMapObjectCopy["region_maps"].array_items()) {
        QString alias = o.object_items().at("alias").string_value();

        RegionMap *newMap = new RegionMap(this->project);
        newMap->setEntries(&this->region_map_entries);
        if (!newMap->loadMapData(o)) {
            delete newMap;
            // TODO: consider continue, just reporting error loading single map?
            this->setupError = true;
            return false;
        }

        connect(newMap, &RegionMap::mapNeedsDisplaying, [this]() {
            displayRegionMap();
        });

        region_maps[alias] = newMap;

        this->history.addStack(&(newMap->editHistory));
    }

    // add to ui
    this->ui->comboBox_regionSelector->clear();
    for (auto p : region_maps) {
        this->ui->comboBox_regionSelector->addItem(p.first);
    }

    // display the first region map in the list
    if (!region_maps.empty()) {
        setRegionMap(region_maps.begin()->second);
    }
    this->setupError = false;
    return true;
}

bool RegionMapEditor::load(bool silent) {
    // check for config json file
    bool badConfig = true;
    if (QFile::exists(this->configFilepath)) {
        ParseUtil parser;
        OrderedJson::object obj;
        if (parser.tryParseOrderedJsonFile(&obj, this->configFilepath)) {
            this->rmConfigJson = OrderedJson(obj);
            this->configSaved = true;
        }
        badConfig = !verifyConfig(this->rmConfigJson);
    }

    if (badConfig) {
        if (silent) return false;
        // show popup explaining next window
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setText("Region map configuration not found.");
        warning.setInformativeText("In order to continue, you must setup porymap to use your region map.");
        warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        warning.setDefaultButton(QMessageBox::Ok);

        if (warning.exec() == QMessageBox::Ok) {
            // there is a separate window that allows to load multiple region maps, 
            if (!buildConfigDialog()) {
                // User canceled config set up
                return false;
            }
        } else {
            // User declined config set up
            return false;
        }
    } else {
        logInfo("Successfully loaded region map configuration file.");
    }

    return setup();
}

void RegionMapEditor::setRegionMap(RegionMap *map) {
    this->region_map = map;
    this->currIndex = this->region_map->firstLayoutIndex();

    if (this->region_map->layoutEnabled()) {
        this->ui->tabWidget_Region_Map->setTabEnabled(1, true);
        this->ui->tabWidget_Region_Map->setTabEnabled(2, true);
    } else {
        this->ui->tabWidget_Region_Map->setTabEnabled(1, false);
        this->ui->tabWidget_Region_Map->setTabEnabled(2, false);
        this->ui->tabWidget_Region_Map->setCurrentIndex(0);
    }

    displayRegionMap();
    this->region_map->editHistory.setActive();
}

bool RegionMapEditor::saveRegionMap(RegionMap *map) {
    if (!map) return false;

    map->save();

    return true;
}

void RegionMapEditor::saveConfig() {
    OrderedJson::array mapArray;
    for (auto it : this->region_maps) {
        OrderedJson::object obj = it.second->config();
        mapArray.append(obj);
    }

    OrderedJson::object mapsObject;
    mapsObject["region_maps"] = mapArray;

    OrderedJson newConfigJson(mapsObject);
    QFile file(this->configFilepath);
    if (!file.open(QIODevice::WriteOnly)) {
        logError(QString("Could not open %1 for writing").arg(this->configFilepath));
        return;
    }
    OrderedJsonDoc jsonDoc(&newConfigJson);
    jsonDoc.dump(&file);
    file.close();

    this->configSaved = true;
}

void RegionMapEditor::on_action_RegionMap_Save_triggered() {
    saveRegionMap(this->region_map);

    // save entries
    saveRegionMapEntries();

    // save config
    saveConfig();
}

void RegionMapEditor::on_actionSave_All_triggered() {
    for (auto it : this->region_maps) {
        saveRegionMap(it.second);
    }
    saveRegionMapEntries();
    saveConfig();
}

void RegionMapEditor::on_action_Configure_triggered() {
    reconfigure();
}

bool RegionMapEditor::reconfigure() {
    this->setupError = false;
    if (this->modified()) {
        QMessageBox warning;
        warning.setIcon(QMessageBox::Warning);
        warning.setText("Reconfiguring region maps will discard any unsaved changes.");
        warning.setInformativeText("Continue?");
        warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        warning.setDefaultButton(QMessageBox::Ok);

        if (warning.exec() == QMessageBox::Ok) {
            if (buildConfigDialog()) {
                return reload();
            }
        }
    }
    else {
        if (buildConfigDialog()) {
            return reload();
        }
    }
    return false;
}

void RegionMapEditor::displayRegionMap() {
    displayRegionMapTileSelector();
    displayRegionMapImage();
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
    displayRegionMapEntriesImage();
    displayRegionMapEntryOptions();

    updateLayerDisplayed();
}

void RegionMapEditor::updateLayerDisplayed() {
    // add layers from layout to combo
    ui->comboBox_layoutLayer->clear();
    ui->comboBox_layoutLayer->addItems(this->region_map->getLayers());
}

void RegionMapEditor::on_comboBox_regionSelector_textActivated(const QString &region) {
    if (this->region_maps.contains(region)) {
        setRegionMap(region_maps.at(region));
    }
}

void RegionMapEditor::displayRegionMapImage() {
    if (!scene_region_map_image) {
        this->scene_region_map_image = new QGraphicsScene;
    }
    if (region_map_item && scene_region_map_image) {
        scene_region_map_image->removeItem(region_map_item);
        delete region_map_item;
    }

    this->region_map_item = new RegionMapPixmapItem(this->region_map, this->mapsquare_selector_item);
    this->region_map_item->draw();

    connect(this->region_map_item, &RegionMapPixmapItem::mouseEvent,
            this, &RegionMapEditor::mouseEvent_region_map);
    connect(this->region_map_item, &RegionMapPixmapItem::hoveredRegionMapTileChanged,
            this, &RegionMapEditor::onHoveredRegionMapTileChanged);
    connect(this->region_map_item, &RegionMapPixmapItem::hoveredRegionMapTileCleared,
            this, &RegionMapEditor::onHoveredRegionMapTileCleared);

    this->scene_region_map_image->addItem(this->region_map_item);
    this->scene_region_map_image->setSceneRect(this->scene_region_map_image->itemsBoundingRect());

    this->ui->graphicsView_Region_Map_BkgImg->setScene(this->scene_region_map_image);

    on_verticalSlider_Zoom_Map_Image_valueChanged(this->ui->verticalSlider_Zoom_Map_Image->value());
}

void RegionMapEditor::displayRegionMapLayout() {
    if (!scene_region_map_layout) {
        this->scene_region_map_layout = new QGraphicsScene;
    }
    if (region_map_layout_item && scene_region_map_layout) {
        this->scene_region_map_layout->removeItem(region_map_layout_item);
        delete region_map_layout_item;
    }

    this->region_map_layout_item = new RegionMapLayoutPixmapItem(this->region_map, this->mapsquare_selector_item);

    connect(this->region_map_layout_item, &RegionMapLayoutPixmapItem::selectedTileChanged,
            this, &RegionMapEditor::onRegionMapLayoutSelectedTileChanged);
    connect(this->region_map_layout_item, &RegionMapLayoutPixmapItem::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapLayoutHoveredTileChanged);
    connect(this->region_map_layout_item, &RegionMapLayoutPixmapItem::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapLayoutHoveredTileCleared);

    this->region_map_layout_item->draw();
    this->region_map_layout_item->select(this->currIndex);

    this->scene_region_map_layout->addItem(region_map_layout_item);
    this->scene_region_map_layout->setSceneRect(this->scene_region_map_layout->itemsBoundingRect());

    this->ui->graphicsView_Region_Map_Layout->setScene(this->scene_region_map_layout);
}

void RegionMapEditor::displayRegionMapLayoutOptions() {
    if (!this->region_map->layoutEnabled()) return;

    const QSignalBlocker b(ui->comboBox_RM_ConnectedMap);
    this->ui->comboBox_RM_ConnectedMap->clear();
    this->ui->comboBox_RM_ConnectedMap->addItems(this->project->locationNames());

    this->ui->frame_RM_Options->setEnabled(true);

    updateRegionMapLayoutOptions(this->currIndex);
}

void RegionMapEditor::updateRegionMapLayoutOptions(int index) {
    const QSignalBlocker b_ConnectedMap(ui->comboBox_RM_ConnectedMap);
    this->ui->comboBox_RM_ConnectedMap->setTextItem(this->region_map->squareMapSection(index));

    this->ui->pushButton_RM_Options_delete->setEnabled(this->region_map->squareHasMap(index));

    const QSignalBlocker b_LayoutWidth(ui->spinBox_RM_LayoutWidth);
    const QSignalBlocker b_LayoutHeight(ui->spinBox_RM_LayoutHeight);
    this->ui->spinBox_RM_LayoutWidth->setMinimum(1);
    this->ui->spinBox_RM_LayoutWidth->setMaximum(this->region_map->tilemapWidth() - this->region_map->padLeft());
    this->ui->spinBox_RM_LayoutHeight->setMinimum(1);
    this->ui->spinBox_RM_LayoutHeight->setMaximum(this->region_map->tilemapHeight() - this->region_map->padTop());
    this->ui->spinBox_RM_LayoutWidth->setValue(this->region_map->layoutWidth());
    this->ui->spinBox_RM_LayoutHeight->setValue(this->region_map->layoutHeight());
}

void RegionMapEditor::displayRegionMapEntriesImage() {
    if (!scene_region_map_entries) {
        this->scene_region_map_entries = new QGraphicsScene;
    }
    if (region_map_entries_item && scene_region_map_entries) {
        this->scene_region_map_entries->removeItem(region_map_entries_item);
        delete region_map_entries_item;
    }

    this->region_map_entries_item = new RegionMapEntriesPixmapItem(this->region_map, this->mapsquare_selector_item);

    connect(this->region_map_entries_item, &RegionMapEntriesPixmapItem::entryPositionChanged,
            this, &RegionMapEditor::onRegionMapEntryDragged);

    if (entriesFirstDraw) {
        QString first = this->ui->comboBox_RM_Entry_MapSection->currentText();
        this->region_map_entries_item->currentSection = first;
        this->activeEntry = first;
        updateRegionMapEntryOptions(first);
        entriesFirstDraw = false;
    }

    this->region_map_entries_item->draw();

    int idx = this->region_map_entries.contains(activeEntry) ? this->region_map->getMapSquareIndex(this->region_map_entries[activeEntry].x + this->region_map->padLeft(),
                                                               this->region_map_entries[activeEntry].y + this->region_map->padTop())
                                                             : 0;
    this->region_map_entries_item->select(idx);


    this->scene_region_map_entries->addItem(region_map_entries_item);
    this->scene_region_map_entries->setSceneRect(this->scene_region_map_entries->itemsBoundingRect());

    this->ui->graphicsView_Region_Map_Entries->setScene(this->scene_region_map_entries);
}

void RegionMapEditor::displayRegionMapEntryOptions() {
    if (!this->region_map->layoutEnabled()) return;

    this->ui->comboBox_RM_Entry_MapSection->clear();
    this->ui->comboBox_RM_Entry_MapSection->addItems(this->project->locationNames());
    this->ui->spinBox_RM_Entry_x->setMaximum(128);
    this->ui->spinBox_RM_Entry_y->setMaximum(128);
    this->ui->spinBox_RM_Entry_width->setMinimum(1);
    this->ui->spinBox_RM_Entry_height->setMinimum(1);
    this->ui->spinBox_RM_Entry_width->setMaximum(128);
    this->ui->spinBox_RM_Entry_height->setMaximum(128);
}

void RegionMapEditor::updateRegionMapEntryOptions(QString section) {
    if (!this->region_map->layoutEnabled()) return;

    const QSignalBlocker b_X(this->ui->spinBox_RM_Entry_x);
    const QSignalBlocker b_Y(this->ui->spinBox_RM_Entry_y);
    const QSignalBlocker b_W(this->ui->spinBox_RM_Entry_width);
    const QSignalBlocker b_H(this->ui->spinBox_RM_Entry_height);

    bool enabled = (section != this->region_map->default_map_section) && this->region_map_entries.contains(section);
    this->ui->spinBox_RM_Entry_x->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_y->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_width->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_height->setEnabled(enabled);
    this->ui->pushButton_entryActivate->setEnabled(section != this->region_map->default_map_section);
    this->ui->pushButton_entryActivate->setText(enabled ? "Remove" : "Add");

    this->ui->comboBox_RM_Entry_MapSection->setTextItem(section);
    this->activeEntry = section;
    this->region_map_entries_item->currentSection = section;
    MapSectionEntry entry = enabled ? this->region_map_entries[section] : MapSectionEntry();
    this->ui->spinBox_RM_Entry_x->setValue(entry.x);
    this->ui->spinBox_RM_Entry_y->setValue(entry.y);
    this->ui->spinBox_RM_Entry_width->setValue(entry.width);
    this->ui->spinBox_RM_Entry_height->setValue(entry.height);
}

void RegionMapEditor::on_pushButton_entryActivate_clicked() {
    QString section = this->ui->comboBox_RM_Entry_MapSection->currentText();
    if (section == this->region_map->default_map_section) return;

    MapSectionEntry oldEntry = this->region_map->getEntry(section);
    if (this->region_map_entries.contains(section)) {
        // disable
        this->region_map->editHistory.push(new RemoveEntry(this->region_map, section, oldEntry, MapSectionEntry()));
    } else {
        // enable
        MapSectionEntry newEntry = MapSectionEntry();
        newEntry.valid = true;
        this->region_map->editHistory.push(new AddEntry(this->region_map, section, oldEntry, newEntry));
    }
    updateRegionMapEntryOptions(section);
}

void RegionMapEditor::displayRegionMapTileSelector() {
    if (!scene_region_map_tiles) {
        this->scene_region_map_tiles = new QGraphicsScene;
    }
    if (mapsquare_selector_item && scene_region_map_tiles) {
        this->scene_region_map_tiles->removeItem(mapsquare_selector_item);
        delete mapsquare_selector_item;
    }

    this->mapsquare_selector_item = new TilemapTileSelector(this->region_map->pngPath(), this->region_map->tilemapFormat(), this->region_map->palPath());

    // Initialize with current settings
    this->mapsquare_selector_item->selectHFlip(ui->checkBox_tileHFlip->isChecked());
    this->mapsquare_selector_item->selectVFlip(ui->checkBox_tileVFlip->isChecked());
    this->mapsquare_selector_item->selectPalette(ui->spinBox_tilePalette->value()); // This will also draw the selector

    this->scene_region_map_tiles->addItem(this->mapsquare_selector_item);

    connect(this->mapsquare_selector_item, &TilemapTileSelector::selectedTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged);
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged);
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared);

    this->ui->graphicsView_RegionMap_Tiles->setScene(this->scene_region_map_tiles);
    on_verticalSlider_Zoom_Image_Tiles_valueChanged(this->ui->verticalSlider_Zoom_Image_Tiles->value());

    this->ui->frame_tileOptions->setEnabled(this->region_map->tilemapFormat() != TilemapFormat::Plain);

    this->mapsquare_selector_item->select(this->selectedImageTile);
}

void RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged(unsigned id) {
    this->selectedImageTile = id;
}

void RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged(unsigned tileId) {
    this->ui->statusbar->showMessage(QString("Tile: %1").arg(Util::toHexString(tileId, 4)));
}

void RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared() {
    this->ui->statusbar->clearMessage();
}

void RegionMapEditor::onRegionMapEntryDragged(int new_x, int new_y) {
    on_spinBox_RM_Entry_x_valueChanged(new_x);
    on_spinBox_RM_Entry_y_valueChanged(new_y);
    updateRegionMapEntryOptions(activeEntry);
}

void RegionMapEditor::onRegionMapLayoutSelectedTileChanged(int index) {
    this->currIndex = index;
    this->region_map_layout_item->highlightedTile = index;

    updateRegionMapLayoutOptions(index);
    this->region_map_layout_item->draw();
}

void RegionMapEditor::onRegionMapLayoutHoveredTileChanged(int index) {
    QString message = QString();
    int x = this->region_map->squareX(index);
    int y = this->region_map->squareY(index);
    if (x >= 0 && y >= 0) {
        message = QString("(%1, %2)").arg(x).arg(y);
        if (this->region_map->squareHasMap(index)) {
            message += QString("\t %1").arg(this->region_map->squareMapSection(index));
        }
    }
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onRegionMapLayoutHoveredTileCleared() {
    this->ui->statusbar->clearMessage();
}

void RegionMapEditor::onHoveredRegionMapTileChanged(int x, int y) {
    shared_ptr<TilemapTile> tile = this->region_map->getTile(x, y);
    QString message = QString("x: %1, y: %2    ").arg(x).arg(y) + tile->info();
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onHoveredRegionMapTileCleared() {
    this->ui->statusbar->clearMessage();
}

void RegionMapEditor::mouseEvent_region_map(QGraphicsSceneMouseEvent *event, RegionMapPixmapItem *item) {
    static unsigned actionId_ = 0;

    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    int index = this->region_map->getMapSquareIndex(x, y);
    if (index > this->region_map->tilemapSize() - 1) return; // TODO: is the math correct here?

    if (event->buttons() & Qt::RightButton) {
        item->select(event);
        // set palette and flips
        auto tile = this->region_map->getTile(x, y);
        this->ui->spinBox_tilePalette->setValue(tile->palette());
        this->ui->checkBox_tileHFlip->setChecked(tile->hFlip());
        this->ui->checkBox_tileVFlip->setChecked(tile->vFlip());
    } else if (event->modifiers() & Qt::ControlModifier) {
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            actionId_++;
        } else {
            QByteArray oldTilemap = this->region_map->getTilemap();
            item->fill(event);
            QByteArray newTilemap = this->region_map->getTilemap();
            EditTilemap *command = new EditTilemap(this->region_map, oldTilemap, newTilemap, actionId_);
            command->setText("Fill Tilemap");
            this->region_map->commit(command);
        }
    } else {
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            actionId_++;
        } else {
            QByteArray oldTilemap = this->region_map->getTilemap();
            item->paint(event);
            QByteArray newTilemap = this->region_map->getTilemap();
            EditTilemap *command = new EditTilemap(this->region_map, oldTilemap, newTilemap, actionId_);
            this->region_map->commit(command);
        }
    }
}

void RegionMapEditor::on_tabWidget_Region_Map_currentChanged(int index) {
    this->ui->stackedWidget_RM_Options->setCurrentIndex(index);
    if (!this->region_map) return;
    switch (index)
    {
        case 0:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(true);
            if (this->region_map_item)
                this->region_map_item->draw();
            break;
        case 1:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(false);
            if (this->region_map_layout_item) {
                this->region_map_layout_item->draw();
                updateRegionMapLayoutOptions(this->currIndex);
            }
            break;
        case 2:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(false);
            on_comboBox_RM_Entry_MapSection_currentTextChanged(ui->comboBox_RM_Entry_MapSection->currentText());
            break;
    }
}

void RegionMapEditor::on_comboBox_RM_ConnectedMap_textActivated(const QString &mapsec) {
    this->region_map->setSquareMapSection(this->currIndex, mapsec);

    onRegionMapLayoutSelectedTileChanged(this->currIndex);// re-draw layout image
}

void RegionMapEditor::on_comboBox_RM_Entry_MapSection_currentTextChanged(const QString &text) {
    this->activeEntry = text;
    this->region_map_entries_item->currentSection = text;
    updateRegionMapEntryOptions(text);

    if (!this->region_map_entries.contains(text)) {
        this->region_map_entries_item->select(0);
        this->region_map_entries_item->draw();
        return;
    }
    int idx = this->region_map->getMapSquareIndex(this->region_map_entries[activeEntry].x + this->region_map->padLeft(),
                                                  this->region_map_entries[activeEntry].y + this->region_map->padTop());
    
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
}

void RegionMapEditor::on_comboBox_layoutLayer_textActivated(const QString &text) {
    this->region_map->setLayer(text);
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
}

void RegionMapEditor::on_spinBox_RM_Entry_x_valueChanged(int x) {
    if (!this->region_map_entries.contains(activeEntry)) return;
    MapSectionEntry oldEntry = this->region_map_entries[activeEntry];
    this->region_map_entries[activeEntry].x = x;
    MapSectionEntry newEntry = this->region_map_entries[activeEntry];
    EditEntry *commit = new EditEntry(this->region_map, activeEntry, oldEntry, newEntry);
    this->region_map->editHistory.push(commit);
    int idx = this->region_map->getMapSquareIndex(this->region_map_entries[activeEntry].x + this->region_map->padLeft(),
                                                  this->region_map_entries[activeEntry].y + this->region_map->padTop());
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
}

void RegionMapEditor::on_spinBox_RM_Entry_y_valueChanged(int y) {
    if (!this->region_map_entries.contains(activeEntry)) return;
    MapSectionEntry oldEntry = this->region_map_entries[activeEntry];
    this->region_map_entries[activeEntry].y = y;
    MapSectionEntry newEntry = this->region_map_entries[activeEntry];
    EditEntry *commit = new EditEntry(this->region_map, activeEntry, oldEntry, newEntry);
    this->region_map->editHistory.push(commit);
    int idx = this->region_map->getMapSquareIndex(this->region_map_entries[activeEntry].x + this->region_map->padLeft(),
                                                  this->region_map_entries[activeEntry].y + this->region_map->padTop());
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
}

void RegionMapEditor::on_spinBox_RM_Entry_width_valueChanged(int width) {
    if (!this->region_map_entries.contains(activeEntry)) return;
    MapSectionEntry oldEntry = this->region_map_entries[activeEntry];
    this->region_map_entries[activeEntry].width = width;
    MapSectionEntry newEntry = this->region_map_entries[activeEntry];
    EditEntry *commit = new EditEntry(this->region_map, activeEntry, oldEntry, newEntry);
    this->region_map->editHistory.push(commit);
    this->region_map_entries_item->draw();
}

void RegionMapEditor::on_spinBox_RM_Entry_height_valueChanged(int height) {
    if (!this->region_map_entries.contains(activeEntry)) return;
    MapSectionEntry oldEntry = this->region_map_entries[activeEntry];
    this->region_map_entries[activeEntry].height = height;
    MapSectionEntry newEntry = this->region_map_entries[activeEntry];
    EditEntry *commit = new EditEntry(this->region_map, activeEntry, oldEntry, newEntry);
    this->region_map->editHistory.push(commit);
    this->region_map_entries_item->draw();
}

void RegionMapEditor::on_spinBox_RM_LayoutWidth_valueChanged(int value) {
    if (this->region_map) {
        int oldWidth = this->region_map->layoutWidth();
        int oldHeight = this->region_map->layoutHeight();
        QMap<QString, QList<LayoutSquare>> oldLayouts = this->region_map->getAllLayouts();

        this->region_map->setLayoutDimensions(value, this->ui->spinBox_RM_LayoutHeight->value());

        int newWidth = this->region_map->layoutWidth();
        int newHeight = this->region_map->layoutHeight();
        QMap<QString, QList<LayoutSquare>> newLayouts = this->region_map->getAllLayouts();

        ResizeRMLayout *commit = new ResizeRMLayout(this->region_map, oldWidth, oldHeight, newWidth, newHeight, oldLayouts, newLayouts);
        this->region_map->editHistory.push(commit);
    }
}

void RegionMapEditor::on_spinBox_RM_LayoutHeight_valueChanged(int value) {
    if (this->region_map) {
        int oldWidth = this->region_map->layoutWidth();
        int oldHeight = this->region_map->layoutHeight();
        QMap<QString, QList<LayoutSquare>> oldLayouts = this->region_map->getAllLayouts();

        this->region_map->setLayoutDimensions(this->ui->spinBox_RM_LayoutWidth->value(), value);

        int newWidth = this->region_map->layoutWidth();
        int newHeight = this->region_map->layoutHeight();
        QMap<QString, QList<LayoutSquare>> newLayouts = this->region_map->getAllLayouts();

        ResizeRMLayout *commit = new ResizeRMLayout(this->region_map, oldWidth, oldHeight, newWidth, newHeight, oldLayouts, newLayouts);
        this->region_map->editHistory.push(commit);
    }
}

void RegionMapEditor::on_pushButton_RM_Options_delete_clicked() {
    int index = this->region_map->tilemapToLayoutIndex(this->currIndex);
    QList<LayoutSquare> oldLayout = this->region_map->getLayout(this->region_map->getLayer());
    this->region_map->resetSquare(index);
    QList<LayoutSquare> newLayout = this->region_map->getLayout(this->region_map->getLayer());
    EditLayout *commit = new EditLayout(this->region_map, this->region_map->getLayer(), this->currIndex, oldLayout, newLayout);
    commit->setText("Reset Layout Square");
    this->region_map->editHistory.push(commit);
    updateRegionMapLayoutOptions(this->currIndex);
    this->region_map_layout_item->draw();
    this->region_map_layout_item->select(this->currIndex);
    // ^ this line necessary?
}

void RegionMapEditor::on_spinBox_tilePalette_valueChanged(int value) {
    if (this->mapsquare_selector_item)
        this->mapsquare_selector_item->selectPalette(value);
}

void RegionMapEditor::setTileHFlip(bool enabled) {
    if (this->mapsquare_selector_item)
        this->mapsquare_selector_item->selectHFlip(enabled);
}

void RegionMapEditor::setTileVFlip(bool enabled) {
    if (this->mapsquare_selector_item)
        this->mapsquare_selector_item->selectVFlip(enabled);
}

void RegionMapEditor::on_action_RegionMap_Resize_triggered() {
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("New Tilemap Dimensions");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    // TODO: limits do not go smaller than layout
    QSpinBox *widthSpinBox = new QSpinBox;
    QSpinBox *heightSpinBox = new QSpinBox;
    widthSpinBox->setMinimum(16);
    heightSpinBox->setMinimum(16);
    widthSpinBox->setMaximum(128);
    heightSpinBox->setMaximum(128);
    widthSpinBox->setValue(this->region_map->tilemapWidth());
    heightSpinBox->setValue(this->region_map->tilemapHeight());
    form.addRow(new QLabel("Width"), widthSpinBox);
    form.addRow(new QLabel("Height"), heightSpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);

    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::rejected, &popup, &QDialog::reject);
    connect(&buttonBox, &QDialogButtonBox::accepted, &popup, &QDialog::accept);

    if (popup.exec() == QDialog::Accepted) {
        resizeTilemap(widthSpinBox->value(), heightSpinBox->value());
    }
    return;
}

void RegionMapEditor::resizeTilemap(int width, int height) {
    QByteArray oldTilemap = this->region_map->getTilemap();
    int oldWidth = this->region_map->tilemapWidth();
    int oldHeight = this->region_map->tilemapHeight();
    this->region_map->resizeTilemap(width, height);
    QByteArray newTilemap = this->region_map->getTilemap();
    int newWidth = this->region_map->tilemapWidth();
    int newHeight = this->region_map->tilemapHeight();
    ResizeTilemap *commit = new ResizeTilemap(this->region_map, oldTilemap, newTilemap, oldWidth, oldHeight, newWidth, newHeight);
    this->region_map->editHistory.push(commit);
    this->currIndex = this->region_map->padLeft() * width + this->region_map->padTop();
}

void RegionMapEditor::on_action_Swap_triggered() {
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("Swap Map Sections");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QComboBox *oldSecBox = new QComboBox();
    oldSecBox->addItems(this->project->locationNames());
    form.addRow(new QLabel("Map Section 1:"), oldSecBox);
    QComboBox *newSecBox = new QComboBox();
    newSecBox->addItems(this->project->locationNames());
    form.addRow(new QLabel("Map Section 2:"), newSecBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);
    form.addRow(&buttonBox);

    QString beforeSection, afterSection;
    connect(&buttonBox, &QDialogButtonBox::rejected, &popup, &QDialog::reject);
    connect(&buttonBox, &QDialogButtonBox::accepted, [&popup, &oldSecBox, &newSecBox, &beforeSection, &afterSection](){
        beforeSection = oldSecBox->currentText();
        afterSection = newSecBox->currentText();
        if (!beforeSection.isEmpty() && !afterSection.isEmpty() && beforeSection != afterSection) {
            popup.accept();
        }
    });

    if (popup.exec() == QDialog::Accepted) {
        QList<LayoutSquare> oldLayout = this->region_map->getLayout(this->region_map->getLayer());
        this->region_map->swapSections(beforeSection, afterSection);
        QList<LayoutSquare> newLayout = this->region_map->getLayout(this->region_map->getLayer());
        EditLayout *commit = new EditLayout(this->region_map, this->region_map->getLayer(), -5, oldLayout, newLayout);
        commit->setText("Swap Layout Sections " + beforeSection + " <<>> " + afterSection);
        this->region_map->editHistory.push(commit);
        displayRegionMapLayout();
        this->region_map_layout_item->select(this->region_map_layout_item->selectedTile);
    }
}

void RegionMapEditor::on_action_Replace_triggered() {
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("Replace Map Section");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QComboBox *oldSecBox = new QComboBox();
    oldSecBox->addItems(this->project->locationNames());
    form.addRow(new QLabel("Old Map Section:"), oldSecBox);
    QComboBox *newSecBox = new QComboBox();
    newSecBox->addItems(this->project->locationNames());
    form.addRow(new QLabel("New Map Section:"), newSecBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);
    form.addRow(&buttonBox);

    QString beforeSection, afterSection;
    connect(&buttonBox, &QDialogButtonBox::rejected, &popup, &QDialog::reject);
    connect(&buttonBox, &QDialogButtonBox::accepted, [&popup, &oldSecBox, &newSecBox, &beforeSection, &afterSection](){
        beforeSection = oldSecBox->currentText();
        afterSection = newSecBox->currentText();
        if (!beforeSection.isEmpty() && !afterSection.isEmpty() && beforeSection != afterSection) {
            popup.accept();
        }
    });

    if (popup.exec() == QDialog::Accepted) {
        QList<LayoutSquare> oldLayout = this->region_map->getLayout(this->region_map->getLayer());
        this->region_map->replaceSection(beforeSection, afterSection);
        QList<LayoutSquare> newLayout = this->region_map->getLayout(this->region_map->getLayer());
        EditLayout *commit = new EditLayout(this->region_map, this->region_map->getLayer(), -2, oldLayout, newLayout);
        commit->setText("Replace Layout Section " + beforeSection + " >> " + afterSection);
        this->region_map->editHistory.push(commit);
        displayRegionMapLayout();
        this->region_map_layout_item->select(this->region_map_layout_item->selectedTile);
    }
}

void RegionMapEditor::on_action_RegionMap_ClearImage_triggered() {
    QByteArray oldTilemap = this->region_map->getTilemap();
    this->region_map->clearImage();
    QByteArray newTilemap = this->region_map->getTilemap();
    
    EditTilemap *commit = new EditTilemap(this->region_map, oldTilemap, newTilemap, -1);
    commit->setText("Clear Tilemap");
    this->region_map->editHistory.push(commit);

    displayRegionMapImage();
    displayRegionMapLayout();
}

void RegionMapEditor::on_action_RegionMap_ClearLayout_triggered() {
    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        "WARNING",
        QString("This action will reset the entire map layout to %1, continue?").arg(this->region_map->default_map_section),
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Yes
    );

    if (result == QMessageBox::Yes) {
        QList<LayoutSquare> oldLayout = this->region_map->getLayout(this->region_map->getLayer());
        this->region_map->clearLayout();
        QList<LayoutSquare> newLayout = this->region_map->getLayout(this->region_map->getLayer());
        EditLayout *commit = new EditLayout(this->region_map, this->region_map->getLayer(), -1, oldLayout, newLayout);
        commit->setText("Clear Layout");
        this->region_map->editHistory.push(commit);
        displayRegionMapLayout();
    } else {
        return;
    }
}

void RegionMapEditor::on_action_RegionMap_ClearEntries_triggered() {
    QMessageBox::StandardButton result = QMessageBox::question(
        this,
        "WARNING",
        "This action will remove the entire mapsection entries list, continue?",
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Yes
    );

    if (result == QMessageBox::Yes) {
        ClearEntries *commit = new ClearEntries(this->region_map, this->region_map_entries);
        this->region_map->editHistory.push(commit);
        displayRegionMapLayout();
    } else {
        return;
    }
}

bool RegionMapEditor::modified() {
    return !this->history.isClean() || !this->configSaved;
}

void RegionMapEditor::closeEvent(QCloseEvent *event)
{
    if (this->modified()) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            QApplication::applicationName(),
            "The region map has been modified, save changes?",
            QMessageBox::No | QMessageBox::Yes | QMessageBox::Cancel,
            QMessageBox::Yes);

        if (result == QMessageBox::Yes) {
            this->on_action_RegionMap_Save_triggered();
            event->accept();
        } else if (result == QMessageBox::No) {
            event->accept();
        } else if (result == QMessageBox::Cancel) {
            event->ignore();
        }
    } else {
        event->accept();
    }

    porymapConfig.setRegionMapEditorGeometry(
        this->saveGeometry(),
        this->saveState()
    );
}

void RegionMapEditor::on_verticalSlider_Zoom_Map_Image_valueChanged(int val) {
    double scale = pow(scaleUpFactor, static_cast<double>(val - initialScale) / initialScale);

    QTransform transform;
    transform.scale(scale, scale);
    int width = ceil(static_cast<double>(this->region_map->pixelWidth()) * scale);
    int height = ceil(static_cast<double>(this->region_map->pixelHeight()) * scale);

    ui->graphicsView_Region_Map_BkgImg->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Region_Map_BkgImg->setTransform(transform);
    ui->graphicsView_Region_Map_BkgImg->setFixedSize(width + 2, height + 2);
    ui->graphicsView_Region_Map_Layout->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Region_Map_Layout->setTransform(transform);
    ui->graphicsView_Region_Map_Layout->setFixedSize(width + 2, height + 2);
    ui->graphicsView_Region_Map_Entries->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Region_Map_Entries->setTransform(transform);
    ui->graphicsView_Region_Map_Entries->setFixedSize(width + 2, height + 2);
}

void RegionMapEditor::on_verticalSlider_Zoom_Image_Tiles_valueChanged(int val) {
    double scale = pow(scaleUpFactor, static_cast<double>(val - initialScale) / initialScale);

    QTransform transform;
    transform.scale(scale, scale);
    int width = ceil(static_cast<double>(this->mapsquare_selector_item->pixelWidth) * scale);
    int height = ceil(static_cast<double>(this->mapsquare_selector_item->pixelHeight) * scale);

    ui->graphicsView_RegionMap_Tiles->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_RegionMap_Tiles->setTransform(transform);
    ui->graphicsView_RegionMap_Tiles->setFixedSize(width + 2, height + 2);
}

// Repopulate the combo boxes that display MAPSEC names.
void RegionMapEditor::setLocations(const QStringList &locations) {
    const QSignalBlocker b_ConnectedMap(ui->comboBox_RM_ConnectedMap);
    auto before = ui->comboBox_RM_ConnectedMap->currentText();
    ui->comboBox_RM_ConnectedMap->clear();
    ui->comboBox_RM_ConnectedMap->addItems(locations);
    ui->comboBox_RM_ConnectedMap->setTextItem(before);

    const QSignalBlocker b_MapSection(ui->comboBox_RM_Entry_MapSection);
    before = ui->comboBox_RM_Entry_MapSection->currentText();
    ui->comboBox_RM_Entry_MapSection->clear();
    ui->comboBox_RM_Entry_MapSection->addItems(locations);
    ui->comboBox_RM_Entry_MapSection->setTextItem(before);
}
