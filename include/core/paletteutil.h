#pragma once
#ifndef PALETTEUTIL_H
#define PALETTEUTIL_H

#include <QList>
#include <QRgb>

namespace PaletteUtil {
    QList<QRgb> parse(QString filepath, bool *error);
    void writeJASC(QString filepath, QVector<QRgb> colors, int offset, int nColors);
}

#endif // PALETTEUTIL_H
