#ifndef MAPPARSER_H
#define MAPPARSER_H

#include "maplayout.h"
#include "project.h"
#include <QList>
#include <QString>

class MapParser
{
public:
    MapParser();
    MapLayout *parse(QString filepath, bool *error, Project *project);
};

#endif // MAPPARSER_H
