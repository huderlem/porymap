#include "mapparser.h"
#include "config.h"
#include "log.h"
#include "project.h"

MapParser::MapParser()
{
}

MapLayout *MapParser::parse(QString filepath, bool *error, Project *project)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open Advance Map 1.92 Map .map file '%1': ").arg(filepath) + file.errorString());
        return nullptr;
    }

    QByteArray in = file.readAll();
    file.close();

    if (in.length() < 20 || in.length() % 2 != 0) {
        *error = true;
        logError(QString("Advance Map 1.92 Map .map file '%1' is an unexpected size.").arg(filepath));
        return nullptr;
    }

    int borderWidth = static_cast<unsigned char>(in.at(16)); // 0 in RSE .map files
    int borderHeight = static_cast<unsigned char>(in.at(17)); // 0 in RSE .map files
    int numBorderTiles = borderWidth * borderHeight; // 0 if RSE

    int mapDataOffset = 20 + (numBorderTiles * 2); // FRLG .map files store border metatile data after the header
    int mapWidth = static_cast<unsigned char>(in.at(0)) |
                   (static_cast<unsigned char>(in.at(1)) << 8) |
                   (static_cast<unsigned char>(in.at(2)) << 16) |
                   (static_cast<unsigned char>(in.at(3)) << 24);
    int mapHeight = static_cast<unsigned char>(in.at(4)) |
                    (static_cast<unsigned char>(in.at(5)) << 8) |
                    (static_cast<unsigned char>(in.at(6)) << 16) |
                    (static_cast<unsigned char>(in.at(7)) << 24);
    int mapPrimaryTilesetNum = static_cast<unsigned char>(in.at(8)) |
                               (static_cast<unsigned char>(in.at(9)) << 8) |
                               (static_cast<unsigned char>(in.at(10)) << 16) |
                               (static_cast<unsigned char>(in.at(11)) << 24);
    int mapSecondaryTilesetNum = static_cast<unsigned char>(in.at(12)) |
                                 (static_cast<unsigned char>(in.at(13)) << 8) |
                                 (static_cast<unsigned char>(in.at(14)) << 16) |
                                 (static_cast<unsigned char>(in.at(15)) << 24);

    int numMetatiles = mapWidth * mapHeight;
    int expectedFileSize = 20 + (numBorderTiles * 2) + (numMetatiles * 2);
    if (in.length() != expectedFileSize) {
        *error = true;
        logError(QString(".map file is an unexpected size. Expected %1 bytes, but it has %2 bytes.").arg(expectedFileSize).arg(in.length()));
        return nullptr;
    }

    Blockdata blockdata;
    for (int i = mapDataOffset; (i + 1) < in.length(); i += 2) {
        uint16_t word = static_cast<uint16_t>((in[i] & 0xff) + ((in[i + 1] & 0xff) << 8));
        blockdata.append(word);
    }

    Blockdata border;
    if (numBorderTiles != 0) {
        for (int i = 20; (i + 1) < mapDataOffset; i += 2) {
            uint16_t word = static_cast<uint16_t>((in[i] & 0xff) + ((in[i + 1] & 0xff) << 8));
            border.append(word);
        }
    }

    MapLayout *mapLayout = new MapLayout();
    mapLayout->width = QString::number(mapWidth);
    mapLayout->height = QString::number(mapHeight);
    mapLayout->border_width = (borderWidth == 0) ?  QString::number(2) :  QString::number(borderWidth);
    mapLayout->border_height = (borderHeight == 0) ?  QString::number(2) :  QString::number(borderHeight);

    QList<QString> tilesets = project->tilesetLabelsOrdered;

    if (mapPrimaryTilesetNum > tilesets.size())
        mapLayout->tileset_primary_label = tilesets.at(0);
    else
        mapLayout->tileset_primary_label = tilesets.at(mapPrimaryTilesetNum);

    if (mapSecondaryTilesetNum > tilesets.size())
        mapLayout->tileset_secondary_label = tilesets.at(1);
    else
        mapLayout->tileset_secondary_label = tilesets.at(mapSecondaryTilesetNum);

    mapLayout->blockdata = blockdata;

    if (!border.isEmpty()) {
        mapLayout->border = border;
    }

    return mapLayout;
}
