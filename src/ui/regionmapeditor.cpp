#include "regionmapeditor.h"
#include "ui_regionmapeditor.h"
#include "regionmapgenerator.h"
#include "config.h"

#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QColor>
#include <QMessageBox>
#include <QDialogButtonBox>

RegionMapEditor::RegionMapEditor(QWidget *parent, Project *project_) :
    QMainWindow(parent),
    ui(new Ui::RegionMapEditor)
{
    this->ui->setupUi(this);
    this->project = project_;
    this->region_map = new RegionMap;
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
    if (project && region_map) {
        region_map->save();
        this->city_map_item->save();
        this->region_map->saveOptions(
            this->region_map_layout_item->selectedTile,
            this->ui->comboBox_RM_ConnectedMap->currentText(),
            this->ui->lineEdit_RM_MapName->text(),
            this->ui->spinBox_RM_Options_x->value(),
            this->ui->spinBox_RM_Options_y->value()
        );
        this->region_map_layout_item->highlightedTile = -1;
        displayRegionMap();
    }
    this->hasUnsavedChanges = false;
}

void RegionMapEditor::loadRegionMapData() {
    this->region_map->init(project);
    this->currIndex = this->region_map->width() * this->region_map->padTop + this->region_map->padLeft;
    displayRegionMap();
}

void RegionMapEditor::loadCityMaps() {
    QDir directory(project->root + "/graphics/pokenav/city_maps");
    QStringList files = directory.entryList(QStringList() << "*.bin", QDir::Files);
    QStringList without_bin;
    for (QString file : files) {
        without_bin.append(file.remove(".bin"));
    }
    this->ui->comboBox_CityMap_picker->addItems(without_bin);
}

void RegionMapEditor::displayRegionMap() {
    displayRegionMapTileSelector();
    displayCityMapTileSelector();
    displayRegionMapImage();
    displayRegionMapLayout();
    displayRegionMapLayoutOptions();
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
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize().width() * scaleRegionMapImage + 2,
                                                           this->region_map->imgSize().height() * scaleRegionMapImage + 2);

    RegionMapHistoryItem *commit = new RegionMapHistoryItem(RegionMapEditorBox::BackgroundImage, this->region_map->getTiles());
    history.push(commit);
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
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize().width() * scaleRegionMapImage + 2,
                                                           this->region_map->imgSize().height() * scaleRegionMapImage + 2);
}

void RegionMapEditor::displayRegionMapLayoutOptions() {
    this->ui->comboBox_RM_ConnectedMap->clear();
    this->ui->comboBox_RM_ConnectedMap->addItems(*(this->project->regionMapSections));

    this->ui->frame_RM_Options->setEnabled(true);

    this->ui->spinBox_RM_Options_x->setMaximum(
        this->region_map->width() - this->region_map->padLeft - this->region_map->padRight - 1
    );
    this->ui->spinBox_RM_Options_y->setMaximum(
        this->region_map->height() - this->region_map->padTop - this->region_map->padBottom - 1
    );

    updateRegionMapLayoutOptions(currIndex);

    // TODO: implement when the code is decompiled
    this->ui->label_RM_CityMap->setVisible(false);
    this->ui->comboBox_RM_CityMap->setVisible(false);
}

void RegionMapEditor::updateRegionMapLayoutOptions(int index) {
    this->ui->spinBox_RM_Options_x->blockSignals(true);
    this->ui->spinBox_RM_Options_y->blockSignals(true);
    this->ui->comboBox_RM_ConnectedMap->blockSignals(true);
    this->ui->lineEdit_RM_MapName->setText(this->project->mapSecToMapHoverName->value(this->region_map->map_squares[index].mapsec));
    this->ui->comboBox_RM_ConnectedMap->setCurrentText(this->region_map->map_squares[index].mapsec);
    this->ui->spinBox_RM_Options_x->setValue(this->region_map->map_squares[index].x);
    this->ui->spinBox_RM_Options_y->setValue(this->region_map->map_squares[index].y);
    this->ui->spinBox_RM_Options_x->blockSignals(false);
    this->ui->spinBox_RM_Options_y->blockSignals(false);
    this->ui->comboBox_RM_ConnectedMap->blockSignals(false);
}

void RegionMapEditor::displayRegionMapTileSelector() {
    if (!scene_region_map_tiles) {
        this->scene_region_map_tiles = new QGraphicsScene;
    }
    if (mapsquare_selector_item && scene_region_map_tiles) {
        this->scene_region_map_tiles->removeItem(mapsquare_selector_item);
        delete mapsquare_selector_item;
    }

    this->mapsquare_selector_item = new TilemapTileSelector(QPixmap(this->region_map->region_map_png_path));
    this->mapsquare_selector_item->draw();

    this->scene_region_map_tiles->addItem(this->mapsquare_selector_item);

    connect(this->mapsquare_selector_item, &TilemapTileSelector::selectedTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged);
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged);
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared);

    this->ui->graphicsView_RegionMap_Tiles->setScene(this->scene_region_map_tiles);
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * scaleRegionMapTiles + 2,
                                                         this->mapsquare_selector_item->pixelHeight * scaleRegionMapTiles + 2);

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

    this->city_map_selector_item = new TilemapTileSelector(QPixmap(this->region_map->city_map_tiles_path));
    this->city_map_selector_item->draw();

    this->scene_city_map_tiles->addItem(this->city_map_selector_item);

    connect(this->city_map_selector_item, &TilemapTileSelector::selectedTileChanged,
            this, &RegionMapEditor::onCityMapTileSelectorSelectedTileChanged);

    this->ui->graphicsView_City_Map_Tiles->setScene(this->scene_city_map_tiles);
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * scaleCityMapTiles + 2,
                                                        this->city_map_selector_item->pixelHeight * scaleCityMapTiles + 2);

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
    this->ui->graphicsView_City_Map->setFixedSize(8 * city_map_item->width() * scaleCityMapImage + 2,
                                                  8 * city_map_item->height() * scaleCityMapImage + 2);

    RegionMapHistoryItem *commit = new RegionMapHistoryItem(
        RegionMapEditorBox::CityMapImage, this->city_map_item->getTiles(), this->city_map_item->file
    );
    history.push(commit);
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

void RegionMapEditor::onRegionMapLayoutSelectedTileChanged(int index) {
    QString message = QString();
    this->currIndex = index;
    if (this->region_map->map_squares[index].has_map) {
        message = QString("\t %1").arg(this->project->mapSecToMapHoverName->value(
                      this->region_map->map_squares[index].mapsec)).remove("{NAME_END}");// ruby-specific
    }
    this->ui->statusbar->showMessage(message);

    updateRegionMapLayoutOptions(index);
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
    QString message = QString("x: %1, y: %2   Tile: 0x").arg(x).arg(y) 
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
        item->paint(event);
        this->hasUnsavedChanges = true;
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            RegionMapHistoryItem *current = history.current();
            bool addToHistory = !(current && current->tiles == this->region_map->getTiles());
            if (addToHistory) {
                RegionMapHistoryItem *commit = new RegionMapHistoryItem(
                    RegionMapEditorBox::BackgroundImage, this->region_map->getTiles()
                );
                history.push(commit);
            }
        }
    }
}

void RegionMapEditor::mouseEvent_city_map(QGraphicsSceneMouseEvent *event, CityMapPixmapItem *item) {
    //
    QPointF pos = event->pos();
    int x = static_cast<int>(pos.x()) / 8;
    int y = static_cast<int>(pos.y()) / 8;
    int index = this->city_map_item->getIndexAt(x, y);

    if (event->buttons() & Qt::RightButton) {// TODO
    //} else if (event->buttons() & Qt::MiddleButton) {// TODO
    } else {
        item->paint(event);
        this->hasUnsavedChanges = true;
        if (event->type() == QEvent::GraphicsSceneMouseRelease) {
            RegionMapHistoryItem *current = history.current();
            bool addToHistory = !(current && current->tiles == this->city_map_item->getTiles());
            if (addToHistory) {
                RegionMapHistoryItem *commit = new RegionMapHistoryItem(
                    RegionMapEditorBox::CityMapImage, this->city_map_item->getTiles(), this->city_map_item->file
                );
                history.push(commit);
            }
        }
    }
}

void RegionMapEditor::on_tabWidget_Region_Map_currentChanged(int index) {
    this->ui->stackedWidget_RM_Options->setCurrentIndex(index);
    switch (index)
    {
        case 0:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(true);
            break;
        case 1:
            this->ui->verticalSlider_Zoom_Image_Tiles->setVisible(false);
            break;
    }
}

void RegionMapEditor::on_spinBox_RM_Options_x_valueChanged(int x) {
    int y = this->ui->spinBox_RM_Options_y->value();
    int red = this->region_map->getMapSquareIndex(x + this->region_map->padLeft, y + this->region_map->padTop);
    this->region_map_layout_item->highlight(x, y, red);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_spinBox_RM_Options_y_valueChanged(int y) {
    int x = this->ui->spinBox_RM_Options_x->value();
    int red = this->region_map->getMapSquareIndex(x + this->region_map->padLeft, y + this->region_map->padTop);
    this->region_map_layout_item->highlight(x, y, red);
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_comboBox_RM_ConnectedMap_activated(const QString &mapsec) {
    this->ui->lineEdit_RM_MapName->setText(this->project->mapSecToMapHoverName->value(mapsec));
    //this->hasUnsavedChanges = true;// sometimes this is called for unknown reasons
}

void RegionMapEditor::on_lineEdit_RM_MapName_textEdited(const QString &text) {
    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_pushButton_RM_Options_delete_clicked() {
    this->region_map->resetSquare(this->region_map_layout_item->selectedTile);
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
    widthSpinBox->setMaximum(60);// w * h * 2 <= 4960
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
    }

    this->hasUnsavedChanges = true;
}

void RegionMapEditor::on_action_RegionMap_Undo_triggered() {
    undo();
    this->hasUnsavedChanges = true;
}

// TODO: add resizing
void RegionMapEditor::undo() {
    RegionMapHistoryItem *commit = history.back();
    if (!commit) return;

    switch (commit->which)
    {
        case RegionMapEditorBox::BackgroundImage:
            this->region_map->setTiles(commit->tiles);
            this->region_map_item->draw();
            break;
        case RegionMapEditorBox::CityMapImage:
            if (commit->cityMap == this->city_map_item->file)
                this->city_map_item->setTiles(commit->tiles);
            this->city_map_item->draw();
            break;
        case RegionMapEditorBox::BackroundResize:
            this->region_map->resize(commit->mapWidth, commit->mapHeight);
            displayRegionMap();
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
            this->region_map->setTiles(commit->tiles);
            this->region_map_item->draw();
            break;
        case RegionMapEditorBox::CityMapImage:
            this->city_map_item->setTiles(commit->tiles);
            this->city_map_item->draw();
            break;
        case RegionMapEditorBox::BackroundResize:
            this->region_map->resize(commit->mapWidth, commit->mapHeight);
            displayRegionMap();
            break;
    }
}

void RegionMapEditor::resize(int w, int h) {
    RegionMapHistoryItem *commitOld = new RegionMapHistoryItem(
        RegionMapEditorBox::BackroundResize, this->region_map->getTiles(), this->region_map->width(), this->region_map->height()
    );
    RegionMapHistoryItem *commitNew = new RegionMapHistoryItem(
        RegionMapEditorBox::BackroundResize, this->region_map->getTiles(), w, h
    );
    history.push(commitOld);
    history.push(commitNew);
    history.back();

    this->region_map->resize(w, h);
    this->currIndex = 2 * w + 1;
    displayRegionMap();
}

void RegionMapEditor::on_comboBox_CityMap_picker_currentTextChanged(const QString &file) {
    this->displayCityMap(file);
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
    bool zoom_in = val > scaleRegionMapImage ? true : false;
    scaleRegionMapImage = val;

    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize().width() * pow(scaleUpFactor, scaleRegionMapImage - 1) + 2,
                                                           this->region_map->imgSize().height() * pow(scaleUpFactor, scaleRegionMapImage - 1) + 2);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize().width() * pow(scaleUpFactor, scaleRegionMapImage - 1) + 2,
                                                           this->region_map->imgSize().height() * pow(scaleUpFactor, scaleRegionMapImage - 1) + 2);

    if (zoom_in) {
        this->ui->graphicsView_Region_Map_BkgImg->scale(scaleUpFactor, scaleUpFactor);
        this->ui->graphicsView_Region_Map_Layout->scale(scaleUpFactor, scaleUpFactor);
    } else {
        //
        this->ui->graphicsView_Region_Map_BkgImg->scale(scaleDownFactor, scaleDownFactor);
        this->ui->graphicsView_Region_Map_Layout->scale(scaleDownFactor, scaleDownFactor);
    }
}

void RegionMapEditor::on_verticalSlider_Zoom_Image_Tiles_valueChanged(int val) {
    bool zoom_in = val > scaleRegionMapTiles ? true : false;
    scaleRegionMapTiles = val;

    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * pow(scaleUpFactor, scaleRegionMapTiles - 1) + 2,
                                                         this->mapsquare_selector_item->pixelHeight * pow(scaleUpFactor, scaleRegionMapTiles - 1) + 2);
    
    if (zoom_in) {
        this->ui->graphicsView_RegionMap_Tiles->scale(scaleUpFactor, scaleUpFactor);
    } else {
        this->ui->graphicsView_RegionMap_Tiles->scale(scaleDownFactor, scaleDownFactor);
    }
}

void RegionMapEditor::on_verticalSlider_Zoom_City_Map_valueChanged(int val) {
    bool zoom_in = val > scaleCityMapImage ? true : false;
    scaleCityMapImage = val;

    this->ui->graphicsView_City_Map->setFixedSize(8 * city_map_item->width() * pow(scaleUpFactor, scaleCityMapImage - 1) + 2, 
                                                  8 * city_map_item->height() * pow(scaleUpFactor, scaleCityMapImage - 1) + 2);

    if (zoom_in) {
        this->ui->graphicsView_City_Map->scale(scaleUpFactor, scaleUpFactor);
    } else {
        this->ui->graphicsView_City_Map->scale(scaleDownFactor, scaleDownFactor);
    }
}

void RegionMapEditor::on_verticalSlider_Zoom_City_Tiles_valueChanged(int val) {
    bool zoom_in = val > scaleCityMapTiles ? true : false;
    scaleCityMapTiles = val;

    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * pow(scaleUpFactor, scaleCityMapTiles - 1) + 2,
                                                        this->city_map_selector_item->pixelHeight * pow(scaleUpFactor, scaleCityMapTiles - 1) + 2);

    if (zoom_in) {
        this->ui->graphicsView_City_Map_Tiles->scale(scaleUpFactor, scaleUpFactor);
    } else {
        this->ui->graphicsView_City_Map_Tiles->scale(scaleDownFactor, scaleDownFactor);
    }
}

void RegionMapEditor::on_action_RegionMap_Generate_triggered() {
    //
    RegionMapGenerator generator(this->project);
    generator.generate("LittlerootTown");
    this->hasUnsavedChanges = true;
}
