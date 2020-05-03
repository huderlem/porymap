#include "paletteeditor.h"
#include "ui_paletteeditor.h"
#include "paletteutil.h"
#include <QFileDialog>
#include <QMessageBox>
#include "log.h"

PaletteEditor::PaletteEditor(Project *project, Tileset *primaryTileset, Tileset *secondaryTileset, int paletteId, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PaletteEditor)
{
    this->project = project;
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->ui->setupUi(this);
    this->ui->spinBox_PaletteId->setMinimum(0);
    this->ui->spinBox_PaletteId->setMaximum(Project::getNumPalettesTotal() - 1);
    this->sliders.clear();
    for (int i = 0; i < 16; i++) {
        this->sliders.append(QList<QSlider*>());
    }
    this->sliders[0].append(this->ui->horizontalSlider);
    this->sliders[0].append(this->ui->horizontalSlider_2);
    this->sliders[0].append(this->ui->horizontalSlider_3);
    this->sliders[1].append(this->ui->horizontalSlider_4);
    this->sliders[1].append(this->ui->horizontalSlider_5);
    this->sliders[1].append(this->ui->horizontalSlider_6);
    this->sliders[2].append(this->ui->horizontalSlider_7);
    this->sliders[2].append(this->ui->horizontalSlider_8);
    this->sliders[2].append(this->ui->horizontalSlider_9);
    this->sliders[3].append(this->ui->horizontalSlider_10);
    this->sliders[3].append(this->ui->horizontalSlider_11);
    this->sliders[3].append(this->ui->horizontalSlider_12);
    this->sliders[4].append(this->ui->horizontalSlider_13);
    this->sliders[4].append(this->ui->horizontalSlider_14);
    this->sliders[4].append(this->ui->horizontalSlider_15);
    this->sliders[5].append(this->ui->horizontalSlider_16);
    this->sliders[5].append(this->ui->horizontalSlider_17);
    this->sliders[5].append(this->ui->horizontalSlider_18);
    this->sliders[6].append(this->ui->horizontalSlider_19);
    this->sliders[6].append(this->ui->horizontalSlider_20);
    this->sliders[6].append(this->ui->horizontalSlider_21);
    this->sliders[7].append(this->ui->horizontalSlider_22);
    this->sliders[7].append(this->ui->horizontalSlider_23);
    this->sliders[7].append(this->ui->horizontalSlider_24);
    this->sliders[8].append(this->ui->horizontalSlider_25);
    this->sliders[8].append(this->ui->horizontalSlider_26);
    this->sliders[8].append(this->ui->horizontalSlider_27);
    this->sliders[9].append(this->ui->horizontalSlider_28);
    this->sliders[9].append(this->ui->horizontalSlider_29);
    this->sliders[9].append(this->ui->horizontalSlider_30);
    this->sliders[10].append(this->ui->horizontalSlider_31);
    this->sliders[10].append(this->ui->horizontalSlider_32);
    this->sliders[10].append(this->ui->horizontalSlider_33);
    this->sliders[11].append(this->ui->horizontalSlider_34);
    this->sliders[11].append(this->ui->horizontalSlider_35);
    this->sliders[11].append(this->ui->horizontalSlider_36);
    this->sliders[12].append(this->ui->horizontalSlider_37);
    this->sliders[12].append(this->ui->horizontalSlider_38);
    this->sliders[12].append(this->ui->horizontalSlider_39);
    this->sliders[13].append(this->ui->horizontalSlider_40);
    this->sliders[13].append(this->ui->horizontalSlider_41);
    this->sliders[13].append(this->ui->horizontalSlider_42);
    this->sliders[14].append(this->ui->horizontalSlider_43);
    this->sliders[14].append(this->ui->horizontalSlider_44);
    this->sliders[14].append(this->ui->horizontalSlider_45);
    this->sliders[15].append(this->ui->horizontalSlider_46);
    this->sliders[15].append(this->ui->horizontalSlider_47);
    this->sliders[15].append(this->ui->horizontalSlider_48);

    this->frames.clear();
    this->frames.append(this->ui->frame);
    this->frames.append(this->ui->frame_2);
    this->frames.append(this->ui->frame_3);
    this->frames.append(this->ui->frame_4);
    this->frames.append(this->ui->frame_5);
    this->frames.append(this->ui->frame_6);
    this->frames.append(this->ui->frame_7);
    this->frames.append(this->ui->frame_8);
    this->frames.append(this->ui->frame_9);
    this->frames.append(this->ui->frame_10);
    this->frames.append(this->ui->frame_11);
    this->frames.append(this->ui->frame_12);
    this->frames.append(this->ui->frame_13);
    this->frames.append(this->ui->frame_14);
    this->frames.append(this->ui->frame_15);
    this->frames.append(this->ui->frame_16);

    this->rgbLabels.clear();
    this->rgbLabels.append(this->ui->label_rgb0);
    this->rgbLabels.append(this->ui->label_rgb1);
    this->rgbLabels.append(this->ui->label_rgb2);
    this->rgbLabels.append(this->ui->label_rgb3);
    this->rgbLabels.append(this->ui->label_rgb4);
    this->rgbLabels.append(this->ui->label_rgb5);
    this->rgbLabels.append(this->ui->label_rgb6);
    this->rgbLabels.append(this->ui->label_rgb7);
    this->rgbLabels.append(this->ui->label_rgb8);
    this->rgbLabels.append(this->ui->label_rgb9);
    this->rgbLabels.append(this->ui->label_rgb10);
    this->rgbLabels.append(this->ui->label_rgb11);
    this->rgbLabels.append(this->ui->label_rgb12);
    this->rgbLabels.append(this->ui->label_rgb13);
    this->rgbLabels.append(this->ui->label_rgb14);
    this->rgbLabels.append(this->ui->label_rgb15);

    // Setup edit-undo history for each of the palettes.
    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        this->palettesHistory.append(History<PaletteHistoryItem*>());
    }

    this->initColorSliders();
    this->setPaletteId(paletteId);
    this->commitEditHistory(this->ui->spinBox_PaletteId->value());
}

PaletteEditor::~PaletteEditor()
{
    delete ui;
}

void PaletteEditor::disableSliderSignals() {
    for (int i = 0; i < this->sliders.length(); i++) {
        this->sliders.at(i).at(0)->blockSignals(true);
        this->sliders.at(i).at(1)->blockSignals(true);
        this->sliders.at(i).at(2)->blockSignals(true);
    }
}

void PaletteEditor::enableSliderSignals() {
    for (int i = 0; i < this->sliders.length(); i++) {
        this->sliders.at(i).at(0)->blockSignals(false);
        this->sliders.at(i).at(1)->blockSignals(false);
        this->sliders.at(i).at(2)->blockSignals(false);
    }
}

void PaletteEditor::initColorSliders() {
    for (int i = 0; i < 16; i++) {
        connect(this->sliders[i][0], &QSlider::valueChanged, [=](int) { this->setColor(i); });
        connect(this->sliders[i][1], &QSlider::valueChanged, [=](int) { this->setColor(i); });
        connect(this->sliders[i][2], &QSlider::valueChanged, [=](int) { this->setColor(i); });
    }
}

void PaletteEditor::refreshColorSliders() {
    disableSliderSignals();
    int paletteNum = this->ui->spinBox_PaletteId->value();
    for (int i = 0; i < 16; i++) {
        QRgb color;
        if (paletteNum < Project::getNumPalettesPrimary()) {
            color = this->primaryTileset->palettes->at(paletteNum).at(i);
        } else {
            color = this->secondaryTileset->palettes->at(paletteNum).at(i);
        }

        this->sliders[i][0]->setValue(qRed(color)   / 8);
        this->sliders[i][1]->setValue(qGreen(color) / 8);
        this->sliders[i][2]->setValue(qBlue(color)  / 8);
    }
    enableSliderSignals();
}

void PaletteEditor::refreshColors() {
    for (int i = 0; i < 16; i++) {
        this->refreshColor(i);
    }
}

void PaletteEditor::refreshColor(int colorIndex) {
    int red = this->sliders[colorIndex][0]->value() * 8;
    int green = this->sliders[colorIndex][1]->value() * 8;
    int blue = this->sliders[colorIndex][2]->value() * 8;
    QString stylesheet = QString("background-color: rgb(%1, %2, %3);")
            .arg(red)
            .arg(green)
            .arg(blue);
    this->frames[colorIndex]->setStyleSheet(stylesheet);
    this->rgbLabels[colorIndex]->setText(QString("RGB(%1, %2, %3)").arg(red).arg(green).arg(blue));
}

void PaletteEditor::setPaletteId(int paletteId) {
    this->ui->spinBox_PaletteId->blockSignals(true);
    this->ui->spinBox_PaletteId->setValue(paletteId);
    this->refreshColorSliders();
    this->refreshColors();
    this->ui->spinBox_PaletteId->blockSignals(false);
}

void PaletteEditor::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->refreshColorSliders();
    this->refreshColors();
}

void PaletteEditor::setColor(int colorIndex) {
    int paletteNum = this->ui->spinBox_PaletteId->value();
    int red = this->sliders[colorIndex][0]->value() * 8;
    int green = this->sliders[colorIndex][1]->value() * 8;
    int blue = this->sliders[colorIndex][2]->value() * 8;
    Tileset *tileset = paletteNum < Project::getNumPalettesPrimary()
            ? this->primaryTileset
            : this->secondaryTileset;
    (*tileset->palettes)[paletteNum][colorIndex] = qRgb(red, green, blue);
    (*tileset->palettePreviews)[paletteNum][colorIndex] = qRgb(red, green, blue);
    this->refreshColor(colorIndex);
    this->commitEditHistory(paletteNum);
    emit this->changedPaletteColor();
}

void PaletteEditor::on_spinBox_PaletteId_valueChanged(int paletteId) {
    this->refreshColorSliders();
    this->refreshColors();
    if (!this->palettesHistory[paletteId].current()) {
        this->commitEditHistory(paletteId);
    }
    emit this->changedPalette(paletteId);
}

void PaletteEditor::commitEditHistory(int paletteId) {
    QList<QRgb> colors;
    for (int i = 0; i < 16; i++) {
        colors.append(qRgb(this->sliders[i][0]->value() * 8, this->sliders[i][1]->value() * 8, this->sliders[i][2]->value() * 8));
    }
    PaletteHistoryItem *commit = new PaletteHistoryItem(colors);
    this->palettesHistory[paletteId].push(commit);
}

void PaletteEditor::on_actionUndo_triggered()
{
    int paletteId = this->ui->spinBox_PaletteId->value();
    PaletteHistoryItem *prev = this->palettesHistory[paletteId].back();
    this->setColorsFromHistory(prev, paletteId);
}

void PaletteEditor::on_actionRedo_triggered()
{
    int paletteId = this->ui->spinBox_PaletteId->value();
    PaletteHistoryItem *next = this->palettesHistory[paletteId].next();
    this->setColorsFromHistory(next, paletteId);
}

void PaletteEditor::setColorsFromHistory(PaletteHistoryItem *history, int paletteId) {
    if (!history) return;

    for (int i = 0; i < 16; i++) {
        if (paletteId < Project::getNumPalettesPrimary()) {
            (*this->primaryTileset->palettes)[paletteId][i] = history->colors.at(i);
            (*this->primaryTileset->palettePreviews)[paletteId][i] = history->colors.at(i);
        } else {
            (*this->secondaryTileset->palettes)[paletteId][i] = history->colors.at(i);
            (*this->secondaryTileset->palettePreviews)[paletteId][i] = history->colors.at(i);
        }
    }

    this->refreshColorSliders();
    this->refreshColors();
    emit this->changedPaletteColor();
}

void PaletteEditor::on_actionImport_Palette_triggered()
{
    QString filepath = QFileDialog::getOpenFileName(
                this,
                QString("Import Tileset Palette"),
                this->project->root,
                "Palette Files (*.pal *.act *tpl *gpl)");
    if (filepath.isEmpty()) {
        return;
    }

    PaletteUtil parser;
    bool error = false;
    QList<QRgb> palette = parser.parse(filepath, &error);
    if (error) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import palette.");
        QString message = QString("The palette file could not be processed. View porymap.log for specific errors.");
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    if (palette.length() < 16) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import palette.");
        QString message = QString("The palette file has %1 colors, but it must have 16 colors.").arg(palette.length());
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    int paletteId = this->ui->spinBox_PaletteId->value();
    for (int i = 0; i < 16; i++) {
        if (paletteId < Project::getNumPalettesPrimary()) {
            (*this->primaryTileset->palettes)[paletteId][i] = palette.at(i);
            (*this->primaryTileset->palettePreviews)[paletteId][i] = palette.at(i);
        } else {
            (*this->secondaryTileset->palettes)[paletteId][i] = palette.at(i);
            (*this->secondaryTileset->palettePreviews)[paletteId][i] = palette.at(i);
        }
    }

    this->refreshColorSliders();
    this->refreshColors();
    this->commitEditHistory(paletteId);
    emit this->changedPaletteColor();
}
