#ifndef CUSTOMATTRIBUTESTABLE_H
#define CUSTOMATTRIBUTESTABLE_H

#include "events.h"
#include <QObject>
#include <QFrame>
#include <QTableWidget>

class CustomAttributesTable : public QFrame
{
public:
    explicit CustomAttributesTable(Event *event, QWidget *parent = nullptr);
    ~CustomAttributesTable();

    static const QMap<QString, QJsonValue> getAttributes(QTableWidget * table);
    static QJsonValue pickType(QWidget * parent, bool * ok = nullptr);
    static void addAttribute(QTableWidget * table, QString key, QJsonValue value, bool isNew = false);
    static bool deleteSelectedAttributes(QTableWidget * table);

private:
    Event *event;
    QTableWidget *table;
    void resizeVertically();
};

#endif // CUSTOMATTRIBUTESTABLE_H
