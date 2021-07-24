#pragma once
#ifndef METATILEPARSER_H
#define METATILEPARSER_H

#include "metatile.h"
#include <QList>

namespace MetatileParser {
    QList<Metatile*> parse(QString filepath, bool *error, bool primaryTileset);
}

#endif // METATILEPARSER_H
