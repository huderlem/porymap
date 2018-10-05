#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QMainWindow>
#include <QSlider>
#include <QFrame>
#include <QLabel>
#include "project.h"

namespace Ui {
class PaletteEditor;
}

class PaletteEditor :  public QMainWindow {
    Q_OBJECT
public:
    explicit PaletteEditor(Project*, Tileset*, Tileset*, QWidget *parent = nullptr);
    ~PaletteEditor();
    void setPaletteId(int);

private:
    Ui::PaletteEditor *ui;
    Project *project = nullptr;
    QList<QList<QSlider*>> sliders;
    QList<QFrame*> frames;
    QList<QLabel*> rgbLabels;
    Tileset *primaryTileset;
    Tileset *secondaryTileset;
    void disableSliderSignals();
    void enableSliderSignals();
    void initColorSliders();
    void refreshColorSliders();
    void refreshColors();
    void refreshColor(int);
    void setColor(int);

signals:
    void closed();
    void changedPaletteColor();
    void changedPalette(int);
private slots:
    void on_spinBox_PaletteId_valueChanged(int arg1);
};

#endif // PALETTEEDITOR_H
