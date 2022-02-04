#include "metatileparser.h"
#include "config.h"
#include "log.h"
#include "project.h"
#include <QString>

QList<Metatile*> MetatileParser::parse(QString filepath, bool *error, bool primaryTileset)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = true;
        logError(QString("Could not open Advance Map 1.92 Metatile .bvd file '%1': ").arg(filepath) + file.errorString());
        return { };
    }

    QByteArray in = file.readAll();
    file.close();

    if (in.length() < 9 || in.length() % 2 != 0) {
        *error = true;
        logError(QString("Advance Map 1.92 Metatile .bvd file '%1' is an unexpected size.").arg(filepath));
        return { };
    }

    int projIdOffset = in.length() - 4;
    int metatileSize = 16;
    BaseGameVersion version;
    if (in.at(projIdOffset + 0) == 'R'
     && in.at(projIdOffset + 1) == 'S'
     && in.at(projIdOffset + 2) == 'E'
     && in.at(projIdOffset + 3) == ' ') {
        // ruby and emerald are handled equally here.
        version = BaseGameVersion::pokeemerald;
    } else if (in.at(projIdOffset + 0) == 'F'
            && in.at(projIdOffset + 1) == 'R'
            && in.at(projIdOffset + 2) == 'L'
            && in.at(projIdOffset + 3) == 'G') {
        version = BaseGameVersion::pokefirered;
    } else {
        *error = true;
        logError(QString("Detected unsupported game type from .bvd file. Last 4 bytes of file must be 'RSE ' or 'FRLG'."));
        return { };
    }

    int attrSize = Metatile::getAttributesSize(version);
    int maxMetatiles = primaryTileset ? Project::getNumMetatilesPrimary() : Project::getNumMetatilesTotal() - Project::getNumMetatilesPrimary();
    int numMetatiles = static_cast<unsigned char>(in.at(0)) |
                                (static_cast<unsigned char>(in.at(1)) << 8) |
                                (static_cast<unsigned char>(in.at(2)) << 16) |
                                (static_cast<unsigned char>(in.at(3)) << 24);
    if (numMetatiles > maxMetatiles) {
        *error = true;
        logError(QString(".bvd file contains data for %1 metatiles, but the maximum number of metatiles is %2.").arg(numMetatiles).arg(maxMetatiles));
        return { };
    }
    if (numMetatiles < 1) {
        *error = true;
        logError(QString(".bvd file contains no data for metatiles."));
        return { };
    }

    int expectedFileSize = 4 + (metatileSize * numMetatiles) + (attrSize * numMetatiles) + 4;
    if (in.length() != expectedFileSize) {
        *error = true;
        logError(QString(".bvd file is an unexpected size. Expected %1 bytes, but it has %2 bytes.").arg(expectedFileSize).arg(in.length()));
        return { };
    }

    QList<Metatile*> metatiles;
    for (int i = 0; i < numMetatiles; i++) {
        Metatile *metatile = new Metatile();
        QList<Tile> tiles;
        for (int j = 0; j < 8; j++) {
            int metatileOffset = 4 + i * metatileSize + j * 2;
            Tile tile(static_cast<uint16_t>(
                        static_cast<unsigned char>(in.at(metatileOffset)) |
                       (static_cast<unsigned char>(in.at(metatileOffset + 1)) << 8)));
            tiles.append(tile);
        }

        int attrOffset = 4 + (numMetatiles * metatileSize) + (i * attrSize);
        uint32_t attributes = 0;
        for (int j = 0; j < attrSize; j++)
            attributes |= static_cast<unsigned char>(in.at(attrOffset + j)) << (8 * j);
        metatile->setAttributes(attributes, version);
        metatile->tiles = tiles;
        metatiles.append(metatile);
    }

    return metatiles;
}
