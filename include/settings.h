#pragma once
#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCursor>

class Settings
{
public:
    Settings();
    bool smartPathsEnabled;
    bool betterCursors;
    bool playerViewRectEnabled;
    bool cursorTileRectEnabled;
};

#endif // SETTINGS_H
