#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCursor>

class Settings
{
public:
    Settings();
    bool smartPathsEnabled;
    bool betterCursors;
    QCursor mapCursor;
    bool playerViewRectEnabled;
};

#endif // SETTINGS_H
