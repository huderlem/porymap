#ifndef TILESETEDITOR_H
#define TILESETEDITOR_H

#include <QMainWindow>
#include "project.h"

namespace Ui {
class TilesetEditor;
}

class TilesetEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit TilesetEditor(QWidget *parent = nullptr, Project *project = nullptr);
    ~TilesetEditor();

private:
    void displayPrimaryTilesetTiles();
    Ui::TilesetEditor *ui;
    Project *project;
    QString primaryTilesetLabel;
};

#endif // TILESETEDITOR_H
