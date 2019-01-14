#include "regionmapeditor.h"
#include "ui_regionmapeditor.h"

#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QColor>

RegionMapEditor::RegionMapEditor(QWidget *parent, Project *pro) :
    QMainWindow(parent),
    ui(new Ui::RegionMapEditor)
{
    this->ui->setupUi(this);
    this->project = pro;
    this->region_map = new RegionMap;
    this->setFixedSize(this->size());
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
        displayRegionMap();
    }
}

void RegionMapEditor::loadRegionMapData() {
    this->region_map->init(project);
    this->currIndex = this->region_map->width() * 2 + 1;
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
    //this->scene_region_map_image->setSceneRect(this->scene_region_map_image->sceneRect());

    this->ui->graphicsView_Region_Map_BkgImg->setScene(this->scene_region_map_image);
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
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
    //this->scene_region_map_layout->setSceneRect(this->scene_region_map_layout->sceneRect());

    this->ui->graphicsView_Region_Map_Layout->setScene(this->scene_region_map_layout);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
}

void RegionMapEditor::displayRegionMapLayoutOptions() {
    this->ui->comboBox_RM_ConnectedMap->addItems(*(this->project->regionMapSections));

    this->ui->frame_RM_Options->setEnabled(true);

    // TODO: change these values to variables
    this->ui->spinBox_RM_Options_x->setMaximum(this->region_map->width() - 5);
    this->ui->spinBox_RM_Options_y->setMaximum(this->region_map->height() - 4);

    updateRegionMapLayoutOptions(currIndex);
}

void RegionMapEditor::updateRegionMapLayoutOptions(int index) {
    this->ui->spinBox_RM_Options_x->blockSignals(true);
    this->ui->spinBox_RM_Options_y->blockSignals(true);
    this->ui->lineEdit_RM_MapName->setText(this->project->mapSecToMapHoverName->value(this->region_map->map_squares[index].mapsec));//this->region_map->map_squares[index].map_name);
    this->ui->comboBox_RM_ConnectedMap->setCurrentText(this->region_map->map_squares[index].mapsec);
    this->ui->spinBox_RM_Options_x->setValue(this->region_map->map_squares[index].x);
    this->ui->spinBox_RM_Options_y->setValue(this->region_map->map_squares[index].y);
    this->ui->spinBox_RM_Options_x->blockSignals(false);
    this->ui->spinBox_RM_Options_y->blockSignals(false);
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

    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged);
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared);

    this->ui->graphicsView_RegionMap_Tiles->setScene(this->scene_region_map_tiles);
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * scaleRegionMapTiles + 2,
                                                         this->mapsquare_selector_item->pixelHeight * scaleRegionMapTiles + 2);
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
    this->scene_city_map_tiles->setSceneRect(this->scene_city_map_tiles->sceneRect());// ?

    // TODO:
    /*connect(this->city_map_selector_item, &TilemapTileSelector::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged);
    connect(this->city_map_selector_item, &TilemapTileSelector::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared);*/

    this->ui->graphicsView_City_Map_Tiles->setScene(this->scene_city_map_tiles);
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * scaleCityMapTiles + 2,
                                                        this->city_map_selector_item->pixelHeight * scaleCityMapTiles + 2);
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
    this->ui->graphicsView_City_Map->setFixedSize(8 * city_map_item->width * scaleCityMapImage + 2,
                                                  8 * city_map_item->height * scaleCityMapImage + 2);
}

bool RegionMapEditor::createCityMap(QString name) {
    bool errored = false;

    QString file = this->project->root + "/graphics/pokenav/city_maps/" + name + ".bin";

    // TODO: use project config for these values?
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
    // TODO: change to x, y coords not index
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
    QString message = QString("x: %1, y: %2   Tile: 0x").arg(x).arg(y) + QString("%1").arg(this->region_map->getTileId(x, y), 4, 16, QChar('0')).toUpper();
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onHoveredRegionMapTileCleared() {
    this->ui->statusbar->clearMessage();
}

void RegionMapEditor::mouseEvent_region_map(QGraphicsSceneMouseEvent *event, RegionMapPixmapItem *item) {
    if (event->buttons() & Qt::RightButton) {
        item->select(event);
    } else if (event->buttons() & Qt::MiddleButton) {// TODO
    } else {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 8;
        int y = static_cast<int>(pos.y()) / 8;
        int index = this->region_map->getMapSquareIndex(x, y);
        if (index > this->region_map->map_squares.size() - 1) return;
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(RegionMapEditorBox::BackgroundImage, index, 
            this->region_map->map_squares[index].tile_img_id, this->mapsquare_selector_item->getSelectedTile());
        history.push(commit);
        item->paint(event);//*/
    }
}

void RegionMapEditor::mouseEvent_city_map(QGraphicsSceneMouseEvent *event, CityMapPixmapItem *item) {
    //
    if (event->buttons() & Qt::RightButton) {// TODO
    } else if (event->buttons() & Qt::MiddleButton) {// TODO
    } else {
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 8;
        int y = static_cast<int>(pos.y()) / 8;
        int index = this->city_map_item->getIndexAt(x, y);
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(RegionMapEditorBox::CityMapImage, index, 
            this->city_map_item->data[index], this->city_map_selector_item->getSelectedTile());
        history.push(commit);
        item->paint(event);
    }
}

void RegionMapEditor::on_tabWidget_Region_Map_currentChanged(int index) {
    this->ui->stackedWidget_RM_Options->setCurrentIndex(index);
}

void RegionMapEditor::on_spinBox_RM_Options_x_valueChanged(int x) {
    int y = this->ui->spinBox_RM_Options_y->value();
    int red = this->region_map->getMapSquareIndex(x + 1, y + 2);
    this->region_map_layout_item->highlight(x, y, red);
}

void RegionMapEditor::on_spinBox_RM_Options_y_valueChanged(int y) {
    int x = this->ui->spinBox_RM_Options_x->value();
    int red = this->region_map->getMapSquareIndex(x + 1, y + 2);
    this->region_map_layout_item->highlight(x, y, red);
}

void RegionMapEditor::on_pushButton_RM_Options_save_clicked() {
    this->region_map->saveOptions(
        this->region_map_layout_item->selectedTile,// TODO: remove
        this->ui->comboBox_RM_ConnectedMap->currentText(),
        this->ui->lineEdit_RM_MapName->text(),
        this->ui->spinBox_RM_Options_x->value(),
        this->ui->spinBox_RM_Options_y->value()
    );
    this->region_map_layout_item->highlightedTile = -1;
    this->region_map_layout_item->draw();
    // TODO: update selected tile index
}

void RegionMapEditor::on_pushButton_CityMap_save_clicked() {
    this->city_map_item->save();
}

void RegionMapEditor::on_pushButton_RM_Options_delete_clicked() {
    this->region_map->resetSquare(this->region_map_layout_item->selectedTile);
    this->region_map_layout_item->draw();
    this->region_map_layout_item->select(this->region_map_layout_item->selectedTile);
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
    widthSpinBox->setMaximum(64);// TODO: come up with real (meaningful) limits?
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
}

void RegionMapEditor::on_action_RegionMap_Undo_triggered() {
    undo();
}

void RegionMapEditor::undo() {
    RegionMapHistoryItem *commit = history.current();
    if (!commit) return;

    uint8_t tile = static_cast<uint8_t>(commit->prev);

    history.back();

    switch (commit->which)
    {
        case RegionMapEditorBox::BackgroundImage:
            history.back();// TODO: why do I need to do this?
            this->region_map->map_squares[commit->index].tile_img_id = tile;
            this->region_map_item->draw();
            break;
        case RegionMapEditorBox::CityMapImage:
            this->city_map_item->data[commit->index] = tile;
            this->city_map_item->draw();
            break;
    }
}

void RegionMapEditor::on_action_RegionMap_Redo_triggered() {
    redo();
}

void RegionMapEditor::redo() {
    RegionMapHistoryItem *commit = history.next();
    if (!commit) return;

    uint8_t tile = static_cast<uint8_t>(commit->tile);

    switch (commit->which)
    {
        case RegionMapEditorBox::BackgroundImage:
            history.next();// TODO: why do I need to do this?
            this->region_map->map_squares[commit->index].tile_img_id = tile;
            this->region_map_item->draw();
            break;
        case RegionMapEditorBox::CityMapImage:
            this->city_map_item->data[commit->index] = tile;
            this->city_map_item->draw();
            break;
    }
}

void RegionMapEditor::resize(int w, int h) {
    this->region_map->resize(w, h);
    this->currIndex = 2 * w + 1;
    displayRegionMap();
}

void RegionMapEditor::on_comboBox_CityMap_picker_currentTextChanged(const QString &file) {
    this->displayCityMap(file);
}

void RegionMapEditor::on_pushButton_Zoom_In_Image_Tiles_clicked() {
    if (scaleRegionMapTiles >= 8.0) return;
    scaleRegionMapTiles *= 2.0;
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * scaleRegionMapTiles + 2,
                                                         this->mapsquare_selector_item->pixelHeight * scaleRegionMapTiles + 2);
    this->ui->graphicsView_RegionMap_Tiles->scale(2.0, 2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_Image_Tiles_clicked() {
    if (scaleRegionMapTiles <= 1.0) return;
    scaleRegionMapTiles /= 2.0;
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * scaleRegionMapTiles + 2,
                                                         this->mapsquare_selector_item->pixelHeight * scaleRegionMapTiles + 2);
    this->ui->graphicsView_RegionMap_Tiles->scale(0.5, 0.5);
}

void RegionMapEditor::on_pushButton_Zoom_In_City_Tiles_clicked() {
    if (scaleCityMapTiles >= 8.0) return;
    scaleCityMapTiles *= 2.0;
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * scaleCityMapTiles + 2,
                                                        this->city_map_selector_item->pixelHeight * scaleCityMapTiles + 2);
    this->ui->graphicsView_City_Map_Tiles->scale(2.0,2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_City_Tiles_clicked() {
    if (scaleCityMapTiles <= 1.0) return;
    scaleCityMapTiles /= 2.0;
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * scaleCityMapTiles + 2,
                                                        this->city_map_selector_item->pixelHeight * scaleCityMapTiles + 2);
    this->ui->graphicsView_City_Map_Tiles->scale(0.5,0.5);
}

void RegionMapEditor::on_pushButton_Zoom_In_City_Map_clicked() {
    if (scaleCityMapImage >= 8.0) return;
    scaleCityMapImage *= 2.0;
    this->ui->graphicsView_City_Map->setFixedSize(8 * city_map_item->width * scaleCityMapImage + 2, 
                                                  8 * city_map_item->height * scaleCityMapImage + 2);
    this->ui->graphicsView_City_Map->scale(2.0,2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_City_Map_clicked() {
    if (scaleCityMapImage <= 1.0) return;
    scaleCityMapImage /= 2.0;
    this->ui->graphicsView_City_Map->setFixedSize(8 * city_map_item->width * scaleCityMapImage + 2,
                                                  8 * city_map_item->height * scaleCityMapImage + 2);
    this->ui->graphicsView_City_Map->scale(0.5,0.5);
}

void RegionMapEditor::on_pushButton_Zoom_In_Map_Image_clicked() {
    resize(40,30);// test
    return;
    if (scaleRegionMapImage >= 8.0) return;
    scaleRegionMapImage *= 2.0;
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_BkgImg->scale(2.0,2.0);
    this->ui->graphicsView_Region_Map_Layout->scale(2.0,2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_Map_Image_clicked() {
    if (scaleRegionMapImage <= 1.0) return;
    scaleRegionMapImage /= 2.0;
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_BkgImg->scale(0.5,0.5);
    this->ui->graphicsView_Region_Map_Layout->scale(0.5,0.5);
}
