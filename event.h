#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QPixmap>
#include <QMap>

class Event
{
public:
    Event();

public:
    int x() {
        return getInt("x");
    }
    int y() {
        return getInt("y");
    }
    int elevation() {
        return getInt("elevation");
    }
    void setX(int x) {
        put("x", x);
    }
    void setY(int y) {
        put("y", y);
    }
    QString get(QString key) {
        return values.value(key);
    }
    int getInt(QString key) {
        return values.value(key).toInt(nullptr, 0);
    }
    void put(QString key, int value) {
        put(key, QString("%1").arg(value));
    }
    void put(QString key, QString value) {
        values.insert(key, value);
    }

    bool is_hidden_item() {
        return getInt("type") >= 5;
    }

    QMap<QString, QString> values;
    QPixmap pixmap;
};

#endif // EVENT_H
