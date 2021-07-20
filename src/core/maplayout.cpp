#include "maplayout.h"

#include <QRegularExpression>

QString MapLayout::layoutConstantFromName(QString mapName) {
    // Transform map names of the form 'GraniteCave_B1F` into layout constants like 'LAYOUT_GRANITE_CAVE_B1F'.
    QString nameWithUnderscores = mapName.replace(QRegularExpression("([a-z])([A-Z])"), "\\1_\\2");
    QString withMapAndUppercase = "LAYOUT_" + nameWithUnderscores.toUpper();
    QString constantName = withMapAndUppercase.replace(QRegularExpression("_+"), "_");

    // Handle special cases.
    // SSTidal should be SS_TIDAL, rather than SSTIDAL
    constantName = constantName.replace("SSTIDAL", "SS_TIDAL");

    return constantName;
}

int MapLayout::getWidth() {
    return width.toInt(nullptr, 0);
}

int MapLayout::getHeight() {
    return height.toInt(nullptr, 0);
}

int MapLayout::getBorderWidth() {
    return border_width.toInt(nullptr, 0);
}

int MapLayout::getBorderHeight() {
    return border_height.toInt(nullptr, 0);
}
