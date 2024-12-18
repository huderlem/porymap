#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QMainWindow>

#include "colorinputwidget.h"
#include "project.h"
#include "history.h"

namespace Ui {
class PaletteEditor;
}

class PaletteHistoryItem {
public:
    QList<QRgb> colors;
    PaletteHistoryItem(QList<QRgb> colors) {
        this->colors = colors;
    }
};

class PaletteEditor :  public QMainWindow {
    Q_OBJECT
public:
    explicit PaletteEditor(Project*, Tileset*, Tileset*, int paletteId, QWidget *parent = nullptr);
    ~PaletteEditor();
    void setPaletteId(int);
    void setTilesets(Tileset*, Tileset*);

private:
    Ui::PaletteEditor *ui;
    Project *project = nullptr;
    QList<ColorInputWidget*> colorInputs;

    Tileset *primaryTileset;
    Tileset *secondaryTileset;

    QList<History<PaletteHistoryItem*>> palettesHistory;

    Tileset* getTileset(int paletteId);
    void refreshColorInputs();
    void commitEditHistory();
    void commitEditHistory(int paletteId);
    void restoreWindowState();
    void closeEvent(QCloseEvent*);

    void setRgb(int index, QRgb rgb);
    void setPalette(int paletteId, const QList<QRgb> &palette);

    void setBitDepth(int bits);
    int bitDepth = 24;

    static const int numColors = 16;

signals:
    void closed();
    void changedPaletteColor();
    void changedPalette(int);
private slots:
    void on_spinBox_PaletteId_valueChanged(int arg1);
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionImport_Palette_triggered();
};

#endif // PALETTEEDITOR_H
