#pragma once
#ifndef PALETTEUTIL_H
#define PALETTEUTIL_H

#include <QList>
#include <QRgb>
#include <QString>

class PaletteUtil {
public:
    PaletteUtil();
    QList<QRgb> parse(QString filepath, bool* error);
    void writeJASC(QString filepath, QVector<QRgb> colors, int offset, int nColors);

private:
    QList<QRgb> parsePal(QString filepath, bool* error);
    QList<QRgb> parseJASC(QString filepath, bool* error);
    QList<QRgb> parseAdvanceMapPal(QString filepath, bool* error);
    QList<QRgb> parseAdobeColorTable(QString filepath, bool* error);
    QList<QRgb> parseTileLayerPro(QString filepath, bool* error);
    QList<QRgb> parseAdvancePaletteEditor(QString filepath, bool* error);
    int clampColorValue(int value);
};

#endif // PALETTEUTIL_H
