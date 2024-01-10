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
    void addNewAttribute(const QString &key, QJsonValue value);

    void setDefaultAttribute(const QString &key, QJsonValue value);
    void unsetDefaultAttribute(const QString &key);

    QSet<QString> keys() const;
    QSet<QString> restrictedKeys() const;
    void setRestrictedKeys(const QSet<QString> keys);

signals:
    void edited();

private:
    QTableWidget *table;

    QSet<QString> m_keys;
    QSet<QString> m_restrictedKeys;

    int addAttribute(const QString &key, QJsonValue value);
    void removeAttribute(const QString &key);
    bool deleteSelectedAttributes();
    void resizeVertically();
};

#endif // CUSTOMATTRIBUTESTABLE_H
