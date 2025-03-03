#include "utility.h"

#include <QCollator>
#include <QRegularExpression>

// Sometimes we want to sort names alphabetically to make them easier to find in large combo box lists.
// QStringList::sort (as of writing) can only sort numbers in lexical order, which has an undesirable
// effect (e.g. 'ROUTE_1, ROUTE_10, ROUTE_2,...' instead of 'ROUTE_1, ROUTE_2,... ROUTE_10').
// We can use QCollator to sort these lists with better handling for numbers.
void Util::numericalModeSort(QStringList &list) {
    static QCollator collator;
    collator.setNumericMode(true);
    std::sort(list.begin(), list.end(), collator);
}

int Util::roundUp(int numToRound, int multiple) {
    if (multiple <= 0)
        return numToRound;

    int remainder = abs(numToRound) % multiple;
    if (remainder == 0)
        return numToRound;

    if (numToRound < 0)
        return -(abs(numToRound) - remainder);
    else
        return numToRound + multiple - remainder;
}

// Ex: input 'GraniteCave_B1F' returns 'GRANITE_CAVE_B1F'.
QString Util::toDefineCase(QString input) {
    static const QRegularExpression re_CaseChange("([a-z])([A-Z])");
    input.replace(re_CaseChange, "\\1_\\2");

    // Remove sequential underscores
    static const QRegularExpression re_Underscores("_+");
    input.replace(re_Underscores, "_");

    return input.toUpper();
}

QString Util::toHexString(uint32_t value, int minLength) {
    return "0x" + QString("%1").arg(value, minLength, 16, QChar('0')).toUpper();
}
