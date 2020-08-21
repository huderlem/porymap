#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCursor>

class Settings
{
public:
    Settings();
    bool smartPathsEnabled;
    bool straightPathsEnabled;
    bool betterCursors;
    QCursor mapCursor;
    bool playerViewRectEnabled;
    bool cursorTileRectEnabled;
};

#endif // SETTINGS_H
