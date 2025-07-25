#include "palettecolorsearch.h"
#include "ui_palettecolorsearch.h"
#include "project.h"
#include "tileset.h"
#include "imageproviders.h"
#include "eventfilters.h"
#include "log.h"
#include "numericsorttableitem.h"

enum ResultsDataRole {
    PairedTilesetName = Qt::UserRole,
};

PaletteColorSearch::PaletteColorSearch(Project *project, const Tileset *primaryTileset, const Tileset *secondaryTileset, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PaletteColorSearch),
    m_project(project),
    m_primaryTileset(primaryTileset),
    m_secondaryTileset(secondaryTileset)
{
    setAttribute(Qt::WA_DeleteOnClose);
    ui->setupUi(this);

    ui->buttonBox->setVisible(isModal());
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);

    // Rather than try to keep track of metatile/tile changes that affect which colors are used,
    // we'll just refresh when the window is activated.
    ActiveWindowFilter *filter = new ActiveWindowFilter(this);
    connect(filter, &ActiveWindowFilter::activated, this, &PaletteColorSearch::refresh);
    this->installEventFilter(filter);

    ui->spinBox_ColorId->setRange(0, Tileset::numColorsPerPalette() - 1);
    connect(ui->spinBox_ColorId, QOverload<int>::of(&QSpinBox::valueChanged), this, &PaletteColorSearch::updateResults);

    ui->spinBox_PaletteId->setRange(0, Project::getNumPalettesTotal() - 1);
    connect(ui->spinBox_PaletteId, QOverload<int>::of(&QSpinBox::valueChanged), this, &PaletteColorSearch::updateResults);
    connect(ui->spinBox_PaletteId, QOverload<int>::of(&QSpinBox::valueChanged), this, &PaletteColorSearch::paletteIdChanged);

    // Set up table header
    static const QStringList labels = {"Tileset", "Metatile"};
    ui->table_Results->setHorizontalHeaderLabels(labels);
    ui->table_Results->horizontalHeader()->setSectionResizeMode(ResultsColumn::TilesetName, QHeaderView::ResizeToContents);
    ui->table_Results->horizontalHeader()->setSectionResizeMode(ResultsColumn::Metatile,    QHeaderView::Stretch);

    // Table is read-only
    ui->table_Results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table_Results->setSelectionMode(QAbstractItemView::NoSelection);

    connect(ui->table_Results, &QTableWidget::cellDoubleClicked, this, &PaletteColorSearch::cellDoubleClicked);
}

PaletteColorSearch::~PaletteColorSearch() {
    delete ui;
}

void PaletteColorSearch::setPaletteId(int paletteId) {
    ui->spinBox_PaletteId->setValue(paletteId);
}

int PaletteColorSearch::currentPaletteId() const {
    return ui->spinBox_PaletteId->value();
}

void PaletteColorSearch::setColorId(int colorId) {
    ui->spinBox_ColorId->setValue(colorId);
}

int PaletteColorSearch::currentColorId() const {
    return ui->spinBox_ColorId->value();
}

void PaletteColorSearch::setTilesets(const Tileset *primaryTileset, const Tileset *secondaryTileset) {
    m_primaryTileset = primaryTileset;
    m_secondaryTileset = secondaryTileset;
    refresh();
}

const Tileset* PaletteColorSearch::currentTileset() const {
    return Tileset::getPaletteTileset(currentPaletteId(), m_primaryTileset, m_secondaryTileset);
}

void PaletteColorSearch::addTableEntry(const RowData &rowData) {
    int row = ui->table_Results->rowCount();
    ui->table_Results->insertRow(row);

    auto tilesetNameItem = new NumericSortTableItem(rowData.tilesetName);
    tilesetNameItem->setData(ResultsDataRole::PairedTilesetName, rowData.pairedTilesetName);

    ui->table_Results->setItem(row, ResultsColumn::TilesetName, tilesetNameItem);
    ui->table_Results->setItem(row, ResultsColumn::Metatile, new QTableWidgetItem(rowData.metatileIcon, rowData.metatileId));
}

QList<PaletteColorSearch::RowData> PaletteColorSearch::search(int colorId) const {
    QList<RowData> results;
    
    // Check our current tilesets for color usage.
    results.append(search(colorId, m_primaryTileset, m_secondaryTileset));
    results.append(search(colorId, m_secondaryTileset, m_primaryTileset));

    // The current palette comes from either the primary or secondary tileset.
    // We need to check all the other tilesets that are paired with the tileset that owns this palette.
    const Tileset *paletteTileset = currentTileset();
    QSet<QString> tilesetsToSearch = m_project->getPairedTilesetLabels(paletteTileset);

    // We exclude the currently-loaded pair (we already checked them, and because they're being
    // edited in the Tileset Editor they may differ from their copies saved in the layout).
    tilesetsToSearch.remove(m_primaryTileset->name);
    tilesetsToSearch.remove(m_secondaryTileset->name);

    for (const auto &label : tilesetsToSearch) {
        Tileset *searchTileset = m_project->getTileset(label);
        if (searchTileset) {
            results.append(search(colorId, searchTileset, paletteTileset));
        }
    }
    return results;
}

QList<PaletteColorSearch::RowData> PaletteColorSearch::search(int colorId, const Tileset *tileset, const Tileset *pairedTileset) const {
    QList<RowData> results;
    QList<uint16_t> metatileIds = tileset->findMetatilesUsingColor(currentPaletteId(), colorId, pairedTileset);
    auto primaryTileset = tileset->is_secondary ? pairedTileset : tileset;
    auto secondaryTileset = tileset->is_secondary ? tileset : pairedTileset;
    for (const auto &metatileId : metatileIds) {
        QImage metatileImage = getMetatileImage(metatileId, primaryTileset, secondaryTileset);
        RowData rowData = {
            .tilesetName = tileset->name,
            .pairedTilesetName = pairedTileset->name,
            .metatileId = Metatile::getMetatileIdString(metatileId),
            .metatileIcon = QIcon(QPixmap::fromImage(metatileImage)),
        };
        results.append(rowData);
    }
    return results;
}

void PaletteColorSearch::refresh() {
    m_resultsCache.clear();
    updateResults();
}

void PaletteColorSearch::updateResults() {
    const Tileset *tileset = currentTileset();
    int paletteId = currentPaletteId();
    int colorId = currentColorId();

    // Update color icon
    QRgb color = tileset->palettePreviews.value(paletteId).value(colorId);
    ui->frame_Color->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(qRed(color)).arg(qGreen(color)).arg(qBlue(color)));

    // Update title
    ui->label_Title->setText(QString("Searching for usage of %1's palette %2.").arg(tileset->name).arg(paletteId));

    // Update table
    ui->table_Results->clearContents();
    ui->table_Results->setRowCount(0);

    QString cacheKey = QString("%1#%2").arg(paletteId).arg(colorId);
    auto it = m_resultsCache.constFind(cacheKey);
    bool inCache = (it != m_resultsCache.constEnd());
    const QList<RowData> results = inCache ? it.value() : search(colorId);

    if (results.isEmpty()) {
        static const RowData noResults = {
            .tilesetName = QStringLiteral("This color is unused."),
            .pairedTilesetName = "",
            .metatileId = QStringLiteral("--"),
            .metatileIcon = QIcon(),
        };
        addTableEntry(noResults);
    } else {
        for (const auto &entry : results) {
            addTableEntry(entry);
        }
    }

    ui->table_Results->sortByColumn(ResultsColumn::TilesetName, Qt::AscendingOrder);

    if (!inCache) m_resultsCache.insert(cacheKey, results);
}

// Double-clicking row data selects the corresponding metatile in the Tileset Editor.
void PaletteColorSearch::cellDoubleClicked(int row, int) {
    auto tilesetNameItem = ui->table_Results->item(row, ResultsColumn::TilesetName);
    auto metatileItem = ui->table_Results->item(row, ResultsColumn::Metatile);
    if (!tilesetNameItem || !metatileItem)
        return;

    // The Tileset Editor (as of writing) has no way to change the selected tilesets independently of
    // the main editor's layout, so if the metatile is not in the current tileset we do nothing.
    // To compare the tileset names, rather than sort out which was the primary or secondary we
    // just make sure it's the same set of names.
    QSet<QString> currentTilesets;
    currentTilesets.insert(m_primaryTileset->name);
    currentTilesets.insert(m_secondaryTileset->name);

    QSet<QString> metatileTilesets;
    metatileTilesets.insert(tilesetNameItem->text());
    metatileTilesets.insert(tilesetNameItem->data(ResultsDataRole::PairedTilesetName).toString());
    if (currentTilesets != metatileTilesets)
        return;

    bool ok;
    uint16_t metatileId = metatileItem->text().toUInt(&ok, 0);
    if (ok) emit metatileSelected(metatileId);
}
