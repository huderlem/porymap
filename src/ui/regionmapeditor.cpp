#include "regionmapeditor.h"
#include "ui_regionmapeditor.h"

#include <QDir>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QColor>
#include <QTimer>

RegionMapEditor::RegionMapEditor(QWidget *parent, Project *pro) :
    QMainWindow(parent),
    ui(new Ui::RegionMapEditor)
{
    this->ui->setupUi(this);
    this->project = pro;
    this->region_map = new RegionMap;
    this->setFixedSize(this->size());//statusbar->setSizeGripEnabled(false);
}

RegionMapEditor::~RegionMapEditor()
{
    delete ui;
}



void RegionMapEditor::on_action_RegionMap_Save_triggered() {
    qDebug() << "Region Map Save Triggered";
    if (project && region_map) {
        qDebug() << "actually saving";
        region_map->save();
        displayRegionMap();
    }
}


void RegionMapEditor::loadRegionMapData() {
    //
    this->region_map->init(project);
    displayRegionMap();
}

void RegionMapEditor::loadCityMaps() {
    //
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

// TODO: change the signal slot to new syntax
// TODO: add scalability?
void RegionMapEditor::displayRegionMapImage() {
    //
    this->region_map_item = new RegionMapPixmapItem(this->region_map, this->mapsquare_selector_item);
    connect(region_map_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*, RegionMapPixmapItem*)),
            this, SLOT(mouseEvent_region_map(QGraphicsSceneMouseEvent*, RegionMapPixmapItem*)));
    connect(region_map_item, SIGNAL(hoveredRegionMapTileChanged(int, int)),
            this, SLOT(onHoveredRegionMapTileChanged(int, int)));
    connect(region_map_item, SIGNAL(hoveredRegionMapTileCleared()),
            this, SLOT(onHoveredRegionMapTileCleared()));
    this->region_map_item->draw();

    this->scene_region_map_image = new QGraphicsScene;
    this->scene_region_map_image->addItem(this->region_map_item);
    this->scene_region_map_image->setSceneRect(this->scene_region_map_image->sceneRect());

    this->ui->graphicsView_Region_Map_BkgImg->setScene(this->scene_region_map_image);
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize());
}

// TODO: add if (item) and if(scene) checks because called more than once per instance
void RegionMapEditor::displayRegionMapLayout() {
    //
    this->region_map_layout_item = new RegionMapLayoutPixmapItem(this->region_map, this->mapsquare_selector_item);
    //*
    connect(this->region_map_layout_item, &RegionMapLayoutPixmapItem::selectedTileChanged,
            this, &RegionMapEditor::onRegionMapLayoutSelectedTileChanged);// TODO: remove this?
    connect(this->region_map_layout_item, &RegionMapLayoutPixmapItem::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapLayoutHoveredTileChanged);
    connect(this->region_map_layout_item, &RegionMapLayoutPixmapItem::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapLayoutHoveredTileCleared);
    //*/
    this->region_map_layout_item->draw();
    this->region_map_layout_item->setDefaultSelection();

    this->scene_region_map_layout = new QGraphicsScene;
    this->scene_region_map_layout->addItem(region_map_layout_item);
    this->scene_region_map_layout->setSceneRect(this->scene_region_map_layout->sceneRect());

    this->ui->graphicsView_Region_Map_Layout->setScene(this->scene_region_map_layout);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize());
}

void RegionMapEditor::displayRegionMapLayoutOptions() {
    //
    this->ui->comboBox_RM_ConnectedMap->addItems(*(this->project->regionMapSections));

    this->ui->frame_RM_Options->setEnabled(true);

    // TODO: change these values to variables
    this->ui->spinBox_RM_Options_x->setMaximum(27);
    this->ui->spinBox_RM_Options_y->setMaximum(14);

    updateRegionMapLayoutOptions(65);
}

void RegionMapEditor::updateRegionMapLayoutOptions(int index) {
    //
    this->ui->lineEdit_RM_MapName->setText(this->project->mapSecToMapHoverName->value(this->region_map->map_squares[index].mapsec));//this->region_map->map_squares[index].map_name);
    this->ui->comboBox_RM_ConnectedMap->setCurrentText(this->region_map->map_squares[index].mapsec);
    this->ui->spinBox_RM_Options_x->setValue(this->region_map->map_squares[index].x);
    this->ui->spinBox_RM_Options_y->setValue(this->region_map->map_squares[index].y);
}

// TODO: get this to display on a decent scale
void RegionMapEditor::displayRegionMapTileSelector() {
    //
    this->mapsquare_selector_item = new TilemapTileSelector(QPixmap(this->region_map->region_map_png_path));
    this->mapsquare_selector_item->draw();

    this->scene_region_map_tiles = new QGraphicsScene;
    this->scene_region_map_tiles->addItem(this->mapsquare_selector_item);

    connect(this->mapsquare_selector_item, &TilemapTileSelector::selectedTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged);// TODO: remove this?
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged);
    connect(this->mapsquare_selector_item, &TilemapTileSelector::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared);

    this->ui->graphicsView_RegionMap_Tiles->setScene(this->scene_region_map_tiles);
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth + 2,
                                                         this->mapsquare_selector_item->pixelHeight + 2);
}

void RegionMapEditor::displayCityMapTileSelector() {
    // city_map_selector_item
    this->city_map_selector_item = new TilemapTileSelector(QPixmap(this->region_map->region_map_city_map_tiles_path));
    this->city_map_selector_item->draw();

    this->scene_city_map_tiles = new QGraphicsScene;
    this->scene_city_map_tiles->addItem(this->city_map_selector_item);

    /*connect(this->city_map_selector_item, &TilemapTileSelector::selectedTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged);// TODO: remove this?
    connect(this->city_map_selector_item, &TilemapTileSelector::hoveredTileChanged,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged);
    connect(this->city_map_selector_item, &TilemapTileSelector::hoveredTileCleared,
            this, &RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared);*/

    this->ui->graphicsView_City_Map_Tiles->setScene(this->scene_city_map_tiles);
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth + 2,
                                                        this->city_map_selector_item->pixelHeight + 2);
    //this->ui->graphicsView_City_Map_Tiles->scale(2,2);
}

void RegionMapEditor::displayCityMap(QString f) {
    //
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

    connect(city_map_item, SIGNAL(mouseEvent(QGraphicsSceneMouseEvent*, CityMapPixmapItem*)),
            this, SLOT(mouseEvent_city_map(QGraphicsSceneMouseEvent*, CityMapPixmapItem*)));

    scene_city_map_image->addItem(city_map_item);
    scene_city_map_image->setSceneRect(this->scene_city_map_image->sceneRect());

    this->ui->graphicsView_City_Map->setScene(scene_city_map_image);
    this->ui->graphicsView_City_Map->setFixedSize(QSize(8 * city_map_item->width + 2, 8 * city_map_item->height + 2));
}

bool RegionMapEditor::createCityMap(QString name) {
    //
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



////

void RegionMapEditor::onRegionMapTileSelectorSelectedTileChanged() {
    //
}

void RegionMapEditor::onRegionMapTileSelectorHoveredTileChanged(unsigned tileId) {
    QString message = QString("Tile: 0x") + QString("%1").arg(tileId, 4, 16, QChar('0')).toUpper();
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onRegionMapTileSelectorHoveredTileCleared() {
    //
    //QString message = QString("Selected Tile: 0x") + QString("%1").arg(this->region_map_layout_item->selectedTile, 4, 16, QChar('0')).toUpper();
    //this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onRegionMapLayoutSelectedTileChanged(int index) {
    //
    QString message = QString();
    if (this->region_map->map_squares[index].has_map) {
        //
        message = QString("\t %1").arg(this->project->mapSecToMapHoverName->value(
                      this->region_map->map_squares[index].mapsec)).remove("{NAME_END}");//.remove("{NAME_END}")
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
        //
            message += QString("\t %1").arg(this->project->mapSecToMapHoverName->value(
                      this->region_map->map_squares[index].mapsec)).remove("{NAME_END}");
        }
    }
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onRegionMapLayoutHoveredTileCleared() {
    //
    int index = this->region_map_layout_item->selectedTile;
    QString message = QString();
    int x = this->region_map->map_squares[index].x;
    int y = this->region_map->map_squares[index].y;
    if (x >= 0 && y >= 0) {
        message = QString("(%1, %2)").arg(x).arg(y);
        if (this->region_map->map_squares[index].has_map) {
        //
            message += QString("\t %1").arg(this->project->mapSecToMapHoverName->value(
                      this->region_map->map_squares[index].mapsec)).remove("{NAME_END}");
        }
    }
    this->ui->statusbar->showMessage(message);
}

void RegionMapEditor::onHoveredRegionMapTileChanged(int x, int y) {
    rmStatusbarMessage = QString("x: %1, y: %2   Tile: 0x").arg(x).arg(y) + QString("%1").arg(this->region_map->getTileId(x, y), 4, 16, QChar('0')).toUpper();
    this->ui->statusbar->showMessage(rmStatusbarMessage);
}

void RegionMapEditor::onHoveredRegionMapTileCleared() {
    this->ui->statusbar->clearMessage();
}

void RegionMapEditor::mouseEvent_region_map(QGraphicsSceneMouseEvent *event, RegionMapPixmapItem *item) {
    //
    if (event->buttons() & Qt::RightButton) {
        //
        item->select(event);
    } else if (event->buttons() & Qt::MiddleButton) {
        // TODO: add functionality here? replace or?
    } else {
        //
        QPointF pos = event->pos();
        int x = static_cast<int>(pos.x()) / 8;
        int y = static_cast<int>(pos.y()) / 8;
        int index = this->region_map->getMapSquareIndex(x, y);
        RegionMapHistoryItem *commit = new RegionMapHistoryItem(RegionMapEditorBox::BackgroundImage, index, 
            this->region_map->map_squares[index].tile_img_id, this->mapsquare_selector_item->getSelectedTile());
        history.push(commit);
        item->paint(event);
    }
}

void RegionMapEditor::mouseEvent_city_map(QGraphicsSceneMouseEvent *event, CityMapPixmapItem *item) {
    //
    if (event->buttons() & Qt::RightButton) {
        //
        //item->select(event);
    } else if (event->buttons() & Qt::MiddleButton) {
        // TODO: add functionality here? replace or?
    } else {
        //
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








////

void RegionMapEditor::on_tabWidget_Region_Map_currentChanged(int index) {
    //
    this->ui->stackedWidget_RM_Options->setCurrentIndex(index);
}

void RegionMapEditor::on_pushButton_RM_Options_save_clicked() {
    //
    this->region_map->saveOptions(
        //
        this->region_map_layout_item->selectedTile,
        this->ui->comboBox_RM_ConnectedMap->currentText(),
        this->ui->lineEdit_RM_MapName->text(),
        this->ui->spinBox_RM_Options_x->value(),
        this->ui->spinBox_RM_Options_y->value()
    );
    this->region_map_layout_item->draw();
}

void RegionMapEditor::on_pushButton_CityMap_save_clicked() {
    this->city_map_item->save();
}

void RegionMapEditor::on_pushButton_RM_Options_delete_clicked() {
    qDebug() << "delete it fat";
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

void RegionMapEditor::on_action_RegionMap_Undo_triggered() {
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

void RegionMapEditor::on_comboBox_CityMap_picker_currentTextChanged(const QString &file) {
    this->displayCityMap(file);
}



// TODO: prevent huge images with limits on zoom
void RegionMapEditor::on_pushButton_Zoom_In_Image_Tiles_clicked() {
    //
    scaleRegionMapTiles *= 2.0;
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * scaleRegionMapTiles + 2,
                                                         this->mapsquare_selector_item->pixelHeight * scaleRegionMapTiles + 2);
    this->ui->graphicsView_RegionMap_Tiles->scale(2.0, 2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_Image_Tiles_clicked() {
    //
    scaleRegionMapTiles /= 2.0;
    this->ui->graphicsView_RegionMap_Tiles->setFixedSize(this->mapsquare_selector_item->pixelWidth * scaleRegionMapTiles + 2,
                                                         this->mapsquare_selector_item->pixelHeight * scaleRegionMapTiles + 2);
    this->ui->graphicsView_RegionMap_Tiles->scale(0.5, 0.5);
}

void RegionMapEditor::on_pushButton_Zoom_In_City_Tiles_clicked() {
    //
    scaleCityMapTiles *= 2.0;
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * scaleCityMapTiles + 2,
                                                        this->city_map_selector_item->pixelHeight * scaleCityMapTiles + 2);
    this->ui->graphicsView_City_Map_Tiles->scale(2.0,2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_City_Tiles_clicked() {
    //
    scaleCityMapTiles /= 2.0;
    this->ui->graphicsView_City_Map_Tiles->setFixedSize(this->city_map_selector_item->pixelWidth * scaleCityMapTiles + 2,
                                                        this->city_map_selector_item->pixelHeight * scaleCityMapTiles + 2);
    this->ui->graphicsView_City_Map_Tiles->scale(0.5,0.5);
}

void RegionMapEditor::on_pushButton_Zoom_In_City_Map_clicked() {
    //
    scaleCityMapImage *= 2.0;
    this->ui->graphicsView_City_Map->setFixedSize(QSize(8 * city_map_item->width * scaleCityMapImage + 2, 8 * city_map_item->height * scaleCityMapImage + 2));
    this->ui->graphicsView_City_Map->scale(2.0,2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_City_Map_clicked() {
    //
    scaleCityMapImage /= 2.0;
    this->ui->graphicsView_City_Map->setFixedSize(QSize(8 * city_map_item->width * scaleCityMapImage + 2, 8 * city_map_item->height * scaleCityMapImage + 2));
    this->ui->graphicsView_City_Map->scale(0.5,0.5);
}

void RegionMapEditor::on_pushButton_Zoom_In_Map_Image_clicked() {
    //
    //qDebug() << "scale:" << scaleRegionMapImage;
    scaleRegionMapImage *= 2.0;
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_BkgImg->scale(2.0,2.0);
    this->ui->graphicsView_Region_Map_Layout->scale(2.0,2.0);
}

void RegionMapEditor::on_pushButton_Zoom_Out_Map_Image_clicked() {
    //
    if (scaleRegionMapImage <= 1.0) return;
    scaleRegionMapImage /= 2.0;
    this->ui->graphicsView_Region_Map_BkgImg->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_Layout->setFixedSize(this->region_map->imgSize() * scaleRegionMapImage);
    this->ui->graphicsView_Region_Map_BkgImg->scale(0.5,0.5);
    this->ui->graphicsView_Region_Map_Layout->scale(0.5,0.5);
}




















