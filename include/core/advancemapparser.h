#ifndef ADVANCEMAPPARSER_H
#define ADVANCEMAPPARSER_H

#include <QList>
#include <QString>
#include <QRgb>

class Project;
class Layout;
class Metatile;

namespace AdvanceMapParser {
    Layout *parseLayout(const QString &filepath, bool *error, const Project *project);
    QList<Metatile*> parseMetatiles(const QString &filepath, bool *error, bool primaryTileset);
    QList<QRgb> parsePalette(const QString &filepath, bool *error);
};

#endif // ADVANCEMAPPARSER_H
