#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <QString>

namespace Util {
    void numericalModeSort(QStringList &list);
    int roundUp(int numToRound, int multiple);
    QString toDefineCase(QString input);
    QString toHexString(uint32_t value, int minLength = 0);
}

#endif // UTILITY_H
