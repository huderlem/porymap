#include "paletteeditor.h"
#include "ui_paletteeditor.h"
#include "paletteutil.h"
#include "config.h"
#include "log.h"

#include <QFileDialog>
#include <QMessageBox>


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

    this->colorInputs.clear();
    const int numColorsPerRow = 4;
    for (int i = 0; i < this->numColors; i++) {
        auto colorInput = new ColorInputWidget(QString("Color %1").arg(i));
        connect(colorInput, &ColorInputWidget::colorChanged, [this, i](QRgb color) { setRgb(i, color); });
        connect(colorInput, &ColorInputWidget::editingFinished, [this] { commitEditHistory(); });
        this->colorInputs.append(colorInput);
        ui->layout_Colors->addWidget(colorInput, i / numColorsPerRow, i % numColorsPerRow);
    }

    // Setup edit-undo history for each of the palettes.
    for (int i = 0; i < Project::getNumPalettesTotal(); i++) {
        this->palettesHistory.append(History<PaletteHistoryItem*>());
    }

    int bitDepth = porymapConfig.paletteEditorBitDepth;
    if (bitDepth == 15) {
        this->ui->bit_depth_15->setChecked(true);
    } else {
        this->ui->bit_depth_24->setChecked(true);
    }
    setBitDepth(bitDepth);

    // Connect bit depth buttons
    connect(this->ui->bit_depth_15, &QRadioButton::toggled, [this](bool checked){ if (checked) this->setBitDepth(15); });
    connect(this->ui->bit_depth_24, &QRadioButton::toggled, [this](bool checked){ if (checked) this->setBitDepth(24); });

    this->setPaletteId(paletteId);
    this->commitEditHistory();
    this->restoreWindowState();
}

PaletteEditor::~PaletteEditor()
{
    delete ui;
}

Tileset* PaletteEditor::getTileset(int paletteId) {
    return (paletteId < Project::getNumPalettesPrimary())
          ? this->primaryTileset
          : this->secondaryTileset;
}

void PaletteEditor::setBitDepth(int bits) {
    this->bitDepth = bits;
    porymapConfig.paletteEditorBitDepth = bits;
    for (const auto &colorInput : this->colorInputs) {
        colorInput->setBitDepth(bits);
    }
}

void PaletteEditor::setRgb(int colorIndex, QRgb rgb) {
    const int paletteId = this->ui->spinBox_PaletteId->value();

    Tileset *tileset = getTileset(paletteId);
    tileset->palettes[paletteId][colorIndex] = rgb;
    tileset->palettePreviews[paletteId][colorIndex] = rgb;

    emit changedPaletteColor();
}

void PaletteEditor::setPalette(int paletteId, const QList<QRgb> &palette) {
    Tileset *tileset = getTileset(paletteId);
    for (int i = 0; i < this->numColors; i++) {
        tileset->palettes[paletteId][i] = palette.at(i);
        tileset->palettePreviews[paletteId][i] = palette.at(i);
    }
    refreshColorInputs();
    emit changedPaletteColor();
}

void PaletteEditor::refreshColorInputs() {
    const int paletteId = ui->spinBox_PaletteId->value();
    Tileset *tileset = getTileset(paletteId);
    for (int i = 0; i < this->numColors; i++) {
        auto colorInput = this->colorInputs.at(i);
        const QSignalBlocker b(colorInput);
        colorInput->setColor(tileset->palettes.at(paletteId).at(i));
    }
}

void PaletteEditor::setPaletteId(int paletteId) {
    const QSignalBlocker b(ui->spinBox_PaletteId);
    this->ui->spinBox_PaletteId->setValue(paletteId);
    this->refreshColorInputs();
}

void PaletteEditor::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->refreshColorInputs();
}

void PaletteEditor::on_spinBox_PaletteId_valueChanged(int paletteId) {
    this->refreshColorInputs();
    if (!this->palettesHistory[paletteId].current()) {
        this->commitEditHistory(paletteId);
    }
    emit this->changedPalette(paletteId);
}

void PaletteEditor::commitEditHistory() {
    commitEditHistory(ui->spinBox_PaletteId->value());
}

void PaletteEditor::commitEditHistory(int paletteId) {
    QList<QRgb> colors;
    for (int i = 0; i < this->numColors; i++) {
        colors.append(this->colorInputs.at(i)->color());
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
    if (prev)
        setPalette(paletteId, prev->colors);
}

void PaletteEditor::on_actionRedo_triggered()
{
    int paletteId = this->ui->spinBox_PaletteId->value();
    PaletteHistoryItem *next = this->palettesHistory[paletteId].next();
    if (next)
        setPalette(paletteId, next->colors);
}

void PaletteEditor::on_actionImport_Palette_triggered()
{
    QString filepath = QFileDialog::getOpenFileName(
                this,
                QString("Import Tileset Palette"),
                this->project->importExportPath,
                "Palette Files (*.pal *.act *tpl *gpl)");
    if (filepath.isEmpty()) {
        return;
    }
    this->project->setImportExportPath(filepath);
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

    if (palette.length() < this->numColors) {
        QMessageBox msgBox(this);
        msgBox.setText("Failed to import palette.");
        QString message = QString("The palette file has %1 colors, but it must have %2 colors.")
                                    .arg(palette.length())
                                    .arg(this->numColors);
        msgBox.setInformativeText(message);
        msgBox.setDefaultButton(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Icon::Critical);
        msgBox.exec();
        return;
    }

    const int paletteId = ui->spinBox_PaletteId->value();
    setPalette(paletteId, palette);
    commitEditHistory(paletteId);
}

void PaletteEditor::closeEvent(QCloseEvent*) {
    porymapConfig.setPaletteEditorGeometry(
        this->saveGeometry(),
        this->saveState()
    );
}
