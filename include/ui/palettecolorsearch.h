#ifndef PALETTECOLORSEARCH_H
#define PALETTECOLORSEARCH_H

#include <QDialog>
#include <QIcon>
#include <QMap>

class Tileset;
class Project;

namespace Ui {
class PaletteColorSearch;
}

class PaletteColorSearch : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteColorSearch(Project *project,
                                const Tileset *primaryTileset,
                                const Tileset *secondaryTileset,
                                QWidget *parent = nullptr);
    ~PaletteColorSearch();

    void setPaletteId(int paletteId);
    int currentPaletteId() const;

    void setColorId(int colorId);
    int currentColorId() const;

    void setTilesets(const Tileset *primaryTileset, const Tileset *secondaryTileset);
    const Tileset* currentTileset() const;

signals:
    void metatileSelected(uint16_t metatileId);
    void paletteIdChanged(int paletteId);

private:
    struct RowData {
        QString tilesetName;
        QString pairedTilesetName;
        QString metatileId;
        QIcon metatileIcon;
    };

    enum ResultsColumn {
        TilesetName,
        Metatile,
    };

    Ui::PaletteColorSearch *ui;
    Project *m_project;
    const Tileset *m_primaryTileset;
    const Tileset *m_secondaryTileset;

    QMap<QString,QList<RowData>> m_resultsCache;

    void addTableEntry(const RowData &rowData);
    QList<RowData> search(int colorId) const;
    QList<RowData> search(int colorId, const Tileset *tileset, const Tileset *pairedTileset) const;
    void refresh();
    void updateResults();
    void cellDoubleClicked(int row, int col);
};

#endif // PALETTECOLORSEARCH_H
