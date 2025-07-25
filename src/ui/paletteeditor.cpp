#include "paletteeditor.h"
#include "ui_paletteeditor.h"
#include "paletteutil.h"
#include "config.h"
#include "log.h"
#include "filedialog.h"
#include "message.h"
#include "eventfilters.h"


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
        auto colorInput = new ColorInputWidget;
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

    this->ui->actionShow_Unused_Colors->setChecked(porymapConfig.showPaletteEditorUnusedColors);
    connect(this->ui->actionShow_Unused_Colors, &QAction::toggled, this, &PaletteEditor::setColorInputTitles);

    ActiveWindowFilter *filter = new ActiveWindowFilter(this);
    connect(filter, &ActiveWindowFilter::activated, this, &PaletteEditor::onWindowActivated);
    this->installEventFilter(filter);

    this->setPaletteId(paletteId);
    this->commitEditHistory();
    this->restoreWindowState();
}

PaletteEditor::~PaletteEditor() {
    delete ui;
}

void PaletteEditor::onWindowActivated() {
    // Rather than try to keep track of metatile/tile changes that affect which colors are used,
    // we'll just refresh when the window is activated.
    invalidateCache();
}

int PaletteEditor::currentPaletteId() const {
    return ui->spinBox_PaletteId->value();
}

bool PaletteEditor::showingUnusedColors() const {
    return ui->actionShow_Unused_Colors->isChecked();
}

Tileset* PaletteEditor::getTileset(int paletteId) const {
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
    const int paletteId = currentPaletteId();
    Tileset *tileset = getTileset(paletteId);
    tileset->palettes[paletteId][colorIndex] = rgb;
    tileset->palettePreviews[paletteId][colorIndex] = rgb;

    emit changedPaletteColor();
}

void PaletteEditor::setPalette(int paletteId, const QList<QRgb> &palette) {
    Tileset *tileset = getTileset(paletteId);
    for (int i = 0; i < this->numColors; i++) {
        tileset->palettes[paletteId][i] = palette.value(i);
        tileset->palettePreviews[paletteId][i] = palette.value(i);
    }
    refreshColorInputs();
    emit changedPaletteColor();
}

void PaletteEditor::refreshColorInputs() {
    const int paletteId = currentPaletteId();
    Tileset *tileset = getTileset(paletteId);
    for (int i = 0; i < this->colorInputs.length(); i++) {
        auto colorInput = this->colorInputs.at(i);
        const QSignalBlocker b(colorInput);
        colorInput->setColor(tileset->palettes.value(paletteId).value(i));
    }
    setColorInputTitles(showingUnusedColors());
}

void PaletteEditor::setPaletteId(int paletteId) {
    const QSignalBlocker b(ui->spinBox_PaletteId);
    this->ui->spinBox_PaletteId->setValue(paletteId);
    this->refreshColorInputs();
}

void PaletteEditor::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    this->invalidateCache();
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
    commitEditHistory(currentPaletteId());
}

void PaletteEditor::commitEditHistory(int paletteId) {
    QList<QRgb> colors;
    for (int i = 0; i < this->colorInputs.length(); i++) {
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

void PaletteEditor::on_actionUndo_triggered() {
    int paletteId = currentPaletteId();
    PaletteHistoryItem *prev = this->palettesHistory[paletteId].back();
    if (prev)
        setPalette(paletteId, prev->colors);
}

void PaletteEditor::on_actionRedo_triggered() {
    int paletteId = currentPaletteId();
    PaletteHistoryItem *next = this->palettesHistory[paletteId].next();
    if (next)
        setPalette(paletteId, next->colors);
}

void PaletteEditor::on_actionImport_Palette_triggered() {
    QString filepath = FileDialog::getOpenFileName(this, "Import Tileset Palette", "", "Palette Files (*.pal *.act *tpl *gpl)");
    if (filepath.isEmpty()) {
        return;
    }
    bool error = false;
    QList<QRgb> palette = PaletteUtil::parse(filepath, &error);
    if (error) {
        RecentErrorMessage::show(QStringLiteral("Failed to import palette."), this);
        return;
    }
    while (palette.length() < this->numColors) {
        palette.append(0);
    }

    const int paletteId = currentPaletteId();
    setPalette(paletteId, palette);
    commitEditHistory(paletteId);
}

void PaletteEditor::invalidateCache() {
    this->unusedColorCache.clear();
    if (showingUnusedColors()) {
        setColorInputTitles(true);
    }
}

QSet<int> PaletteEditor::getUnusedColorIds() const {
    const int paletteId = currentPaletteId();

    if (this->unusedColorCache.contains(paletteId)) {
        return this->unusedColorCache.value(paletteId);
    }
    this->unusedColorCache[paletteId] = {};

    // Check our current tilesets for color usage.
    QSet<int> unusedColorIds = this->primaryTileset->getUnusedColorIds(paletteId, this->secondaryTileset);
    if (unusedColorIds.isEmpty())
        return {};
    unusedColorIds = this->secondaryTileset->getUnusedColorIds(paletteId, this->primaryTileset, unusedColorIds);
    if (unusedColorIds.isEmpty())
        return {};

    // The current palette comes from either the primary or secondary tileset.
    // We need to check all the other tilesets that are paired with the tileset that owns this palette.
    Tileset *paletteTileset = getTileset(paletteId);
    QSet<QString> tilesetsToSearch = this->project->getPairedTilesetLabels(paletteTileset);

    // We exclude the currently-loaded pair (we already checked them, and because they're being
    // edited in the Tileset Editor they may differ from their copies saved in the layout).
    tilesetsToSearch.remove(this->primaryTileset->name);
    tilesetsToSearch.remove(this->secondaryTileset->name);

    for (const auto &label : tilesetsToSearch) {
        Tileset *searchTileset = this->project->getTileset(label);
        if (!searchTileset) continue;
        unusedColorIds = searchTileset->getUnusedColorIds(paletteId, paletteTileset, unusedColorIds);
        if (unusedColorIds.isEmpty())
            return {};
    }

    this->unusedColorCache[paletteId] = unusedColorIds;
    return unusedColorIds;
}

void PaletteEditor::setColorInputTitles(bool showUnused) {
    porymapConfig.showPaletteEditorUnusedColors = showUnused;

    QSet<int> unusedColorIds = showUnused ? getUnusedColorIds() : QSet<int>();
    ui->label_AllColorsUsed->setVisible(showUnused && unusedColorIds.isEmpty());
    for (int i = 0; i < this->colorInputs.length(); i++) {
        QString title = QString("Color %1").arg(i);
        if (unusedColorIds.contains(i)) {
            title.append(QStringLiteral(" (Unused)"));
        }
        this->colorInputs.at(i)->setTitle(title);
    }
}

void PaletteEditor::closeEvent(QCloseEvent*) {
    porymapConfig.setPaletteEditorGeometry(
        this->saveGeometry(),
        this->saveState()
    );
}
