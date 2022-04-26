#include "regionmapeditor.h"
#include "ui_regionmapeditor.h"
#include "regionmappropertiesdialog.h"
#include "regionmapeditcommands.h"
#include "imageexport.h"
#include "shortcut.h"
#include "config.h"
#include "log.h"

#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QColor>
#include <QMessageBox>
#include <QTransform>
#include <math.h>

using OrderedJson = poryjson::Json;
using OrderedJsonDoc = poryjson::JsonDoc;

RegionMapEditor::RegionMapEditor(QWidget *parent, Project *project_) :
    QMainWindow(parent),
    ui(new Ui::RegionMapEditor)
{
    this->ui->setupUi(this);
    this->project = project_;
    this->ui->action_RegionMap_Resize->setVisible(false);
    this->initShortcuts();
    this->restoreWindowState();
    //on_verticalSlider_Zoom_Map_Image_valueChanged(50);
}

RegionMapEditor::~RegionMapEditor()
{
    delete ui;
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
    delete city_map_selector_item;
    delete city_map_item;
    delete scene_city_map_tiles;
    delete scene_city_map_image;
    delete scene_region_map_layout;
    delete scene_region_map_tiles;
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

    connect(&(this->history), &QUndoGroup::indexChanged, [this](int) {
        on_tabWidget_Region_Map_currentChanged(this->ui->tabWidget_Region_Map->currentIndex());
    });

    shortcutsConfig.load();
    shortcutsConfig.setDefaultShortcuts(shortcutableObjects());
    applyUserShortcuts();
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
    this->region_map_entries.clear();

    QString regionMapSectionFilepath = QString("%1/src/data/region_map/region_map_sections.json").arg(this->project->root);

    ParseUtil parser;
    QJsonDocument sectionsDoc;
    if (!parser.tryParseJsonFile(&sectionsDoc, regionMapSectionFilepath)) {
        logError(QString("Failed to read map data from %1").arg(regionMapSectionFilepath));
        return false;
    }

    // for some unknown reason, the OrderedJson class would not parse this properly
    // perhaps updating nlohmann/json here would fix it, but that also requires using C++17
    QJsonObject object = sectionsDoc.object();

    for (auto entryRef : object["map_sections"].toArray()) {
        QJsonObject entryObject = entryRef.toObject();
        QString entryMapSection = entryObject["map_section"].toString();
        MapSectionEntry entry;
        entry.name = entryObject["name"].toString();
        entry.x = entryObject["x"].toInt();
        entry.y = entryObject["y"].toInt();
        entry.width = entryObject["width"].toInt();
        entry.height = entryObject["height"].toInt();
        entry.valid = true;
        this->region_map_entries[entryMapSection] = entry;
    }
}

bool RegionMapEditor::saveRegionMapEntries() {
    QString regionMapSectionFilepath = QString("%1/%2").arg(this->project->root).arg(this->region_map->entriesPath());

    QFile sectionsFile(regionMapSectionFilepath);
    if (!sectionsFile.open(QIODevice::WriteOnly)) {
        logError(QString("Error: Could not open %1 for writing").arg(regionMapSectionFilepath));
        return false;
    }

    OrderedJson::object object;
    OrderedJson::array mapSectionArray;

    for (auto pair : this->region_map_entries) {
        QString section = pair.first;
        MapSectionEntry entry = pair.second;

        OrderedJson::object entryObject;
        entryObject["map_section"] = section;
        entryObject["name"] = entry.name;
        entryObject["x"] = entry.x;
        entryObject["y"] = entry.y;
        entryObject["width"] = entry.width;
        entryObject["height"] = entry.height;

        mapSectionArray.append(entryObject);
    }

    object["map_sections"] = mapSectionArray;

    OrderedJson sectionsJson(object);
    OrderedJsonDoc jsonDoc(&sectionsJson);
    jsonDoc.dump(&sectionsFile);
    sectionsFile.close();

    return true;
}

void buildEmeraldDefaults(poryjson::Json &json) {
    ParseUtil parser;
    QString emeraldDefault = parser.readTextFile(":/text/region_map_default_emerald.json");

    QString err;
    json = poryjson::Json::parse(emeraldDefault, err);

    // TODO: error check these
}

void buildFireredDefaults(poryjson::Json &json) {

    ParseUtil parser;
    QString fireredDefault = parser.readTextFile(":/text/region_map_default_firered.json");    

    QString err;
    json = poryjson::Json::parse(fireredDefault, err);
}

poryjson::Json RegionMapEditor::buildDefaultJson() {
    poryjson::Json defaultJson;
    switch (projectConfig.getBaseGameVersion()) {
        case BaseGameVersion::pokeemerald:
            buildEmeraldDefaults(defaultJson);
            break;

        case BaseGameVersion::pokeruby:
            // TODO: pokeruby defaults
            break;

        case BaseGameVersion::pokefirered:
            buildFireredDefaults(defaultJson);
            break;
    }

    return defaultJson;
}

void RegionMapEditor::buildConfigDialog() {
    // use this temporary object while window is active, save only when user accepts
    poryjson::Json rmConfigJsonUpdate = this->rmConfigJson;

    QDialog dialog(this);
    dialog.setWindowTitle("Configure Region Maps");
    dialog.setWindowModality(Qt::ApplicationModal);

    QFormLayout form(&dialog);

    // need to display current region map(s) if there are some
    QListWidget *regionMapList = new QListWidget(&dialog);

    form.addRow(new QLabel("Region Map List:"));
    form.addRow(regionMapList);

    // lambda to update the current map list displayed in the config window with a
    // widget item that contains that region map's json data
    auto updateMapList = [this, regionMapList, &rmConfigJsonUpdate]() {
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

    // when double clicking a region map from the list, bring up the configuration window 
    // and populate it with the current values
    // the user can edit and save the config for an existing map this way
    connect(regionMapList, &QListWidget::itemDoubleClicked, [this, &rmConfigJsonUpdate, updateMapList, regionMapList](QListWidgetItem *item) {
        int itemIndex = regionMapList->indexFromItem(item).row();

        QString err;
        poryjson::Json clickedJson = poryjson::Json::parse(item->data(Qt::UserRole).toString(), err);

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
        } else {
            // TODO: anything?
        }
    });

    QPushButton *addMapButton = new QPushButton("Add Region Map...");
    form.addRow(addMapButton);

    // allow user to add region maps
    connect(addMapButton, &QPushButton::clicked, [this, regionMapList] {
        poryjson::Json resultJson = configRegionMapDialog();
        poryjson::Json::object resultObj = resultJson.object_items();

        // TODO: check duplicate alias? other error-checking

        QString resultStr;
        int tab = 0;
        resultJson.dump(resultStr, &tab);

        QListWidgetItem *newItem = new QListWidgetItem;
        newItem->setText(resultObj["alias"].string_value());
        newItem->setData(Qt::UserRole, resultStr);
        regionMapList->addItem(newItem);
    });

    // TODO: city maps (leaving this for now)
    // QListWidget *cityMapList = new QListWidget(&dialog);
    // // TODO: double clicking can delete them? bring up the tiny menu thing like sory by maps
    // //cityMapList->setSelectionMode(QAbstractItemView::NoSelection);
    // //cityMapList->setFocusPolicy(Qt::NoFocus);
    // form.addRow(new QLabel("City Map List:"));
    // form.addRow(cityMapList);

    // for (int i = 0; i < 20; i++) {
    //     QListWidgetItem *newItem = new QListWidgetItem;
    //     newItem->setText(QString("city %1").arg(i));
    //     cityMapList->addItem(newItem);
    // }

    // QPushButton *addCitiesButton = new QPushButton("Configure City Maps...");
    // form.addRow(addCitiesButton);
    // // needs to pick tilemaps and such here too


    // for sake of convenience, option to just use defaults for each basegame version
    QPushButton *config_useProjectDefault;
    switch (projectConfig.getBaseGameVersion()) {
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
    } else {
        // TODO: return bool from this function so can close RME on error
    }
}

// TODO: set tooltips, default values in spinboxes
poryjson::Json RegionMapEditor::configRegionMapDialog() {

    RegionMapPropertiesDialog dialog(this);
    dialog.setProject(this->project);

    if (dialog.exec() == QDialog::Accepted) {
        poryjson::Json regionMapJson = dialog.saveToJson();
        return regionMapJson;
    } else {
        // TODO: anything?
    }

    return poryjson::Json();
}

void RegionMapEditor::buildUpdateConfigDialog() {
    // unused
    // TODO: get rid of this function
}

poryjson::Json RegionMapEditor::getJsonFromAlias(QString alias) {
    // unused
    // TODO: get a map Json from an alias
}

bool RegionMapEditor::load() {
    // check for config json file
    QString jsonConfigFilepath = this->project->root + "/src/data/region_map/region_map_config.json";
    if (!QFile::exists(jsonConfigFilepath)) {
        logWarn("Region Map config file not found.");

        // show popup explaining next window
        QMessageBox warning;
        warning.setText("Region map configuration not found.");
        warning.setInformativeText("In order to continue, you must setup porymap to use your region map.");
        warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        warning.setDefaultButton(QMessageBox::Ok);

        if (warning.exec() == QMessageBox::Ok) {
            // there is a separate window that allows to load multiple region maps, 
            buildConfigDialog();

            // TODO: need to somehow handle (by making above function return bool?)
            //       when user closes the config dialog without setting up maps so we
            //       can return false from here in that case as well
        } else {
            // TODO: Do not open region map editor (maybe just return false is sufficient?)
            return false;
        }
    }
    else {
        logInfo("Region map configuration file found.");

        // TODO: load rmConfigJson from the config file, and verify it
    }

    // if all has gone well, this->rmConfigJson should be populated
    // next, load the entries
    loadRegionMapEntries();

    // load the region maps into this->region_maps
    poryjson::Json::object regionMapObjectCopy = this->rmConfigJson.object_items();
    for (auto o : regionMapObjectCopy["region_maps"].array_items()) {
        // TODO: could this crash?
        QString alias = o.object_items().at("alias").string_value();

        RegionMap *newMap = new RegionMap(this->project);
        // TODO: is there a better way than this pointer to get entries to pixmap item? (surely)
        newMap->setEntries(&this->region_map_entries);
        newMap->loadMapData(o);

        region_maps[alias] = newMap;

        this->history.addStack(&(newMap->editHistory));
    }

    // add to ui
    for (auto p : region_maps) {
        ui->comboBox_regionSelector->addItem(p.first);
    }

    // display the first region map in the list
    if (!region_maps.empty()) {
        this->region_map = region_maps.begin()->second;
        this->currIndex = this->region_map->firstLayoutIndex();
        this->region_map->editHistory.setActive();

        displayRegionMap();
    }

    return true;
}

bool RegionMapEditor::loadRegionMapData() {
    // unused
    // TODO: city maps
}

bool RegionMapEditor::loadCityMaps() {
    // unused
    // TODO

    return false;
}

void RegionMapEditor::on_action_RegionMap_Save_triggered() {
    // TODO: save current region map, add "Save All" to save all region maps
    // TODO: save the config json as well
    this->region_map->save();

    // save entries
}

void RegionMapEditor::setCurrentSquareOptions() {
    // TODO

}

void RegionMapEditor::displayRegionMap() {
    displayRegionMapTileSelector();
    //< displayCityMapTileSelector();

    displayRegionMapImage();
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
    displayRegionMapEntriesImage();
    displayRegionMapEntryOptions();

    updateLayerDisplayed();

    this->hasUnsavedChanges = false;
}

void RegionMapEditor::updateLayerDisplayed() {
    // add layers from layout to combo
    ui->comboBox_layoutLayer->clear();
    ui->comboBox_layoutLayer->addItems(this->region_map->getLayers());
}

void RegionMapEditor::on_comboBox_regionSelector_textActivated(const QString &region) {
    //
    if (this->region_maps.contains(region)) {
        this->region_map = region_maps.at(region);
        this->region_map->editHistory.setActive();
        this->currIndex = this->region_map->firstLayoutIndex();
        // TODO: make the above into a function that takes an alias string? in case there is more to it

        // TODO: anything else needed here?
        displayRegionMap();

        //this->editGroup.setActiveStack(&(this->region_map->editHistory));
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
    this->ui->comboBox_RM_ConnectedMap->clear();
    this->ui->comboBox_RM_ConnectedMap->addItems(this->project->mapSectionValueToName.values());

    this->ui->frame_RM_Options->setEnabled(true);

    updateRegionMapLayoutOptions(this->currIndex);
}

void RegionMapEditor::updateRegionMapLayoutOptions(int index) {
    this->ui->comboBox_RM_ConnectedMap->blockSignals(true);
    this->ui->comboBox_RM_ConnectedMap->setCurrentText(this->region_map->squareMapSection(index));
    this->ui->comboBox_RM_ConnectedMap->blockSignals(false);
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
    this->ui->comboBox_RM_Entry_MapSection->addItems(this->project->mapSectionValueToName.values());
    int width = this->region_map->tilemapWidth() - this->region_map->padLeft() - this->region_map->padRight();
    int height = this->region_map->tilemapHeight() - this->region_map->padTop() - this->region_map->padBottom();
    this->ui->spinBox_RM_Entry_x->setMaximum(width - 1);
    this->ui->spinBox_RM_Entry_y->setMaximum(height - 1);
    this->ui->spinBox_RM_Entry_width->setMinimum(1);
    this->ui->spinBox_RM_Entry_height->setMinimum(1);
    this->ui->spinBox_RM_Entry_width->setMaximum(width);
    this->ui->spinBox_RM_Entry_height->setMaximum(height);
}

void RegionMapEditor::updateRegionMapEntryOptions(QString section) {
    bool enabled = (section != "MAPSEC_NONE") && (this->region_map_entries.contains(section));

    this->ui->lineEdit_RM_MapName->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_x->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_y->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_width->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_height->setEnabled(enabled);

    this->ui->lineEdit_RM_MapName->blockSignals(true);
    this->ui->spinBox_RM_Entry_x->blockSignals(true);
    this->ui->spinBox_RM_Entry_y->blockSignals(true);
    this->ui->spinBox_RM_Entry_width->blockSignals(true);
    this->ui->spinBox_RM_Entry_height->blockSignals(true);

    this->ui->comboBox_RM_Entry_MapSection->setCurrentText(section);
    this->activeEntry = section;
    this->region_map_entries_item->currentSection = section;
    MapSectionEntry entry = enabled ? this->region_map_entries[section] : MapSectionEntry();
    this->ui->lineEdit_RM_MapName->setText(entry.name);
    this->ui->spinBox_RM_Entry_x->setValue(entry.x);
    this->ui->spinBox_RM_Entry_y->setValue(entry.y);
    this->ui->spinBox_RM_Entry_width->setValue(entry.width);
    this->ui->spinBox_RM_Entry_height->setValue(entry.height);

    // TODO: if not enabled, button to enable, otherwise button to disable / remove entry from map

    this->ui->lineEdit_RM_MapName->blockSignals(false);
    this->ui->spinBox_RM_Entry_x->blockSignals(false);
    this->ui->spinBox_RM_Entry_y->blockSignals(false);
    this->ui->spinBox_RM_Entry_width->blockSignals(false);
    this->ui->spinBox_RM_Entry_height->blockSignals(false);
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

    this->mapsquare_selector_item->draw();

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

void RegionMapEditor::displayCityMapTileSelector() {
    return;
    /*
    if (!scene_city_map_tiles) {
        this->scene_city_map_tiles = new QGraphicsScene;
    }
    if (city_map_selector_item && scene_city_map_tiles) {
        scene_city_map_tiles->removeItem(city_map_selector_item);
        delete city_map_selector_item;
    }

    this->city_map_selector_item = new TilemapTileSelector(QPixmap(this->region_map->cityTilesPath()));
    this->city_map_selector_item->draw();

    this->scene_city_map_tiles->addItem(this->city_map_selector_item);

    connect(this->city_map_selector_item, &TilemapTileSelector::selectedTileChanged,
            this, &RegionMapEditor::onCityMapTileSelectorSelectedTileChanged);

    this->ui->graphicsView_City_Map_Tiles->setScene(this->scene_city_map_tiles);
    on_verticalSlider_Zoom_City_Tiles_valueChanged(this->ui->verticalSlider_Zoom_City_Tiles->value());

    this->city_map_selector_item->select(this->selectedCityTile);
    */
}

void RegionMapEditor::displayCityMap(QString f) {
    return;
    /*
    QString file = this->project->root + "/graphics/pokenav/city_maps/" + f + ".bin";

    if (!scene_city_map_image) {
        scene_city_map_image = new QGraphicsScene;
    }
    if (city_map_item && scene_city_map_image) {
        scene_city_map_image->removeItem(city_map_item);
        delete city_map_item;
    }

    city_map_item = new CityMapPixmapItem(file, this->city_map_selector_item);
    city_map_item->draw();

    connect(this->city_map_item, &CityMapPixmapItem::mouseEvent,
            this, &RegionMapEditor::mouseEvent_city_map);

    scene_city_map_image->addItem(city_map_item);
    scene_city_map_image->setSceneRect(this->scene_city_map_image->sceneRect());

    this->ui->graphicsView_City_Map->setScene(scene_city_map_image);
    on_verticalSlider_Zoom_City_Map_valueChanged(this->ui->verticalSlider_Zoom_City_Map->value());
    */
}

bool RegionMapEditor::createCityMap(QString name) {
    return false;
    /*
    bool errored = false;

    QString file = this->project->root + "/graphics/pokenav/city_maps/" + name + ".bin";

    uint8_t filler = 0x30;
    uint8_t border = 0x7;
    uint8_t blank  = 0x1;
    QByteArray new_data(400, filler);
    for (int i = 0; i < new_data.size(); i++) {
        if (i % 2) continue;
        int x = i % 20;
        int y = i / 20;
        if (y <= 1 || y >= 8 || x <= 3 || x >= 16)
            new_data[i] = border;
        else
            new_data[i] = blank;
    }

    QFile binFile(file);
    if (!binFile.open(QIODevice::WriteOnly)) errored = true;
    binFile.write(new_data);
    binFile.close();

    loadCityMaps();
    this->ui->comboBox_CityMap_picker->setCurrentText(name);

    return !errored;
    */
}

bool RegionMapEditor::tryInsertNewMapEntry(QString mapsec) {
    // TODO
    return false;
}

void RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged(unsigned id) {
    this->selectedImageTile = id;
}

void RegionMapEditor::onCityMapTileSelectorSelectedTileChanged(unsigned id) {
    // TODO
    //< this->selectedCityTile = id;
}

void RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged(unsigned tileId) {
    QString message = QString("Tile: 0x") + QString("%1").arg(tileId, 4, 16, QChar('0')).toUpper();
    this->ui->statusbar->showMessage(message);
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
    setCurrentSquareOptions();
    QString message = QString();
    this->currIndex = index;
    this->region_map_layout_item->highlightedTile = index;
    if (this->region_map->squareHasMap(index)) {
        message = QString("\t %1").arg(this->project->mapSecToMapHoverName.value(
                      this->region_map->squareMapSection(index))).remove("{NAME_END}");
    }
    this->ui->statusbar->showMessage(message);

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
            message += QString("\t %1").arg(this->project->mapSecToMapHoverName.value(
                           this->region_map->squareMapSection(index))).remove("{NAME_END}");
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
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    int index = this->region_map->getMapSquareIndex(x, y);
    if (index > this->region_map->tilemapSize() - 1) return; // TODO: is the math correct here?

    if (event->buttons() & Qt::RightButton) {
        item->select(event);
    //} else if (event->buttons() & Qt::MiddleButton) {// TODO
    } else {
        item->paint(event);
        this->region_map_layout_item->draw();
        this->hasUnsavedChanges = true;
    }
}

void RegionMapEditor::mouseEvent_city_map(QGraphicsSceneMouseEvent *event, CityMapPixmapItem *item) {
    return;
    //< TODO: city maps
    /*
    if (cityMapFirstDraw) {
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(
            RegionMapEditorBox::CityMapImage, this->city_map_item->getTiles(), this->city_map_item->file
        );
        history.push(commit);
        cityMapFirstDraw = false;
    }

    if (event->buttons() & Qt::RightButton) {// TODO
    //} else if (event->buttons() & Qt::MiddleButton) {// TODO
    } else {
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            RegionMapHistoryItem *current = history.current();
            bool addToHistory = !(current && current->tiles == this->city_map_item->getTiles());
            if (addToHistory) {
                RegionMapHistoryItem *commit = new RegionMapHistoryItem(
                    RegionMapEditorBox::CityMapImage, this->city_map_item->getTiles(), this->city_map_item->file
                );
                history.push(commit);
            }
        } else {
            item->paint(event);
            this->hasUnsavedChanges = true;
        }
    }
    //*/
}

void RegionMapEditor::on_tabWidget_Region_Map_currentChanged(int index) {
    this->ui->stackedWidget_RM_Options->setCurrentIndex(index);
    if (!region_map) return;
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
            on_comboBox_RM_Entry_MapSection_textActivated(ui->comboBox_RM_Entry_MapSection->currentText());
            break;
    }
}

void RegionMapEditor::on_comboBox_RM_ConnectedMap_textActivated(const QString &mapsec) {
    QString layer = this->region_map->getLayer();
    QList<LayoutSquare> oldLayout = this->region_map->getLayout(layer);
    this->region_map->setSquareMapSection(this->currIndex, mapsec);
    QList<LayoutSquare> newLayout = this->region_map->getLayout(layer);

    EditLayout *command = new EditLayout(this->region_map, layer, this->currIndex, oldLayout, newLayout);
    this->region_map->commit(command);

    this->hasUnsavedChanges = true;// TODO: sometimes this is called for unknown reasons

    onRegionMapLayoutSelectedTileChanged(this->currIndex);// re-draw layout image
}

void RegionMapEditor::on_comboBox_RM_Entry_MapSection_textActivated(const QString &text) {
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
    // TODO: verify whether layer is legit
    this->region_map->setLayer(text);
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
}

void RegionMapEditor::on_spinBox_RM_Entry_x_valueChanged(int x) {
    //tryInsertNewMapEntry(activeEntry);
    if (!this->region_map_entries.contains(activeEntry)) return;
    this->region_map_entries[activeEntry].x = x;
    int idx = this->region_map->getMapSquareIndex(this->region_map_entries[activeEntry].x + this->region_map->padLeft(),
                                                  this->region_map_entries[activeEntry].y + this->region_map->padTop());
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
    this->ui->spinBox_RM_Entry_width->setMaximum(this->region_map->tilemapWidth() - this->region_map->padLeft() - this->region_map->padRight() - x);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Entry_y_valueChanged(int y) {
    //tryInsertNewMapEntry(activeEntry);
    if (!this->region_map_entries.contains(activeEntry)) return;
    this->region_map_entries[activeEntry].y = y;
    int idx = this->region_map->getMapSquareIndex(this->region_map_entries[activeEntry].x + this->region_map->padLeft(),
                                                  this->region_map_entries[activeEntry].y + this->region_map->padTop());
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
    this->ui->spinBox_RM_Entry_height->setMaximum(this->region_map->tilemapHeight() - this->region_map->padTop() - this->region_map->padBottom() - y);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Entry_width_valueChanged(int width) {
    //tryInsertNewMapEntry(activeEntry);
    if (!this->region_map_entries.contains(activeEntry)) return;
    this->region_map_entries[activeEntry].width = width;
    this->region_map_entries_item->draw();
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Entry_height_valueChanged(int height) {
    //tryInsertNewMapEntry(activeEntry);
    if (!this->region_map_entries.contains(activeEntry)) return;
    this->region_map_entries[activeEntry].height = height;
    this->region_map_entries_item->draw();
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_lineEdit_RM_MapName_textEdited(const QString &text) {
    if (!this->region_map_entries.contains(activeEntry)) return;
    this->region_map_entries[this->ui->comboBox_RM_Entry_MapSection->currentText()].name = text;
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_pushButton_RM_Options_delete_clicked() {
    // TODO: crashing
    int index = this->region_map->tilemapToLayoutIndex(this->currIndex);
    QList<LayoutSquare> oldLayout = this->region_map->getLayout(this->region_map->getLayer());
    //this->region_map->resetSquare(this->region_map_layout_item->selectedTile);
    this->region_map->resetSquare(index);
    QList<LayoutSquare> newLayout = this->region_map->getLayout(this->region_map->getLayer());
    EditLayout *commit = new EditLayout(this->region_map, this->region_map->getLayer(), this->currIndex, oldLayout, newLayout);
    commit->setText("Reset Layout Square");
    this->region_map->editHistory.push(commit);
    //updateRegionMapLayoutOptions(this->region_map_layout_item->selectedTile);
    updateRegionMapLayoutOptions(this->currIndex);
    this->region_map_layout_item->draw();
    //this->region_map_layout_item->select(this->region_map_layout_item->selectedTile);
    this->region_map_layout_item->select(this->currIndex);
    // ^ this line necessary?
    this->hasUnsavedChanges = true;
}

// TODO: check value bounds for current palette?
void RegionMapEditor::on_spinBox_tilePalette_valueChanged(int value) {
    this->mapsquare_selector_item->selectPalette(value);
}

void RegionMapEditor::on_checkBox_tileHFlip_stateChanged(int state) {
    this->mapsquare_selector_item->selectHFlip(state == Qt::Checked);
}

void RegionMapEditor::on_checkBox_tileVFlip_stateChanged(int state) {
    this->mapsquare_selector_item->selectVFlip(state == Qt::Checked);
}

void RegionMapEditor::on_pushButton_CityMap_add_clicked() {
    return;
    //< TODO: city maps
    /*
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("New City Map");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QLineEdit *input = new QLineEdit();
    input->setClearButtonEnabled(true);
    form.addRow(new QLabel("Name:"), input);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);

    QString name;

    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::rejected, &popup, &QDialog::reject);
    connect(&buttonBox, &QDialogButtonBox::accepted, [&popup, &input, &name](){
        name = input->text().remove(QRegularExpression("[^a-zA-Z0-9_]+"));
        if (!name.isEmpty())
            popup.accept();
    });

    if (popup.exec() == QDialog::Accepted) {
        createCityMap(name);
    }

    this->hasUnsavedChanges = true;
    //*/
}

void RegionMapEditor::on_action_RegionMap_Resize_triggered() {
    // TODO: this whole feature
    /*
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("New Region Map Dimensions");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QSpinBox *widthSpinBox = new QSpinBox();
    QSpinBox *heightSpinBox = new QSpinBox();
    widthSpinBox->setMinimum(32);
    heightSpinBox->setMinimum(20);
    widthSpinBox->setMaximum(128);// TODO: find real limits... 128
    heightSpinBox->setMaximum(128);
    // TODO width, height
    widthSpinBox->setValue(this->region_map->width());
    heightSpinBox->setValue(this->region_map->height());
    form.addRow(new QLabel("Width"), widthSpinBox);
    form.addRow(new QLabel("Height"), heightSpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);

    form.addRow(&buttonBox);
    connect(&buttonBox, &QDialogButtonBox::rejected, &popup, &QDialog::reject);
    connect(&buttonBox, &QDialogButtonBox::accepted, &popup, &QDialog::accept);

    if (popup.exec() == QDialog::Accepted) {
        resize(widthSpinBox->value(), heightSpinBox->value());
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(
            RegionMapEditorBox::BackgroundImage, this->region_map->getTiles(), widthSpinBox->value(), heightSpinBox->value()
        );
        history.push(commit);
    }

    this->hasUnsavedChanges = true;
    */
    return;
}

void RegionMapEditor::resize(int w, int h) {
    this->region_map->resize(w, h);
    this->currIndex = this->region_map->padLeft() * w + this->region_map->padTop();
    displayRegionMapImage();
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
}

void RegionMapEditor::on_action_Swap_triggered() {
    // TODO: does this function still work?
    // TODO: fix for string ids
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("Swap Map Sections");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QComboBox *oldSecBox = new QComboBox();
    oldSecBox->addItems(this->project->mapSectionValueToName.values());
    form.addRow(new QLabel("Old Map Section:"), oldSecBox);
    QComboBox *newSecBox = new QComboBox();
    newSecBox->addItems(this->project->mapSectionValueToName.values());
    form.addRow(new QLabel("New Map Section:"), newSecBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);
    form.addRow(&buttonBox);

    QString beforeSection, afterSection;
    uint8_t oldId, newId; 
    connect(&buttonBox, &QDialogButtonBox::rejected, &popup, &QDialog::reject);
    connect(&buttonBox, &QDialogButtonBox::accepted, [this, &popup, &oldSecBox, &newSecBox, 
                                                      &beforeSection, &afterSection, &oldId, &newId](){
        beforeSection = oldSecBox->currentText();
        afterSection = newSecBox->currentText();
        if (!beforeSection.isEmpty() && !afterSection.isEmpty()) {
            oldId = static_cast<uint8_t>(this->project->mapSectionNameToValue.value(beforeSection));
            newId = static_cast<uint8_t>(this->project->mapSectionNameToValue.value(afterSection));
            popup.accept();
        }
    });

    if (popup.exec() == QDialog::Accepted) {
        this->region_map->replaceSectionId(oldId, newId);
        displayRegionMapLayout();
        this->region_map_layout_item->select(this->region_map_layout_item->selectedTile);
        this->hasUnsavedChanges = true;
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
        "This action will reset the entire map layout to MAPSEC_NONE, continue?",
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

void RegionMapEditor::on_comboBox_CityMap_picker_currentTextChanged(const QString &file) {
    //< TODO: city maps
    //< this->displayCityMap(file);
    //< this->cityMapFirstDraw = true;
}

void RegionMapEditor::closeEvent(QCloseEvent *event)
{
    if (this->hasUnsavedChanges) {
        QMessageBox::StandardButton result = QMessageBox::question(
            this,
            "porymap",
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
