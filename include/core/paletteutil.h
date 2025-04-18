#pragma once
#ifndef PALETTEUTIL_H
#define PALETTEUTIL_H

#include <QList>
#include <QRgb>

namespace PaletteUtil {
    QList<QRgb> parse(QString filepath, bool *error);
    bool writeJASC(const QString &filepath, const QVector<QRgb> &colors, int offset, int nColors);
}

#endif // PALETTEUTIL_H
