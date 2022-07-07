#ifndef PALETTEEDITOR_H
#define PALETTEEDITOR_H

#include <QMainWindow>
#include <QValidator>
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
    QList<QToolButton *> pickButtons;
    QList<QLineEdit *> hexEdits;

    Tileset *primaryTileset;
    Tileset *secondaryTileset;

    QList<History<PaletteHistoryItem*>> palettesHistory;

    void refreshColorUis();
    void updateColorUi(int index, QRgb color);
    void commitEditHistory(int paletteid);
    void restoreWindowState();
    void setSignalsEnabled(bool enabled);
    void setColorsFromHistory(PaletteHistoryItem*, int);
    void closeEvent(QCloseEvent*);
    void pickColor(int i);

    void setRgb(int index, QRgb rgb);
    void setRgbFromSliders(int colorIndex);
    void setRgbFromHexEdit(int colorIndex);
    void setRgbFromSpinners(int colorIndex);

    void setBitDepth(int bits);
    int bitDepth = 24;

    class HexCodeValidator : public QValidator {
        virtual QValidator::State validate(QString &input, int &) const override {
            input = input.toUpper();
            return QValidator::Acceptable;
        }
    };

    HexCodeValidator *hexValidator = nullptr;

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
