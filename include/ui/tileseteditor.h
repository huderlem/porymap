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

private slots:
    void onHoveredMetatileChanged(uint16_t);
    void onHoveredMetatileCleared();
    void onSelectedMetatileChanged(uint16_t);
    void onHoveredTileChanged(uint16_t);
    void onHoveredTileCleared();
    void onSelectedTileChanged(uint16_t);
    void onMetatileLayerTileChanged(int);

    void on_spinBox_paletteSelector_valueChanged(int arg1);

    void on_checkBox_xFlip_stateChanged(int arg1);

    void on_checkBox_yFlip_stateChanged(int arg1);

private:
    void initMetatileSelector();
    void initTileSelector();
    void initSelectedTileItem();
    void initMetatileLayersItem();
    void drawSelectedTile();
    Ui::TilesetEditor *ui;
    TilesetEditorMetatileSelector *metatileSelector = nullptr;
    TilesetEditorTileSelector *tileSelector = nullptr;
    MetatileLayersItem *metatileLayersItem = nullptr;
    Project *project;
    Metatile *metatile;
    int paletteId;
    bool tileXFlip;
    bool tileYFlip;
    QString primaryTilesetLabel;
    QString secondaryTilesetLabel;
    QGraphicsScene *metatilesScene = nullptr;
    QGraphicsScene *tilesScene = nullptr;
    QGraphicsScene *selectedTileScene = nullptr;
    QGraphicsPixmapItem *selectedTilePixmapItem = nullptr;
    QGraphicsScene *metatileLayersScene = nullptr;
};

#endif // TILESETEDITOR_H
