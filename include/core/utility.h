#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <QString>

namespace Util {
    void numericalModeSort(QStringList &list);
    int roundUp(int numToRound, int multiple);
    QString toDefineCase(QString input);
}

#endif // UTILITY_H
