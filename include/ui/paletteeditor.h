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
    QList<QList<QSpinBox *>> spinners;
    QList<QFrame*> frames;
    QList<QLabel*> rgbLabels;
    QList<QToolButton *> pickButtons;
    QList<QLineEdit *> hexEdits;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    QList<History<PaletteHistoryItem*>> palettesHistory;
    void initConnections();
    void refreshColorSliders();
    void refreshColorUis();
    void refreshColors();
    void refreshColor(int);
    void setColor(int);
    void commitEditHistory(int paletteid);
    void restoreWindowState();
    void setColorsFromHistory(PaletteHistoryItem*, int);
    void closeEvent(QCloseEvent*);
    void pickColor(int i);

    void updateColorUi(int index, QRgb color);
    QRgb roundRgb(QRgb rgb); // rgb5 * 8 each component
    QColor roundColor(QColor color);
    void setRgb(int index, QRgb rgb);

    void setRgbFromSliders(int colorIndex);
    void setRgbFromHexEdit(int colorIndex);
    void setRgbFromSpinners(int colorIndex);

    void setSignalsEnabled(bool enabled);

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
