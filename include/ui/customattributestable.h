#ifndef CUSTOMATTRIBUTESTABLE_H
#define CUSTOMATTRIBUTESTABLE_H

#include <QObject>
#include <QJsonValue>
#include <QTableWidget>

class CustomAttributesTable : public QTableWidget
{
    Q_OBJECT

public:
    explicit CustomAttributesTable(QWidget *parent = nullptr);
    ~CustomAttributesTable() {};

    QJsonObject getAttributes() const;
    void setAttributes(const QJsonObject &attributes);

    void addNewAttribute(const QString &key, const QJsonValue &value);
    bool deleteSelectedAttributes();

    bool isEmpty() const;
    bool isSelectionEmpty() const;

    QSet<QString> keys() const { return m_keys; }
    QSet<QString> restrictedKeys() const { return m_restrictedKeys; }
    void setRestrictedKeys(const QSet<QString> &keys) { m_restrictedKeys = keys; }

signals:
    void edited();

protected:
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    QSet<QString> m_keys;           // All keys currently in the table
    QSet<QString> m_restrictedKeys; // All keys not allowed in the table

    QPair<QString, QJsonValue> getAttribute(int row) const;
    int addAttribute(const QString &key, const QJsonValue &value);
    void removeAttribute(const QString &key);
    void resizeVertically();
};

#endif // CUSTOMATTRIBUTESTABLE_H
