#include "maplayout.h"

#include <QRegularExpression>

QString MapLayout::layoutConstantFromName(QString mapName) {
    // Transform map names of the form 'GraniteCave_B1F` into layout constants like 'LAYOUT_GRANITE_CAVE_B1F'.
    static const QRegularExpression caseChange("([a-z])([A-Z])");
    QString nameWithUnderscores = mapName.replace(caseChange, "\\1_\\2");
    QString withMapAndUppercase = "LAYOUT_" + nameWithUnderscores.toUpper();
    static const QRegularExpression underscores("_+");
    QString constantName = withMapAndUppercase.replace(underscores, "_");

    // Handle special cases.
    // SSTidal should be SS_TIDAL, rather than SSTIDAL
    constantName = constantName.replace("SSTIDAL", "SS_TIDAL");

    return constantName;
}

int MapLayout::getWidth() {
    return width;
}

int MapLayout::getHeight() {
    return height;
}

int MapLayout::getBorderWidth() {
    return border_width;
}

int MapLayout::getBorderHeight() {
    return border_height;
}
