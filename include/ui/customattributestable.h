#ifndef CUSTOMATTRIBUTESTABLE_H
#define CUSTOMATTRIBUTESTABLE_H

#include "event.h"
#include <QObject>
#include <QFrame>
#include <QTableWidget>

class CustomAttributesTable : public QFrame
{
public:
    explicit CustomAttributesTable(Event *event, QWidget *parent = nullptr);
    ~CustomAttributesTable();

private:
    Event *event;
    QTableWidget *table;
    void resizeVertically();
    QMap<QString, QString> getTableFields();
};

#endif // CUSTOMATTRIBUTESTABLE_H
