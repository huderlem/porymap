#pragma once
#ifndef UTILITY_H
#define UTILITY_H

#include <QString>

namespace Util {
    void numericalModeSort(QStringList &list);
    int roundUp(int numToRound, int multiple);
    QString toDefineCase(QString input);
    QString toHexString(uint32_t value, int minLength = 0);
    Qt::Orientations getOrientation(bool xflip, bool yflip);
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 7, 0))
    typedef Qt::CheckState CheckState;
#else
    typedef int CheckState;
#endif

#endif // UTILITY_H
