#pragma once
#ifndef METATILEPARSER_H
#define METATILEPARSER_H

#include "metatile.h"
#include <QList>
#include <QString>

class MetatileParser
{
public:
    MetatileParser();
    QList<Metatile*> *parse(QString filepath, bool *error, bool primaryTileset);
};

#endif // METATILEPARSER_H
