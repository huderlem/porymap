#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCursor>

struct GridSettings {
    uint width = 16;
    uint height = 16;
    int offsetX = 0;
    int offsetY = 0;
    Qt::PenStyle style = Qt::SolidLine;
    QColor color = Qt::black;
};

class Settings
{
public:
    Settings();
    bool smartPathsEnabled;
    bool betterCursors;
    QCursor mapCursor;
    bool playerViewRectEnabled;
    bool cursorTileRectEnabled;
};

#endif // SETTINGS_H
