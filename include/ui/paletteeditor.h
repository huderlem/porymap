#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QMainWindow>
#include <QPointer>

#include "colorinputwidget.h"
#include "project.h"
#include "history.h"
#include "palettecolorsearch.h"

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

signals:
    void metatileSelected(uint16_t metatileId);

private:
    Ui::PaletteEditor *ui;
    Project *project;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;

    QList<ColorInputWidget*> colorInputs;
    QList<History<PaletteHistoryItem*>> palettesHistory;
    QMap<int,QSet<int>> unusedColorCache;
    QPointer<PaletteColorSearch> colorSearchWindow;

    Tileset* getTileset(int paletteId) const;
    void refreshColorInputs();
    void refreshPaletteId();
    void commitEditHistory();
    void commitEditHistory(int paletteId);
    void restoreWindowState();
    void invalidateCache();
    void closeEvent(QCloseEvent*);
    void setColorInputTitles(bool show);
    QSet<int> getUnusedColorIds();
    void openColorSearch();

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
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionImport_Palette_triggered();
};

#endif // PALETTEEDITOR_H
