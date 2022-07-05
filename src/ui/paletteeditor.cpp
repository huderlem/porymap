#include "paletteeditor.h"
#include "ui_paletteeditor.h"
#include "colorpicker.h"
#include "paletteutil.h"
#include "config.h"
#include "log.h"

#include <cmath>
#include <QFileDialog>
#include <QMessageBox>

static inline int rgb5(int rgb) { return round(static_cast<double>(rgb * 31) / 255.0); }

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
        QList<QSlider *> rgbSliders;
        rgbSliders.append(this->ui->container->findChild<QSlider *>("slider_red_" + QString::number(i)));
        rgbSliders.append(this->ui->container->findChild<QSlider *>("slider_green_" + QString::number(i)));
        rgbSliders.append(this->ui->container->findChild<QSlider *>("slider_blue_" + QString::number(i)));
        this->sliders.append(rgbSliders);

        connect(this->sliders[i][0], &QSlider::valueChanged, [=](int) { setRgbFromSliders(i); });
        connect(this->sliders[i][1], &QSlider::valueChanged, [=](int) { setRgbFromSliders(i); });
        connect(this->sliders[i][2], &QSlider::valueChanged, [=](int) { setRgbFromSliders(i); });
    }

    this->spinners.clear();
    for (int i = 0; i < 16; i++) {
        QList<QSpinBox *> rgbSpinners;
        rgbSpinners.append(this->ui->container->findChild<QSpinBox *>("spin_red_" + QString::number(i)));
        rgbSpinners.append(this->ui->container->findChild<QSpinBox *>("spin_green_" + QString::number(i)));
        rgbSpinners.append(this->ui->container->findChild<QSpinBox *>("spin_blue_" + QString::number(i)));
        this->spinners.append(rgbSpinners);

        connect(this->spinners[i][0], &QSpinBox::valueChanged, [=](int) { setRgbFromSpinners(i); });
        connect(this->spinners[i][1], &QSpinBox::valueChanged, [=](int) { setRgbFromSpinners(i); });
        connect(this->spinners[i][2], &QSpinBox::valueChanged, [=](int) { setRgbFromSpinners(i); });
    }

    this->frames.clear();
    for (int i = 0; i < 16; i++) {
        this->frames.append(this->ui->container->findChild<QFrame *>("colorFrame_" + QString::number(i)));
    }

    this->rgbLabels.clear();
    for (int i = 0; i < 16; i++) {
        this->rgbLabels.append(this->ui->container->findChild<QLabel *>("rgb_" + QString::number(i)));
        this->rgbLabels[i]->setStyleSheet("QLabel { font-family: \"Courier\" }");
    }

    this->pickButtons.clear();
    for (int i = 0; i < 16; i++) {
        this->pickButtons.append(this->ui->container->findChild<QToolButton *>("pick_" + QString::number(i)));
    }

    this->hexValidator = new HexCodeValidator;
    this->hexEdits.clear();
    for (int i = 0; i < 16; i++) {
        this->hexEdits.append(this->ui->container->findChild<QLineEdit *>("hex_" + QString::number(i)));
        this->hexEdits[i]->setValidator(hexValidator);
        this->hexEdits[i]->setInputMask("HHHHHH");
        this->hexEdits[i]->setMaxLength(6);
    }

    // Connect to function that will update color when hex edit is changed
    for (int i = 0; i < this->hexEdits.length(); i++) {
        connect(this->hexEdits[i], &QLineEdit::textEdited, [this, i](QString text){
            if (text.length() == 6) setRgbFromHexEdit(i);
        });
    }

    // Setup edit-undo history for each of the palettes.
    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        this->palettesHistory.append(History<PaletteHistoryItem*>());
    }

    // Connect the color picker's selection to the correct color index
    for (int i = 0; i < 16; i++) {
        connect(this->pickButtons[i], &QToolButton::clicked, [this, i](){ this->pickColor(i); });
    }

    this->setPaletteId(paletteId);
    this->commitEditHistory(this->ui->spinBox_PaletteId->value());
    this->restoreWindowState();
}

PaletteEditor::~PaletteEditor()
{
    delete ui;
    delete this->hexValidator;
}

void PaletteEditor::updateColorUi(int colorIndex, QRgb rgb) {
    setSignalsEnabled(false);

    int red = qRed(rgb);
    int green = qGreen(rgb);
    int blue = qBlue(rgb);
    
    // sliders
    this->sliders[colorIndex][0]->setValue(rgb5(red));
    this->sliders[colorIndex][1]->setValue(rgb5(green));
    this->sliders[colorIndex][2]->setValue(rgb5(blue));

    // hex
    QColor color(red, green, blue);
    QString hexcode = color.name().remove(0, 1).toUpper();
    this->hexEdits[colorIndex]->setText(hexcode);

    // spinners
    this->spinners[colorIndex][0]->setValue(red);
    this->spinners[colorIndex][1]->setValue(green);
    this->spinners[colorIndex][2]->setValue(blue);

    // frame
    QString stylesheet = QString("background-color: rgb(%1, %2, %3);").arg(red).arg(green).arg(blue);
    this->frames[colorIndex]->setStyleSheet(stylesheet);
    
    // rgb label
    int w = 3;
    QChar spc = ' ';
    int base = 10;
    this->rgbLabels[colorIndex]->setText(QString("  RGB(%1, %2, %3)").arg(red, w, base, spc).arg(green, w, base, spc).arg(blue, w, base, spc));

    setSignalsEnabled(true);
}

void PaletteEditor::setSignalsEnabled(bool enabled) {
    // spinners, sliders, hexbox
    for (int i = 0; i < this->sliders.length(); i++) {
        this->sliders.at(i).at(0)->blockSignals(!enabled);
        this->sliders.at(i).at(1)->blockSignals(!enabled);
        this->sliders.at(i).at(2)->blockSignals(!enabled);
    }

    for (int i = 0; i < this->spinners.length(); i++) {
        this->spinners.at(i).at(0)->blockSignals(!enabled);
        this->spinners.at(i).at(1)->blockSignals(!enabled);
        this->spinners.at(i).at(2)->blockSignals(!enabled);
    }

    for (int i = 0; i < this->hexEdits.length(); i++) {
        this->hexEdits.at(i)->blockSignals(!enabled);
    }
}

void PaletteEditor::setRgb(int colorIndex, QRgb rgb) {
    int paletteNum = this->ui->spinBox_PaletteId->value();

    Tileset *tileset = paletteNum < Project::getNumPalettesPrimary()
            ? this->primaryTileset
            : this->secondaryTileset;
    tileset->palettes[paletteNum][colorIndex] = rgb;
    tileset->palettePreviews[paletteNum][colorIndex] = rgb;

    this->updateColorUi(colorIndex, rgb);
    
    this->commitEditHistory(paletteNum);
    emit this->changedPaletteColor();
}

void PaletteEditor::setRgbFromSliders(int colorIndex) {
    setRgb(colorIndex, qRgb(round(this->sliders[colorIndex][0]->value() * 255. / 31.),
                            round(this->sliders[colorIndex][1]->value() * 255. / 31.),
                            round(this->sliders[colorIndex][2]->value() * 255. / 31.)));
}

void PaletteEditor::setRgbFromHexEdit(int colorIndex) {
    QString text = this->hexEdits[colorIndex]->text();
    bool ok = false;
    QRgb rgb = text.toInt(&ok, 16);
    if (!ok) rgb = 0xFFFFFFFF;
    setRgb(colorIndex, rgb);
}

void PaletteEditor::setRgbFromSpinners(int colorIndex) {
    setRgb(colorIndex, qRgb(this->spinners[colorIndex][0]->value(),
                            this->spinners[colorIndex][1]->value(),
                            this->spinners[colorIndex][2]->value()));
}

void PaletteEditor::refreshColorUis() {
    int paletteNum = this->ui->spinBox_PaletteId->value();
    for (int i = 0; i < 16; i++) {
        QRgb color;
        if (paletteNum < Project::getNumPalettesPrimary()) {
            color = this->primaryTileset->palettes.at(paletteNum).at(i);
        } else {
            color = this->secondaryTileset->palettes.at(paletteNum).at(i);
        }

        this->updateColorUi(i, color);
    }
}

void PaletteEditor::setPaletteId(int paletteId) {
    this->ui->spinBox_PaletteId->blockSignals(true);
    this->ui->spinBox_PaletteId->setValue(paletteId);
    this->refreshColorUis();
    this->ui->spinBox_PaletteId->blockSignals(false);
}

void PaletteEditor::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->refreshColorUis();
}

void PaletteEditor::pickColor(int index) {
    ColorPicker picker(this);
    if (picker.exec() == QDialog::Accepted) {
        QColor c = picker.getColor();
        this->setRgb(index, c.rgb());
    }
    return;
}

void PaletteEditor::on_spinBox_PaletteId_valueChanged(int paletteId) {
    this->refreshColorUis();
    if (!this->palettesHistory[paletteId].current()) {
        this->commitEditHistory(paletteId);
    }
    emit this->changedPalette(paletteId);
}

void PaletteEditor::commitEditHistory(int paletteId) {
    QList<QRgb> colors;
    for (int i = 0; i < 16; i++) {
        colors.append(qRgb(this->spinners[i][0]->value(), this->spinners[i][1]->value(), this->spinners[i][2]->value()));
    }
    PaletteHistoryItem *commit = new PaletteHistoryItem(colors);
    this->palettesHistory[paletteId].push(commit);
}

void PaletteEditor::restoreWindowState() {
    logInfo("Restoring palette editor geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getPaletteEditorGeometry();
    this->restoreGeometry(geometry.value("palette_editor_geometry"));
    this->restoreState(geometry.value("palette_editor_state"));
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
            this->primaryTileset->palettes[paletteId][i] = history->colors.at(i);
            this->primaryTileset->palettePreviews[paletteId][i] = history->colors.at(i);
        } else {
            this->secondaryTileset->palettes[paletteId][i] = history->colors.at(i);
            this->secondaryTileset->palettePreviews[paletteId][i] = history->colors.at(i);
        }
    }

    this->refreshColorUis();
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

    bool error = false;
    QList<QRgb> palette = PaletteUtil::parse(filepath, &error);
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
            this->primaryTileset->palettes[paletteId][i] = palette.at(i);
            this->primaryTileset->palettePreviews[paletteId][i] = palette.at(i);
        } else {
            this->secondaryTileset->palettes[paletteId][i] = palette.at(i);
            this->secondaryTileset->palettePreviews[paletteId][i] = palette.at(i);
        }
    }

    this->refreshColorUis();
    this->commitEditHistory(paletteId);
    emit this->changedPaletteColor();
}

void PaletteEditor::closeEvent(QCloseEvent*) {
    porymapConfig.setPaletteEditorGeometry(
        this->saveGeometry(),
        this->saveState()
    );
}
