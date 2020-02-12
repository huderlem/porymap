#include "regionmapeditor.h"
#include "ui_regionmapeditor.h"
#include "imageexport.h"
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
#include <math.h>

RegionMapEditor::RegionMapEditor(QWidget *parent, Project *project_) :
    QMainWindow(parent),
    ui(new Ui::RegionMapEditor)
{
    this->ui->setupUi(this);
    this->project = project_;
    this->region_map = new RegionMap;
    this->ui->action_RegionMap_Resize->setVisible(false);
}

RegionMapEditor::~RegionMapEditor()
{
    delete ui;
    delete region_map;
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

void RegionMapEditor::on_action_RegionMap_Save_triggered() {
    setCurrentSquareOptions();
    if (project && region_map) {
        this->region_map->save();
        this->city_map_item->save();
    }
    this->hasUnsavedChanges = false;
}

void RegionMapEditor::setCurrentSquareOptions() {
    if (project && region_map) {
        this->region_map->saveOptions(
            this->currIndex,
            this->ui->comboBox_RM_ConnectedMap->currentText(),
            this->ui->lineEdit_RM_MapName->text(),
            this->region_map->map_squares[this->currIndex].x,
            this->region_map->map_squares[this->currIndex].y
        );
    }
}

bool RegionMapEditor::loadRegionMapData() {
    if (!this->region_map->init(project)) {
        return false;
    }

    this->currIndex = this->region_map->width() * this->region_map->padTop + this->region_map->padLeft;
    displayRegionMap();
    return true;
}

bool RegionMapEditor::loadCityMaps() {
    QDir directory(project->root + "/graphics/pokenav/city_maps");
    QStringList files = directory.entryList(QStringList() << "*.bin", QDir::Files);
    QStringList without_bin;
    for (QString file : files) {
        without_bin.append(file.remove(".bin"));
    }
    this->ui->comboBox_CityMap_picker->addItems(without_bin);
    return true;
}

void RegionMapEditor::displayRegionMap() {
    displayRegionMapTileSelector();
    displayCityMapTileSelector();
    displayRegionMapImage();
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
    displayRegionMapEntriesImage();
    displayRegionMapEntryOptions();
    this->hasUnsavedChanges = false;
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

    if (regionMapFirstDraw) {
        on_verticalSlider_Zoom_Map_Image_valueChanged(this->ui->verticalSlider_Zoom_Map_Image->value());
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(
            RegionMapEditorBox::BackgroundImage, this->region_map->getTiles(), this->region_map->width(), this->region_map->height()
        );
        history.push(commit);
        regionMapFirstDraw = false;
    }
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

    updateRegionMapLayoutOptions(this->currIndex);
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

    // TODO: implement when the code is decompiled
    this->ui->label_RM_CityMap->setVisible(false);
    this->ui->comboBox_RM_CityMap->setVisible(false);
}

void RegionMapEditor::updateRegionMapLayoutOptions(int index) {
    this->ui->comboBox_RM_ConnectedMap->blockSignals(true);
    this->ui->lineEdit_RM_MapName->setText(this->project->mapSecToMapHoverName->value(this->region_map->map_squares[index].mapsec));
    this->ui->comboBox_RM_ConnectedMap->setCurrentText(this->region_map->map_squares[index].mapsec);
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
        QString first = this->project->mapSectionValueToName.first();
        this->region_map_entries_item->currentSection = first;
        this->activeEntry = first;
        updateRegionMapEntryOptions(first);
        entriesFirstDraw = false;
    }

    this->region_map_entries_item->draw();

    int idx = this->region_map->getMapSquareIndex(this->region_map->mapSecToMapEntry.value(activeEntry).x + this->region_map->padLeft,
                                                  this->region_map->mapSecToMapEntry.value(activeEntry).y + this->region_map->padTop);
    this->region_map_entries_item->select(idx);

    this->scene_region_map_entries->addItem(region_map_entries_item);
    this->scene_region_map_entries->setSceneRect(this->scene_region_map_entries->itemsBoundingRect());

    this->ui->graphicsView_Region_Map_Entries->setScene(this->scene_region_map_entries);
}

void RegionMapEditor::displayRegionMapEntryOptions() {
    this->ui->comboBox_RM_Entry_MapSection->addItems(this->project->mapSectionValueToName.values());
    int width = this->region_map->width() - this->region_map->padLeft - this->region_map->padRight;
    int height = this->region_map->height() - this->region_map->padTop - this->region_map->padBottom;
    this->ui->spinBox_RM_Entry_x->setMaximum(width - 1);
    this->ui->spinBox_RM_Entry_y->setMaximum(height - 1);
    this->ui->spinBox_RM_Entry_width->setMinimum(1);
    this->ui->spinBox_RM_Entry_height->setMinimum(1);
    this->ui->spinBox_RM_Entry_width->setMaximum(width);
    this->ui->spinBox_RM_Entry_height->setMaximum(height);
}

void RegionMapEditor::updateRegionMapEntryOptions(QString section) {
    bool enabled = section == "MAPSEC_NONE" ? false : true;

    this->ui->spinBox_RM_Entry_x->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_y->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_width->setEnabled(enabled);
    this->ui->spinBox_RM_Entry_height->setEnabled(enabled);

    this->ui->spinBox_RM_Entry_x->blockSignals(true);
    this->ui->spinBox_RM_Entry_y->blockSignals(true);
    this->ui->spinBox_RM_Entry_width->blockSignals(true);
    this->ui->spinBox_RM_Entry_height->blockSignals(true);

    this->ui->comboBox_RM_Entry_MapSection->setCurrentText(section);
    this->activeEntry = section;
    this->region_map_entries_item->currentSection = section;
    RegionMapEntry entry = this->region_map->mapSecToMapEntry.value(section);
    this->ui->spinBox_RM_Entry_x->setValue(entry.x);
    this->ui->spinBox_RM_Entry_y->setValue(entry.y);
    this->ui->spinBox_RM_Entry_width->setValue(entry.width);
    this->ui->spinBox_RM_Entry_height->setValue(entry.height);

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

    this->mapsquare_selector_item = new TilemapTileSelector(QPixmap(this->region_map->pngPath()));
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

    this->mapsquare_selector_item->select(this->selectedImageTile);
}

void RegionMapEditor::displayCityMapTileSelector() {
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
}

void RegionMapEditor::displayCityMap(QString f) {
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
}

bool RegionMapEditor::createCityMap(QString name) {
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
}

bool RegionMapEditor::tryInsertNewMapEntry(QString mapsec) {
    if (!this->region_map->mapSecToMapEntry.keys().contains(mapsec) && mapsec != "MAPSEC_NONE") {
        RegionMapEntry entry(0, 0, 1, 1, region_map->fixCase(mapsec));
        this->region_map->sMapNamesMap.insert(region_map->fixCase(mapsec), QString());
        this->region_map->mapSecToMapEntry.insert(mapsec, entry);
        this->region_map->sMapNames.append(region_map->fixCase(mapsec));
        return true;
    }
    return false;
}

void RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged(unsigned id) {
    this->selectedImageTile = id;
}

void RegionMapEditor::onCityMapTileSelectorSelectedTileChanged(unsigned id) {
    this->selectedCityTile = id;
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
    if (this->region_map->map_squares[index].has_map) {
        message = QString("\t %1").arg(this->project->mapSecToMapHoverName->value(
                      this->region_map->map_squares[index].mapsec)).remove("{NAME_END}");
    }
    this->ui->statusbar->showMessage(message);

    updateRegionMapLayoutOptions(index);
    this->region_map_layout_item->draw();
}

void RegionMapEditor::onRegionMapLayoutHoveredTileChanged(int index) {
    QString message = QString();
    int x = this->region_map->map_squares[index].x;
    int y = this->region_map->map_squares[index].y;
    if (x >= 0 && y >= 0) {
        message = QString("(%1, %2)").arg(x).arg(y);
        if (this->region_map->map_squares[index].has_map) {
            message += QString("\t %1").arg(this->project->mapSecToMapHoverName->value(
                           this->region_map->map_squares[index].mapsec)).remove("{NAME_END}");
        }
    }
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onRegionMapLayoutHoveredTileCleared() {
    this->ui->statusbar->clearMessage();
}

void RegionMapEditor::onHoveredRegionMapTileChanged(int x, int y) {
    QString message = QString("x: %1, y: %2    Tile: 0x").arg(x).arg(y) 
        + QString("%1").arg(this->region_map->getTileId(x, y), 4, 16, QChar('0')).toUpper();
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
    if (index > this->region_map->map_squares.size() - 1) return;

    if (event->buttons() & Qt::RightButton) {
        item->select(event);
    //} else if (event->buttons() & Qt::MiddleButton) {// TODO
    } else {
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            RegionMapHistoryItem *current = history.current();
            bool addToHistory = !(current && current->tiles == this->region_map->getTiles());
            if (addToHistory) {
                RegionMapHistoryItem *commit = new RegionMapHistoryItem(
                    RegionMapEditorBox::BackgroundImage, this->region_map->getTiles(), this->region_map->width(), this->region_map->height()
                );
                history.push(commit);
            }
        } else {
            item->paint(event);
            this->region_map_layout_item->draw();
            this->hasUnsavedChanges = true;
        }
    }
}

void RegionMapEditor::mouseEvent_city_map(QGraphicsSceneMouseEvent *event, CityMapPixmapItem *item) {
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
}

void RegionMapEditor::on_tabWidget_Region_Map_currentChanged(int index) {
    this->ui->stackedWidget_RM_Options->setCurrentIndex(index);
    switch (index)
    {
        case 0:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(true);
            this->region_map_item->draw();
            break;
        case 1:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(false);
            this->region_map_layout_item->draw();
            break;
        case 2:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(false);
            this->region_map_entries_item->draw();
            break;
    }
}

void RegionMapEditor::on_comboBox_RM_ConnectedMap_activated(const QString &mapsec) {
    this->ui->lineEdit_RM_MapName->setText(this->project->mapSecToMapHoverName->value(mapsec));
    onRegionMapLayoutSelectedTileChanged(this->currIndex);// re-draw layout image
    this->hasUnsavedChanges = true;// sometimes this is called for unknown reasons
}

void RegionMapEditor::on_comboBox_RM_Entry_MapSection_activated(const QString &text) {
    this->activeEntry = text;
    this->region_map_entries_item->currentSection = activeEntry;
    updateRegionMapEntryOptions(activeEntry);

    int idx = this->region_map->getMapSquareIndex(this->region_map->mapSecToMapEntry.value(activeEntry).x + this->region_map->padLeft,
                                                  this->region_map->mapSecToMapEntry.value(activeEntry).y + this->region_map->padTop);
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
}

void RegionMapEditor::on_spinBox_RM_Entry_x_valueChanged(int x) {
    tryInsertNewMapEntry(activeEntry);
    this->region_map->mapSecToMapEntry[activeEntry].setX(x);
    int idx = this->region_map->getMapSquareIndex(this->region_map->mapSecToMapEntry.value(activeEntry).x + this->region_map->padLeft,
                                                  this->region_map->mapSecToMapEntry.value(activeEntry).y + this->region_map->padTop);
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
    this->ui->spinBox_RM_Entry_width->setMaximum(this->region_map->width() - this->region_map->padLeft - this->region_map->padRight - x);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Entry_y_valueChanged(int y) {
    tryInsertNewMapEntry(activeEntry);
    this->region_map->mapSecToMapEntry[activeEntry].setY(y);
    int idx = this->region_map->getMapSquareIndex(this->region_map->mapSecToMapEntry.value(activeEntry).x + this->region_map->padLeft,
                                                  this->region_map->mapSecToMapEntry.value(activeEntry).y + this->region_map->padTop);
    this->region_map_entries_item->select(idx);
    this->region_map_entries_item->draw();
    this->ui->spinBox_RM_Entry_height->setMaximum(this->region_map->height() - this->region_map->padTop - this->region_map->padBottom - y);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Entry_width_valueChanged(int width) {
    tryInsertNewMapEntry(activeEntry);
    this->region_map->mapSecToMapEntry[activeEntry].setWidth(width);
    this->region_map_entries_item->draw();
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Entry_height_valueChanged(int height) {
    tryInsertNewMapEntry(activeEntry);
    this->region_map->mapSecToMapEntry[activeEntry].setHeight(height);
    this->region_map_entries_item->draw();
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_lineEdit_RM_MapName_textEdited(const QString &) {
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_pushButton_RM_Options_delete_clicked() {
    this->region_map->resetSquare(this->region_map_layout_item->selectedTile);
    updateRegionMapLayoutOptions(this->region_map_layout_item->selectedTile);
    this->region_map_layout_item->draw();
    this->region_map_layout_item->select(this->region_map_layout_item->selectedTile);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_pushButton_CityMap_add_clicked() {
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("New City Map");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QLineEdit *input = new QLineEdit();
    form.addRow(new QLabel("Name:"), input);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);

    QString name;

    form.addRow(&buttonBox);
    connect(&buttonBox, SIGNAL(rejected()), &popup, SLOT(reject()));
    connect(&buttonBox, &QDialogButtonBox::accepted, [&popup, &input, &name](){
        name = input->text().remove(QRegularExpression("[^a-zA-Z0-9_]+"));
        if (!name.isEmpty())
            popup.accept();
    });

    if (popup.exec() == QDialog::Accepted) {
        createCityMap(name);
    }

    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_action_RegionMap_Resize_triggered() {
    QDialog popup(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    popup.setWindowTitle("New Region Map Dimensions");
    popup.setWindowModality(Qt::NonModal);

    QFormLayout form(&popup);

    QSpinBox *widthSpinBox = new QSpinBox();
    QSpinBox *heightSpinBox = new QSpinBox();
    widthSpinBox->setMinimum(32);
    heightSpinBox->setMinimum(20);
    widthSpinBox->setMaximum(60);// TODO: find real limits
    heightSpinBox->setMaximum(40);
    widthSpinBox->setValue(this->region_map->width());
    heightSpinBox->setValue(this->region_map->height());
    form.addRow(new QLabel("Width"), widthSpinBox);
    form.addRow(new QLabel("Height"), heightSpinBox);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &popup);

    form.addRow(&buttonBox);
    connect(&buttonBox, SIGNAL(rejected()), &popup, SLOT(reject()));
    connect(&buttonBox, SIGNAL(accepted()), &popup, SLOT(accept()));

    if (popup.exec() == QDialog::Accepted) {
        resize(widthSpinBox->value(), heightSpinBox->value());
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(
            RegionMapEditorBox::BackgroundImage, this->region_map->getTiles(), widthSpinBox->value(), heightSpinBox->value()
        );
        history.push(commit);
    }

    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_action_RegionMap_Undo_triggered() {
    undo();
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::undo() {
    RegionMapHistoryItem *commit = history.back();
    if (!commit) return;

    switch (commit->which)
    {
        case RegionMapEditorBox::BackgroundImage:
            if (commit->mapWidth != this->region_map->width() || commit->mapHeight != this->region_map->height())
                this->resize(commit->mapWidth, commit->mapHeight);
            this->region_map->setTiles(commit->tiles);
            this->region_map_item->draw();
            this->region_map_layout_item->draw();
            this->region_map_entries_item->draw();
            break;
        case RegionMapEditorBox::CityMapImage:
            if (commit->cityMap == this->city_map_item->file)
                this->city_map_item->setTiles(commit->tiles);
            this->city_map_item->draw();
            break;
    }
}

void RegionMapEditor::on_action_RegionMap_Redo_triggered() {
    redo();
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::redo() {
    RegionMapHistoryItem *commit = history.next();
    if (!commit) return;

    switch (commit->which)
    {
        case RegionMapEditorBox::BackgroundImage:
            if (commit->mapWidth != this->region_map->width() || commit->mapHeight != this->region_map->height())
                this->resize(commit->mapWidth, commit->mapHeight);
            this->region_map->setTiles(commit->tiles);
            this->region_map_item->draw();
            this->region_map_layout_item->draw();
            this->region_map_entries_item->draw();
            break;
        case RegionMapEditorBox::CityMapImage:
            this->city_map_item->setTiles(commit->tiles);
            this->city_map_item->draw();
            break;
    }
}

void RegionMapEditor::resize(int w, int h) {
    this->region_map->resize(w, h);
    this->currIndex = this->region_map->padLeft * w + this->region_map->padTop;
    displayRegionMapImage();
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
}

void RegionMapEditor::on_action_Swap_triggered() {
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
    connect(&buttonBox, SIGNAL(rejected()), &popup, SLOT(reject()));
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
    this->region_map->clearImage();
    RegionMapHistoryItem *commit = new RegionMapHistoryItem(
        RegionMapEditorBox::BackgroundImage, this->region_map->getTiles(), this->region_map->width(), this->region_map->height()
    );
    history.push(commit);

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
        this->region_map->clearLayout();
        displayRegionMapLayout();
    } else {
        return;
    }
}

void RegionMapEditor::on_action_Import_RegionMap_ImageTiles_triggered() {
    importTileImage(false);
}

void RegionMapEditor::on_action_Import_CityMap_ImageTiles_triggered() {
    importTileImage(true);
}

void RegionMapEditor::importTileImage(bool city) {
    QString descriptor = city ? "City Map" : "Region Map";

    QString infilepath = QFileDialog::getOpenFileName(this, QString("Import %1 Tiles Image").arg(descriptor),
                                                    this->project->root, "Image Files (*.png *.bmp *.jpg *.dib)");
    if (infilepath.isEmpty()) {
        return;
    }

    logInfo(QString("Importing %1 Tiles from '%2'").arg(descriptor).arg(infilepath));

    // Read image data from buffer so that the built-in QImage doesn't try to detect file format
    // purely from the extension name.
    QFile file(infilepath);
    QImage image;
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray imageData = file.readAll();
        image = QImage::fromData(imageData);
    } else {
        QString errorMessage = QString("Failed to open image file: '%1'").arg(infilepath);
        logError(errorMessage);
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(errorMessage);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }
    if (image.width() == 0 || image.height() == 0 || image.width() % 8 != 0 || image.height() % 8 != 0) {
        QString errorMessage = QString("The image dimensions (%1 x %2) are invalid. Width and height must be multiples of 8 pixels.")
                                      .arg(image.width())
                                      .arg(image.height());
        logError(errorMessage);
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(errorMessage);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Validate total number of tiles in image.
    int numTilesWide = image.width() / 8;
    int numTilesHigh = image.height() / 8;
    int totalTiles = numTilesHigh * numTilesWide;
    int maxAllowedTiles = 0x100;
    if (totalTiles > maxAllowedTiles) {
        QString errorMessage = QString("The total number of tiles in the provided image (%1) is greater than the allowed number (%2).")
                                      .arg(totalTiles)
                                      .arg(maxAllowedTiles);
        logError(errorMessage);
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(errorMessage);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Validate the image's palette.
    QString palMessage = QString();
    bool palError = false;
    if (image.colorCount() == 0) {
        palMessage = QString("The provided image is not indexed.");
        palError = true;
    } else if (!city && image.colorCount() != 256) {
        palMessage = QString("The provided image has a palette with %1 colors. You must provide an indexed imaged with a 256 color palette.").arg(image.colorCount());
        palError = true;
    } else if (city && image.colorCount() != 16) {
        palMessage = QString("The provided image has a palette with %1 colors. You must provide an indexed imaged with a 16 color palette.").arg(image.colorCount());
        palError = true;
    }

    if (palError) {
        logError(palMessage);
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import tiles.");
        msgBox.setInformativeText(palMessage);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    // Use the image from the correct path.
    if (city) {
        this->region_map->setTemporaryCityTilesPath(infilepath);
    } else {
        this->region_map->setTemporaryPngPath(infilepath);
    }
    this->hasUnsavedChanges = true;

    // Redload and redraw images.
    displayRegionMap();
    displayCityMap(this->ui->comboBox_CityMap_picker->currentText());
}

void RegionMapEditor::on_comboBox_CityMap_picker_currentTextChanged(const QString &file) {
    this->displayCityMap(file);
    this->cityMapFirstDraw = true;
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
}

void RegionMapEditor::on_verticalSlider_Zoom_Map_Image_valueChanged(int val) {
    double scale = pow(scaleUpFactor, static_cast<double>(val - initialScale) / initialScale);

    QMatrix matrix;
    matrix.scale(scale, scale);
    int width = ceil(static_cast<double>(this->region_map->imgSize().width()) * scale);
    int height = ceil(static_cast<double>(this->region_map->imgSize().height()) * scale);

    ui->graphicsView_Region_Map_BkgImg->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Region_Map_BkgImg->setMatrix(matrix);
    ui->graphicsView_Region_Map_BkgImg->setFixedSize(width + 2, height + 2);
    ui->graphicsView_Region_Map_Layout->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Region_Map_Layout->setMatrix(matrix);
    ui->graphicsView_Region_Map_Layout->setFixedSize(width + 2, height + 2);
    ui->graphicsView_Region_Map_Entries->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_Region_Map_Entries->setMatrix(matrix);
    ui->graphicsView_Region_Map_Entries->setFixedSize(width + 2, height + 2);
}

void RegionMapEditor::on_verticalSlider_Zoom_Image_Tiles_valueChanged(int val) {
    double scale = pow(scaleUpFactor, static_cast<double>(val - initialScale) / initialScale);

    QMatrix matrix;
    matrix.scale(scale, scale);
    int width = ceil(static_cast<double>(this->mapsquare_selector_item->pixelWidth) * scale);
    int height = ceil(static_cast<double>(this->mapsquare_selector_item->pixelHeight) * scale);

    ui->graphicsView_RegionMap_Tiles->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_RegionMap_Tiles->setMatrix(matrix);
    ui->graphicsView_RegionMap_Tiles->setFixedSize(width + 2, height + 2);
}

void RegionMapEditor::on_verticalSlider_Zoom_City_Map_valueChanged(int val) {
    double scale = pow(scaleUpFactor, static_cast<double>(val - initialScale) / initialScale);

    QMatrix matrix;
    matrix.scale(scale, scale);
    int width = ceil(static_cast<double>(8 * city_map_item->width()) * scale);
    int height = ceil(static_cast<double>(8 * city_map_item->height()) * scale);

    ui->graphicsView_City_Map->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_City_Map->setMatrix(matrix);
    ui->graphicsView_City_Map->setFixedSize(width + 2, height + 2);
}

void RegionMapEditor::on_verticalSlider_Zoom_City_Tiles_valueChanged(int val) {
    double scale = pow(scaleUpFactor, static_cast<double>(val - initialScale) / initialScale);

    QMatrix matrix;
    matrix.scale(scale, scale);
    int width = ceil(static_cast<double>(this->city_map_selector_item->pixelWidth) * scale);
    int height = ceil(static_cast<double>(this->city_map_selector_item->pixelHeight) * scale);

    ui->graphicsView_City_Map_Tiles->setResizeAnchor(QGraphicsView::NoAnchor);
    ui->graphicsView_City_Map_Tiles->setMatrix(matrix);
    ui->graphicsView_City_Map_Tiles->setFixedSize(width + 2, height + 2);
}
