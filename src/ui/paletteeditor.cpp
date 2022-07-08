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
static inline int rgb8(int rgb) { return round(rgb * 255. / 31.); }
static inline int gbaRed(int rgb) { return rgb & 0x1f; }
static inline int gbaGreen(int rgb) { return (rgb >> 5) & 0x1f; }
static inline int gbaBlue(int rgb) { return (rgb >> 10) & 0x1f; }

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

        connect(this->spinners[i][0], QOverload<int>::of(&QSpinBox::valueChanged), [=](int) { setRgbFromSpinners(i); });
        connect(this->spinners[i][1], QOverload<int>::of(&QSpinBox::valueChanged), [=](int) { setRgbFromSpinners(i); });
        connect(this->spinners[i][2], QOverload<int>::of(&QSpinBox::valueChanged), [=](int) { setRgbFromSpinners(i); });
    }

    this->frames.clear();
    for (int i = 0; i < 16; i++) {
        this->frames.append(this->ui->container->findChild<QFrame *>("colorFrame_" + QString::number(i)));
        this->frames[i]->setFrameStyle(QFrame::NoFrame);
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
    }

    // Connect to function that will update color when hex edit is changed
    for (int i = 0; i < this->hexEdits.length(); i++) {
        connect(this->hexEdits[i], &QLineEdit::textEdited, [this, i](QString text){
            if ((this->bitDepth == 24 && text.length() == 6) || (this->bitDepth == 15 && text.length() == 4)) setRgbFromHexEdit(i);
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

    setBitDepth(24);

    // Connect bit depth buttons
    connect(this->ui->bit_depth_15, &QRadioButton::toggled, [this](bool checked){ if (checked) this->setBitDepth(15); });
    connect(this->ui->bit_depth_24, &QRadioButton::toggled, [this](bool checked){ if (checked) this->setBitDepth(24); });

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

    if (this->bitDepth == 15) {
        // sliders
        this->sliders[colorIndex][0]->setValue(rgb5(red));
        this->sliders[colorIndex][1]->setValue(rgb5(green));
        this->sliders[colorIndex][2]->setValue(rgb5(blue));

        // hex
        int hex15 = (rgb5(blue) << 10) | (rgb5(green) << 5) | rgb5(red);
        QString hexcode = QString("%1").arg(hex15, 4, 16, QLatin1Char('0')).toUpper();
        this->hexEdits[colorIndex]->setText(hexcode);

        // spinners
        this->spinners[colorIndex][0]->setValue(rgb5(red));
        this->spinners[colorIndex][1]->setValue(rgb5(green));
        this->spinners[colorIndex][2]->setValue(rgb5(blue));
    } else {
        // sliders
        this->sliders[colorIndex][0]->setValue(red);
        this->sliders[colorIndex][1]->setValue(green);
        this->sliders[colorIndex][2]->setValue(blue);

        // hex
        QColor color(red, green, blue);
        QString hexcode = color.name().remove(0, 1).toUpper();
        this->hexEdits[colorIndex]->setText(hexcode);

        // spinners
        this->spinners[colorIndex][0]->setValue(red);
        this->spinners[colorIndex][1]->setValue(green);
        this->spinners[colorIndex][2]->setValue(blue);
    }

    // frame
    QString stylesheet = QString("background-color: rgb(%1, %2, %3);").arg(red).arg(green).arg(blue);
    this->frames[colorIndex]->setStyleSheet(stylesheet);

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

void PaletteEditor::setBitDepth(int bits) {
    setSignalsEnabled(false);
    switch (bits) {
    case 15:
        for (int i = 0; i < 16; i++) {
            // sliders ranged [0, 31] with 1 single step and 4 page step
            this->sliders[i][0]->setSingleStep(1);
            this->sliders[i][1]->setSingleStep(1);
            this->sliders[i][2]->setSingleStep(1);
            this->sliders[i][0]->setPageStep(4);
            this->sliders[i][1]->setPageStep(4);
            this->sliders[i][2]->setPageStep(4);
            this->sliders[i][0]->setMaximum(31);
            this->sliders[i][1]->setMaximum(31);
            this->sliders[i][2]->setMaximum(31);

            // spinners limited [0, 31] with 1 step
            this->spinners[i][0]->setSingleStep(1);
            this->spinners[i][1]->setSingleStep(1);
            this->spinners[i][2]->setSingleStep(1);
            this->spinners[i][0]->setMaximum(31);
            this->spinners[i][1]->setMaximum(31);
            this->spinners[i][2]->setMaximum(31);

            // hex box now 4 digits
            this->hexEdits[i]->setInputMask("HHHH");
            this->hexEdits[i]->setMaxLength(4);
        }
        break;
    case 24:
    default:
        for (int i = 0; i < 16; i++) {
            // sliders ranged [0, 31] with 1 single step and 4 page step
            this->sliders[i][0]->setSingleStep(8);
            this->sliders[i][1]->setSingleStep(8);
            this->sliders[i][2]->setSingleStep(8);
            this->sliders[i][0]->setPageStep(24);
            this->sliders[i][1]->setPageStep(24);
            this->sliders[i][2]->setPageStep(24);
            this->sliders[i][0]->setMaximum(255);
            this->sliders[i][1]->setMaximum(255);
            this->sliders[i][2]->setMaximum(255);

            // spinners limited [0, 31] with 1 step
            this->spinners[i][0]->setSingleStep(8);
            this->spinners[i][1]->setSingleStep(8);
            this->spinners[i][2]->setSingleStep(8);
            this->spinners[i][0]->setMaximum(255);
            this->spinners[i][1]->setMaximum(255);
            this->spinners[i][2]->setMaximum(255);

            // hex box now 4 digits
            this->hexEdits[i]->setInputMask("HHHHHH");
            this->hexEdits[i]->setMaxLength(6);
        }
        break;
    }
    this->bitDepth = bits;
    refreshColorUis();
    setSignalsEnabled(true);
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
    if (this->bitDepth == 15) {
        setRgb(colorIndex, qRgb(rgb8(this->sliders[colorIndex][0]->value()),
                                rgb8(this->sliders[colorIndex][1]->value()),
                                rgb8(this->sliders[colorIndex][2]->value())));
    } else {
        setRgb(colorIndex, qRgb(this->sliders[colorIndex][0]->value(),
                                this->sliders[colorIndex][1]->value(),
                                this->sliders[colorIndex][2]->value()));
    }
}

void PaletteEditor::setRgbFromHexEdit(int colorIndex) {
    QString text = this->hexEdits[colorIndex]->text();
    bool ok = false;
    if (this->bitDepth == 15) {
        int rgb15 = text.toInt(&ok, 16);
        int rc = gbaRed(rgb15);
        int gc = gbaGreen(rgb15);
        int bc = gbaBlue(rgb15);
        QRgb rgb = qRgb(rc, gc, bc);
        if (!ok) rgb = 0xFFFFFFFF;
        setRgb(colorIndex, rgb);
    } else {
        QRgb rgb = text.toInt(&ok, 16);
        if (!ok) rgb = 0xFFFFFFFF;
        setRgb(colorIndex, rgb);
    }
}

void PaletteEditor::setRgbFromSpinners(int colorIndex) {
    if (this->bitDepth == 15) {
        setRgb(colorIndex, qRgb(rgb8(this->spinners[colorIndex][0]->value()),
                                rgb8(this->spinners[colorIndex][1]->value()),
                                rgb8(this->spinners[colorIndex][2]->value())));
    } else {
        setRgb(colorIndex, qRgb(this->spinners[colorIndex][0]->value(),
                                this->spinners[colorIndex][1]->value(),
                                this->spinners[colorIndex][2]->value()));
    }
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
