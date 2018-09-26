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
};

#endif // SETTINGS_H
