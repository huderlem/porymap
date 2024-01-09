#ifndef CUSTOMATTRIBUTESTABLE_H
#define CUSTOMATTRIBUTESTABLE_H

#include "events.h"
#include <QObject>
#include <QFrame>
#include <QTableWidget>

class CustomAttributesTable : public QFrame
{
    Q_OBJECT

public:
    explicit CustomAttributesTable(QWidget *parent = nullptr);
    ~CustomAttributesTable();

    QMap<QString, QJsonValue> getAttributes() const;
    void setAttributes(const QMap<QString, QJsonValue> attributes);
    bool deleteSelectedAttributes();

signals:
    void edited();

private:
    QTableWidget *table;
    QJsonValue pickType(bool * ok = nullptr);
    void addAttribute(QString key, QJsonValue value, bool init);
    void resizeVertically();
};

#endif // CUSTOMATTRIBUTESTABLE_H
