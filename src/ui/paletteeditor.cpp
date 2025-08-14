#include "paletteeditor.h"
#include "ui_paletteeditor.h"
#include "paletteutil.h"
#include "config.h"
#include "log.h"
#include "filedialog.h"
#include "message.h"
#include "eventfilters.h"
#include "utility.h"


PaletteEditor::PaletteEditor(Project *project, Tileset *primaryTileset, Tileset *secondaryTileset, int paletteId, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PaletteEditor),
    project(project),
    primaryTileset(primaryTileset),
    secondaryTileset(secondaryTileset)
{
    setAttribute(Qt::WA_DeleteOnClose);
    this->ui->setupUi(this);

    const int numColorsPerRow = 4;
    for (int i = 0; i < this->numColors; i++) {
        auto colorInput = new ColorInputWidget;
        connect(colorInput, &ColorInputWidget::colorChanged, [this, i](QRgb color) { setRgb(i, color); });
        connect(colorInput, &ColorInputWidget::editingFinished, [this] { commitEditHistory(); });
        this->colorInputs.append(colorInput);
        ui->layout_Colors->addWidget(colorInput, i / numColorsPerRow, i % numColorsPerRow);
    }

    int bitDepth = porymapConfig.paletteEditorBitDepth;
    if (bitDepth == 15) {
        this->ui->bit_depth_15->setChecked(true);
    } else {
        this->ui->bit_depth_24->setChecked(true);
    }
    setBitDepth(bitDepth);

    // Connect bit depth buttons
    connect(this->ui->bit_depth_15, &QRadioButton::toggled, [this](bool checked){ if (checked) setBitDepth(15); });
    connect(this->ui->bit_depth_24, &QRadioButton::toggled, [this](bool checked){ if (checked) setBitDepth(24); });

    this->ui->actionShow_Unused_Colors->setChecked(porymapConfig.showPaletteEditorUnusedColors);
    connect(this->ui->actionShow_Unused_Colors, &QAction::toggled, this, &PaletteEditor::setColorInputTitles);

    connect(this->ui->toolButton_ColorSearch, &QToolButton::clicked, this, &PaletteEditor::openColorSearch);
    connect(this->ui->actionFind_Color_Usage, &QAction::triggered, this, &PaletteEditor::openColorSearch);

    // Rather than try to keep track of metatile/tile changes that affect which colors are used,
    // we'll just refresh when the window is activated.
    ActiveWindowFilter *filter = new ActiveWindowFilter(this);
    connect(filter, &ActiveWindowFilter::activated, this, &PaletteEditor::invalidateCache);
    this->installEventFilter(filter);

    this->ui->spinBox_PaletteId->setRange(0, Project::getNumPalettesTotal() - 1);
    this->ui->spinBox_PaletteId->setValue(paletteId);
    connect(this->ui->spinBox_PaletteId, QOverload<int>::of(&QSpinBox::valueChanged), this, &PaletteEditor::refreshPaletteId);
    connect(this->ui->spinBox_PaletteId, QOverload<int>::of(&QSpinBox::valueChanged), this, &PaletteEditor::changedPalette);

    ui->actionRedo->setShortcuts({ui->actionRedo->shortcut(), QKeySequence("Ctrl+Shift+Z")});

    refreshPaletteId();
    restoreWindowState();
}

PaletteEditor::~PaletteEditor() {
    delete ui;
}

int PaletteEditor::currentPaletteId() const {
    return ui->spinBox_PaletteId->value();
}

void PaletteEditor::setPaletteId(int paletteId) {
    ui->spinBox_PaletteId->setValue(paletteId);
}

bool PaletteEditor::showingUnusedColors() const {
    return ui->actionShow_Unused_Colors->isChecked();
}

Tileset* PaletteEditor::getTileset(int paletteId) const {
    return Tileset::getPaletteTileset(paletteId, this->primaryTileset, this->secondaryTileset);
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

void PaletteEditor::refreshPaletteId() {
    refreshColorInputs();

    int paletteId = currentPaletteId();

    if (!this->palettesHistory[paletteId].current()) {
        // The original colors are saved as an initial commit.
        commitEditHistory(paletteId);
    } else {
        updateEditHistoryActions();
    }
    if (this->colorSearchWindow) {
        this->colorSearchWindow->setPaletteId(paletteId);
    }
}

void PaletteEditor::setTilesets(Tileset *primaryTileset, Tileset *secondaryTileset) {
    this->primaryTileset = primaryTileset;
    this->secondaryTileset = secondaryTileset;
    invalidateCache();
    if (this->colorSearchWindow) {
        this->colorSearchWindow->setTilesets(primaryTileset, secondaryTileset);
    }
    refreshColorInputs();
}

void PaletteEditor::commitEditHistory() {
    commitEditHistory(currentPaletteId());
}

void PaletteEditor::commitEditHistory(int paletteId) {
    QList<QRgb> colors;
    for (int i = 0; i < this->colorInputs.length(); i++) {
        colors.append(this->colorInputs.at(i)->color());
    }
    this->palettesHistory[paletteId].push(new PaletteHistoryItem(colors));
    updateEditHistoryActions();
}

void PaletteEditor::restoreWindowState() {
    logInfo("Restoring palette editor geometry from previous session.");
    QMap<QString, QByteArray> geometry = porymapConfig.getPaletteEditorGeometry();
    restoreGeometry(geometry.value("palette_editor_geometry"));
    restoreState(geometry.value("palette_editor_state"));
}

void PaletteEditor::updateEditHistoryActions() {
    int paletteId = currentPaletteId();
    // We have an initial commit that shouldn't be available to Undo, so we ignore that.
    ui->actionUndo->setEnabled(this->palettesHistory[paletteId].index() > 0);
    ui->actionRedo->setEnabled(this->palettesHistory[paletteId].canRedo());
}

void PaletteEditor::on_actionUndo_triggered() {
    int paletteId = currentPaletteId();
    PaletteHistoryItem *commit = this->palettesHistory[paletteId].back();
    if (!commit) return;
    setPalette(paletteId, commit->colors);
    updateEditHistoryActions();
}

void PaletteEditor::on_actionRedo_triggered() {
    int paletteId = currentPaletteId();
    PaletteHistoryItem *commit = this->palettesHistory[paletteId].next();
    if (!commit) return;
    setPalette(paletteId, commit->colors);
    updateEditHistoryActions();
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

void PaletteEditor::openColorSearch() {
    if (!this->colorSearchWindow) {
        this->colorSearchWindow = new PaletteColorSearch(this->project, this->primaryTileset, this->secondaryTileset, this);
        this->colorSearchWindow->setPaletteId(currentPaletteId());
        connect(this->colorSearchWindow, &PaletteColorSearch::metatileSelected, this, &PaletteEditor::metatileSelected);
        connect(this->colorSearchWindow, &PaletteColorSearch::paletteIdChanged, this, &PaletteEditor::setPaletteId);
    }
    Util::show(this->colorSearchWindow);
}

void PaletteEditor::invalidateCache() {
    this->unusedColorCache.clear();
    if (showingUnusedColors()) {
        setColorInputTitles(true);
    }
}

QSet<int> PaletteEditor::getUnusedColorIds() {
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
        saveGeometry(),
        saveState()
    );

    // Opening the color search window then closing the Palette Editor sets
    // focus to the main editor window instead of the parent (Tileset Editor).
    // Make sure the parent is active when we close.
    auto p = dynamic_cast<QWidget*>(parent());
    if (p && p->isVisible()) {
        p->raise();
        p->activateWindow();
    }
}
