#ifndef NUMERICSORTTABLEITEM_H
#define NUMERICSORTTABLEITEM_H

#include <QTableWidgetItem>
#include <QCollator>

class NumericSortTableItem : public QTableWidgetItem
{
public:
    explicit NumericSortTableItem(const QString &text) : QTableWidgetItem(text) {};

protected:
    virtual bool operator<(const QTableWidgetItem &other) const override {
        QCollator collator;
        collator.setNumericMode(true);
        return collator.compare(text(), other.text()) < 0;
    }
};

#endif // NUMERICSORTTABLEITEM_H
