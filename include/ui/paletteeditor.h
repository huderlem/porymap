#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QMainWindow>
#include <QSlider>
#include <QFrame>
#include <QLabel>
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
    QList<QList<QSlider*>> sliders;
    QList<QFrame*> frames;
    QList<QLabel*> rgbLabels;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QList<History<PaletteHistoryItem*>> palettesHistory;
    void disableSliderSignals();
    void enableSliderSignals();
    void initColorSliders();
    void refreshColorSliders();
    void refreshColors();
    void refreshColor(int);
    void setColor(int);
    void commitEditHistory(int paletteid);
    void setColorsFromHistory(PaletteHistoryItem*, int);

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
