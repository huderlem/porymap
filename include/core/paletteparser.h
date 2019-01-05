#ifndef PALETTEPARSER_H
#define PALETTEPARSER_H

#include <QList>
#include <QRgb>
#include <QString>

class PaletteParser
{
public:
    PaletteParser();
    QList<QRgb> parse(QString filepath, bool *error);
private:
    QList<QRgb> parsePal(QString filepath, bool *error);
    QList<QRgb> parseJASC(QString filepath, bool *error);
    QList<QRgb> parseAdvanceMapPal(QString filepath, bool *error);
    QList<QRgb> parseAdobeColorTable(QString filepath, bool *error);
    QList<QRgb> parseTileLayerPro(QString filepath, bool *error);
    QList<QRgb> parseAdvancePaletteEditor(QString filepath, bool *error);
    int clampColorValue(int value);
};

#endif // PALETTEPARSER_H
