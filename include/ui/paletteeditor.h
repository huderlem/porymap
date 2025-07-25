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
    int currentPaletteId() const;

    void setTilesets(Tileset*, Tileset*);

    bool showingUnusedColors() const;

private:
    Ui::PaletteEditor *ui;
    Project *project = nullptr;
    QList<ColorInputWidget*> colorInputs;

    Tileset *primaryTileset;
    Tileset *secondaryTileset;

    QList<History<PaletteHistoryItem*>> palettesHistory;
    QMap<int,QSet<int>> unusedColorCache;

    Tileset* getTileset(int paletteId) const;
    void refreshColorInputs();
    void commitEditHistory();
    void commitEditHistory(int paletteId);
    void restoreWindowState();
    void onWindowActivated();
    void invalidateCache();
    void closeEvent(QCloseEvent*);
    void setColorInputTitles(bool show);
    QSet<int> getUnusedColorIds() const;

    void setRgb(int index, QRgb rgb);
    void setPalette(int paletteId, const QList<QRgb> &palette);

    void setBitDepth(int bits);
    int bitDepth = 24;

    static const int numColors = Tileset::numColorsPerPalette();

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
