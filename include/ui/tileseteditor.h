#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QMainWindow>
#include "project.h"
#include "tileseteditormetatileselector.h"
#include "tileseteditortileselector.h"

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

private:
    void initMetatileSelector();
    void initTileSelector();
    Ui::TilesetEditor *ui;
    TilesetEditorMetatileSelector *metatileSelector;
    TilesetEditorTileSelector *tileSelector;
    Project *project;
    uint paletteNum;
    QString primaryTilesetLabel;
    QString secondaryTilesetLabel;
    QGraphicsScene *metatilesScene;
    QGraphicsScene *tilesScene;
};

#endif // TILESETEDITOR_H
