#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QMainWindow>
#include "project.h"
#include "history.h"
#include "paletteeditor.h"
#include "tileseteditormetatileselector.h"
#include "tileseteditortileselector.h"
#include "metatilelayersitem.h"

namespace Ui {
class TilesetEditor;
}

class MetatileHistoryItem {
public:
    MetatileHistoryItem(uint16_t metatileId, Metatile *prevMetatile, Metatile *newMetatile) {
        this->metatileId = metatileId;
        this->prevMetatile = prevMetatile;
        this->newMetatile = newMetatile;
    }
    ~MetatileHistoryItem() {
        delete this->prevMetatile;
        delete this->newMetatile;
    }
    uint16_t metatileId;
    Metatile *prevMetatile;
    Metatile *newMetatile;
};

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
    void onMetatileLayerSelectionChanged(QPoint, int, int);
    void onPaletteEditorChangedPaletteColor();
    void onPaletteEditorChangedPalette(int);

    void on_spinBox_paletteSelector_valueChanged(int arg1);

    void on_checkBox_xFlip_stateChanged(int arg1);

    void on_checkBox_yFlip_stateChanged(int arg1);

    void on_actionSave_Tileset_triggered();

    void on_actionImport_Primary_Tiles_triggered();

    void on_actionImport_Secondary_Tiles_triggered();

    void on_actionChange_Metatiles_Count_triggered();

    void on_actionChange_Palettes_triggered();

    void on_actionUndo_triggered();

    void on_actionRedo_triggered();

    void on_comboBox_metatileBehaviors_activated(const QString &arg1);

    void on_lineEdit_metatileLabel_editingFinished();

    void on_comboBox_layerType_activated(int arg1);

    void on_comboBox_encounterType_activated(int arg1);

    void on_comboBox_terrainType_activated(int arg1);

    void on_actionExport_Primary_Tiles_Image_triggered();

    void on_actionExport_Secondary_Tiles_Image_triggered();

    void on_actionImport_Primary_Metatiles_triggered();

    void on_actionImport_Secondary_Metatiles_triggered();

private:
    void closeEvent(QCloseEvent*);
    void initMetatileSelector();
    void initTileSelector();
    void initSelectedTileItem();
    void initMetatileLayersItem();
    void drawSelectedTiles();
    void importTilesetTiles(Tileset*, bool);
    void importTilesetMetatiles(Tileset*, bool);
    void refresh();
    void saveMetatileLabel();
    Ui::TilesetEditor *ui;
    History<MetatileHistoryItem*> metatileHistory;
    TilesetEditorMetatileSelector *metatileSelector = nullptr;
    TilesetEditorTileSelector *tileSelector = nullptr;
    MetatileLayersItem *metatileLayersItem = nullptr;
    PaletteEditor *paletteEditor = nullptr;
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
};

#endif // TILESETEDITOR_H
