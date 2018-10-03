#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QMainWindow>
#include "project.h"
#include "tileseteditormetatileselector.h"
#include "tileseteditortileselector.h"
#include "metatilelayersitem.h"

namespace Ui {
class TilesetEditor;
}

class TilesetEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit TilesetEditor(Project*, QString, QString, QWidget *parent = nullptr);
    ~TilesetEditor();
    void setTilesets(QString, QString);
    void init(Project*, QString, QString);

private slots:
    void onHoveredMetatileChanged(uint16_t);
    void onHoveredMetatileCleared();
    void onSelectedMetatileChanged(uint16_t);
    void onHoveredTileChanged(uint16_t);
    void onHoveredTileCleared();
    void onSelectedTilesChanged();
    void onMetatileLayerTileChanged(int, int);

    void on_spinBox_paletteSelector_valueChanged(int arg1);

    void on_checkBox_xFlip_stateChanged(int arg1);

    void on_checkBox_yFlip_stateChanged(int arg1);

    void on_comboBox_metatileBehaviors_currentIndexChanged(const QString &arg1);

    void on_comboBox_layerType_currentIndexChanged(int index);

    void on_actionSave_Tileset_triggered();

    void on_actionImport_Primary_Tiles_triggered();

    void on_actionImport_Secondary_Tiles_triggered();

    void on_actionChange_Metatiles_Count_triggered();

private:
    void closeEvent(QCloseEvent*);
    void initMetatileSelector();
    void initTileSelector();
    void initSelectedTileItem();
    void initMetatileLayersItem();
    void drawSelectedTiles();
    void importTilesetTiles(Tileset*, bool);
    void refresh();
    Ui::TilesetEditor *ui;
    TilesetEditorMetatileSelector *metatileSelector = nullptr;
    TilesetEditorTileSelector *tileSelector = nullptr;
    MetatileLayersItem *metatileLayersItem = nullptr;
    Project *project = nullptr;
    Metatile *metatile = nullptr;
    int paletteId;
    bool tileXFlip;
    bool tileYFlip;
    bool hasUnsavedChanges;
    Tileset *primaryTileset = nullptr;
    Tileset *secondaryTileset = nullptr;
    QGraphicsScene *metatilesScene = nullptr;
    QGraphicsScene *tilesScene = nullptr;
    QGraphicsScene *selectedTileScene = nullptr;
    QGraphicsPixmapItem *selectedTilePixmapItem = nullptr;
    QGraphicsScene *metatileLayersScene = nullptr;

signals:
    void tilesetsSaved(QString, QString);
    void closed();
};

#endif // TILESETEDITOR_H
