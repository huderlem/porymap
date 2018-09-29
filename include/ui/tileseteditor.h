#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QMainWindow>
#include "project.h"
#include "tileseteditormetatileselector.h"

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

private:
    void initMetatilesSelector();
    Ui::TilesetEditor *ui;
    TilesetEditorMetatileSelector *metatileSelector;
    Project *project;
    QString primaryTilesetLabel;
    QString secondaryTilesetLabel;
    QGraphicsScene *metatilesScene;
};

#endif // TILESETEDITOR_H
